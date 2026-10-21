// Wi-Fi (or first-time setup hotspot), settings storage and all internet data fetching
#include "app.h"
#include "hw.h"
#include "util.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "net_secure.h"
#include "secure_store.h"
#include <Preferences.h>
#include <ArduinoJson.h>

const char *SCREEN_KEYS[N_SCREENS] = {"home", "forecast", "hourly", "sun", "moon", "sky", "planets", "solar", "events", "iss",
                                      "clock", "calendar", "air", "stargaze", "countdown", "quote"};
Config CFG;
Weather WX = {};
Air AQ = {};
Iss ISS = {};
PlanetReport PLAN[5];
bool planOk = false;
SkyEvent EVENTS[6];
int nEvents = 0;
SemaphoreHandle_t dataMx;
volatile bool wifiOk = false, setupMode = false, cfgDirty = true, timeOk = false, setupFailed = false;
volatile int curScreen = S_CONNECTING;
volatile int navRequest = 0;
volatile int showRequest = -1;
volatile bool holdScreen = false;
volatile uint32_t nextSyncAt = 0;
char wifiSsid[33] = "";
char ipStr[16] = "";

int64_t nowUtc() { return (int64_t)time(nullptr); }
int32_t cityOff() { return WX.ok ? WX.off : CFG.off; }

/* ---------------- config ---------------- */
void cfgDefaults(Config &c) {
  memset(&c, 0, sizeof c);
  strcpy(c.city, "London"); strcpy(c.country, "United Kingdom"); strcpy(c.cc, "GB");
  c.lat = 51.5085; c.lon = -0.1257; c.off = 3600;
  c.rotate = 15;
  c.screens = (1u << N_SCREENS) - 1;
  for (int i = 0; i < N_SCREENS; i++) c.order[i] = i;
  c.wc[0] = {"New York", 40.7143, -74.006, -4 * 3600};
  c.wc[1] = {"London", 51.5085, -0.1257, 3600};
  c.wc[2] = {"Tokyo", 35.6895, 139.6917, 9 * 3600};
  strcpy(c.cdLabel, "New Year"); strcpy(c.cdDate, "2027-01-01T00:00");
  c.bDay = 100; c.bNight = 25; c.nFrom = 23; c.nTo = 7;
  c.autoUpdate = true;
}

String cfgToJson(const Config &c) {
  JsonDocument d;
  JsonObject city = d["city"].to<JsonObject>();
  city["name"] = c.city; city["country"] = c.country; city["cc"] = c.cc; city["lat"] = c.lat; city["lon"] = c.lon;
  d["off"] = c.off;
  d["units"] = c.fahrenheit ? "f" : "c";
  d["h12"] = c.h12;
  d["rotate"] = c.rotate;
  d["rotation"] = c.rotation;
  JsonArray s = d["screens"].to<JsonArray>();
  JsonArray o = d["order"].to<JsonArray>();
  for (int i = 0; i < N_SCREENS; i++) {
    int k = c.order[i];
    o.add(SCREEN_KEYS[k]);
    if (c.screens & (1u << k)) s.add(SCREEN_KEYS[k]);
  }
  JsonArray w = d["worldClocks"].to<JsonArray>();
  for (auto &x : c.wc) { JsonObject o = w.add<JsonObject>(); o["city"] = x.name; o["lat"] = x.lat; o["lon"] = x.lon; o["off"] = x.off; }
  d["countdown"]["label"] = c.cdLabel; d["countdown"]["date"] = c.cdDate;
  JsonObject b = d["brightness"].to<JsonObject>();
  b["day"] = c.bDay; b["night"] = c.bNight; b["nightFrom"] = c.nFrom; b["nightTo"] = c.nTo;
  d["update"]["auto"] = c.autoUpdate;
  String out;
  serializeJson(d, out);
  return out;
}

bool cfgFromJson(const String &str, Config &c) {
  JsonDocument d;
  if (deserializeJson(d, str)) return false;
  if (d["city"].is<JsonObject>()) {
    strlcpy(c.city, d["city"]["name"] | c.city, sizeof c.city);
    strlcpy(c.country, d["city"]["country"] | "", sizeof c.country);
    strlcpy(c.cc, d["city"]["cc"] | "", sizeof c.cc);
    c.lat = d["city"]["lat"] | c.lat; c.lon = d["city"]["lon"] | c.lon;
  }
  c.off = d["off"] | c.off;
  if (d["units"].is<const char *>()) c.fahrenheit = String((const char *)d["units"]) == "f";
  c.h12 = d["h12"] | c.h12;
  c.rotate = constrain((int)(d["rotate"] | c.rotate), 0, 600);
  int rot = d["rotation"] | c.rotation;
  if (rot == 0 || rot == 90 || rot == 180 || rot == 270) c.rotation = rot;
  if (d["screens"].is<JsonArray>()) {
    uint32_t m = 0;
    for (JsonVariant v : d["screens"].as<JsonArray>()) {
      String k = v.as<String>();
      for (int i = 0; i < N_SCREENS; i++) if (k == SCREEN_KEYS[i]) m |= 1u << i;
      if (k == "timer") m |= 1u << S_STARGAZE;   // 1.0 focus timer -> 1.1 stargazing
    }
    c.screens = m ? m : 1;
  }
  if (d["order"].is<JsonArray>()) {
    uint8_t ord[N_SCREENS];
    int n = 0;
    uint32_t seen = 0;
    for (JsonVariant v : d["order"].as<JsonArray>()) {
      String k = v.as<String>();
      if (k == "timer") k = "stargaze";
      for (int i = 0; i < N_SCREENS; i++)
        if (k == SCREEN_KEYS[i] && !(seen & (1u << i)) && n < N_SCREENS) { ord[n++] = i; seen |= 1u << i; }
    }
    for (int i = 0; i < N_SCREENS; i++) if (!(seen & (1u << i))) ord[n++] = i;
    memcpy(c.order, ord, sizeof ord);
  }
  if (d["worldClocks"].is<JsonArray>()) {
    int i = 0;
    for (JsonObject o : d["worldClocks"].as<JsonArray>()) {
      if (i >= 3) break;
      strlcpy(c.wc[i].name, o["city"] | c.wc[i].name, sizeof c.wc[i].name);
      c.wc[i].lat = o["lat"] | c.wc[i].lat; c.wc[i].lon = o["lon"] | c.wc[i].lon; c.wc[i].off = o["off"] | c.wc[i].off;
      i++;
    }
  }
  if (d["countdown"].is<JsonObject>()) {
    strlcpy(c.cdLabel, d["countdown"]["label"] | c.cdLabel, sizeof c.cdLabel);
    strlcpy(c.cdDate, d["countdown"]["date"] | c.cdDate, sizeof c.cdDate);
  }
  if (d["brightness"].is<JsonObject>()) {
    c.bDay = constrain((int)(d["brightness"]["day"] | c.bDay), 0, 100);
    c.bNight = constrain((int)(d["brightness"]["night"] | c.bNight), 0, 100);
    c.nFrom = constrain((int)(d["brightness"]["nightFrom"] | c.nFrom), 0, 23);
    c.nTo = constrain((int)(d["brightness"]["nightTo"] | c.nTo), 0, 23);
  }
  if (d["update"].is<JsonObject>()) c.autoUpdate = d["update"]["auto"] | c.autoUpdate;
  return true;
}

/* ---------------- pairing code & setup hotspot password ---------------- */
char pairPin[8] = "";
char apPass[12] = "";
volatile bool otaForce = false;
volatile bool otaPeek = false;
void secNewPin() {
  snprintf(pairPin, sizeof pairPin, "%06lu", (unsigned long)(esp_random() % 1000000));
  hwFlashBegin();
  Preferences p;
  p.begin("sec", false);
  p.putString("pin", pairPin);
  p.end();
  hwFlashEnd();
}
void secLoad() {
  Preferences p;
  p.begin("sec", true);
  String s = p.getString("pin", "");
  p.end();
  if (s.length() == 6) strlcpy(pairPin, s.c_str(), sizeof pairPin);
  else secNewPin();
}

void cfgLoad() {
  cfgDefaults(CFG);
  Preferences p;
  p.begin("desk", true);
  String s = p.getString("cfg", "");
  p.end();
  if (s.length()) cfgFromJson(s, CFG);
}
void cfgSave() {
  String js = cfgToJson(CFG);
  hwFlashBegin();
  Preferences p;
  p.begin("desk", false);
  p.putString("cfg", js);
  p.end();
  hwFlashEnd();
}

/* ---------------- HTTP helper ---------------- */
static bool getJson(const String &url, JsonDocument &doc) {
  NetworkClientSecure cli;
  secureClient(cli);   // check the server certificate
  HTTPClient http;
  http.setTimeout(12000);
  http.setConnectTimeout(8000);
  if (!http.begin(cli, url)) return false;
  int code = http.GET();
  bool ok = false;
  if (code == 200) ok = !deserializeJson(doc, http.getString());
  http.end();
  return ok;
}

/* ---------------- data fetchers ---------------- */
static void fetchWeather(const Config &c) {
  char url[640];
  snprintf(url, sizeof url,
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,apparent_temperature,relative_humidity_2m,wind_speed_10m,weather_code,is_day"
           "&hourly=temperature_2m,precipitation_probability,cloud_cover"
           "&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset,precipitation_probability_max"
           "&timezone=auto&timeformat=unixtime&forecast_days=7&past_hours=1&forecast_hours=27%s",
           c.lat, c.lon, c.fahrenheit ? "&temperature_unit=fahrenheit&wind_speed_unit=mph" : "");
  JsonDocument d;
  if (!getJson(url, d)) return;
  Weather w = {};
  w.ok = true; w.fetched = nowUtc();
  w.off = d["utc_offset_seconds"] | 0;
  JsonObject cur = d["current"];
  w.temp = cur["temperature_2m"]; w.feels = cur["apparent_temperature"]; w.hum = cur["relative_humidity_2m"];
  w.wind = cur["wind_speed_10m"]; w.code = cur["weather_code"]; w.isDay = (int)cur["is_day"] == 1;
  JsonObject dl = d["daily"];
  w.nd = min(7, (int)dl["time"].size());
  for (int i = 0; i < w.nd; i++) {
    w.d[i].t = dl["time"][i]; w.d[i].code = dl["weather_code"][i];
    w.d[i].tmax = dl["temperature_2m_max"][i]; w.d[i].tmin = dl["temperature_2m_min"][i];
    w.d[i].pop = dl["precipitation_probability_max"][i] | 0;
    w.d[i].rise = dl["sunrise"][i]; w.d[i].set = dl["sunset"][i];
  }
  JsonObject hr = d["hourly"];
  int n = hr["time"].size(), start = 0;
  int64_t now = nowUtc();
  for (int i = 0; i < n; i++) if ((int64_t)hr["time"][i] <= now) start = i;
  w.nh = 0;
  for (int i = start; i < n && w.nh < 13; i++, w.nh++) {
    w.h[w.nh].t = hr["time"][i]; w.h[w.nh].temp = hr["temperature_2m"][i]; w.h[w.nh].pop = hr["precipitation_probability"][i] | 0;
  }
  w.nc = 0;
  for (int i = start; i < n && w.nc < 26; i++, w.nc++) { w.c[w.nc].t = hr["time"][i]; w.c[w.nc].cloud = hr["cloud_cover"][i] | -1; }
  LOCK(); WX = w; CFG.off = w.off; UNLOCK();
}

static void fetchAir(const Config &c) {
  char url[200];
  snprintf(url, sizeof url, "https://air-quality-api.open-meteo.com/v1/air-quality?latitude=%.4f&longitude=%.4f&current=us_aqi&timezone=auto", c.lat, c.lon);
  JsonDocument d;
  if (!getJson(url, d) || d["current"]["us_aqi"].isNull()) return;
  LOCK(); AQ.ok = true; AQ.aqi = d["current"]["us_aqi"]; UNLOCK();
}

static void fetchWorldOffsets(const Config &c) {
  char url[300];
  snprintf(url, sizeof url, "https://api.open-meteo.com/v1/forecast?latitude=%.4f,%.4f,%.4f&longitude=%.4f,%.4f,%.4f&current=is_day&timezone=auto&forecast_days=1",
           c.wc[0].lat, c.wc[1].lat, c.wc[2].lat, c.wc[0].lon, c.wc[1].lon, c.wc[2].lon);
  JsonDocument d;
  if (!getJson(url, d) || !d.is<JsonArray>()) return;
  LOCK();
  for (int i = 0; i < 3 && i < (int)d.size(); i++) CFG.wc[i].off = d[i]["utc_offset_seconds"] | CFG.wc[i].off;
  UNLOCK();
}

static void fetchTrack() {
  // 20 points, 5 minutes apart, from 10 minutes ago to 85 minutes ahead (two requests of 10)
  int64_t now = nowUtc();
  Iss iss = {};
  for (int part = 0; part < 2; part++) {
    String ts;
    for (int i = 0; i < 10; i++) { if (i) ts += ","; ts += String((long)(now + (part * 10 + i - 2) * 300)); }
    JsonDocument d;
    if (!getJson("https://api.wheretheiss.at/v1/satellites/25544/positions?timestamps=" + ts + "&units=kilometers", d) || !d.is<JsonArray>()) return;
    for (JsonObject o : d.as<JsonArray>()) {
      if (iss.nt >= 20) break;
      iss.tr[iss.nt].lat = o["latitude"]; iss.tr[iss.nt].lon = o["longitude"]; iss.tr[iss.nt].ts = o["timestamp"];
      if (part == 0 && iss.nt == 2) { iss.alt = o["altitude"]; iss.vel = o["velocity"]; }
      iss.nt++;
    }
    delay(1100);   // the API allows about one request per second
  }
  iss.ok = iss.nt >= 2;
  LOCK(); ISS = iss; UNLOCK();
}

/* ---------------- Wi-Fi: connect for a sync every 15 minutes, otherwise off ---------------- */
std::atomic<uint32_t> dataVersion{0};
volatile int64_t lastSync = 0;
static volatile uint32_t awakeUntil = 0;
void wifiWake(uint32_t ms) {
  uint32_t t = millis() + ms;
  if ((int32_t)(t - awakeUntil) > 0) awakeUntil = t;
}

static void startSetupAp() {
  WiFi.persistent(false);   // keep the Wi-Fi driver from saving its own copy of passwords in flash
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP_STA);
  // a fresh random password each time; it is shown on the screen and in its QR code
  const char *A = "abcdefghjkmnpqrstuvwxyz23456789";
  for (int i = 0; i < 8; i++) apPass[i] = A[esp_random() % 31];
  apPass[8] = 0;
  WiFi.softAP(SETUP_AP, apPass);
  strlcpy(ipStr, WiFi.softAPIP().toString().c_str(), sizeof ipStr);
  setupMode = true;
  webStart();
}

static bool connectWifi(const String &ssid, const String &pass, uint32_t timeoutMs) {
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.persistent(false);   // don't rewrite flash on every connect (flash writes stall the screen feed)
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(ssid.c_str(), pass.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) delay(200);
  if (WiFi.status() != WL_CONNECTED) { WiFi.disconnect(true); WiFi.mode(WIFI_OFF); return false; }
  strlcpy(ipStr, WiFi.localIP().toString().c_str(), sizeof ipStr);
  MDNS.end();
  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);
  configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");   // restarts SNTP → fresh time
  return true;
}

static void disconnectWifi() {
  MDNS.end();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiOk = false;
}

static void netTask(void *) {
  wifiMigrateAtBoot();   // encrypt a plaintext password left from first setup, and wipe the old bytes
  String ssid, pass;
  bool plainPass = false;
  if (!wifiLoad(ssid, pass, plainPass)) ssid = "";   // can't decrypt (shouldn't happen): ask for Wi-Fi again
  strlcpy(wifiSsid, ssid.c_str(), sizeof wifiSsid);
  if (!ssid.length()) startSetupAp();
  else if (!connectWifi(ssid, pass, 25000)) { setupFailed = true; startSetupAp(); }
  else {
    wifiOk = true;
    if (plainPass && ssCreateKey()) {
      // first connection after setup: the radio is on, so the chip can make its key now; store the password
      // encrypted, then restart so the old plaintext bytes are wiped before Wi-Fi starts again
      Serial.println("secure: chip key created");
      wifiSave(ssid, pass);
      Preferences f;
      f.begin("wifi", false);
      f.putBool("wipe", true);
      f.end();
      delay(300);
      ESP.restart();
    }
    webStart();
    wifiWake(10 * 60000);
  }

  uint32_t lastSyncMs = 0, lastPlan = 0, lastEv = 0, lastOta = 0;
  bool pendingVerify = otaPendingVerify();
  bool synced = false;
  for (;;) {
    uint32_t ms = millis();
    bool dirty = cfgDirty;
    if (dirty) cfgDirty = false;
    Config c;
    LOCK(); c = CFG; UNLOCK();

    if (!setupMode) {
      bool due = !synced || dirty || otaForce || ms - lastSyncMs > SYNC_MINUTES * 60000u;
      nextSyncAt = lastSyncMs + SYNC_MINUTES * 60000u;
      bool awake = (int32_t)(awakeUntil - ms) > 0;
      if (due || awake) wifiOk = connectWifi(ssid, pass, 20000);
      else wifiOk = false;
      if (wifiOk && due) {
        for (int i = 0; i < 50 && time(nullptr) < 1700000000; i++) delay(200);   // wait for NTP
        if (time(nullptr) > 1700000000) {
          timeOk = true;
          fetchWeather(c);
          fetchAir(c);
          fetchWorldOffsets(c);
          fetchTrack();
          dataVersion++;   // one redraw once everything has arrived
          synced = true;
          lastSyncMs = ms;
          lastSync = nowUtc();
          otaMarkGood();   // this firmware works: tell the bootloader to keep it
          pendingVerify = false;
        }
      } else if (due && !wifiOk) lastSyncMs = ms - (SYNC_MINUTES - 2) * 60000u;   // retry in 2 minutes
      // firmware updates: when asked from the settings page, or once a day between 3 and 5 at night
      if (wifiOk && timeOk && synced) {
        int h = toLocal(nowUtc(), cityOff()).h;
        bool nightly = c.autoUpdate && h >= 3 && h < 5 && (!lastOta || ms - lastOta > 20 * 3600000u);
        if (otaForce || nightly) {
          otaForce = false;
          otaPeek = false;
          lastOta = ms;
          if (otaCheckAndInstall(OTA_REPO)) { delay(1500); ESP.restart(); }
          otaPercent = -1;
          dataVersion++;
        } else if (otaPeek) {
          otaPeek = false;
          otaCheckAndInstall(OTA_REPO, false);
          otaPercent = -1;
          dataVersion++;
        }
      }
      if (wifiOk && !awake && synced && !otaForce) disconnectWifi();
    }

    // a freshly installed version that never manages an update within 10 minutes is treated as broken
    if (pendingVerify && ms > 10 * 60000) otaRollbackNow();
    if (!timeOk && time(nullptr) > 1700000000) timeOk = true;
    if (timeOk) {
      int32_t off = cityOff();
      if (dirty || !planOk || ms - lastPlan > 10 * 60000) {
        PlanetReport r[5];
        planetReport((double)nowUtc(), c.lat, c.lon, off, c.h12, r);
        LOCK(); memcpy(PLAN, r, sizeof r); planOk = true; UNLOCK();
        lastPlan = ms; dataVersion++;
      }
      if (dirty || nEvents == 0 || ms - lastEv > 60 * 60000) {
        static SkyEvent ev[6];
        int n = buildEvents((double)nowUtc(), off, c.lat, ev, 6);
        LOCK(); memcpy(EVENTS, ev, sizeof ev); nEvents = n; UNLOCK();
        lastEv = ms; dataVersion++;
      }
    }
    if (dirty) dataVersion++;
    delay(500);
  }
}

void netStart() {
  dataMx = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(netTask, "net", 16384, nullptr, 1, nullptr, 0);
}
