// Quick settings: swipe down from the top edge of any screen.
//
// A menu of four tiles (brightness, clock, night mode, screen time); tapping one slides its page in from the right,
// swiping left/right moves between pages, ‹ or a swipe down goes back, a swipe up on the menu closes the panel.
//
// Smooth on this panel: every page is drawn once into its own PSRAM picture, and the animations only slide those
// finished pictures (row copies), so nothing is re-rendered per frame. Settings are saved once, when the panel
// closes, because writing flash disturbs the screen.
#include "quick.h"
#include "app.h"
#include "hw.h"
#include "gfx.h"
#include "util.h"
#include "net_secure.h"
#include "mic.h"

extern bool quickLog;
namespace {

constexpr int N = LCD_W;            // square panel
constexpr int PAGES = 5;
constexpr int MENU = -1, NONE = -2;
enum { P_LIGHT, P_NIGHT, P_SOUND, P_CLOCK, P_TIME };   // light settings next to each other
const char *TITLES[PAGES] = {"BRIGHTNESS", "NIGHT MODE", "WAKE ON SOUND", "CLOCK", "SCREEN TIME"};
const char *NAMES[PAGES] = {"Brightness", "Night mode", "Wake on sound", "Clock", "Screen time"};
const int TIMES[7] = {0, 10, 15, 30, 60, 120, 300};

enum Mode { Q_OFF, Q_ON };
Mode mode = Q_OFF;
bool closedFlag = false;

// pictures: what was on screen when the panel opened, the menu, and two page slots
uint16_t *base = nullptr, *menuBuf = nullptr, *pageBuf[PAGES] = {};
bool menuDirty = true, pageDirty[PAGES] = {true, true, true, true, true};

// what is shown: a vertical offset (panel over the old screen) and a horizontal slide between two pictures
float oy = -N;                      // -N closed, 0 open
int viewA = MENU, viewB = NONE;     // sliding from A to B
float slide = 0;                    // 0 = A shown, 1 = B shown
int slideDir = 1;                   // +1: B comes from the right, -1: from the left
int page = MENU;                    // the picture that is settled on screen
bool frameDirty = true;

struct Anim { bool on; bool vertical; float from, to; uint32_t t0; int dur; } anim = {};

// editing state
bool changed = false;
int savedFrom = 22, savedTo = 7;    // night hours to restore when night mode is turned back on
uint32_t lastTouch = 0, lastMeter = 0;

// touch tracking
bool tDown = false, owning = false, decided = false, sliderDrag = false;
int sx, sy, lx, ly;
uint32_t st;
enum Gesture { G_NONE, G_PULL, G_VERT, G_HORZ, G_TAP } gesture = G_NONE;

float easeOut(float t) { t = 1 - t; return 1 - t * t * t; }

/* ---------------- drawing helpers ---------------- */
void backChevron() {
  gLine(46, 38, 36, 49, 3, C_SOFT);
  gLine(36, 49, 46, 60, 3, C_SOFT);
}
void title(const char *s, float x) { gText(s, x, 56, 14, F500, C_DIM, LEFT, 2.2f); }
void pageDots(int idx) {
  float x = 240 - ((PAGES - 1) * 12 + 14) / 2.f;
  for (int i = 0; i < PAGES; i++) {
    if (i == idx) { gRoundRect(x, 463, 14, 5, 2.5f, C_DIM); x += 21; }
    else { gCircle(x + 2.5f, 465.5f, 2.5f, C_LINE); x += 12; }
  }
}
String hourText(int h) {
  if (!CFG.h12) { char b[8]; snprintf(b, sizeof b, "%02d:00", h); return b; }
  char b[8]; snprintf(b, sizeof b, "%d %s", h % 12 ? h % 12 : 12, h < 12 ? "AM" : "PM"); return b;
}
String timeText(int s) {
  if (s <= 0) return "Off";
  char b[12];
  if (s < 60) snprintf(b, sizeof b, "%d s", s); else snprintf(b, sizeof b, "%d min", s / 60);
  return b;
}
bool nightOn() { return CFG.nFrom != CFG.nTo; }
String nightText() { return nightOn() ? hourText(CFG.nFrom) + " \xE2\x80\x93 " + hourText(CFG.nTo) : String("Off"); }

void icon(int i, float cx, float cy, uint16_t bg) {
  switch (i) {
    case P_LIGHT:
      gCircle(cx, cy, 8, C_SUN);
      for (int k = 0; k < 8; k++) { float a = k * PI / 4; gLine(cx + cosf(a) * 12.5f, cy + sinf(a) * 12.5f, cx + cosf(a) * 17, cy + sinf(a) * 17, 2.6f, C_SUN); }
      break;
    case P_CLOCK:
      gRing(cx, cy, 15, 2.6f, C_RAIN);
      gLine(cx, cy, cx, cy - 9, 2.6f, C_RAIN);
      gLine(cx, cy, cx + 7, cy + 3, 2.6f, C_RAIN);
      break;
    case P_NIGHT:
      gCircle(cx, cy, 14, C_MOON);
      gCircle(cx + 7, cy - 6, 12, bg);
      break;
    case P_SOUND:   // a speaker with sound waves
      gRoundRect(cx - 14, cy - 5, 7, 10, 1.5f, C_RAIN);
      { float tri[] = {cx - 8, cy - 5, cx - 1, cy - 11, cx - 1, cy + 11, cx - 8, cy + 5}; gPolygon(tri, 4, C_RAIN); }
      gArc(cx, cy, 7, 2.4f, -0.7f, 0.7f, C_RAIN);
      gArc(cx, cy, 13, 2.4f, -0.7f, 0.7f, C_RAIN);
      break;
    case P_TIME:
      gRing(cx, cy, 15, 2.6f, C_LINE);
      gArc(cx, cy, 15, 2.6f, -HALF_PI, PI * 0.9f, C_GOOD);
      gCircle(cx, cy, 3, C_GOOD);
      break;
  }
}

/* ---------------- the pictures ---------------- */
// menu tiles: 2 x 2
// menu rows: icon, name, value, ›
void tileRect(int i, float &x, float &y) { x = 36; y = 84 + i * 70; }
constexpr float TW = 408, TH = 62;

void drawMenu() {
  gClear(C_BG);
  gRoundRect(220, 14, 40, 5, 2.5f, C_LINE);
  title("QUICK SETTINGS", 36);
  if (timeOk) gText(fmtHM(nowUtc(), cityOff(), CFG.h12, true).c_str(), 444, 56, 15, F400, C_SOFT, RIGHT);
  for (int i = 0; i < PAGES; i++) {
    float x, y; tileRect(i, x, y);
    gRoundRect(x, y, TW, TH, 20, C_FAINT);
    icon(i, x + 34, y + TH / 2, C_FAINT);
    gText(NAMES[i], x + 66, y + 38, 18, F500, C_FG);
    String v = i == P_LIGHT ? String(CFG.bDay) + "%" : i == P_NIGHT ? nightText()
             : i == P_SOUND ? String(CFG.soundWake ? "On" : "Off") : i == P_CLOCK ? String(CFG.h12 ? "12-hour" : "24-hour")
             : timeText(CFG.rotate);
    gText(v.c_str(), x + TW - 40, y + 38, 16, F400, C_SOFT, RIGHT);
    gLine(x + TW - 24, y + 25, x + TW - 18, y + 31, 2.4f, C_DIM);
    gLine(x + TW - 18, y + 31, x + TW - 24, y + 37, 2.4f, C_DIM);
  }
  gText("Swipe up to close", 240, 460, 13, F400, C_DIM, CENTER);
}

/* One pattern for every page:
   - yellow (C_SUN) is the only accent: a switch that's on, a slider's fill, the chosen option
   - options are dark cards; the chosen one gets a yellow outline and yellow text (big cards also a check)
   - neutral controls (−/+ buttons, empty slider track) are dark grey
   - a big value, when the page has one, sits at the same place; one helper line at the bottom of every page */
constexpr float SL_X0 = 60, SL_X1 = 420, SL_Y = 300, HERO_Y = 200, HELP_Y = 432;
void helper(const char *t) { gText(t, 240, HELP_Y, 15, F400, C_DIM, CENTER); }
void hero(const String &t) { gText(t.c_str(), 240, HERO_Y, 96, F300, C_FG, CENTER, -2, true); }
void option(float x, float y, float w, float h, float r, bool sel) {
  gRoundRect(x, y, w, h, r, C_FAINT);
  if (sel) gRoundRectStroke(x, y, w, h, r, 2.5f, C_SUN);
}
void check(float cx, float cy) {
  gCircle(cx, cy, 13, C_SUN);
  gLine(cx - 6, cy, cx - 1.5f, cy + 4.5f, 2.6f, C_BG);
  gLine(cx - 1.5f, cy + 4.5f, cx + 6.5f, cy - 4.5f, 2.6f, C_BG);
}
void toggle(float x, float y, bool on) {
  gRoundRect(x, y, 72, 40, 20, on ? C_SUN : C_LINE);
  gCircle(on ? x + 52 : x + 20, y + 20, 15, on ? C_BG : C_FG);
}
void slider(float y, int v) {
  float kx = SL_X0 + (SL_X1 - SL_X0) * v / 100.f;
  gRoundRect(SL_X0, y - 5, SL_X1 - SL_X0, 10, 5, C_FAINT);
  gRoundRect(SL_X0, y - 5, max(10.f, kx - SL_X0), 10, 5, C_SUN);
  gCircle(kx, y, 17, C_FG);
}

// brightness: the daytime level
void drawLight() {
  hero(String(CFG.bDay) + "%");
  slider(SL_Y, CFG.bDay);
  helper(nightOn() ? "Daytime level \xC2\xB7 the night level is in Night mode" : "Changes live as you drag");
}

// clock: two big option cards
void clockCard(float y, bool h12) {
  bool sel = CFG.h12 == h12;
  option(36, y, 408, 128, 24, sel);
  String t = timeOk ? fmtHM(nowUtc(), cityOff(), h12, true) : String(h12 ? "2:30 PM" : "14:30");
  gText(t.c_str(), 64, y + 82, 50, F300, C_FG);
  gText(h12 ? "12-hour" : "24-hour", 416, y + 56, 17, F500, sel ? C_SUN : C_SOFT, RIGHT);
  if (sel) check(402, y + 88);
}
void drawClock() {
  clockCard(104, false);
  clockCard(252, true);
  helper("Applies to every screen");
}

// night mode: switch, hours, its own brightness
constexpr float NC[2] = {140, 340}, ST_Y = 268, NS_Y = 380;
void stepButton(float cx, float cy, bool plus) {
  gCircle(cx, cy, 24, C_FAINT);
  gLine(cx - 8, cy, cx + 8, cy, 2.6f, C_FG);
  if (plus) gLine(cx, cy - 8, cx, cy + 8, 2.6f, C_FG);
}
void drawNight() {
  bool on = nightOn();
  gText("Dim the screen at night", 36, 124, 19, F500, C_FG);
  toggle(372, 98, on);
  if (!on) {
    gText("The screen stays at its day", 240, 252, 17, F400, C_DIM, CENTER);
    gText("brightness all the time.", 240, 278, 17, F400, C_DIM, CENTER);
    helper("Turn it on to choose hours and a night level");
    return;
  }
  const char *lab[2] = {"FROM", "UNTIL"};
  int hv[2] = {CFG.nFrom, CFG.nTo};
  for (int i = 0; i < 2; i++) {
    gText(lab[i], NC[i], 178, 13, F500, C_DIM, CENTER, 1.8f);
    gText(hourText(hv[i]).c_str(), NC[i], 226, CFG.h12 ? 36 : 44, F300, C_FG, CENTER, 0, true);
    stepButton(NC[i] - 40, ST_Y, false);
    stepButton(NC[i] + 40, ST_Y, true);
  }
  gText("Brightness at night", 36, 340, 15, F500, C_DIM);
  char b[8]; snprintf(b, sizeof b, "%d%%", CFG.bNight);
  gText(b, 444, 340, 17, F500, C_FG, RIGHT);
  slider(NS_Y, CFG.bNight);
  helper("Changes live as you drag");
}

// wake on sound: switch, sensitivity, live meter
constexpr float SC_Y = 206, MT_Y = 348;
void drawSound() {
  gText("Brighten at night on sound", 36, 124, 19, F500, C_FG);
  toggle(372, 98, CFG.soundWake);
  gText("SENSITIVITY", 240, 184, 13, F500, C_DIM, CENTER, 1.8f);
  const char *lab[3] = {"Low", "Medium", "High"};
  for (int i = 0; i < 3; i++) {
    float x = 36 + i * 140;
    bool sel = CFG.soundSense == i;
    option(x, SC_Y, 128, 52, 26, sel);
    gText(lab[i], x + 64, SC_Y + 33, 17, F500, sel ? C_SUN : C_SOFT, CENTER);
  }
  gText("Sound now", 36, 314, 15, F500, C_DIM);
  if (micRunning()) {
    auto pos = [](float db) { return SL_X0 + (SL_X1 - SL_X0) * constrain((db + 75) / 60, 0.f, 1.f); };
    float lx = pos(micLevel()), tx = pos(micFloor() + micMargin(CFG.soundSense));
    bool hears = micHears(CFG.soundSense);
    gText(hears ? "Heard" : "Quiet", 444, 314, 15, F500, hears ? C_SUN : C_SOFT, RIGHT);
    gRoundRect(SL_X0, MT_Y - 5, SL_X1 - SL_X0, 10, 5, C_FAINT);
    gRoundRect(SL_X0, MT_Y - 5, max(10.f, lx - SL_X0), 10, 5, hears ? C_SUN : C_SOFT);
    gRoundRect(tx - 1.5f, MT_Y - 14, 3, 28, 1.5f, C_SUN);   // where it starts to count
  } else gText("Microphone starting\xE2\x80\xA6", 444, 314, 15, F400, C_DIM, RIGHT);
  helper("Only during night hours \xC2\xB7 nothing is recorded");
}

// screen time: option chips
void chipRect(int i, float &x, float &y, float &w) {
  if (i < 4) { w = 93; x = 36 + i * 105; y = 268; }
  else { w = 128; x = 36 + (i - 4) * 140; y = 334; }
}
void drawTime() {
  hero(timeText(CFG.rotate));
  for (int i = 0; i < 7; i++) {
    float x, y, w; chipRect(i, x, y, w);
    bool sel = CFG.rotate == TIMES[i];
    option(x, y, w, 52, 26, sel);
    gText(timeText(TIMES[i]).c_str(), x + w / 2, y + 33, 17, F500, sel ? C_SUN : C_SOFT, CENTER);
  }
  helper(CFG.rotate ? "How long each screen stays before the next" : "Screens don't change by themselves");
}

void drawPage(int id) {
  gClear(C_BG);
  backChevron();
  title(TITLES[id], 64);
  switch (id) {
    case P_LIGHT: drawLight(); break;
    case P_CLOCK: drawClock(); break;
    case P_NIGHT: drawNight(); break;
    case P_SOUND: drawSound(); break;
    case P_TIME: drawTime(); break;
  }
  pageDots(id);
}

// the picture for a view id, drawn if needed
uint16_t *picture(int id) {
  if (id == NONE) return nullptr;
  if (id == MENU) {
    if (menuDirty) { gTarget(menuBuf); drawMenu(); menuDirty = false; }
    return menuBuf;
  }
  if (pageDirty[id]) { gTarget(pageBuf[id]); drawPage(id); pageDirty[id] = false; }
  return pageBuf[id];
}
void invalidate(int id) {
  if (id == MENU) { menuDirty = true; return; }
  pageDirty[id] = true;
}
void invalidateAll() { menuDirty = true; for (bool &d : pageDirty) d = true; }
// draw the pages next to the one on screen ahead of time, so a swipe never waits for drawing
bool prerender() {
  if (page < 0) return false;
  if (menuDirty) { picture(MENU); return true; }
  int id = (page + 1) % PAGES;
  if (pageDirty[id]) { picture(id); return true; }
  return false;
}

// copy one row of src into dst, shifted right by off pixels (only the visible part)
inline void rowShift(uint16_t *dst, const uint16_t *src, int off) {
  if (off >= N || off <= -N) return;
  if (off >= 0) memcpy(dst + off, src, (N - off) * 2);
  else memcpy(dst, src - off, (N + off) * 2);
}

/* ---------------- state changes ---------------- */
void startAnim(bool vertical, float from, float to, int dur = 240) {
  anim = {true, vertical, from, to, millis(), dur};
  frameDirty = true;
}
void slideTo(int from, int to, int dir) {   // animated slide between two pictures
  viewA = from; viewB = to; slideDir = dir; slide = 0;
  startAnim(false, 0, 1);
}
bool allocate() {
  // every picture the panel uses: the old screen, the menu and one per page (all of them, whatever PAGES is)
  if (!base) base = (uint16_t *)ps_malloc(N * N * 2);
  if (!menuBuf) menuBuf = (uint16_t *)ps_malloc(N * N * 2);
  for (auto &p : pageBuf) if (!p) p = (uint16_t *)ps_malloc(N * N * 2);
  if (!base || !menuBuf) return false;
  for (auto *p : pageBuf) if (!p) return false;
  invalidateAll();
  return true;
}
void release() {
  free(base); base = nullptr;
  free(menuBuf); menuBuf = nullptr;
  for (auto &p : pageBuf) { free(p); p = nullptr; }
}
void openPull() {   // a finger started pulling the panel down from the top edge
  if (!allocate()) { release(); return; }
  memcpy(base, previewFrame(), N * N * 2);
  mode = Q_ON;
  page = MENU; viewA = MENU; viewB = NONE; slide = 0;
  changed = false;
  if (nightOn()) { savedFrom = CFG.nFrom; savedTo = CFG.nTo; }
  oy = -N;
  lastTouch = millis();
  frameDirty = true;
}
void finishClose() {
  mode = Q_OFF;
  anim.on = false;
  release();
  if (changed) { cfgSave(); cfgVersion = cfgVersion + 1; }
  closedFlag = true;
}
void closePanel() { viewA = page; viewB = NONE; slide = 0; startAnim(true, oy, -N, 260); }

void setLight(bool night, int v) {   // a slider moved: store it and show that brightness live
  v = constrain((v + 2) / 5 * 5, night ? 0 : 5, 100);
  int &dst = night ? CFG.bNight : CFG.bDay;
  if (dst == v) return;
  LOCK(); dst = v; UNLOCK();
  changed = true;
  invalidate(night ? P_NIGHT : P_LIGHT); invalidate(MENU);
  hwBacklight(max(v, 3));
  frameDirty = true;
}
void restoreLight() { hwBacklight(max(nightNow() ? CFG.bNight : CFG.bDay, 3)); }   // after a live preview
void touchPage(int x, int y) {   // a tap on a settings page
  switch (page) {
    case P_CLOCK: {
      int pick = y > 96 && y < 240 ? 0 : y > 244 && y < 388 ? 1 : -1;
      if (pick >= 0 && CFG.h12 != (pick == 1)) {
        LOCK(); CFG.h12 = pick == 1; UNLOCK();
        changed = true; invalidateAll(); frameDirty = true;
      }
      break;
    }
    case P_NIGHT:
      if (y > 88 && y < 150 && x > 340) {   // the switch
        LOCK();
        if (nightOn()) { savedFrom = CFG.nFrom; savedTo = CFG.nTo; CFG.nFrom = CFG.nTo = 0; }
        else { CFG.nFrom = savedFrom; CFG.nTo = savedTo == savedFrom ? (savedFrom + 9) % 24 : savedTo; }
        UNLOCK();
        changed = true; invalidateAll(); frameDirty = true;
      } else if (nightOn() && y > ST_Y - 32 && y < ST_Y + 32) {
        for (int i = 0; i < 2; i++)
          for (int d = -1; d <= 1; d += 2)
            if (fabsf(x - (NC[i] + d * 40)) < 34) {
              LOCK();
              int &h = i == 0 ? CFG.nFrom : CFG.nTo;
              int nh = (h + d + 24) % 24;
              if (nh == (i == 0 ? CFG.nTo : CFG.nFrom)) nh = (nh + d + 24) % 24;   // from and until can't be equal
              h = nh;
              UNLOCK();
              changed = true; invalidateAll(); frameDirty = true;
            }
      }
      break;
    case P_SOUND:
      if (y > 88 && y < 150 && x > 340) {
        LOCK(); CFG.soundWake = !CFG.soundWake; UNLOCK();
        changed = true; invalidate(P_SOUND); invalidate(MENU); frameDirty = true;
      } else if (y > SC_Y - 6 && y < SC_Y + 58) {
        int i = constrain((x - 36) / 140, 0, 2);
        if (CFG.soundSense != i) { LOCK(); CFG.soundSense = i; UNLOCK(); changed = true; invalidate(P_SOUND); frameDirty = true; }
      }
      break;
    case P_TIME:
      for (int i = 0; i < 7; i++) {
        float cx, cy, w; chipRect(i, cx, cy, w);
        if (x >= cx && x <= cx + w && y >= cy - 6 && y <= cy + 58 && CFG.rotate != TIMES[i]) {
          LOCK(); CFG.rotate = TIMES[i]; UNLOCK();
          changed = true; invalidate(P_TIME); invalidate(MENU); frameDirty = true;
        }
      }
      break;
  }
}

}  // namespace

bool quickLog = false;

/* ---------------- public ---------------- */
bool quickActive() { return mode != Q_OFF; }
bool quickWantsMic() { return mode != Q_OFF && (page == P_SOUND || viewB == P_SOUND); }
void quickDebug(int id) {
  if (mode == Q_OFF) openPull();
  if (mode == Q_OFF) return;
  anim.on = false; oy = 0; page = viewA = constrain(id, -1, PAGES - 1); viewB = NONE; slide = 0; frameDirty = true;
}
bool quickJustClosed() { bool c = closedFlag; closedFlag = false; return c; }
void quickAbort() { if (mode != Q_OFF) finishClose(); }

bool quickTouch(bool down, int x, int y) {
  bool start = down && !tDown, end = !down && tDown;
  tDown = down;
  if (start) { sx = lx = x; sy = ly = y; st = millis(); decided = false; sliderDrag = false; gesture = G_NONE; owning = mode != Q_OFF; }
  if (down) { lx = x; ly = y; }
  int dx = lx - sx, dy = ly - sy;

  if (mode == Q_OFF) {
    // a pull down that starts at the top edge opens the panel; the finger then drags it
    if (down && !owning && sy < 70 && dy > 10 && dy > abs(dx) && !setupMode && otaPercent < 0) {
      openPull();
      if (mode == Q_OFF) return false;
      owning = true; decided = true; gesture = G_PULL;
    }
    if (mode == Q_OFF) { if (end) owning = false; return false; }
  }

  if (down || end) lastTouch = millis();
  if (anim.on && start) anim.on = false;   // catch a moving page with the finger

  // settled on a page: decide what this touch is
  if (down && !decided) {
    if (viewB == NONE && oy == 0 && ((page == P_LIGHT && fabsf(sy - SL_Y) < 26) || (page == P_NIGHT && nightOn() && fabsf(sy - NS_Y) < 26))) {
      sliderDrag = true; decided = true;
    }
    else if (abs(dx) > 12 || abs(dy) > 12) {
      decided = true;
      gesture = abs(dy) > abs(dx) ? G_VERT : G_HORZ;
      if (gesture == G_HORZ && page == MENU) gesture = G_NONE;   // the menu doesn't scroll sideways
    }
  }

  if (down) {
    if (sliderDrag) {
      setLight(page == P_NIGHT, (int)roundf((x - SL_X0) * 100.f / (SL_X1 - SL_X0)));
    } else if (gesture == G_PULL) {
      oy = constrain(dy - N + 40, -N, 0);   // the panel's bottom edge follows the finger
      frameDirty = true;
    } else if (gesture == G_VERT && dy < 0 && viewB == NONE) {
      oy = constrain((float)dy, -N, 0);     // swiping up from anywhere drags the panel away
      frameDirty = true;
    } else if (gesture == G_HORZ && page >= 0) {
      // left to right: back to the menu, like any app (the menu comes in from the left);
      // right to left: the next option, looping
      viewA = page;
      slideDir = dx < 0 ? 1 : -1;
      viewB = dx > 0 ? MENU : (page + 1) % PAGES;
      slide = min(1.f, abs(dx) / (float)N);
      frameDirty = true;
    }
  }

  if (end) {
    if (quickLog) Serial.printf("%lu quick: gesture %d page %d dx %d dy %d (%lu ms)\n", millis(), (int)gesture, page, dx, dy, millis() - st);
    if (sliderDrag) restoreLight();   // back to the level for this time of day
    // touch is sampled only every few frames during a flick, so any clear pull counts
    else if (gesture == G_PULL) startAnim(true, oy, dy > 40 ? 0 : -N, 240);
    else if (gesture == G_VERT && dy < 0) {    // swipe up: close, from the menu or any page
      if (dy < -50) closePanel(); else if (oy < 0) startAnim(true, oy, 0, 200);
    } else if (gesture == G_VERT && page >= 0 && dy > 50) {
      slideTo(page, MENU, -1);                 // swipe down on a page: back to the menu
    } else if (gesture == G_HORZ && page >= 0) {
      if (viewB != NONE && abs(dx) > 40) startAnim(false, slide, 1, 220);
      else startAnim(false, slide, 0, 200);
    } else if (!decided && millis() - st < 600) {   // a tap
      if (page == MENU) {
        for (int i = 0; i < PAGES; i++) {
          float tx, ty; tileRect(i, tx, ty);
          if (x >= tx && x <= tx + TW && y >= ty && y <= ty + TH) { picture(i); slideTo(MENU, i, 1); }
        }
      } else if (page >= 0) {
        if (sx < 110 && sy < 96) slideTo(page, MENU, -1);   // ‹ back
        else touchPage(sx, sy);
      }
    }
    bool own = owning;
    owning = false;
    return own || mode != Q_OFF;
  }
  return true;
}

bool quickWantsFrame() {
  if (mode == Q_OFF) return false;
  if (otaPercent >= 0) { quickAbort(); return false; }
  if (!anim.on && !tDown && millis() - lastTouch > 30000 && oy == 0) closePanel();   // idle: close by itself
  if (anim.on) {
    float t = min(1.f, (millis() - anim.t0) / (float)anim.dur);
    float v = anim.from + (anim.to - anim.from) * easeOut(t);
    if (anim.vertical) oy = v; else slide = v;
    if (t >= 1) {
      anim.on = false;
      if (anim.vertical && anim.to <= -N) { finishClose(); return false; }
      if (!anim.vertical) {
        if (slide >= 1 && viewB != NONE) { page = viewB; viewA = viewB; }
        viewB = NONE; slide = 0;
        if (page == MENU) invalidate(MENU);
      }
    }
    frameDirty = true;
  }
  if (page == P_SOUND && viewB == NONE && oy == 0 && !anim.on && millis() - lastMeter > 150) {
    lastMeter = millis(); invalidate(P_SOUND); frameDirty = true;
  }
  bool f = frameDirty;
  frameDirty = false;
  if (!f && !anim.on && !tDown) prerender();
  return f;
}

void quickCompose(uint16_t *dst) {
  const uint16_t *a = picture(viewA);
  const uint16_t *b = picture(viewB);
  int iy = (int)roundf(oy);
  int ax = (int)roundf(-slideDir * slide * N), bx = ax + slideDir * N;
  for (int y = 0; y < N; y++) {
    uint16_t *row = dst + y * N;
    int pr = y - iy;       // row inside the panel picture (the panel's bottom edge is at oy + N)
    if (pr >= N) { memcpy(row, base + y * N, N * 2); continue; }   // the old screen below the panel
    if (ax == 0 && !b) { memcpy(row, a + pr * N, N * 2); continue; }
    memset(row, 0, N * 2);
    rowShift(row, a + pr * N, ax);
    if (b) rowShift(row, b + pr * N, bx);
  }
  if (iy < 0 && iy > -N) {   // a soft edge under the sliding panel
    int ey = iy + N;
    for (int k = 0; k < 6 && ey + k < N; k++) {
      uint16_t *row = dst + (ey + k) * N;
      uint8_t al = 150 - k * 25;
      for (int x = 0; x < N; x++) row[x] = gBlend(row[x], C_BG, al);
    }
  }
}
