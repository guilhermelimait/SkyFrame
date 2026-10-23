// SkyFrame — standalone firmware for the Waveshare ESP32-S3-Touch-LCD-4B
// Tap right/left side (or swipe) to change screen, hold for 1 s to turn on Wi-Fi and see the settings address,
// swipe down from the top edge for quick settings (quick.cpp).
//
// The screen is redrawn only when something on it changes (usually once a minute). Moving to a neighbouring
// screen slides finished pictures (see "sliding between screens"); other jumps dip the backlight instead of
// blending frames. The RGB panel is fed from PSRAM, and heavy PSRAM traffic makes the picture slip.
#include "app.h"
#include "hw.h"
#include "gfx.h"
#include "util.h"
#include "net_secure.h"
#include "quick.h"
#include "mic.h"
#include "esp_heap_caps.h"

static int shown = -1;              // screen currently on the display
static int current = S_CONNECTING;  // screen we want to show
static int beforeInfo = S_HOME;
static uint32_t lastNav = 0, pausedUntil = 0, infoUntil = 0, lastBl = 0;
static uint32_t lastKey = 0;
static bool forceRedraw = true;
static bool everConnected = false;
static bool navAuto = false;         // the last screen change was the automatic one (it slides more gently)
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
    } else if (!strncmp(line, "quick ", 6)) {
      quickDebug(atoi(line + 6));
      Serial.print("OK quick\n");
    } else if (!strcmp(line, "mic on")) {
      Serial.printf("MIC %s\n", micStart() ? "on" : "failed");
    } else if (!strcmp(line, "mic off")) {
      micStop();
      Serial.print("MIC off\n");
    } else if (!strcmp(line, "mic")) {
      Serial.printf("MIC %.1f %.1f dB%s\n", micLevel(0), micLevel(1), micRunning() ? "" : " (off)");
    } else if (!strcmp(line, "i2c")) {
      hwI2cScan(Serial);
    } else if (!strcmp(line, "imu")) {
      float ax, ay, az;
      if (hwImu(ax, ay, az)) Serial.printf("IMU %.2f %.2f %.2f rotation %d\n", ax, ay, az, CFG.rotation); else Serial.print("IMU none\n");
    } else if (!strcmp(line, "resync")) {
      Serial.printf("RESYNC %d\n", hwResync());
    } else if (!strcmp(line, "update")) {
      wifiWake(10 * 60000);
      otaForce = true;
      Serial.print("OK update\n");
    } else if (!strcmp(line, "log")) {
      dbg = !dbg;
      quickLog = dbg;
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
  navAuto = !manual;
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
  mix(s); mix((uint32_t)(now / unit)); mix(dataVersion); mix(cfgVersion);
  mix(timeOk | (wifiOk << 1) | (setupMode << 2) | (setupFailed << 3));
  mix(s == S_CONNECTING ? millis() / 500 : 0);
  mix(s == S_UPDATE ? otaPercent + otaPhase * 1000 : 0);
  return h;
}

static void backlightRamp(int from, int to) {
  for (int i = 1; i <= 8; i++) { hwBacklight(from + (to - from) * i / 8); delay(14); }
}

/* ---- sliding between screens ----
   Three finished pictures are kept (upright): the current screen and its neighbours in the user's order. The
   neighbours are drawn ahead of time while nothing happens, so a finger drag or a slide only moves pictures
   that are already there (row copies, like the quick-settings panel) and never waits for drawing. */
static uint16_t *pic[3] = {};       // 0 the current screen, 1 the previous one, 2 the next one
static int picId[3] = {-1, -1, -1};
static uint32_t picKey[3], picT[3];
static bool dragging = false;
static int dragDir = 0;
static float dragF = 0;
// what is on the glass, upright: the panel or a slide while one is showing, otherwise the current screen's picture
const uint16_t *previewFrame() { return (quickActive() || dragging || !pic[0]) ? (CFG.rotation ? upright : hwFront()) : pic[0]; }

static void renderPic(int slot, int s) {
  int idx, count;
  dotInfo(s, idx, count);
  gTarget(pic[slot]);
  drawScreen(s, idx, count);
  picId[slot] = s; picKey[slot] = redrawKey(s); picT[slot] = millis();
}
static bool picFresh(int slot, int s) { return picId[slot] == s && picKey[slot] == redrawKey(s); }

static void showBuf(const uint16_t *src) {
  int rot = CFG.rotation;
  if (rot) rotateInto(src, hwBack(), rot); else memcpy(hwBack(), src, LCD_W * LCD_H * 2);
  hwPresent();
  frameNo++;
}
static inline void rowShift(uint16_t *dst, const uint16_t *src, int off) {
  if (off >= LCD_W || off <= -LCD_W) return;
  if (off >= 0) memcpy(dst + off, src, (LCD_W - off) * 2);
  else memcpy(dst, src - off, (LCD_W + off) * 2);
}
// the current picture moved by f of a screen, its neighbour coming in from the right (dir +1) or left (-1)
static void composeSlide(const uint16_t *a, const uint16_t *b, int dir, float f) {
  int rot = CFG.rotation;
  uint16_t *dst = rot ? upright : hwBack();
  int ax = (int)lroundf(-dir * f * LCD_W), bx = ax + dir * LCD_W;
  for (int y = 0; y < LCD_H; y++) {
    rowShift(dst + y * LCD_W, a + y * LCD_W, ax);
    rowShift(dst + y * LCD_W, b + y * LCD_W, bx);
  }
  if (rot) rotateInto(upright, hwBack(), rot);
  hwPresent();
  frameNo++;
}
static void slideAnim(int slot, int dir, float from, float to, int dur) {
  uint32_t t0 = millis();
  for (;;) {
    float t = min(1.f, (millis() - t0) / (float)dur);
    float e = 1 - (1 - t) * (1 - t) * (1 - t);   // ease out
    composeSlide(pic[0], pic[slot], dir, from + (to - from) * e);
    if (t >= 1) break;
  }
}
// the neighbour in slot (1 previous, 2 next) is now the current screen
static void commitSlide(int slot) {
  int other = slot == 1 ? 2 : 1;
  uint16_t *p0 = pic[0], *po = pic[other];
  int id0 = picId[0];
  uint32_t k0 = picKey[0], t0 = picT[0];
  pic[0] = pic[slot]; picId[0] = picId[slot]; picKey[0] = picKey[slot]; picT[0] = picT[slot];
  pic[other] = p0; picId[other] = id0; picKey[other] = k0; picT[other] = t0;   // the old screen is a neighbour now
  pic[slot] = po; picId[slot] = -1;                                           // the far side is drawn later
  current = shown = picId[0];
  lastKey = picKey[0];   // if it was drawn a while ago, the normal redraw refreshes it in place
}
// idle time: draw the neighbours ahead (again only if they changed and are 20 s old, so a ticking
// countdown next door isn't redrawn every second)
static void prerenderNeighbours() {
  int n = stepScreen(current, 1), p = stepScreen(current, -1);
  auto stale = [](int slot, int s) { return picId[slot] != s || (picKey[slot] != redrawKey(s) && millis() - picT[slot] > 20000); };
  if (n != current && stale(2, n)) { renderPic(2, n); return; }
  if (p != current && stale(1, p)) renderPic(1, p);
}

static void present(int s) {
  bool change = shown >= 0 && shown != s;
  uint32_t t0 = millis();
  if (change) backlightRamp(blLevel, 0);                    // a jump that isn't a neighbour: dip to black…
  renderPic(0, s);
  showBuf(pic[0]);
  hwResync();   // heavy drawing (rotation especially) can leave the panel out of step: restart it cleanly
  if (change) { delay(20); backlightRamp(0, blLevel); }     // …and back up on the new screen
  shown = s;
  if (dbg) Serial.printf("%lu draw screen %d %s %lu ms\n", millis(), s, change ? "change" : "update", millis() - t0);
}

bool nightNow() {
  if (!timeOk) return false;
  int h = toLocal(nowUtc(), cityOff()).h;
  return CFG.nFrom > CFG.nTo ? (h >= CFG.nFrom || h < CFG.nTo) : (h >= CFG.nFrom && h < CFG.nTo);
}

/* ---- auto-rotation from the motion sensor ----
   Gravity in the sensor's frame, measured on this board: +Y = upright (0°), −X = 90°, −Y = 180° (USB-C on the
   left), +X = 270°.
   The picture turns only after the new position has been steady for a second, never while it's lying flat
   (no clear "up") or close to 45°, and never while a finger is on the screen or the panel is open. */
static int sensedRotation() {
  float ax, ay, az;
  if (!hwImu(ax, ay, az)) return -1;
  float fx = fabsf(ax), fy = fabsf(ay);
  if (hypotf(ax, ay) < 0.45f) return -1;                              // lying (nearly) flat: keep what we have
  if (fx > fy * 1.3f) return ax < 0 ? 90 : 270;
  if (fy > fx * 1.3f) return ay > 0 ? 0 : 180;
  return -1;                                                          // near a diagonal
}
static void autoRotate(bool touching) {
  static uint32_t lastRead = 0, since = 0;
  static int candidate = -1;
  if (!CFG.autoRotate || !hwImuReady() || touching || dragging || quickActive() || millis() - lastRead < 200) return;
  lastRead = millis();
  int r = sensedRotation();
  if (r < 0 || r == CFG.rotation) { candidate = -1; return; }
  if (r != candidate) { candidate = r; since = millis(); return; }
  if (millis() - since < 700) return;
  candidate = -1;
  backlightRamp(blLevel, 0);
  LOCK(); CFG.rotation = r; UNLOCK();
  if (pic[0] && picId[0] == shown) showBuf(pic[0]); else present(current);
  hwResync();
  delay(20);
  backlightRamp(0, blLevel);
  if (dbg) Serial.printf("%lu auto-rotate -> %d\n", millis(), r);
}

/* ---- wake on sound ----
   At night (with the setting on) the microphone listens; while there is sound above the room's background,
   and for a minute after, the screen uses the day brightness instead of the night level. The microphone only
   runs while it's needed (or while the quick-settings sound page shows its meter). */
static uint32_t soundUntil = 0;
static bool soundAwake() { return (int32_t)(soundUntil - millis()) > 0; }
static void soundCheck() {
  static uint32_t lastTry = 0;
  bool need = everConnected && ((CFG.soundWake && nightNow()) || quickWantsMic()) && otaPercent < 0;
  if (need && !micRunning() && millis() - lastTry > 30000) { lastTry = millis(); if (!micStart() && dbg) Serial.print("mic: start failed\n"); }
  if (!need && micRunning()) micStop();
  if (CFG.soundWake && nightNow() && micHears(CFG.soundSense)) {
    if (!soundAwake() && dbg) Serial.printf("%lu sound: wake\n", millis());
    soundUntil = millis() + 60000;
  }
}

static void updateBacklight() {
  int pct = nightNow() && !soundAwake() ? CFG.bNight : CFG.bDay;
  pct = max(pct, 3);
  if (pct != blLevel) { backlightRamp(blLevel < 0 ? pct : blLevel, pct); blLevel = pct; }
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
  for (auto &p : pic) p = (uint16_t *)ps_malloc(LCD_W * LCD_H * 2);
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
  bool quick = everConnected && current < N_SCREENS && quickTouch(down, x, y);   // quick settings own this touch
  if (quick) { wasDown = down; longFired = true; }
  else if (down && !wasDown) { downX = x; downY = y; downT = millis(); longFired = false; if (dbg) Serial.printf("%lu touch down %d %d\n", millis(), x, y); }
  if (!quick && down) { lastX = x; lastY = y; }
  // a sideways drag on a screen: the screen follows the finger, its neighbour comes in from the side
  if (!quick && down && !dragging && !longFired && everConnected && current < N_SCREENS && current == shown && picId[0] == current &&
      abs(x - downX) > 14 && abs(x - downX) > abs(y - downY)) {
    int dir = x < downX ? 1 : -1, slot = dir > 0 ? 2 : 1, target = stepScreen(current, dir);
    if (target != current) {
      if (picId[slot] != target) renderPic(slot, target);
      dragging = true; dragDir = dir; dragF = 0;
    }
  }
  if (!quick && dragging && down) {
    dragF = constrain(-dragDir * (x - downX) / (float)LCD_W, 0.f, 1.f);
    composeSlide(pic[0], pic[dragDir > 0 ? 2 : 1], dragDir, dragF);
  }
  if (!quick && down && !dragging && !longFired && !setupMode && everConnected && millis() - downT > 1000 && abs(x - downX) < 25 && abs(y - downY) < 25) {
    longFired = true;
    if (current != S_INFO) beforeInfo = current;
    current = S_INFO;
    infoUntil = millis() + 60000;
    wifiWake(10 * 60000);                                    // settings page reachable for 10 minutes
  }
  if (!quick && !down && wasDown && !longFired) {
    int dx = lastX - downX, dy = lastY - downY;
    if (dbg) Serial.printf("%lu touch up %d %d (held %lu ms)\n", millis(), lastX, lastY, millis() - downT);
    if (dragging) {   // let go: finish the slide (past ~1.5 cm) or spring back
      int slot = dragDir > 0 ? 2 : 1;
      if (dragF * LCD_W > 60) {
        slideAnim(slot, dragDir, dragF, 1, 200);
        commitSlide(slot);
        lastNav = millis(); pausedUntil = millis() + 30000;
      } else slideAnim(slot, dragDir, dragF, 0, 180);
      dragging = false;
    } else if (current == S_INFO) current = beforeInfo;
    else if (current < N_SCREENS) {
      if (abs(dx) > 50 && abs(dx) > abs(dy)) nav(dx < 0 ? 1 : -1, true);
      else if (screenTap(current, downX, downY)) forceRedraw = true;
      else nav(downX < LCD_W / 3 ? -1 : 1, true);
    }
  }
  if (!quick) wasDown = down;

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
  if (current < N_SCREENS && !holdScreen && !quickActive() && CFG.rotate > 0 && millis() - lastNav > (uint32_t)CFG.rotate * 1000 && millis() > pausedUntil &&
      !down)
    nav(1, false);
  serialDebug();
  if (otaPercent >= 0) current = S_UPDATE;
  else if (current == S_UPDATE) current = stepScreen(-1, 1);
  if (forced >= 0) current = forced;
  curScreen = current;

  soundCheck();   // before the panel's early return: its sound page needs the microphone too

  // ---- quick settings: the panel draws itself while it's open ----
  if (quickActive()) {
    if (quickWantsFrame()) {
      int rot = CFG.rotation;
      quickCompose(rot ? upright : hwBack());
      if (rot) rotateInto(upright, hwBack(), rot);
      hwPresent();
      frameNo++;
    }
    if (quickActive()) { delay(2); return; }
  }
  if (quickJustClosed()) { forceRedraw = true; blLevel = -1; lastNav = millis(); }

  // ---- draw only when needed ----
  uint32_t key = redrawKey(current);
  if (dragging) {}
  else if (current != shown || key != lastKey || forceRedraw) {
    forceRedraw = false;
    int slot = 0;
    if (current != shown && shown >= 0 && shown < N_SCREENS && current < N_SCREENS && picId[0] == shown)
      slot = current == stepScreen(shown, 1) ? 2 : current == stepScreen(shown, -1) ? 1 : 0;
    if (slot) {   // a neighbouring screen: slide to it (gentler when it's the automatic change)
      int target = current;
      if (!picFresh(slot, target)) renderPic(slot, target);
      slideAnim(slot, slot == 2 ? 1 : -1, 0, 1, navAuto ? 520 : 260);
      commitSlide(slot);
      hwResync();
    } else {
      lastKey = key;
      present(current);
    }
  } else if (!down && current < N_SCREENS && current == shown && everConnected) prerenderNeighbours();

  autoRotate(down);
  static uint32_t lastSound = 0;   // brighten within ~0.1 s of a sound, not on the next 1 s tick
  if (everConnected && soundAwake() && millis() - lastSound > 100) { lastSound = millis(); updateBacklight(); }
  if (millis() - lastBl > 1000) { lastBl = millis(); hwResync(); if (everConnected) updateBacklight(); }   // self-heal any drift within a second
  delay(10);
}
