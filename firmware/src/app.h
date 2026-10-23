#pragma once
#include <Arduino.h>
#include <atomic>
#include "astro.h"

#ifndef FW_VERSION          // the update-test build overrides it (see platformio.ini)
#define FW_VERSION "1.3"
#endif
#define HOSTNAME "skyframe"
#define SETUP_AP "SkyFrame-Setup"
// Where updates come from (GitHub "owner/repo") and the links shown in the settings page.
// Forks: point OTA_REPO at your own repository and create your own signing key (tools/keygen.py).
#define OTA_REPO "guilhermelimait/SkyFrame"
#define PROJECT_URL "https://github.com/" OTA_REPO
#define DONATE_URL "https://ko-fi.com/guilhermelimait"

enum ScreenId { S_HOME, S_FORECAST, S_HOURLY, S_SUN, S_MOON, S_SKY, S_PLANETS, S_SOLAR, S_EVENTS, S_ISS,
                S_CLOCK, S_CALENDAR, S_AIR, S_STARGAZE, S_COUNTDOWN, S_QUOTE, N_SCREENS,
                S_SETUP = 100, S_INFO, S_CONNECTING, S_UPDATE };
extern const char *SCREEN_KEYS[N_SCREENS];

struct WClock { char name[32]; double lat, lon; int32_t off; };
struct Config {
  char city[48], country[48], cc[4];
  double lat, lon;
  int32_t off;                 // city UTC offset in seconds (refreshed from the weather service)
  bool fahrenheit, h12;
  int rotate;                  // seconds, 0 = off
  int rotation;                // picture rotation, clockwise: 0, 90, 180 or 270
  bool autoRotate;             // follow the motion sensor (rotation is then set by the sensor, not saved)
  uint32_t screens;            // bit mask of ScreenId
  uint8_t order[N_SCREENS];    // display order of all screens (ScreenId values)
  WClock wc[3];
  char cdLabel[32], cdDate[20];   // "YYYY-MM-DDTHH:MM" local time at the city
  int bDay, bNight, nFrom, nTo;
  bool soundWake;              // at night, brighten while there is sound around
  int soundSense;              // 0 low, 1 medium, 2 high
  bool autoUpdate;             // install new releases automatically at night
};
extern Config CFG;
void cfgDefaults(Config &c);
void cfgLoad();
void cfgSave();
String cfgToJson(const Config &c);
bool cfgFromJson(const String &s, Config &c);

struct Weather {
  bool ok; int64_t fetched; int32_t off;
  float temp, feels, wind; int hum, code; bool isDay;
  int nd; struct { int64_t t; int code; float tmax, tmin; int pop; int64_t rise, set; } d[7];
  int nh; struct { int64_t t; float temp; int pop; } h[13];
  int nc; struct { int64_t t; int cloud; } c[26];   // cloud cover %, hourly from now (for stargazing)
};
struct Air { bool ok; int aqi; };
struct Iss {   // a ~95-minute track fetched at each sync; the live position is interpolated from it
  bool ok; float alt, vel;
  int nt; struct { float lat, lon; int64_t ts; } tr[20];
};
extern Weather WX;
extern Air AQ;
extern Iss ISS;
extern PlanetReport PLAN[5];
extern bool planOk;
extern SkyEvent EVENTS[6];
extern int nEvents;
extern SemaphoreHandle_t dataMx;
#define LOCK() xSemaphoreTake(dataMx, portMAX_DELAY)
#define UNLOCK() xSemaphoreGive(dataMx)

extern volatile bool wifiOk, setupMode, cfgDirty, timeOk, setupFailed;   // cfgDirty: settings need new data
extern volatile uint32_t cfgVersion;   // bumped on every settings change, so the screen redraws at once
extern std::atomic<uint32_t> dataVersion;    // bumped whenever new data arrives (screens redraw on change)
extern volatile int64_t lastSync;        // unix time of the last successful internet update
void wifiWake(uint32_t ms);              // keep Wi-Fi on for this long (settings page access)
#define SYNC_MINUTES 15
extern volatile int curScreen;
extern volatile int navRequest;    // set by the settings page: +1 next, -1 previous
extern volatile int showRequest;   // settings page: show this ScreenId now (-1 = none)
extern volatile bool holdScreen;   // settings page: stop changing screens
extern volatile uint32_t nextSyncAt;   // millis() of the next planned internet update
bool nightNow();
extern std::atomic<uint32_t> frameNo;   // counts pictures shown (the settings page refreshes its preview when it changes)
extern char wifiSsid[33];
extern char ipStr[16];
extern char pairPin[8];          // 6-digit code needed by the settings page (shown when you hold the screen)
extern char apPass[12];          // password of the first-time setup hotspot (shown on screen)
extern volatile bool otaForce;   // "Check for updates now" from the settings page
extern volatile bool radioOff;   // USB debug: keep Wi-Fi off (to check its effect on the screen)
extern volatile bool otaPeek;    // settings page opened: look for a new version without installing
void secLoad();
void secNewPin();

int64_t nowUtc();
int32_t cityOff();
void netStart();
void webStart();
void webLoop();

void drawScreen(int id, int dotIndex, int dotCount);
bool screenTap(int id, int x, int y);   // true if the screen used the tap itself
const uint16_t *previewFrame();          // the current screen, upright (for the settings preview / USB dump)
