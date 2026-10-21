#pragma once
#include <Arduino.h>

// Chip-bound encryption of the Wi-Fi password (see secure_store.cpp)
bool ssKeyReady();
bool ssCreateKey();   // one-time eFuse write; call while Wi-Fi is running
bool ssEncrypt(const String &plain, uint8_t *out, size_t &outLen, size_t outCap);
bool ssDecrypt(const uint8_t *in, size_t inLen, String &plain);

bool wifiLoad(String &ssid, String &pass, bool &plain);
bool wifiSave(const String &ssid, const String &pass);   // true if stored encrypted
void wifiMigrateAtBoot();
void nvsWipeKeep();
bool wifiPasswordProtected();
const char *wifiPasswordState();   // "encrypted", "pending" or "unavailable"
