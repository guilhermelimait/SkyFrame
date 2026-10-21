#pragma once
#include <NetworkClientSecure.h>

// Mozilla's root certificate bundle, already compiled into the ESP-IDF libraries.
extern const uint8_t x509_bundle_start[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t x509_bundle_end[] asm("_binary_x509_crt_bundle_end");

// Every HTTPS connection checks the server's certificate against that bundle.
inline void secureClient(NetworkClientSecure &c) {
  c.setCACertBundle(x509_bundle_start, x509_bundle_end - x509_bundle_start);
}

// OTA (ota.cpp)
extern volatile int otaPercent;      // -1 when idle, 0-100 while installing
extern char otaStatus[72];
extern char otaNewVersion[16];
bool otaCheckAndInstall(const char *repo, bool install = true);   // install=false only looks for a newer version
extern char otaAvailable[16];   // newer version found by the last check ("" if none)
void otaMarkGood();
bool otaPendingVerify();
void otaRollbackNow();
bool otaSelfTest(const char *msg, const char *hex);
