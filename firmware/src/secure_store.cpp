// Wi-Fi password encryption with a key that never leaves the chip.
//
// The first time it's needed, the chip generates a random 256-bit key and burns it into a free eFuse key
// block with purpose HMAC_UP and read protection: software can't read it back, only ask the HMAC peripheral
// to use it. The AES-256-GCM key for the password is HMAC(chip key, label), so a copy of the flash alone is
// useless for recovering the password.
//
// Burning the eFuse is a one-time, permanent write that uses one of the chip's six key blocks. It doesn't
// affect booting, USB flashing or updates.
#include "secure_store.h"
#include "hw.h"
#include <Preferences.h>
#include <nvs_flash.h>
#include <esp_efuse.h>
#include <esp_hmac.h>
#include <esp_random.h>
#include "mbedtls/gcm.h"

// Keep this label as it is, even after the rename to SkyFrame: it is part of the key derivation,
// and changing it would make already stored passwords impossible to decrypt.
static const char LABEL[] = "deskdisplay/wifi-password/v1";

static bool findKey(esp_efuse_block_t &blk) { return esp_efuse_find_purpose(ESP_EFUSE_KEY_PURPOSE_HMAC_UP, &blk); }

bool ssKeyReady() {
  esp_efuse_block_t b;
  return findKey(b) && esp_efuse_get_key_dis_read(b);
}

// Needs a true random source: call it while Wi-Fi is running (the radio feeds the hardware RNG).
bool ssCreateKey() {
  if (ssKeyReady()) return true;
  esp_efuse_block_t b = esp_efuse_find_unused_key_block();
  if (b == EFUSE_BLK_KEY_MAX) return false;   // no free key block on this chip
  uint8_t key[32];
  esp_fill_random(key, sizeof key);
  esp_err_t err = esp_efuse_write_key(b, ESP_EFUSE_KEY_PURPOSE_HMAC_UP, key, sizeof key);
  memset(key, 0, sizeof key);
  if (err != ESP_OK) return false;
  if (!esp_efuse_get_key_dis_read(b)) esp_efuse_set_key_dis_read(b);   // make sure it can't be read back
  return ssKeyReady();
}

static bool deriveKey(uint8_t out[32]) {
  esp_efuse_block_t b;
  if (!findKey(b)) return false;
  return esp_hmac_calculate((hmac_key_id_t)(b - EFUSE_BLK_KEY0), LABEL, sizeof LABEL - 1, out) == ESP_OK;
}

// output: 12-byte nonce | ciphertext | 16-byte tag
bool ssEncrypt(const String &plain, uint8_t *out, size_t &outLen, size_t outCap) {
  size_t n = plain.length();
  if (outCap < 12 + n + 16) return false;
  uint8_t key[32];
  if (!deriveKey(key)) return false;
  esp_fill_random(out, 12);
  mbedtls_gcm_context g;
  mbedtls_gcm_init(&g);
  bool ok = mbedtls_gcm_setkey(&g, MBEDTLS_CIPHER_ID_AES, key, 256) == 0 &&
            mbedtls_gcm_crypt_and_tag(&g, MBEDTLS_GCM_ENCRYPT, n, out, 12, nullptr, 0, (const uint8_t *)plain.c_str(), out + 12, 16, out + 12 + n) == 0;
  mbedtls_gcm_free(&g);
  memset(key, 0, sizeof key);
  outLen = 12 + n + 16;
  return ok;
}

bool ssDecrypt(const uint8_t *in, size_t inLen, String &plain) {
  if (inLen < 12 + 16 || inLen > 12 + 128 + 16) return false;
  size_t n = inLen - 12 - 16;
  uint8_t key[32], buf[129];
  if (!deriveKey(key)) return false;
  mbedtls_gcm_context g;
  mbedtls_gcm_init(&g);
  bool ok = mbedtls_gcm_setkey(&g, MBEDTLS_CIPHER_ID_AES, key, 256) == 0 &&
            mbedtls_gcm_auth_decrypt(&g, n, in, 12, nullptr, 0, in + 12 + n, 16, in + 12, buf) == 0;
  mbedtls_gcm_free(&g);
  memset(key, 0, sizeof key);
  if (ok) { buf[n] = 0; plain = String((const char *)buf); }
  memset(buf, 0, sizeof buf);
  return ok;
}

/* ---------------- Wi-Fi credentials in NVS ---------------- */
// namespace "wifi": ssid, pass_enc (encrypted)  — or, before the chip key exists, pass (plain, temporary)

bool wifiLoad(String &ssid, String &pass, bool &plain) {
  Preferences p;
  p.begin("wifi", true);
  ssid = p.getString("ssid", "");
  plain = p.isKey("pass");
  bool ok = true;
  if (plain) pass = p.getString("pass", "");
  else if (p.isKey("pass_enc")) {
    uint8_t buf[160];
    size_t n = p.getBytes("pass_enc", buf, sizeof buf);
    ok = ssDecrypt(buf, n, pass);
    memset(buf, 0, sizeof buf);
  } else pass = "";
  p.end();
  return ok;
}

bool wifiSave(const String &ssid, const String &pass) {
  Preferences p;
  p.begin("wifi", false);
  p.putString("ssid", ssid);
  bool enc = false;
  if (ssKeyReady()) {
    uint8_t buf[160];
    size_t n;
    if (ssEncrypt(pass, buf, n, sizeof buf)) { p.putBytes("pass_enc", buf, n); p.remove("pass"); enc = true; }
    memset(buf, 0, sizeof buf);
  }
  if (!enc) { p.putString("pass", pass); p.remove("pass_enc"); }   // first setup: encrypted after the first connection
  p.end();
  return enc;
}

// Erase the whole NVS partition and write our settings back. Deleting an NVS entry only marks it deleted,
// the old bytes stay in flash until the page is recycled, so this is the only way to really remove a plaintext
// password. Must run before Wi-Fi starts.
void nvsWipeKeep() {
  String cfg, pin, ssid;
  uint8_t enc[160];
  size_t encLen = 0;
  Preferences p;
  p.begin("desk", true); cfg = p.getString("cfg", ""); p.end();
  p.begin("sec", true); pin = p.getString("pin", ""); p.end();
  p.begin("wifi", true); ssid = p.getString("ssid", ""); if (p.isKey("pass_enc")) encLen = p.getBytes("pass_enc", enc, sizeof enc); p.end();
  nvs_flash_deinit();
  nvs_flash_erase();
  nvs_flash_init();
  if (cfg.length()) { p.begin("desk", false); p.putString("cfg", cfg); p.end(); }
  if (pin.length()) { p.begin("sec", false); p.putString("pin", pin); p.end(); }
  p.begin("wifi", false);
  if (ssid.length()) p.putString("ssid", ssid);
  if (encLen) p.putBytes("pass_enc", enc, encLen);
  p.end();
  memset(enc, 0, sizeof enc);
}

// Called at boot before Wi-Fi: if a plaintext password is stored and the chip key exists, encrypt it and wipe.
void wifiMigrateAtBoot() {
  Preferences f;
  f.begin("wifi", true);
  bool wipe = f.isKey("wipe");   // set after the chip key was created and the password re-saved encrypted
  f.end();
  if (wipe) { nvsWipeKeep(); Serial.println("secure: settings storage wiped, password kept only encrypted"); return; }
  String ssid, pass;
  bool plain;
  if (!ssKeyReady() || !wifiLoad(ssid, pass, plain) || !plain || !ssid.length()) return;
  if (wifiSave(ssid, pass)) { nvsWipeKeep(); Serial.println("secure: password encrypted, settings storage wiped"); }
  pass = String();
}

// Worked out once: reading settings storage goes through the flash driver, which pauses the panel's picture
// memory, so doing it on every status request (every 2 s while the settings page is open) made the screen glitch.
// The answer only changes with a restart (saving Wi-Fi and the encryption step both restart the board).
const char *wifiPasswordState() {
  static const char *state = nullptr;
  if (state) return state;
  hwFlashBegin();
  if (wifiPasswordProtected()) state = "encrypted";
  else if (!ssKeyReady() && esp_efuse_find_unused_key_block() == EFUSE_BLK_KEY_MAX) state = "unavailable";
  else state = "pending";
  hwFlashEnd();
  return state;
}

bool wifiPasswordProtected() {
  Preferences p;
  p.begin("wifi", true);
  bool plain = p.isKey("pass");
  p.end();
  return !plain && ssKeyReady();
}
