// Firmware updates from GitHub Releases, verified with an ECDSA P-256 signature before installing.
//
// A release must have a tag like "v1.2" and two assets made by tools/release.py:
//   firmware.bin  — the new firmware
//   firmware.sig  — signature of SHA-256(firmware.bin) with keys/ota_signing_key.pem
// The board only installs a newer version whose signature matches the public key built into it.
// After installing it reboots; if the new version doesn't come up properly, the bootloader rolls back.
#include "app.h"
#include "hw.h"
#include "net_secure.h"
#include "ota_pubkey.h"
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <esp_ota_ops.h>
#include "mbedtls/sha256.h"
#include "mbedtls/pk.h"

volatile int otaPercent = -1;
char otaStatus[72] = "";
char otaNewVersion[16] = "";
char otaAvailable[16] = "";

static int verCmp(const char *a, const char *b) {   // compares "1.2.3" style versions, ignores a leading 'v'
  if (*a == 'v' || *a == 'V') a++;
  if (*b == 'v' || *b == 'V') b++;
  for (int i = 0; i < 3; i++) {
    long x = strtol(a, (char **)&a, 10), y = strtol(b, (char **)&b, 10);
    if (x != y) return x < y ? -1 : 1;
    if (*a == '.') a++;
    if (*b == '.') b++;
  }
  return 0;
}

static void setStatus(const char *fmt, const char *arg = "") { snprintf(otaStatus, sizeof otaStatus, fmt, arg); }

static bool verifySignature(const uint8_t hash[32], const uint8_t *sig, size_t sigLen) {
  mbedtls_pk_context pk;
  mbedtls_pk_init(&pk);
  bool ok = mbedtls_pk_parse_public_key(&pk, (const unsigned char *)OTA_PUBKEY_PEM, sizeof(OTA_PUBKEY_PEM)) == 0 &&
            mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, 32, sig, sigLen) == 0;
  mbedtls_pk_free(&pk);
  return ok;
}

static bool beginGet(HTTPClient &http, NetworkClientSecure &cli, const String &url) {
  secureClient(cli);
  http.setUserAgent("skyframe/" FW_VERSION);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);   // GitHub asset links redirect to its download servers
  http.setTimeout(20000);
  if (!http.begin(cli, url)) return false;
  http.addHeader("Accept", "application/vnd.github+json");
  return true;
}

// Returns true if a new firmware was installed (the caller then reboots).
bool otaCheckAndInstall(const char *repo, bool install) {
  if (!repo || !strchr(repo, '/')) { setStatus("No GitHub repository set"); return false; }
  NetworkClientSecure cli;
  HTTPClient http;
  String url = String("https://api.github.com/repos/") + repo + "/releases/latest";
  if (!beginGet(http, cli, url)) { setStatus("Couldn't reach GitHub"); return false; }
  int code = http.GET();
  if (code != 200) {
    http.end();
    if (code == 404) setStatus("No releases published yet");
    else { char b[24]; snprintf(b, sizeof b, "%d", code); setStatus("GitHub answered with error %s", b); }
    return false;
  }
  JsonDocument filter;
  filter["tag_name"] = true;
  filter["assets"][0]["name"] = true;
  filter["assets"][0]["browser_download_url"] = true;
  filter["assets"][0]["size"] = true;
  JsonDocument rel;
  DeserializationError err = deserializeJson(rel, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) { setStatus("Couldn't read the release info"); return false; }
  String tag = rel["tag_name"] | "";
  if (verCmp(tag.c_str(), FW_VERSION) <= 0) { otaAvailable[0] = 0; setStatus("Up to date (v" FW_VERSION ")"); return false; }
  strlcpy(otaAvailable, tag.c_str(), sizeof otaAvailable);
  if (!install) { setStatus("Version %s is available", tag.c_str()); return false; }
  String binUrl, sigUrl;
  int binSize = 0;
  for (JsonObject a : rel["assets"].as<JsonArray>()) {
    String n = a["name"] | "";
    if (n == "firmware.bin") { binUrl = a["browser_download_url"] | ""; binSize = a["size"] | 0; }
    if (n == "firmware.sig") sigUrl = a["browser_download_url"] | "";
  }
  if (!binUrl.length() || !sigUrl.length()) { setStatus("Release %s is missing firmware.bin or firmware.sig", tag.c_str()); return false; }

  // signature (small)
  uint8_t sig[160];
  size_t sigLen = 0;
  {
    NetworkClientSecure c2;
    HTTPClient h2;
    if (!beginGet(h2, c2, sigUrl) || h2.GET() != 200) { h2.end(); setStatus("Couldn't download the signature"); return false; }
    String s = h2.getString();   // hex text of the DER signature
    h2.end();
    s.trim();
    for (size_t i = 0; i + 1 < s.length() && sigLen < sizeof sig; i += 2) sig[sigLen++] = (uint8_t)strtol(s.substring(i, i + 2).c_str(), nullptr, 16);
    if (sigLen < 8) { setStatus("The signature file is empty or damaged"); return false; }
  }

  // firmware: stream into the spare app slot while hashing it
  strlcpy(otaNewVersion, tag.c_str(), sizeof otaNewVersion);
  NetworkClientSecure c3;
  HTTPClient h3;
  if (!beginGet(h3, c3, binUrl) || h3.GET() != 200) { h3.end(); setStatus("Couldn't download the firmware"); return false; }
  int total = h3.getSize() > 0 ? h3.getSize() : binSize;
  if (total <= 0 || !Update.begin(total)) { h3.end(); setStatus("Not enough room for the update"); return false; }
  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts(&sha, 0);
  NetworkClient *stream = h3.getStreamPtr();
  static uint8_t buf[4096];
  int got = 0;
  uint32_t lastData = millis();
  otaPercent = 0;
  while (got < total && millis() - lastData < 20000) {
    int n = stream->available() ? stream->readBytes(buf, min((int)sizeof buf, total - got)) : 0;
    if (n <= 0) { delay(5); continue; }
    lastData = millis();
    mbedtls_sha256_update(&sha, buf, n);
    if (Update.write(buf, n) != (size_t)n) break;
    if ((got ^ (got + n)) & ~0xFFFF) hwResync();   // every 64 KB
    got += n;
    otaPercent = got * 100 / total;
  }
  h3.end();
  uint8_t hash[32];
  mbedtls_sha256_finish(&sha, hash);
  mbedtls_sha256_free(&sha);
  if (got != total) { Update.abort(); otaPercent = -1; setStatus("Download interrupted, will retry later"); return false; }
  if (!verifySignature(hash, sig, sigLen)) {
    Update.abort(); otaPercent = -1;
    setStatus("Rejected %s: signature doesn't match", tag.c_str());
    return false;
  }
  if (!Update.end(true)) { otaPercent = -1; setStatus("Couldn't finish installing"); return false; }
  setStatus("Installed %s, restarting", tag.c_str());
  return true;
}

// The core asks whether to confirm the running firmware itself; we confirm it only after it has proven it works.
bool verifyRollbackLater() { return true; }

void otaMarkGood() {
  esp_ota_img_states_t st;
  const esp_partition_t *run = esp_ota_get_running_partition();
  if (esp_ota_get_state_partition(run, &st) == ESP_OK && st == ESP_OTA_IMG_PENDING_VERIFY) esp_ota_mark_app_valid_cancel_rollback();
}
bool otaPendingVerify() {
  esp_ota_img_states_t st;
  return esp_ota_get_state_partition(esp_ota_get_running_partition(), &st) == ESP_OK && st == ESP_OTA_IMG_PENDING_VERIFY;
}
void otaRollbackNow() { esp_ota_mark_app_invalid_rollback_and_reboot(); }

// Debug: verify a signature of a short test message (same key, signature format and check as an update).
bool otaSelfTest(const char *msg, const char *hex) {
  uint8_t sig[160];
  size_t sigLen = 0;
  for (size_t i = 0; hex[i] && hex[i + 1] && sigLen < sizeof sig; i += 2) {
    char b[3] = {hex[i], hex[i + 1], 0};
    sig[sigLen++] = (uint8_t)strtol(b, nullptr, 16);
  }
  uint8_t hash[32];
  mbedtls_sha256((const unsigned char *)msg, strlen(msg), hash, 0);
  return verifySignature(hash, sig, sigLen);
}
