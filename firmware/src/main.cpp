// SkyFrame — standalone firmware for the Waveshare ESP32-S3-Touch-LCD-4B
// Tap right/left side (or swipe) to change screen, hold for 1 s to turn on Wi-Fi and see the settings address.
//
// The screen is redrawn only when something on it changes (usually once a minute), and screen changes
// dip the backlight instead of blending frames — the RGB panel is fed from PSRAM, and heavy PSRAM traffic
// makes the picture slip.
#include "app.h"
#include "hw.h"
#include "gfx.h"
#include "util.h"
#include "net_secure.h"
#include "esp_heap_caps.h"

static int shown = -1;              // screen currently on the display
static int current = S_CONNECTING;  // screen we want to show
static int beforeInfo = S_HOME;
static uint32_t lastNav = 0, pausedUntil = 0, infoUntil = 0, lastBl = 0;
static uint32_t lastKey = 0;
static bool forceRedraw = true;
static bool everConnected = false;
static int blLevel = 80;            // current backlight %
std::atomic<uint32_t> frameNo{0};
static uint16_t *upright = nullptr; // screens are drawn here first when the picture is rotated

// The ST7701S can't rotate in hardware, so a rotated picture is turned in software after drawing.
// Mapping (clockwise): 90: (x,y)->(W-1-y, x)   180: (x,y)->(W-1-x, H-1-y)   270: (x,y)->(y, H-1-x)
static void rotateInto(const uint16_t *src, uint16_t *dst, int rot) {
  const int N = LCD_W;   // square panel
  if (rot == 180) { for (int i = 0, j = N * N - 1; i < N * N; i++, j--) dst[j] = src[i]; return; }
  for (int by = 0; by < N; by += 16)        // 16x16 tiles keep PSRAM access cache-friendly
    for (int bx = 0; bx < N; bx += 16)
      for (int y = by; y < by + 16; y++) {
        const uint16_t *s = src + y * N;
        for (int x = bx; x < bx + 16; x++) {
          if (rot == 90) dst[x * N + (N - 1 - y)] = s[x];
          else dst[(N - 1 - x) * N + y] = s[x];
        }
      }
}
// touch arrives in panel coordinates; turn it back into the upright picture's coordinates
static void touchToUpright(int &x, int &y) {
  int tx = x, ty = y, N = LCD_W;
  switch (CFG.rotation) {
    case 90: x = ty; y = N - 1 - tx; break;
    case 180: x = N - 1 - tx; y = N - 1 - ty; break;
    case 270: x = N - 1 - ty; y = tx; break;
  }
}
const uint16_t *previewFrame() { return CFG.rotation ? upright : hwFront(); }

/* ---- USB debug channel (used to check the screens without Wi-Fi) ----
   "dump"        -> header line "FB480", then the raw RGB565 frame
   "t <unix>"    -> set the clock
   "s <id>"      -> force a screen (-1 to release)
   "demo"        -> load sample weather / air / ISS data
   "log"         -> toggle event logging
   "update"      -> check GitHub for a new version now and install it (progress over USB) */
static int forced = -1;
static bool dbg = false;
static void loadDemo() {
  LOCK();
  int64_t now = nowUtc();
  WX = {}; WX.ok = true; WX.fetched = now; WX.off = CFG.off;
  WX.temp = 15; WX.feels = 13; WX.wind = 12; WX.hum = 68; WX.code = 2; WX.isDay = true;
  const int codes[7] = {2, 61, 3, 0, 1, 80, 2};
  int64_t day0 = (now + WX.off) / 86400 * 86400 - WX.off;
  WX.nd = 7;
  for (int i = 0; i < 7; i++) {
    WX.d[i].t = day0 + i * 86400; WX.d[i].code = codes[i]; WX.d[i].tmax = 16 + (i % 3) * 2; WX.d[i].tmin = 8 + (i % 4);
    WX.d[i].pop = i == 1 ? 80 : i == 5 ? 45 : 5; WX.d[i].rise = WX.d[i].t + 7 * 3600 + 30 * 60; WX.d[i].set = WX.d[i].t + 18 * 3600;
  }
  WX.nh = 13;
  for (int k = 0; k < 13; k++) { WX.h[k].t = now / 3600 * 3600 + k * 3600; WX.h[k].temp = 15 + 2 * sinf(k / 3.f) - k * .3f; WX.h[k].pop = k > 6 ? (k - 6) * 9 : 0; }
  WX.nc = 26;
  for (int k = 0; k < 26; k++) { WX.c[k].t = now / 3600 * 3600 + k * 3600; WX.c[k].cloud = k < 8 ? 70 - k * 8 : (k < 15 ? 8 : 40); }
  AQ.ok = true; AQ.aqi = 42;
  ISS = {}; ISS.ok = true; ISS.alt = 418; ISS.vel = 27580; ISS.nt = 20;
  for (int i = 0; i < 20; i++) { ISS.tr[i].ts = now + (i - 2) * 300; ISS.tr[i].lat = 51.6f * sinf((i - 2) * 0.34f + 0.62f); ISS.tr[i].lon = fmodf(10 + (i - 2) * 19 + 540, 360) - 180; }
  lastSync = now;
  dataVersion++;
  UNLOCK();
}
static void serialDebug() {
  static char line[256];
  static int n = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '\n' && c != '\r') { if (n < 255) line[n++] = c; continue; }
    line[n] = 0; n = 0;
    if (!strcmp(line, "dump")) {
      Serial.print("FB480\n");
      const uint8_t *p = (const uint8_t *)previewFrame();
      for (int i = 0; i < LCD_W * LCD_H * 2; i += 4096) Serial.write(p + i, min(4096, LCD_W * LCD_H * 2 - i));
    } else if (!strncmp(line, "t ", 2)) {
      struct timeval tv = {(time_t)atoll(line + 2), 0};
      settimeofday(&tv, nullptr);
      timeOk = true;
      forceRedraw = true;
      Serial.print("OK time\n");
    } else if (!strncmp(line, "s ", 2)) {
      forced = atoi(line + 2);
      Serial.print("OK screen\n");
    } else if (!strncmp(line, "sigtest ", 8)) {
      char *sp = strchr(line + 8, ' ');
            // "sigtest <hex-signature> <message>"
      if (sp) *sp = 0;
      bool ok = sp && otaSelfTest(sp + 1, line + 8);
      Serial.printf("SIGTEST %s\n", ok ? "PASS" : "FAIL");
    } else if (!strncmp(line, "radio ", 6)) {
      radioOff = line[6] == '0';
      Serial.printf("OK radio %s\n", radioOff ? "off" : "on");
    } else if (!strcmp(line, "resync")) {
      Serial.printf("RESYNC %d\n", hwResync());
    } else if (!strcmp(line, "update")) {
      wifiWake(10 * 60000);
      otaForce = true;
      Serial.print("OK update\n");
    } else if (!strcmp(line, "log")) {
      dbg = !dbg;
      Serial.printf("OK log %d\n", dbg);
    } else if (!strcmp(line, "demo")) {
      loadDemo();
      Serial.print("OK demo\n");
    }
  }
}

static bool enabled(int s) { return s < N_SCREENS && (CFG.screens & (1u << s)); }
static int posOf(int s) { for (int i = 0; i < N_SCREENS; i++) if (CFG.order[i] == s) return i; return -1; }
// next enabled screen in the user's order; from = -1 gives the first one
static int stepScreen(int from, int dir) {
  int p = from >= 0 && from < N_SCREENS ? posOf(from) : (dir > 0 ? -1 : N_SCREENS);
  for (int i = 0; i < N_SCREENS; i++) {
    p = (p + dir + N_SCREENS) % N_SCREENS;
    if (enabled(CFG.order[p])) return CFG.order[p];
  }
  return S_HOME;
}
static void dotInfo(int s, int &idx, int &count) {
  idx = -1; count = 0;
  for (int i = 0; i < N_SCREENS; i++) { int k = CFG.order[i]; if (enabled(k)) { if (k == s) idx = count; count++; } }
}
static void nav(int dir, bool manual) {
  if (current >= N_SCREENS) return;
  current = stepScreen(current, dir);
  lastNav = millis();
  if (manual) pausedUntil = millis() + 30000;
  if (dbg) Serial.printf("%lu nav %+d %s -> %d\n", millis(), dir, manual ? "manual" : "auto", current);
}

// What the screen depends on: when this value changes, it is redrawn.
static uint32_t redrawKey(int s) {
  int64_t now = nowUtc();
  int unit = 60;                                              // most screens: once a minute
  if (s == S_COUNTDOWN) unit = 1;
  if (s == S_ISS) unit = 10;
  if (s == S_CONNECTING) unit = 1;
  uint32_t h = 2166136261u;
  auto mix = [&](uint32_t v) { h = (h ^ v) * 16777619u; };
  mix(s); mix((uint32_t)(now / unit)); mix(dataVersion);
  mix(timeOk | (wifiOk << 1) | (setupMode << 2) | (setupFailed << 3));
  mix(s == S_CONNECTING ? millis() / 500 : 0);
  mix(s == S_UPDATE ? otaPercent + otaPhase * 1000 : 0);
  return h;
}

static void backlightRamp(int from, int to) {
  for (int i = 1; i <= 8; i++) { hwBacklight(from + (to - from) * i / 8); delay(14); }
}

static void present(int s) {
  bool change = shown >= 0 && shown != s;
  uint32_t t0 = millis();
  int idx, count;
  dotInfo(s, idx, count);
  if (change) backlightRamp(blLevel, 0);                    // dip to black…
  int rot = CFG.rotation;
  gTarget(rot ? upright : hwBack());
  drawScreen(s, idx, count);
  if (rot) rotateInto(upright, hwBack(), rot);
  hwPresent();
  hwResync();   // heavy drawing (rotation especially) can leave the panel out of step: restart it cleanly
  frameNo++;
  if (change) { delay(20); backlightRamp(0, blLevel); }     // …and back up on the new screen
  shown = s;
  if (dbg) Serial.printf("%lu draw screen %d %s %lu ms\n", millis(), s, change ? "change" : "update", millis() - t0);
}

bool nightNow() {
  if (!timeOk) return false;
  int h = toLocal(nowUtc(), cityOff()).h;
  return CFG.nFrom > CFG.nTo ? (h >= CFG.nFrom || h < CFG.nTo) : (h >= CFG.nFrom && h < CFG.nTo);
}

static void updateBacklight() {
  int pct = nightNow() ? CFG.bNight : CFG.bDay;
  pct = max(pct, 3);
  if (pct != blLevel) { blLevel = pct; hwBacklight(blLevel); }
}

void setup() {
  // Blocks up to 32 KB (TLS and JSON buffers) stay in internal RAM, falling back to PSRAM only if it's full:
  // the panel is fed from PSRAM, and network traffic there makes the picture glitch.
  heap_caps_malloc_extmem_enable(32 * 1024);
  Serial.begin(115200);
  hwInit();
  gInit();
  cfgLoad();
  secLoad();
  upright = (uint16_t *)ps_malloc(LCD_W * LCD_H * 2);
  if (!upright) CFG.rotation = 0;
  netStart();
  present(S_CONNECTING);
  hwBacklight(blLevel);
}

void loop() {
  // ---- touch gestures ----
  static bool wasDown = false, longFired = false;
  static int downX, downY, lastX, lastY;
  static uint32_t downT;
  int x, y;
  bool down = hwTouch(x, y);
  touchToUpright(x, y);
  if (down && !wasDown) { downX = x; downY = y; downT = millis(); longFired = false; if (dbg) Serial.printf("%lu touch down %d %d\n", millis(), x, y); }
  if (down) { lastX = x; lastY = y; }
  if (down && !longFired && !setupMode && everConnected && millis() - downT > 1000 && abs(x - downX) < 25 && abs(y - downY) < 25) {
    longFired = true;
    if (current != S_INFO) beforeInfo = current;
    current = S_INFO;
    infoUntil = millis() + 60000;
    wifiWake(10 * 60000);                                    // settings page reachable for 10 minutes
  }
  if (!down && wasDown && !longFired) {
    int dx = lastX - downX, dy = lastY - downY;
    if (dbg) Serial.printf("%lu touch up %d %d (held %lu ms)\n", millis(), lastX, lastY, millis() - downT);
    if (current == S_INFO) current = beforeInfo;
    else if (current < N_SCREENS) {
      if (abs(dx) > 50 && abs(dx) > abs(dy)) nav(dx < 0 ? 1 : -1, true);
      else if (screenTap(current, downX, downY)) forceRedraw = true;
      else nav(downX < LCD_W / 3 ? -1 : 1, true);
    }
  }
  wasDown = down;

  // ---- what to show ----
  if (setupMode) current = S_SETUP;
  else if (!everConnected && (wifiOk || lastSync)) { everConnected = true; current = stepScreen(-1, 1); lastNav = millis(); }
  else if (!everConnected) current = S_CONNECTING;
  if (navRequest) { int d = navRequest; navRequest = 0; if (current == S_INFO) current = beforeInfo; nav(d, true); }
  if (showRequest >= 0 && everConnected && !setupMode) {   // "show this screen now" from the settings page
    current = showRequest; showRequest = -1;
    lastNav = millis(); pausedUntil = millis() + 30000;
  }
  if (current == S_INFO && millis() > infoUntil) current = beforeInfo;
  if (current < N_SCREENS && !enabled(current) && millis() > pausedUntil) current = stepScreen(current, 1);
  if (current < N_SCREENS && !holdScreen && CFG.rotate > 0 && millis() - lastNav > (uint32_t)CFG.rotate * 1000 && millis() > pausedUntil &&
      !down)
    nav(1, false);
  serialDebug();
  if (otaPercent >= 0) current = S_UPDATE;
  else if (current == S_UPDATE) current = stepScreen(-1, 1);
  if (forced >= 0) current = forced;
  curScreen = current;

  // ---- draw only when needed ----
  uint32_t key = redrawKey(current);
  if (current != shown || key != lastKey || forceRedraw) {
    forceRedraw = false;
    lastKey = key;
    present(current);
  }

  if (millis() - lastBl > 1000) { lastBl = millis(); hwResync(); if (everConnected) updateBacklight(); }   // self-heal any drift within a second
  delay(10);
}
