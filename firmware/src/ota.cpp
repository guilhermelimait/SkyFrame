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
volatile int otaPhase = 0;
volatile bool otaCancel = false;
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

static void setStatus(const char *fmt, const char *arg = "") { snprintf(otaStatus, sizeof otaStatus, fmt, arg); Serial.printf("ota: %s\n", otaStatus); }

// a download step failed: say which, with the HTTP code (or connection error), on the page and over USB
static void failStep(const char *what, int code) {
  snprintf(otaStatus, sizeof otaStatus, "Couldn't download the %s (error %d)", what, code);
  Serial.printf("ota: %s: %s, internal heap %u (largest block %u)\n", otaStatus, HTTPClient::errorToString(code).c_str(),
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL), (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
}

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

// GitHub download links redirect to its file servers. The HTTP library's own redirect handling reconnects
// without the certificate bundle (the TLS handshake then fails), so redirects are followed here instead,
// each hop on a fresh, certificate-checked connection. Returns the final address, or "" on error.
static String resolveDownload(String url, int &code) {
  for (int hop = 0; hop < 5; hop++) {
    NetworkClientSecure cli;
    HTTPClient http;
    secureClient(cli);
    http.setUserAgent("skyframe/" FW_VERSION);
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    http.setTimeout(20000);
    const char *keep[] = {"Location"};
    http.collectHeaders(keep, 1);
    if (!http.begin(cli, url)) { code = -100; return ""; }
    code = http.GET();
    String next = http.header("Location");
    http.end();
    if (code == 200) return url;
    if ((code == 301 || code == 302 || code == 303 || code == 307 || code == 308) && next.startsWith("https://")) { url = next; continue; }
    return "";
  }
  code = -101;   // too many redirects
  return "";
}

// Returns true if a new firmware was installed (the caller then reboots).
bool otaCheckAndInstall(const char *repo, bool install) {
  otaCancel = false;   // a cancel only applies to the update it was meant for
  if (!repo || !strchr(repo, '/')) { setStatus("No GitHub repository set"); return false; }
  // the latest release's description. GitHub sometimes sends it "chunked", so it's read through the HTTP library
  // (which removes the chunk markers) rather than parsed from the raw stream; a flaky connection gets 3 tries.
  String url = String("https://api.github.com/repos/") + repo + "/releases/latest";
  JsonDocument filter;
  filter["tag_name"] = true;
  filter["assets"][0]["name"] = true;
  filter["assets"][0]["browser_download_url"] = true;
  filter["assets"][0]["size"] = true;
  JsonDocument rel;
  for (int attempt = 1;; attempt++) {
    NetworkClientSecure cli;
    HTTPClient http;
    if (!beginGet(http, cli, url)) { setStatus("Couldn't reach GitHub"); return false; }
    int code = http.GET();
    if (code == 404) { http.end(); setStatus("No releases published yet"); return false; }
    String body = code == 200 ? http.getString() : String();
    http.end();
    DeserializationError err = code == 200 ? deserializeJson(rel, body, DeserializationOption::Filter(filter)) : DeserializationError::InvalidInput;
    if (code == 200 && !err) break;
    if (attempt >= 3) {
      if (code != 200) { char b[24]; snprintf(b, sizeof b, "%d", code); setStatus("GitHub answered with error %s", b); }
      else setStatus("Couldn't read the release info (%s)", err.c_str());
      return false;
    }
    delay(2000);
  }
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
    int code2;
    sigUrl = resolveDownload(sigUrl, code2);
    if (sigUrl.length()) code2 = beginGet(h2, c2, sigUrl) ? h2.GET() : -100;
    if (code2 != 200) { h2.end(); failStep("signature", code2); return false; }
    String s = h2.getString();   // hex text of the DER signature
    h2.end();
    s.trim();
    for (size_t i = 0; i + 1 < s.length() && sigLen < sizeof sig; i += 2) sig[sigLen++] = (uint8_t)strtol(s.substring(i, i + 2).c_str(), nullptr, 16);
    if (sigLen < 8) { setStatus("The signature file is empty or damaged"); return false; }
  }

  // firmware: download it whole into PSRAM first. Writing flash blocks the panel's picture memory, so writing
  // while downloading made the screen go wild for the whole download; this way the screen stays calm, nothing
  // touches flash unless the signature is valid, and the flash write takes a few seconds with the backlight off.
  strlcpy(otaNewVersion, tag.c_str(), sizeof otaNewVersion);
  if (otaCancel) { otaCancel = false; otaPercent = -1; otaPhase = 0; setStatus("Update cancelled"); return false; }
  NetworkClientSecure c3;
  HTTPClient h3;
  int code3;
  binUrl = resolveDownload(binUrl, code3);
  if (binUrl.length()) code3 = beginGet(h3, c3, binUrl) ? h3.GET() : -100;
  if (code3 != 200) { h3.end(); failStep("firmware", code3); return false; }
  int total = h3.getSize() > 0 ? h3.getSize() : binSize;
  const esp_partition_t *slot = esp_ota_get_next_update_partition(nullptr);
  uint8_t *img = total > 0 && slot && (uint32_t)total <= slot->size ? (uint8_t *)ps_malloc(total) : nullptr;
  if (!img) { h3.end(); setStatus("Not enough room for the update"); return false; }
  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts(&sha, 0);
  NetworkClient *stream = h3.getStreamPtr();
  int got = 0;
  uint32_t lastData = millis();
  otaPhase = 1;
  otaPercent = 0;
  while (got < total && millis() - lastData < 20000 && !otaCancel) {
    int n = stream->available() ? stream->readBytes(img + got, min(4096, total - got)) : 0;
    if (n <= 0) { delay(5); continue; }
    lastData = millis();
    mbedtls_sha256_update(&sha, img + got, n);
    got += n;
    otaPercent = got * 100 / total;
  }
  h3.end();
  uint8_t hash[32];
  mbedtls_sha256_finish(&sha, hash);
  mbedtls_sha256_free(&sha);
  if (otaCancel) { otaCancel = false; free(img); otaPercent = -1; otaPhase = 0; setStatus("Update cancelled"); return false; }
  if (got != total) { free(img); otaPercent = -1; otaPhase = 0; setStatus("Download interrupted, will retry later"); return false; }
  if (!verifySignature(hash, sig, sigLen)) {
    free(img); otaPercent = -1; otaPhase = 0;
    setStatus("Rejected %s: signature doesn't match", tag.c_str());
    return false;
  }

  // install: show "installing", then write flash in one go with the backlight off
  otaPhase = 2;
  delay(1200);                       // let the screen show the message first
  hwFlashBegin();
  bool ok = Update.begin(total);
  for (int off = 0; ok && off < total; off += 65536) {
    int n = min(65536, total - off);
    ok = Update.write(img + off, n) == (size_t)n;
  }
  ok = ok && Update.end(true);
  if (!ok) Update.abort();
  free(img);
  otaPhase = ok ? 3 : 0;
  // installed: stay dark until the restart, so the next thing on screen is the new version starting
  // (lighting a "restarting" frame right after the flash write showed a shifted, flickering picture)
  if (ok) hwStayDark();
  hwFlashEnd();
  if (!ok) { otaPercent = -1; setStatus("Couldn't finish installing"); return false; }
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
