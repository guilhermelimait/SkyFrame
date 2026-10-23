// Web server on the board: settings app (normal mode) or Wi-Fi picker with captive portal (setup mode).
// In normal mode every /api call needs the 6-digit pairing code (header X-Pin), shown on the display
// when you hold the screen. Five wrong codes lock the API for a minute.
#include "app.h"
#include "hw.h"
#include "net_secure.h"
#include "secure_store.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "web_pages.h"
#include "land_dots.h"

extern const uint8_t inter300[] asm("_binary_data_inter300_ttf_start");
extern const uint8_t inter300End[] asm("_binary_data_inter300_ttf_end");
extern const uint8_t inter400[] asm("_binary_data_inter400_ttf_start");
extern const uint8_t inter400End[] asm("_binary_data_inter400_ttf_end");
extern const uint8_t inter500[] asm("_binary_data_inter500_ttf_start");
extern const uint8_t inter500End[] asm("_binary_data_inter500_ttf_end");

static WebServer server(80);
static DNSServer dns;
static volatile bool started = false;
static uint32_t lockUntil = 0;
static int fails = 0;

static void sendJson(const String &s, int code = 200) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", s);
}

static bool authed() {
  if (setupMode) return true;   // the setup hotspot itself is password protected
  if ((int32_t)(lockUntil - millis()) > 0) { sendJson("{\"error\":\"locked\"}", 429); return false; }
  String pin = server.header("X-Pin");
  if (!pin.length()) { sendJson("{\"error\":\"pin\"}", 401); return false; }   // no code sent (e.g. an old open tab): not a guess
  uint8_t diff = pin.length() == 6 ? 0 : 1;
  for (int i = 0; i < 6; i++) diff |= (i < (int)pin.length() ? pin[i] : 0) ^ pairPin[i];   // constant-time compare
  if (diff) {
    if (++fails >= 5) { lockUntil = millis() + 60000; fails = 0; }
    sendJson("{\"error\":\"pin\"}", 401);
    return false;
  }
  fails = 0;
  wifiWake(5 * 60000);   // an open settings page keeps Wi-Fi on
  return true;
}

static void setupRoutes() {
  server.on("/", HTTP_GET, [] {
    wifiWake(5 * 60000);
    server.sendHeader("Cache-Control", "no-store");   // always the page of the firmware that is running now
    server.send(200, "text/html", setupMode ? SETUP_HTML : SETTINGS_HTML);
  });

  server.on("/api/scan", HTTP_GET, [] {
    if (!authed()) return;
    int n = WiFi.scanNetworks();
    JsonDocument d;
    JsonArray a = d.to<JsonArray>();
    for (int i = 0; i < n && i < 20; i++) {
      String s = WiFi.SSID(i);
      if (!s.length()) continue;
      bool dup = false;
      for (JsonObject o : a) if (o["ssid"] == s) dup = true;
      if (dup) continue;
      JsonObject o = a.add<JsonObject>();
      o["ssid"] = s; o["rssi"] = WiFi.RSSI(i); o["open"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
    }
    String out; serializeJson(d, out); sendJson(out);
  });

  server.on("/api/wifi", HTTP_POST, [] {
    if (!authed()) return;
    JsonDocument d;
    if (deserializeJson(d, server.arg("plain"))) return sendJson("{\"error\":\"bad json\"}", 400);
    wifiSave(String((const char *)(d["ssid"] | "")), String((const char *)(d["pass"] | "")));
    sendJson("{\"ok\":true}");
    delay(1500);
    ESP.restart();
  });

  server.on("/api/wifi/forget", HTTP_POST, [] {
    if (!authed()) return;
    Preferences p;
    p.begin("wifi", false);
    p.clear();
    p.end();
    sendJson("{\"ok\":true}");
    delay(1000);
    ESP.restart();
  });

  server.on("/api/auth", HTTP_GET, [] { if (authed()) sendJson("{\"ok\":true}"); });

  server.on("/api/pin/new", HTTP_POST, [] {
    if (!authed()) return;
    secNewPin();
    sendJson(String("{\"pin\":\"") + pairPin + "\"}");
  });

  server.on("/api/config", HTTP_GET, [] {
    if (!authed()) return;
    LOCK(); String s = cfgToJson(CFG); UNLOCK();
    sendJson(s);
  });
  server.on("/api/config", HTTP_POST, [] {
    if (!authed()) return;
    LOCK();
    Config c = CFG;
    bool ok = cfgFromJson(server.arg("plain"), c);
    bool needData = false;
    if (ok) {
      bool placeChanged = c.lat != CFG.lat || c.lon != CFG.lon || c.fahrenheit != CFG.fahrenheit;
      bool clocksChanged = false;
      for (int i = 0; i < 3; i++) clocksChanged |= c.wc[i].lat != CFG.wc[i].lat || c.wc[i].lon != CFG.wc[i].lon;
      needData = placeChanged || clocksChanged;
      CFG = c;
      if (placeChanged) { WX.ok = false; AQ.ok = false; }
    }
    String s = cfgToJson(CFG);
    UNLOCK();
    if (!ok) return sendJson("{\"error\":\"bad json\"}", 400);
    cfgSave();
    // only a new place, units or world-clock cities need fresh data: a full refresh is ~5 HTTPS downloads, and that
    // Wi-Fi traffic disturbs the screen, so rotation, interval, brightness or screen order just redraw
    if (needData) cfgDirty = true;
    cfgVersion = cfgVersion + 1;
    sendJson(s);
  });

  server.on("/api/status", HTTP_GET, [] {
    if (!authed()) return;
    JsonDocument d;
    d["ssid"] = wifiSsid; d["ip"] = ipStr; d["rssi"] = WiFi.RSSI(); d["version"] = FW_VERSION;
    d["uptime"] = millis() / 1000; d["heap"] = ESP.getFreeHeap(); d["psram"] = ESP.getFreePsram();
    d["update"] = otaStatus; d["updating"] = otaPercent; d["available"] = otaAvailable; d["phase"] = otaPhase;
    d["projectUrl"] = PROJECT_URL; d["donateUrl"] = DONATE_URL;
    d["current"] = curScreen >= 0 && curScreen < N_SCREENS ? SCREEN_KEYS[curScreen] : "";
    d["hold"] = (bool)holdScreen;
    d["frame"] = (uint32_t)frameNo;
    d["data"] = (uint32_t)dataVersion;
    d["night"] = nightNow();
    d["lastSyncAgo"] = lastSync ? (long)(nowUtc() - lastSync) : -1;
    d["nextSyncIn"] = nextSyncAt ? (long)((int32_t)(nextSyncAt - millis()) / 1000) : -1;
    d["pwState"] = wifiPasswordState();
    LOCK();
    if (WX.ok) d["weatherAgo"] = (long)(nowUtc() - WX.fetched); else d["weatherAgo"] = nullptr;
    d["syncMinutes"] = SYNC_MINUTES;
    UNLOCK();
    String out; serializeJson(d, out); sendJson(out);
  });

  server.on("/api/update/check", HTTP_POST, [] {
    if (!authed()) return;
    if (otaAvailable[0]) {   // installing a version we already know about: show the update screen right away
      strlcpy(otaNewVersion, otaAvailable, sizeof otaNewVersion);
      strlcpy(otaStatus, "Starting the update\xE2\x80\xA6", sizeof otaStatus);
      otaPhase = 1;
      otaPercent = 0;
    } else strlcpy(otaStatus, "Checking GitHub\xE2\x80\xA6", sizeof otaStatus);
    wifiWake(5 * 60000);
    otaForce = true;
    sendJson("{\"ok\":true}");
  });

  // cancel an update that is still checking or downloading (once flash is being written it can't be stopped)
  server.on("/api/update/cancel", HTTP_POST, [] {
    if (!authed()) return;
    bool can = otaPhase <= 1;
    if (can && otaForce) {            // asked for but not started yet
      otaForce = false; otaPercent = -1; otaPhase = 0;
      strlcpy(otaStatus, "Update cancelled", sizeof otaStatus);
    } else if (can && otaPercent >= 0) otaCancel = true;   // checking or downloading: stops at the next block
    sendJson(can ? "{\"ok\":true}" : "{\"ok\":false}");
  });

  // the settings page opened: look for a new version (at most every 10 minutes, GitHub limits anonymous requests)
  server.on("/api/update/peek", HTTP_POST, [] {
    if (!authed()) return;
    static uint32_t last = 0;
    if (!otaForce && (!last || millis() - last > 10 * 60000u)) {
      last = millis();
      strlcpy(otaStatus, "Checking GitHub\xE2\x80\xA6", sizeof otaStatus);
      otaPeek = true;
    }
    sendJson("{\"ok\":true}");
  });

  server.on("/api/nav", HTTP_POST, [] {
    if (!authed()) return;
    JsonDocument d;
    deserializeJson(d, server.arg("plain"));
    navRequest = (int)(d["dir"] | 1) < 0 ? -1 : 1;
    sendJson("{\"ok\":true}");
  });

  // Everything the screens show, so the settings page can draw them itself (instant and sharp)
  server.on("/api/data", HTTP_GET, [] {
    if (!authed()) return;
    JsonDocument d;
    LOCK();
    d["now"] = (long long)nowUtc(); d["off"] = cityOff(); d["timeOk"] = (bool)timeOk; d["lastSync"] = (long long)lastSync;
    d["city"]["name"] = CFG.city; d["city"]["lat"] = CFG.lat; d["city"]["lon"] = CFG.lon;
    d["h12"] = CFG.h12; d["f"] = CFG.fahrenheit;
    d["cdLabel"] = CFG.cdLabel; d["cdDate"] = CFG.cdDate;
    JsonArray wc = d["wc"].to<JsonArray>();
    for (auto &w : CFG.wc) { JsonObject o = wc.add<JsonObject>(); o["name"] = w.name; o["off"] = w.off; }
    if (WX.ok) {
      JsonObject w = d["wx"].to<JsonObject>();
      w["temp"] = WX.temp; w["feels"] = WX.feels; w["wind"] = WX.wind; w["hum"] = WX.hum; w["code"] = WX.code; w["isDay"] = WX.isDay;
      JsonArray dd = w["d"].to<JsonArray>();
      for (int i = 0; i < WX.nd; i++) { JsonArray a = dd.add<JsonArray>(); a.add((long long)WX.d[i].t); a.add(WX.d[i].code); a.add(WX.d[i].tmax); a.add(WX.d[i].tmin); a.add(WX.d[i].pop); a.add((long long)WX.d[i].rise); a.add((long long)WX.d[i].set); }
      JsonArray hh = w["h"].to<JsonArray>();
      for (int i = 0; i < WX.nh; i++) { JsonArray a = hh.add<JsonArray>(); a.add((long long)WX.h[i].t); a.add(WX.h[i].temp); a.add(WX.h[i].pop); }
      JsonArray cc = w["c"].to<JsonArray>();
      for (int i = 0; i < WX.nc; i++) { JsonArray a = cc.add<JsonArray>(); a.add((long long)WX.c[i].t); a.add(WX.c[i].cloud); }
    }
    if (AQ.ok) d["aqi"] = AQ.aqi;
    JsonObject iss = d["iss"].to<JsonObject>();
    iss["ok"] = ISS.ok; iss["alt"] = ISS.alt; iss["vel"] = ISS.vel;
    JsonArray tr = iss["tr"].to<JsonArray>();
    for (int i = 0; i < ISS.nt; i++) { JsonArray a = tr.add<JsonArray>(); a.add(ISS.tr[i].lat); a.add(ISS.tr[i].lon); a.add((long long)ISS.tr[i].ts); }
    if (planOk) {
      JsonArray pl = d["plan"].to<JsonArray>();
      for (auto &r : PLAN) { JsonArray a = pl.add<JsonArray>(); a.add(r.p); a.add(r.status); a.add(r.big); a.add(r.sub); a.add(r.kind); a.add(r.distAu); }
    }
    JsonArray ev = d["ev"].to<JsonArray>();
    for (int i = 0; i < nEvents; i++) { JsonArray a = ev.add<JsonArray>(); a.add((long long)EVENTS[i].t); a.add(EVENTS[i].type); a.add(EVENTS[i].title); a.add(EVENTS[i].sub); }
    d["data"] = (uint32_t)dataVersion;
    UNLOCK();
    String out; serializeJson(d, out); sendJson(out);
  });

  // the dotted world map for the Space Station globe (lat, lon in 1/100 degree)
  server.on("/api/land", HTTP_GET, [] {
    if (!authed()) return;
    String out; out.reserve(sizeof(LAND_DOTS) * 3);
    out = "[";
    for (size_t i = 0; i < sizeof(LAND_DOTS) / sizeof(LAND_DOTS[0]); i++) {
      if (i) out += ",";
      out += LAND_DOTS[i][0]; out += ","; out += LAND_DOTS[i][1];
    }
    out += "]";
    server.sendHeader("Cache-Control", "max-age=86400");
    server.send(200, "application/json", out);
  });

  server.on("/api/show", HTTP_POST, [] {
    if (!authed()) return;
    JsonDocument d;
    deserializeJson(d, server.arg("plain"));
    String k = d["screen"] | "";
    for (int i = 0; i < N_SCREENS; i++) if (k == SCREEN_KEYS[i]) showRequest = i;
    sendJson("{\"ok\":true}");
  });

  server.on("/api/hold", HTTP_POST, [] {
    if (!authed()) return;
    JsonDocument d;
    deserializeJson(d, server.arg("plain"));
    holdScreen = d["on"] | false;
    sendJson(holdScreen ? "{\"hold\":true}" : "{\"hold\":false}");
  });

  server.on("/api/screen", HTTP_GET, [] {
    if (!authed()) return;
    // half-size preview (240x240): a quarter of the data, so it barely disturbs the screen's memory
    const int step = server.hasArg("full") ? 1 : 2;   // ?full=1 for one sharp full-size picture
    const int S = LCD_W / step;
    const uint16_t *src = previewFrame();
    server.sendHeader("Cache-Control", "no-store");
    server.setContentLength(S * S * 2);
    server.send(200, "application/octet-stream", "");
    WiFiClient cl = server.client();
    static uint16_t row[LCD_W];
    for (int y = 0; y < S && cl.connected(); y++) {
      const uint16_t *r = src + (y * step) * LCD_W;
      for (int x = 0; x < S; x++) row[x] = r[x * step];
      cl.write((const uint8_t *)row, S * 2);
    }
  });

  // the display's font (Inter, SIL OFL 1.1) for the settings page; not secret, so no pairing code needed
  static const struct { const char *path; const uint8_t *start, *end; } FONTS[] = {
    {"/font/inter300.ttf", inter300, inter300End}, {"/font/inter400.ttf", inter400, inter400End}, {"/font/inter500.ttf", inter500, inter500End}};
  for (auto &f : FONTS) {
    const uint8_t *start = f.start, *end = f.end;
    server.on(f.path, HTTP_GET, [start, end] {
      server.sendHeader("Cache-Control", "public, max-age=31536000, immutable");
      server.send_P(200, "font/ttf", (PGM_P)start, end - start);
    });
  }

  // captive portal: send every unknown address to the setup page
  server.onNotFound([] {
    if (setupMode) {
      server.sendHeader("Location", String("http://") + ipStr + "/", true);
      server.send(302, "text/plain", "");
    } else server.send(404, "text/plain", "Not found");
  });
}

static void webTask(void *) {
  for (;;) {
    if (setupMode) dns.processNextRequest();
    server.handleClient();
    delay(2);
  }
}

void webStart() {
  if (started) return;
  started = true;
  if (setupMode) dns.start(53, "*", WiFi.softAPIP());
  setupRoutes();
  const char *hdrs[] = {"X-Pin"};
  server.collectHeaders(hdrs, 1);
  server.begin();
  xTaskCreatePinnedToCore(webTask, "web", 8192, nullptr, 1, nullptr, 0);
}
void webLoop() {}
