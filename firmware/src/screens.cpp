// All screens, drawn the same way as the web design (480x480, black, Inter)
#include "app.h"
#include "gfx.h"
#include "hw.h"
#include "util.h"
#include "land_dots.h"
#include "net_secure.h"
#include <qrcode.h>

static Config C;
static Weather W;
static Air A;
static Iss I;
static PlanetReport P[5];
static bool pOk;
static SkyEvent E[6];
static int nE;
static int64_t NOW;
static int32_t OFF;

static const float RADf = PI / 180.f, DEGf = 180.f / PI;

static uint16_t lerp565(uint16_t a, uint16_t b, float t) { return gBlend(a, b, (uint8_t)(constrain(t, 0.f, 1.f) * 255)); }
static void label(const char *s, float x = 36, float y = 52, Align al = LEFT, uint16_t c = C_DIM) { gText(s, x, y, 14, F500, c, al, 2.2f); }
static void statCol(float x, Align al, const char *key, const String &val, const char *unit, int vsize = 28) {
  gText(key, x, 404, 13, F500, C_DIM, al, 1.8f);
  int uw = unit ? gTextW(unit, 15, F400) + 3 : 0;
  int vw = gTextW(val.c_str(), vsize, F400, 0, true);
  float vx = al == LEFT ? x : al == CENTER ? x - (vw + uw) / 2.f : x - vw - uw;
  gText(val.c_str(), vx, 440, vsize, F400, C_FG, LEFT, 0, true);
  if (unit) gText(unit, vx + vw + 3, 440, 15, F400, C_DIM);
}
static bool stale() { return lastSync ? NOW - lastSync > 3600 : millis() > 120000; }
static void sep(float y) { gFillRect(36, (int)y, 408, 1, C_FAINT); }

/* ---------------- weather icons (64-unit design space, like the web SVGs) ---------------- */
enum Wk { WK_CLEAR, WK_PARTLY, WK_CLOUD, WK_FOG, WK_DRIZZLE, WK_RAIN, WK_SNOW, WK_STORM };
static void wmo(int code, Wk &k, const char *&lbl) {
  if (code == 0) { k = WK_CLEAR; lbl = "Clear"; }
  else if (code == 1) { k = WK_PARTLY; lbl = "Mostly clear"; }
  else if (code == 2) { k = WK_PARTLY; lbl = "Partly cloudy"; }
  else if (code == 3) { k = WK_CLOUD; lbl = "Cloudy"; }
  else if (code == 45 || code == 48) { k = WK_FOG; lbl = "Foggy"; }
  else if (code >= 51 && code <= 57) { k = WK_DRIZZLE; lbl = "Drizzle"; }
  else if (code >= 61 && code <= 67) { k = WK_RAIN; lbl = code >= 65 ? "Heavy rain" : "Rain"; }
  else if (code >= 71 && code <= 77) { k = WK_SNOW; lbl = "Snow"; }
  else if (code >= 80 && code <= 82) { k = WK_RAIN; lbl = "Showers"; }
  else if (code == 85 || code == 86) { k = WK_SNOW; lbl = "Snow showers"; }
  else if (code >= 95) { k = WK_STORM; lbl = "Thunderstorm"; }
  else { k = WK_CLOUD; lbl = "\xE2\x80\x94"; }
}
static float IX, IY, IS;
static float ix(float u) { return IX + (u - 32) * IS; }
static float iy(float v) { return IY + (v - 32) * IS; }
static void iSun(float u, float v, float r) {
  for (int i = 0; i < 8; i++) {
    float a = i * PI / 4;
    gLine(ix(u + cosf(a) * (r + 4)), iy(v + sinf(a) * (r + 4)), ix(u + cosf(a) * (r + 9)), iy(v + sinf(a) * (r + 9)), 3 * IS, C_SUN);
  }
  gCircle(ix(u), iy(v), r * IS, C_SUN);
}
static void iMoon(float tx, float ty, float s) {
  gCircle(ix(tx + 36 * s), iy(ty + 31 * s), 17 * s * IS, C_MOON);
  gCircle(ix(tx + 46 * s), iy(ty + 23 * s), 14 * s * IS, C_BG);
}
static void iCloud(float tx, float ty, uint16_t c) {
  gCircle(ix(tx + 22), iy(ty + 40), 10 * IS, c);
  gCircle(ix(tx + 34), iy(ty + 31), 14 * IS, c);
  gCircle(ix(tx + 46), iy(ty + 40), 10 * IS, c);
  gRoundRect(ix(tx + 22), iy(ty + 36), 24 * IS, 14 * IS, 0, c);
}
static void drawIcon(Wk k, bool day, float cx, float cy, float size) {
  IX = cx; IY = cy; IS = size / 64;
  switch (k) {
    case WK_CLEAR: if (day) iSun(32, 32, 12); else iMoon(-2, 2, 1); break;
    case WK_PARTLY: if (day) iSun(42, 20, 8); else iMoon(14, -6, .7f); iCloud(-4, 2, C_CLOUD); break;
    case WK_CLOUD: iCloud(-2, -4, C_CLOUD2); iCloud(-2, 4, C_CLOUD); break;
    case WK_FOG:
      iCloud(-2, -10, C_CLOUD);
      gLine(ix(12), iy(48), ix(52), iy(48), 3 * IS, C_CLOUD, 180);
      gLine(ix(18), iy(56), ix(46), iy(56), 3 * IS, C_CLOUD, 180);
      break;
    case WK_DRIZZLE:
    case WK_RAIN: {
      iCloud(-2, -10, C_CLOUD);
      const float L[5][4] = {{22, 46, 19, 54}, {32, 46, 29, 54}, {42, 46, 39, 54}, {27, 56, 25, 61}, {37, 56, 35, 61}};
      for (int i = 0; i < (k == WK_RAIN ? 5 : 3); i++) gLine(ix(L[i][0]), iy(L[i][1]), ix(L[i][2]), iy(L[i][3]), 3 * IS, C_RAIN);
      break;
    }
    case WK_SNOW: {
      iCloud(-2, -10, C_CLOUD);
      const float D[5][3] = {{21, 50, 2.6}, {32, 54, 2.6}, {43, 50, 2.6}, {26, 59, 2.2}, {38, 59, 2.2}};
      for (auto &d : D) gCircle(ix(d[0]), iy(d[1]), d[2] * IS, C_SNOW);
      break;
    }
    case WK_STORM: {
      iCloud(-2, -10, C_CLOUD);
      const float B[7][2] = {{34, 42}, {26, 54}, {33, 54}, {29, 64}, {40, 50}, {33, 50}, {37, 42}};
      float xy[14];
      for (int i = 0; i < 7; i++) { xy[2 * i] = ix(B[i][0]); xy[2 * i + 1] = iy(B[i][1]); }
      gPolygon(xy, 7, C_SUN);
      break;
    }
  }
}

/* ---------------- screens ---------------- */
static void sHome() {
  String t = deg(W.temp, W.ok);
  Wk k; const char *lbl = "Loading\xE2\x80\xA6";
  if (W.ok) wmo(W.code, k, lbl); else if (stale()) lbl = "Offline";
  int tw = gTextW(t.c_str(), 88, F400, 0, true);
  float total = W.ok ? 104 + 18 + tw : tw, x0 = 240 - total / 2;
  if (W.ok) drawIcon(k, W.isDay, x0 + 52, 103, 104);
  gText(t.c_str(), W.ok ? x0 + 122 : x0, 137, 88, F400, C_FG, LEFT, -1.5f, true);
  gText(lbl, 240, 204, 34, F400, C_FG, CENTER);
  if (timeOk) {
    gText(fmtHM(NOW, OFF, C.h12).c_str(), 240, 346, 132, F500, C_FG, CENTER, -4, true);
    LT l = toLocal(NOW, OFF);
    char b[16]; snprintf(b, sizeof b, "%s %d", MONTHS_SHORT[l.mo - 1], l.d);
    gText(b, 240, 402, 30, F400, C_SOFT, CENTER);
  } else gText("--:--", 240, 346, 132, F500, C_FAINT, CENTER, -4, true);
}

static void sForecast() {
  String lab = String(C.city);
  lab.toUpperCase();
  lab += " \xC2\xB7 THIS WEEK";
  label(lab.c_str(), 36, 52);
  if (!W.ok || W.nd < 6) { gText("Waiting for the forecast\xE2\x80\xA6", 240, 260, 18, F400, C_DIM, CENTER); return; }
  float lo = 999, hi = -999;
  for (int i = 0; i < 6; i++) { lo = fminf(lo, W.d[i].tmin); hi = fmaxf(hi, W.d[i].tmax); }
  for (int i = 0; i < 6; i++) {
    float top = 70 + i * 62, cy = top + 31;
    if (i) sep(top);
    const char *name = i == 0 ? "Today" : WEEKDAYS_SHORT[toLocal(W.d[i].t + 43200, OFF).wd];
    gText(name, 36, cy + 8, 22, F500, C_FG);
    Wk k; const char *l; wmo(W.d[i].code, k, l);
    drawIcon(k, true, 145, cy, 38);
    if (W.d[i].pop >= 20) { char b[8]; snprintf(b, sizeof b, "%d%%", W.d[i].pop); gText(b, 170, cy + 5, 14, F500, C_RAIN); }
    gText(deg(W.d[i].tmin).c_str(), 250, cy + 7, 21, F400, C_DIM, RIGHT, 0, true);
    gText(deg(W.d[i].tmax).c_str(), 408, cy + 7, 21, F400, C_FG, LEFT, 0, true);
    float bx0 = 260, bx1 = 398, a = (W.d[i].tmin - lo) / fmaxf(hi - lo, 1), b = (W.d[i].tmax - lo) / fmaxf(hi - lo, 1);
    gRoundRect(bx0, cy - 3, bx1 - bx0, 6, 3, C_FAINT);
    float s0 = bx0 + a * (bx1 - bx0), s1 = bx0 + b * (bx1 - bx0);
    for (int x = (int)s0; x <= (int)s1; x++) {
      uint16_t c = lerp565(C_RAIN, C_SUN, (x - s0) / fmaxf(s1 - s0, 1));
      for (int y = (int)cy - 3; y < (int)cy + 3; y++) gPixel(x, y, c);
    }
    gCircle(s0, cy, 3, C_RAIN);
    gCircle(s1, cy, 3, C_SUN);
  }
}

static void sHourly() {
  label("NEXT 12 HOURS");
  gText(deg(W.temp, W.ok).c_str(), 36, 128, 64, F300, C_FG, LEFT, 0, true);
  if (!W.ok || W.nh < 13) return;
  char b[48];
  snprintf(b, sizeof b, "High %s \xC2\xB7 Low %s", deg(W.d[0].tmax).c_str(), deg(W.d[0].tmin).c_str());
  gText(b, 444, 98, 18, F400, C_SOFT, RIGHT);
  int maxPop = 0;
  for (int k = 0; k < 13; k++) maxPop = max(maxPop, W.h[k].pop);
  if (maxPop >= 20) snprintf(b, sizeof b, "Rain chance up to %d%%", maxPop); else strcpy(b, "No rain expected");
  gText(b, 444, 125, 18, F400, C_RAIN, RIGHT);
  float tMin = 1e9, tMax = -1e9;
  for (int k = 0; k < 13; k++) { tMin = fminf(tMin, W.h[k].temp); tMax = fmaxf(tMax, W.h[k].temp); }
  tMin -= 1; tMax += 1;
  const float top = 234, bot = 374;
  auto X = [](int k) { return 62.f + k * (356.f / 12); };
  auto Y = [&](float v) { return bot - (v - tMin) / (tMax - tMin) * (bot - top); };
  // smooth curve (same midpoint Béziers as the web chart), sampled into columns
  static float colY[480];
  for (int x = 0; x < 480; x++) colY[x] = NAN;
  float px = X(0), py = Y(W.h[0].temp), minY = py;
  for (int k = 1; k < 13; k++) {
    float x0 = X(k - 1), y0 = Y(W.h[k - 1].temp), x1 = X(k), y1 = Y(W.h[k].temp), cx = (x0 + x1) / 2;
    for (int s = 1; s <= 24; s++) {
      float t = s / 24.f, u = 1 - t;
      float x = u * u * u * x0 + 3 * u * u * t * cx + 3 * u * t * t * cx + t * t * t * x1;
      float y = u * u * u * y0 + 3 * u * u * t * y0 + 3 * u * t * t * y1 + t * t * t * y1;
      for (int c = (int)ceilf(px); c <= (int)floorf(x); c++) if (c >= 0 && c < 480) colY[c] = py + (y - py) * ((c - px) / fmaxf(x - px, .001f));
      minY = fminf(minY, y);
      px = x; py = y;
    }
  }
  for (int x = 0; x < 480; x++) {
    if (isnan(colY[x])) continue;
    for (int y = (int)colY[x]; y < (int)bot; y++) gPixel(x, y, C_SUN, (uint8_t)(90 * (1 - (y - minY) / (bot - minY))));
  }
  for (int x = 1; x < 480; x++)
    if (!isnan(colY[x]) && !isnan(colY[x - 1])) gLine(x - 1, colY[x - 1], x, colY[x], 3, C_SUN);
  for (int k = 0; k < 13; k++)
    if (W.h[k].pop > 0) gRoundRect(X(k) - 5, bot + 4, 10, fmaxf(2, W.h[k].pop / 100.f * 28), 3, C_RAIN, 220);
  for (int k = 0; k <= 12; k += 3) {
    float x = X(k), y = Y(W.h[k].temp);
    gCircle(x, y, 5.2f, C_SUN); gCircle(x, y, 2.8f, C_BG);
    gText(deg(W.h[k].temp).c_str(), x, y - 14, 17, F400, C_FG, CENTER);
    LT hl = toLocal(W.h[k].t, OFF);
    char hb[12];
    if (C.h12) snprintf(hb, sizeof hb, "%d %s", hl.h % 12 ? hl.h % 12 : 12, hl.h < 12 ? "AM" : "PM");
    else snprintf(hb, sizeof hb, "%02d", hl.h);
    gText(k == 0 ? "Now" : hb, x, 440, 14, F400, C_DIM, CENTER);
  }
}

static void sSun() {
  label("SUN & MOON");
  if (!W.ok) return;
  int64_t rise = W.d[0].rise, set = W.d[0].set;
  float t = constrain((float)(NOW - rise) / (float)(set - rise), 0.f, 1.f);
  bool up = NOW > rise && NOW < set;
  const float cx = 240, cy = 294, R = 180;
  gDashedArc(cx, cy, R, 2, PI, TWO_PI, 4, 7, C_LINE);
  if (t > 0) gArc(cx, cy, R, 3, PI, PI + PI * t, C_SUN);
  gFillRect(36, (int)cy, 408, 1, C_LINE);
  float a = PI + PI * t, sx = cx + R * cosf(a), sy = cy + R * sinf(a);
  if (up) { gCircle(sx, sy, 26, C_SUN, 38); gCircle(sx, sy, 14, C_SUN); }
  else {
    char b[40];
    if (NOW < rise) snprintf(b, sizeof b, "Sun rises in %dh", (int)ceil((rise - NOW) / 3600.0)); else strcpy(b, "Sun has set");
    gText(b, cx, cy - 60, 18, F400, C_DIM, CENTER);
  }
  int len = (set - rise) / 60;
  char b[16]; snprintf(b, sizeof b, "%dh %02dm", len / 60, len % 60);
  statCol(36, LEFT, "SUNRISE", fmtHM(rise, OFF, C.h12, true), nullptr, 30);
  statCol(240, CENTER, "DAYLIGHT", b, nullptr, 30);
  statCol(444, RIGHT, "SUNSET", fmtHM(set, OFF, C.h12, true), nullptr, 30);
}

static void sMoon() {
  double p = moonPhase((double)NOW);
  const float cx = 240, cy = 192, r = 120;
  float k = cosf(TWO_PI * p);
  gCircle(cx, cy, r, RGB(21, 23, 27));
  for (int y = (int)(cy - r - 1); y <= (int)(cy + r + 1); y++) {
    float v = (y + .5f - cy) / r;
    if (fabsf(v) > 1) continue;
    float s = sqrtf(1 - v * v);
    for (int x = (int)(cx - r - 1); x <= (int)(cx + r + 1); x++) {
      float u = (x + .5f - cx) / r, d = sqrtf(u * u + v * v);
      float edge = (1 - d) * r + .5f;
      if (edge <= 0) continue;
      float term = p < .5 ? (u - k * s) * r : (-k * s - u) * r;   // distance (px) into the lit side
      float cov = fminf(1, edge) * constrain(term + .5f, 0.f, 1.f);
      if (cov <= 0) continue;
      float g = constrain(hypotf(u + .2f, v + .3f) / 1.3f, 0.f, 1.f);
      uint16_t c = lerp565(RGB(251, 248, 238), RGB(201, 195, 174), g);
      gPixel(x, y, c, (uint8_t)(cov * 255));
    }
  }
  static const char *NAMES[] = {"New Moon", "Waxing Crescent", "First Quarter", "Waxing Gibbous", "Full Moon", "Waning Gibbous", "Last Quarter", "Waning Crescent"};
  static const float CUTS[] = {.034, .216, .284, .466, .534, .716, .784, .966};
  int ci = 0;
  while (ci < 8 && p >= CUTS[ci]) ci++;
  const char *name = NAMES[ci % 8];
  int illum = (int)lround((1 - cos(TWO_PI * p)) / 2 * 100);
  double toFull = fmod(0.5 - p + 1, 1.0) * 29.530588853;
  gText(name, 240, 375, 32, F500, C_FG, CENTER);
  char b[64]; snprintf(b, sizeof b, "%d%% illuminated \xC2\xB7 Full moon in %d days", illum, (int)lround(toFull));
  gText(b, 240, 408, 18, F400, C_DIM, CENTER);
}

static void sSky() {
  double t = (double)NOW;
  String when = "Now";
  if (altaz(eclToEq(sunGeo(t)), t, C.lat, C.lon).alt > -6) {
    LT l = toLocal(NOW, OFF);
    int64_t tt = localToUtc(l.y, l.mo, l.d, 22, 0, OFF);
    if (tt <= NOW) tt += 86400;
    t = (double)tt;
    when = "Tonight at " + fmtHM(tt, OFF, C.h12, true);
  }
  const float cx = 240, cy = 252, R = 194;
  for (int y = (int)(cy - R - 1); y <= (int)(cy + R + 1); y++)
    for (int x = (int)(cx - R - 1); x <= (int)(cx + R + 1); x++) {
      float d = hypotf(x + .5f - cx, y + .5f - cy);
      if (d > R + 1) continue;
      gPixel(x, y, lerp565(RGB(11, 20, 38), RGB(3, 6, 12), d / R), (uint8_t)(constrain(R - d + .5f, 0.f, 1.f) * 255));
    }
  gRing(cx, cy, R, 1, RGB(31, 39, 51));
  for (int a : {30, 60}) gDashedArc(cx, cy, R * (90 - a) / 90.f, 1, 0, TWO_PI, 2, 6, RGB(18, 28, 44));
  gClipCircle(cx, cy, R);
  auto P2 = [&](double alt, double az, float &x, float &y) { float r = R * (90 - alt) / 90; x = cx - r * sinf(az * RADf); y = cy - r * cosf(az * RADf); };
  static AltAz pos[100];
  static float sx[100], sy[100];
  for (int i = 0; i < N_STARS; i++) {
    pos[i] = altaz({STARS[i].ra * 15 * (PI / 180), STARS[i].dec * (PI / 180)}, t, C.lat, C.lon);
    P2(pos[i].alt, pos[i].az, sx[i], sy[i]);
  }
  auto idx = [](const char *n) { for (int i = 0; i < N_STARS; i++) if (!strcmp(STARS[i].name, n)) return i; return 0; };
  for (int c = 0; c < N_CONS; c++) {
    float sumX = 0, maxY = -1e9; int seen = 0; bool used[100] = {};
    for (int l = 0; l < N_CON_LINES; l++) {
      if (CON_LINES[l].con != c) continue;
      int a = idx(CON_LINES[l].a), b = idx(CON_LINES[l].b);
      if (pos[a].alt < -3 && pos[b].alt < -3) continue;
      gLine(sx[a], sy[a], sx[b], sy[b], 1, RGB(42, 61, 92));
      for (int q : {a, b}) if (pos[q].alt > 0 && !used[q]) { used[q] = true; seen++; sumX += sx[q]; maxY = fmaxf(maxY, sy[q]); }
    }
    if (seen >= 3) {
      String n = CON_NAMES[c]; n.toUpperCase();
      gText(n.c_str(), sumX / seen, maxY + 20, 10, F500, RGB(62, 88, 128), CENTER, 1.6f);
    }
  }
  for (int i = 0; i < N_STARS; i++) {
    if (pos[i].alt < -1) continue;
    float m = STARS[i].mag, r = fmaxf(.7f, 2.9f - .62f * m), o = fminf(1, .5f + (2.6f - m) * .2f);
    gCircle(sx[i], sy[i], r, RGB(242, 245, 255), (uint8_t)(o * 255));
    if ((m < .9f || !strcmp(STARS[i].name, "Polaris")) && pos[i].alt > 3) gText(STARS[i].name, sx[i] + r + 4, sy[i] + 3.5f, 10, F400, RGB(133, 147, 168));
  }
  for (int n = 0; n < 5; n++) {
    int k = NAKED[n];
    AltAz h = altaz(eclToEq(geoPlanet(k, t)), t, C.lat, C.lon);
    if (h.alt < -1) continue;
    float x, y; P2(h.alt, h.az, x, y);
    float r = PLANET[k].size * .45f + 2;
    gCircle(x, y, r, PLANET[k].color);
    gText(PLANET[k].name, x + r + 5, y + 4, 11, F500, PLANET[k].color);
  }
  AltAz mh = altaz(eclToEq(moonGeo(t)), t, C.lat, C.lon);
  if (mh.alt > -1) {
    float x, y; P2(mh.alt, mh.az, x, y);
    gCircle(x, y, 16, C_MOON, 30); gCircle(x, y, 7, C_MOON);
    gText("Moon", x + 12, y + 4, 11, F500, C_MOON);
  }
  gClipNone();
  gText("N", cx, cy - R + 17, 13, F500, C_DIM, CENTER);
  gText("S", cx, cy + R - 9, 13, F500, C_DIM, CENTER);
  gText("E", cx - R + 14, cy + 4, 13, F500, C_DIM, CENTER);
  gText("W", cx + R - 14, cy + 4, 13, F500, C_DIM, CENTER);
  label("SKY", 26, 40);
  gText(when.c_str(), 454, 40, 14, F400, C_SOFT, RIGHT);
}

static void sPlanets() {
  label("PLANETS TONIGHT");
  if (!pOk) { gText("Working out the sky\xE2\x80\xA6", 240, 260, 18, F400, C_DIM, CENTER); return; }
  for (int n = 0; n < 5; n++) {
    const PlanetReport &r = P[n];
    float top = 64 + n * 76;
    if (n) sep(top);
    const PlanetInfo &pi = PLANET[r.p];
    gCircle(48, top + 38, fminf(10, pi.size + 2), pi.color, r.kind ? 255 : 90);
    gText(pi.name, 72, top + 36, 22, F500, C_FG);
    uint16_t col = r.kind == 2 ? C_GOOD : r.kind == 1 ? C_SUN : C_DIM;
    String st = r.status; st.toUpperCase();
    int sw = gText(st.c_str(), 72, top + 59, 12, F500, col, LEFT, 1.2f);
    String sub = String(" \xC2\xB7 ") + r.sub;
    int bigW = r.big[0] ? gTextW(r.big, 26, F300) : 20;
    gText(gEllipsize(sub, 14, F400, 444 - bigW - 16 - (72 + sw)).c_str(), 72 + sw, top + 59, 14, F400, C_DIM);
    gText(r.big[0] ? r.big : "\xE2\x80\x94", 444, top + 38, 26, F300, C_FG, RIGHT);
    float lm = r.distAu * 8.317f;
    char b[32];
    if (lm < 60) snprintf(b, sizeof b, "%d light-min away", (int)lroundf(lm)); else snprintf(b, sizeof b, "%dh %dm light-time", (int)(lm / 60), (int)lroundf(fmodf(lm, 60)));
    gText(b, 444, top + 59, 13, F400, C_DIM, RIGHT);
  }
}

static void sSolar() {
  label("SOLAR SYSTEM TODAY");
  const float cx = 240, cy = 214;
  const float RADII[8] = {22, 36, 51, 65, 90, 112, 131, 148};
  for (float r : RADII) gRing(cx, cy, r, 1, RGB(26, 31, 39));
  for (int r = 17; r > 0; r--) gCircle(cx, cy, r, C_SUN, (uint8_t)(16 + (17 - r) * 9));
  gCircle(cx, cy, 6.5f, RGB(255, 216, 102));
  double t = (double)NOW;
  V3 e = helio(EARTH, t);
  int nearP = -1; double nearD = 1e9;
  for (int k = 0; k < N_PLANETS; k++) {
    V3 p = helio(k, t);
    float lam = atan2f(p.y, p.x), r = RADII[k], x = cx + r * cosf(lam), y = cy - r * sinf(lam);
    const PlanetInfo &pi = PLANET[k];
    if (k == SATURN) {   // ring as a thin tilted ellipse
      for (int s = 0; s < 64; s++) {
        float a0 = s * TWO_PI / 64, a1 = (s + 1) * TWO_PI / 64, rx = pi.size * 1.9f, ry = pi.size * .6f, ca = cosf(-20 * RADf), sa = sinf(-20 * RADf);
        float x0 = rx * cosf(a0), y0 = ry * sinf(a0), x1 = rx * cosf(a1), y1 = ry * sinf(a1);
        gLine(x + x0 * ca - y0 * sa, y + x0 * sa + y0 * ca, x + x1 * ca - y1 * sa, y + x1 * sa + y1 * ca, 1.4f, pi.color, 200);
      }
    }
    if (k == EARTH) gRing(x, y, pi.size + 4, 1, C_RAIN, 140);
    gCircle(x, y, pi.size, pi.color);
    if (k >= EARTH) {
      bool right = cosf(lam) >= 0;
      gText(pi.name, x + (right ? 1 : -1) * (pi.size + 6), y + 4, 11, F400, k == EARTH ? C_SOFT : C_DIM, right ? LEFT : RIGHT);
    }
    if (k != EARTH) { double d = sqrt((p.x - e.x) * (p.x - e.x) + (p.y - e.y) * (p.y - e.y) + (p.z - e.z) * (p.z - e.z)); if (d < nearD) { nearD = d; nearP = k; } }
  }
  gText("positions true \xC2\xB7 distances not to scale", 444, 372, 10, F400, RGB(58, 63, 71), RIGHT);
  double dSun = vlen(e), lt = dSun * 499.005;
  char b1[16], b2[16], b3[16];
  snprintf(b1, sizeof b1, "%.1f", dSun * 149.598);
  snprintf(b2, sizeof b2, "%dm %02ds", (int)(lt / 60), (int)lround(fmod(lt, 60)));
  snprintf(b3, sizeof b3, "%dM km", (int)lround(nearD * 149.6));
  statCol(36, LEFT, "TO THE SUN", b1, "M km", 22);
  statCol(220, CENTER, "LIGHT TIME", b2, nullptr, 22);
  gText("NEAREST", 444, 404, 13, F500, C_DIM, RIGHT, 1.8f);
  int w3 = gTextW(b3, 15, F400);
  gText(b3, 444, 440, 15, F400, C_DIM, RIGHT);
  gText(PLANET[nearP].name, 444 - w3 - 6, 440, 22, F400, C_FG, RIGHT);
}

static void sEvents() {
  label("COMING UP IN THE SKY");
  static const uint16_t COL[5] = {RGB(159, 208, 255), C_MOON, RGB(255, 159, 67), C_GOOD, C_SUN};
  LT today = toLocal(NOW, OFF);
  int64_t todayDay = daysFromCivil(today.y, today.mo, today.d);
  for (int i = 0; i < nE && i < 6; i++) {
    float top = 64 + i * 64;
    if (i) sep(top);
    LT d = toLocal((int64_t)E[i].t, OFF);
    String m = MONTHS_SHORT[d.mo - 1]; m.toUpperCase();
    gText(m.c_str(), 61, top + 26, 12, F500, COL[E[i].type], CENTER, 1.6f);
    gText(String(d.d).c_str(), 61, top + 53, 26, F400, C_FG, CENTER, 0, true);
    int dd = (int)(daysFromCivil(d.y, d.mo, d.d) - todayDay);
    bool night = E[i].type <= 1;
    char when[20];
    if (dd <= 0) strcpy(when, night ? "Tonight" : "Today");
    else if (dd == 1) strcpy(when, "Tomorrow");
    else if (dd < 60) snprintf(when, sizeof when, "in %d days", dd);
    else snprintf(when, sizeof when, "in %d months", (int)lround(dd / 30.4));
    int ww = gText(when, 444, top + 38, 14, F400, C_SOFT, RIGHT);
    int maxW = 444 - ww - 12 - 100;
    gText(gEllipsize(E[i].title, 18, F500, maxW).c_str(), 100, top + 30, 18, F500, C_FG);
    gText(gEllipsize(E[i].sub, 13, F400, maxW).c_str(), 100, top + 50, 13, F400, C_DIM);
  }
}

static void sIss() {
  // header with pulsing live dot
  gCircle(40, 47, 8, C_BAD, 50);
  gCircle(40, 47, 4, C_BAD);
  // live position: interpolate the downloaded track at the current time
  float ilat = 0, ilon = 0;
  bool have = false;
  for (int i = 0; i + 1 < I.nt && !have; i++) {
    if (NOW < I.tr[i].ts || NOW > I.tr[i + 1].ts) continue;
    float f = (float)(NOW - I.tr[i].ts) / (float)(I.tr[i + 1].ts - I.tr[i].ts);
    float a1 = I.tr[i].lat * RADf, o1 = I.tr[i].lon * RADf, a2 = I.tr[i + 1].lat * RADf, o2 = I.tr[i + 1].lon * RADf, v[3];
    float A[3] = {cosf(a1) * cosf(o1), cosf(a1) * sinf(o1), sinf(a1)}, B[3] = {cosf(a2) * cosf(o2), cosf(a2) * sinf(o2), sinf(a2)};
    for (int j = 0; j < 3; j++) v[j] = A[j] * (1 - f) + B[j] * f;
    float n = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    ilat = asinf(v[2] / n) * DEGf; ilon = atan2f(v[1], v[0]) * DEGf;
    have = true;
  }
  label("SPACE STATION \xC2\xB7 LIVE", 54, 52);
  if (!have) { gText(I.ok ? "Waiting for the next update\xE2\x80\xA6" : "Finding the station\xE2\x80\xA6", 240, 250, 18, F400, C_DIM, CENTER); return; }
  char b[48];
  snprintf(b, sizeof b, "%.1f\xC2\xB0%c  %.1f\xC2\xB0%c", fabsf(ilat), ilat >= 0 ? 'N' : 'S', fabsf(ilon), ilon >= 0 ? 'E' : 'W');
  gText(b, 444, 52, 14, F400, C_SOFT, RIGHT, 0, true);
  const float cx = 240, cy = 226, R = 150, lat0 = ilat * RADf, lon0 = ilon * RADf;
  auto proj = [&](float la, float lo, float &x, float &y) {
    la *= RADf; lo *= RADf;
    float cl = cosf(la), dl = lo - lon0;
    x = cx + R * cl * sinf(dl);
    y = cy - R * (cosf(lat0) * sinf(la) - sinf(lat0) * cl * cosf(dl));
    return sinf(lat0) * sinf(la) + cosf(lat0) * cl * cosf(dl);
  };
  for (int y = (int)(cy - R - 1); y <= (int)(cy + R + 1); y++)
    for (int x = (int)(cx - R - 1); x <= (int)(cx + R + 1); x++) {
      float d = hypotf(x + .5f - cx, y + .5f - cy);
      if (d > R + 1) continue;
      float g = hypotf(x - (cx - .24f * R), y - (cy - .36f * R)) / (1.5f * R);
      gPixel(x, y, lerp565(RGB(14, 26, 44), RGB(3, 6, 11), g), (uint8_t)(constrain(R - d + .5f, 0.f, 1.f) * 255));
    }
  gRing(cx, cy, R, 1, RGB(29, 42, 60));
  auto curve = [&](float la0, float lo0, float la1, float lo1, int steps, uint16_t c, float th, uint8_t a, bool dashed) {
    float px = 0, py = 0; bool pv = false;
    for (int s = 0; s <= steps; s++) {
      float f = (float)s / steps, x, y;
      bool v = proj(la0 + (la1 - la0) * f, lo0 + (lo1 - lo0) * f, x, y) > 0;
      if (v && pv && (!dashed || (s & 1))) gLine(px, py, x, y, th, c, a);
      px = x; py = y; pv = v;
    }
  };
  for (int lo = -180; lo < 180; lo += 30) curve(-90, lo, 90, lo, 45, RGB(20, 33, 58), 1, 255, false);
  for (int la = -60; la <= 60; la += 30) curve(la, -180, la, 180, 90, RGB(20, 33, 58), 1, 255, false);
  const int NL = sizeof(LAND_DOTS) / sizeof(LAND_DOTS[0]);
  for (int i = 0; i < NL; i++) {
    float x, y, k = proj(LAND_DOTS[i][0] / 100.f, LAND_DOTS[i][1] / 100.f, x, y);
    if (k > .03f) gCircle(x, y, 1.1f + .7f * k, RGB(79, 112, 153), (uint8_t)((.25f + .75f * k) * 255));
  }
  // orbit track (past solid, future dotted), interpolated along great circles
  for (int i = 0; i + 1 < I.nt; i++) {
    if (I.tr[i + 1].ts < NOW - 1800 || I.tr[i].ts > NOW + 2700) continue;   // ~30 min behind, 45 min ahead
    float a1 = I.tr[i].lat * RADf, o1 = I.tr[i].lon * RADf, a2 = I.tr[i + 1].lat * RADf, o2 = I.tr[i + 1].lon * RADf;
    float A[3] = {cosf(a1) * cosf(o1), cosf(a1) * sinf(o1), sinf(a1)}, B[3] = {cosf(a2) * cosf(o2), cosf(a2) * sinf(o2), sinf(a2)};
    float px = 0, py = 0; bool pv = false;
    for (int s = 0; s <= 12; s++) {
      float f = s / 12.f, v[3];
      for (int j = 0; j < 3; j++) v[j] = A[j] * (1 - f) + B[j] * f;
      float n = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
      float x, y, k = proj(asinf(v[2] / n) * DEGf, atan2f(v[1], v[0]) * DEGf, x, y);
      bool past = I.tr[i].ts + f * (I.tr[i + 1].ts - I.tr[i].ts) <= NOW;
      bool vis = k > 0;
      if (vis && pv) { if (past) gLine(px, py, x, y, 2, C_SUN, 115); else if (s & 1) gLine(px, py, x, y, 2, C_SUN); }
      px = x; py = y; pv = vis;
    }
  }
  float ux, uy;
  if (proj(C.lat, C.lon, ux, uy) > 0) {
    gCircle(ux, uy, 4, C_RAIN); gRing(ux, uy, 9, 1, C_RAIN, 128);
    gText("You", ux + 12, uy + 4, 11, F400, RGB(156, 200, 255));
  }
  gCircle(cx, cy, 16, C_FG, 26); gCircle(cx, cy, 5.5f, C_FG);
  float km = 0;
  {
    float p1 = C.lat * RADf, p2 = ilat * RADf, dl = (ilon - C.lon) * RADf;
    km = 6371 * 2 * asinf(sqrtf(powf(sinf((p2 - p1) / 2), 2) + cosf(p1) * cosf(p2) * powf(sinf(dl / 2), 2)));
  }
  char v1[12], v2[12], v3[12];
  snprintf(v1, sizeof v1, "%d", (int)lroundf(I.alt));
  int sp = (int)lroundf(I.vel); snprintf(v2, sizeof v2, "%d,%03d", sp / 1000, sp % 1000);
  int kk = (int)lroundf(km); if (kk >= 1000) snprintf(v3, sizeof v3, "%d,%03d", kk / 1000, kk % 1000); else snprintf(v3, sizeof v3, "%d", kk);
  statCol(36, LEFT, "ALTITUDE", v1, "km", 22);
  statCol(240, CENTER, "SPEED", v2, "km/h", 22);
  statCol(444, RIGHT, "FROM YOU", v3, "km", 22);
}

static void sClock() {
  label("WORLD");
  for (int i = 0; i < 3; i++) {
    float top = 66 + i * 126, cy = top + 63;
    if (i) sep(top);
    const WClock &w = C.wc[i];
    LT l = toLocal(NOW, w.off);
    bool day = l.h >= 7 && l.h < 19;
    gCircle(42, cy - 9, 4.5f, day ? C_SUN : RGB(111, 134, 166));
    gText(gEllipsize(w.name, 24, F500, 200).c_str(), 56, cy, 24, F500, C_FG);
    int diff = (w.off - OFF) / 60;
    LT me = toLocal(NOW, OFF);
    int dayDiff = (int)(daysFromCivil(l.y, l.mo, l.d) - daysFromCivil(me.y, me.mo, me.d));
    char b[40];
    if (!diff) strcpy(b, "Same time");
    else if (diff % 60) snprintf(b, sizeof b, "%s%d:%02dh", diff > 0 ? "+" : "\xE2\x88\x92", abs(diff) / 60, abs(diff) % 60);
    else snprintf(b, sizeof b, "%s%dh", diff > 0 ? "+" : "\xE2\x88\x92", abs(diff) / 60);
    if (dayDiff) strcat(b, dayDiff > 0 ? " \xC2\xB7 tomorrow" : " \xC2\xB7 yesterday");
    gText(b, 42, cy + 26, 15, F400, C_DIM);
    String t = fmtHM(NOW, w.off, C.h12);
    if (C.h12) {
      const char *ap = l.h < 12 ? "AM" : "PM";
      int aw = gTextW(ap, 18, F400);
      gText(ap, 444, cy + 22, 18, F400, C_DIM, RIGHT);
      gText(t.c_str(), 444 - aw - 6, cy + 22, 62, F300, C_FG, RIGHT, -1, true);
    } else gText(t.c_str(), 444, cy + 22, 62, F300, C_FG, RIGHT, -1, true);
  }
}

static void sCalendar() {
  LT l = toLocal(NOW, OFF);
  gText(MONTHS_LONG[l.mo - 1], 36, 74, 34, F500, C_FG);
  gText(String(l.y).c_str(), 444, 74, 22, F400, C_DIM, RIGHT);
  static const char *WD[7] = {"M", "T", "W", "T", "F", "S", "S"};
  const float cw = 408 / 7.f;
  for (int i = 0; i < 7; i++) gText(WD[i], 36 + cw * i + cw / 2, 116, 13, F500, C_DIM, CENTER, 1.3f);
  int64_t first = daysFromCivil(l.y, l.mo, 1);
  int wdFirst = (int)(((first % 7) + 11) % 7);   // 0 = Sunday
  int shift = (wdFirst + 6) % 7;                // Monday first
  int64_t todayD = daysFromCivil(l.y, l.mo, l.d);
  for (int i = 0; i < 42; i++) {
    int64_t dd = first - shift + i;
    LT d = toLocal(dd * 86400, 0);
    if (i >= 35 && d.mo != l.mo) break;
    float x = 36 + cw * (i % 7) + cw / 2, y = 152 + (i / 7) * 48;
    uint16_t col = d.mo != l.mo ? RGB(58, 63, 71) : (i % 7 >= 5) ? C_SOFT : C_FG;
    if (dd == todayD) { gCircle(x, y, 22, C_SUN); col = C_BG; }
    gText(String(d.d).c_str(), x, y + 8, 21, dd == todayD ? F500 : F400, col, CENTER, 0, true);
  }
}

static void sAir() {
  label("AIR QUALITY");
  if (A.ok) {
    int v = A.aqi;
    struct Band { int max; const char *cat; uint16_t col; const char *desc; };
    static const Band B[] = {{50, "Good", C_GOOD, "Air is clean. Great for a walk."}, {100, "Moderate", C_MOD, "Acceptable for most people."},
                             {150, "Sensitive", C_USG, "Sensitive groups should take it easy."}, {200, "Unhealthy", C_BAD, "Limit time outdoors."},
                             {100000, "Very poor", C_VBAD, "Stay inside if you can."}};
    const Band *b = B;
    while (v > b->max) b++;
    String n = String(v);
    int nw = gText(n.c_str(), 32, 196, 120, F300, b->col, LEFT, -4, true);
    gText(b->cat, 32 + nw + 24, 146, 28, F500, C_FG);
    String lines[3];
    int nl = gWrap(b->desc, 16, F400, 444 - (32 + nw + 24), lines, 3);
    for (int i = 0; i < nl; i++) gText(lines[i].c_str(), 32 + nw + 24, 174 + i * 22, 16, F400, C_DIM);
    gRoundRect(36, 234, 408, 8, 4, C_FAINT);
    const float seg[6] = {0, .16f, .33f, .5f, .66f, 1};
    const uint16_t sc[5] = {C_GOOD, C_MOD, C_USG, C_BAD, C_VBAD};
    for (int i = 0; i < 5; i++) gRoundRect(36 + 408 * seg[i], 234, 408 * (seg[i + 1] - seg[i]) + (i < 4 ? 4 : 0), 8, i == 0 || i == 4 ? 4 : 0, sc[i]);
    float mx = 36 + 408 * fminf(1, v / 300.f);
    gRoundRect(mx - 5, 225, 10, 26, 4, C_BG);
    gRoundRect(mx - 2, 228, 4, 20, 2, C_FG);
  } else gText(stale() ? "No update yet" : "Loading\xE2\x80\xA6", 36, 196, 28, F400, C_DIM);
  if (W.ok) {
    statCol(36, LEFT, "HUMIDITY", String(W.hum), "%");
    statCol(240, CENTER, "WIND", String((int)lroundf(W.wind)), C.fahrenheit ? "mph" : "km/h");
    statCol(444, RIGHT, "FEELS LIKE", deg(W.feels), nullptr);
  }
}

/* ---------------- stargazing tonight ---------------- */
// Scores each hour of tonight's darkness (Sun below -12 degrees) from 0 to 10:
// clear sky counts most, a bright Moon above the horizon takes points away.
static void sStargaze() {
  label("STARGAZING TONIGHT");
  if (!timeOk) return;
  const double step = 900;   // 15 minutes
  double t = (double)NOW, start = 0, end = 0;
  for (int i = 0; i < 4 * 30 && !end; i++, t += step) {
    bool dark = altaz(eclToEq(sunGeo(t)), t, C.lat, C.lon).alt < -12;
    if (dark && !start) start = t;
    if (!dark && start) end = t;
  }
  if (!start) { gText("No real darkness tonight", 240, 250, 22, F400, C_SOFT, CENTER); return; }
  if (!end) end = start + 12 * 3600;
  double p = moonPhase(start);
  float illum = (1 - cosf(TWO_PI * p)) / 2;
  // hourly slots across the dark window
  struct Slot { double t, e; int cloud; bool moonUp; float score; };   // t..e, aligned to clock hours
  Slot sl[16];
  int n = 0;
  bool haveCloud = false;
  for (double h = start; h < end && n < 16; n++) {
    // first slot runs from dusk to the next full hour, the last one ends at dawn
    double e = fmin(end, (double)(((int64_t)h + OFF) / 3600 * 3600 + 3600 - OFF));
    double mid = (h + e) / 2;
    sl[n].t = h;
    sl[n].e = e;
    h = e;
    sl[n].cloud = -1;
    for (int k = 0; k < W.nc; k++) if (W.c[k].t <= mid && mid < W.c[k].t + 3600 && W.c[k].cloud >= 0) { sl[n].cloud = W.c[k].cloud; haveCloud = true; }
    sl[n].moonUp = altaz(eclToEq(moonGeo(mid)), mid, C.lat, C.lon).alt > 0;
    float clear = sl[n].cloud >= 0 ? 1 - sl[n].cloud / 100.f : .5f;
    sl[n].score = 10 * clear * (1 - .7f * illum * (sl[n].moonUp ? 1 : 0));
  }
  int best = 0;
  for (int i = 1; i < n; i++) if (sl[i].score > sl[best].score) best = i;
  int w0 = best, w1 = best;   // best window: neighbouring hours within 1 point of the best
  while (w0 > 0 && sl[w0 - 1].score >= sl[best].score - 1) w0--;
  while (w1 + 1 < n && sl[w1 + 1].score >= sl[best].score - 1) w1++;
  float sc = sl[best].score;
  uint16_t col = sc >= 7 ? C_GOOD : sc >= 4 ? C_SUN : C_DIM;
  const char *word = sc >= 8 ? "Excellent" : sc >= 6 ? "Good" : sc >= 4 ? "Fair" : sc >= 2 ? "Poor" : "Not tonight";
  int avgCloud = 0, cn = 0;
  for (int i = w0; i <= w1; i++) if (sl[i].cloud >= 0) { avgCloud += sl[i].cloud; cn++; }
  if (cn) avgCloud /= cn;

  // score + verdict
  char b[48];
  snprintf(b, sizeof b, "%d", (int)lroundf(sc));
  int nw = gText(b, 32, 196, 120, F300, col, LEFT, -4, true);
  gText("/10", 32 + nw + 4, 196, 26, F400, C_DIM);
  float tx = 32 + nw + 70;
  gText(word, tx, 138, 28, F500, C_FG);
  snprintf(b, sizeof b, "Best %s \xE2\x80\x93 %s", fmtHM((int64_t)sl[w0].t, OFF, C.h12, true).c_str(), fmtHM((int64_t)sl[w1].e, OFF, C.h12, true).c_str());
  gText(b, tx, 168, 16, F400, C_SOFT);
  const char *why = !haveCloud ? "No cloud forecast yet" : avgCloud > 60 ? "Mostly cloudy" : (sl[best].moonUp && illum > .5f) ? "Bright Moon in the sky" : avgCloud > 25 ? "Some clouds" : "Clear, dark skies";
  gText(why, tx, 192, 16, F400, C_DIM);

  // timeline: one bar per dark hour (taller = clearer), Moon strip underneath, best window outlined
  const float x0 = 36, wTot = 408, top = 238, base = 326, bw = wTot / max(n, 1);
  for (int i = 0; i < n; i++) {
    float x = x0 + i * bw, clear = sl[i].cloud >= 0 ? 1 - sl[i].cloud / 100.f : .5f;
    gRoundRect(x + 3, top, bw - 6, base - top, 3, C_FAINT);
    float hgt = fmaxf(4, (base - top) * clear);
    gRoundRect(x + 3, base - hgt, bw - 6, hgt, 3, i >= w0 && i <= w1 ? col : C_RAIN, i >= w0 && i <= w1 ? 255 : 150);
    if (sl[i].moonUp) gRoundRect(x + 3, base + 6, bw - 6, 4, 2, C_MOON, (uint8_t)(80 + 175 * illum));
    LT l = toLocal((int64_t)sl[i].e, OFF);   // label each bar with the hour it leads up to
    if ((l.h % 2 == 0 || n <= 8) && sl[i].e < end) {
      char hb[8];
      if (C.h12) snprintf(hb, sizeof hb, "%d%s", l.h % 12 ? l.h % 12 : 12, l.h < 12 ? "a" : "p"); else snprintf(hb, sizeof hb, "%02d", l.h);
      gText(hb, x + bw / 2, 358, 13, F400, C_DIM, CENTER);
    }
  }

  // stats
  int darkMin = (int)((end - start) / 60);
  char v1[8], v2[8], v3[12];
  if (cn) snprintf(v1, sizeof v1, "%d", avgCloud); else strcpy(v1, "--");
  snprintf(v2, sizeof v2, "%d", (int)lroundf(illum * 100));
  snprintf(v3, sizeof v3, "%dh %02dm", darkMin / 60, darkMin % 60);
  statCol(36, LEFT, "CLOUDS", v1, "%", 22);
  statCol(240, CENTER, "MOON", v2, "% lit", 22);
  statCol(444, RIGHT, "DARKNESS", v3, nullptr, 22);
}

static void sCountdown() {
  int y, mo, d, h = 0, mi = 0;
  bool ok = sscanf(C.cdDate, "%d-%d-%dT%d:%d", &y, &mo, &d, &h, &mi) >= 3;
  int64_t target = ok ? localToUtc(y, mo, d, h, mi, OFF) : NOW;
  int64_t s = max<int64_t>(0, target - NOW);
  String lbl = C.cdLabel; lbl.toUpperCase();
  gText(lbl.c_str(), 240, 132, 16, F500, C_SUN, CENTER, 3.2f);
  gText(String((long)(s / 86400)).c_str(), 240, 290, 170, F300, C_FG, CENTER, -8, true);
  gText("days to go", 240, 330, 22, F400, C_SOFT, CENTER);
  s %= 86400;
  char b[24]; snprintf(b, sizeof b, "%02d : %02d : %02d", (int)(s / 3600), (int)(s % 3600 / 60), (int)(s % 60));
  gText(b, 240, 388, 34, F300, C_DIM, CENTER, 1.3f, true);
}

static const char *QUOTES[][2] = {
  {"The best time to plant a tree was 20 years ago. The second best time is now.", "Proverb"},
  {"You have power over your mind, not outside events.", "Marcus Aurelius"},
  {"Well begun is half done.", "Aristotle"},
  {"It is not that we have a short time to live, but that we waste a lot of it.", "Seneca"},
  {"Simplicity is the ultimate sophistication.", "Attributed to Leonardo da Vinci"},
  {"The journey of a thousand miles begins with one step.", "Lao Tzu"},
  {"Nothing is so painful to the human mind as a great and sudden change.", "Mary Shelley"},
  {"Little by little, one travels far.", "Proverb"},
  {"We suffer more often in imagination than in reality.", "Seneca"},
  {"He who has a why to live can bear almost any how.", "Friedrich Nietzsche"},
  {"Act as if what you do makes a difference. It does.", "William James"},
  {"Waste no more time arguing what a good person should be. Be one.", "Marcus Aurelius"},
};
static void sQuote() {
  LT l = toLocal(NOW, OFF);
  int n = sizeof(QUOTES) / sizeof(QUOTES[0]);
  const char **q = QUOTES[(l.yd + 1) % n];
  String lines[6];
  int nl = gWrap(q[0], 31, F300, 408, lines, 6);
  float h = 50 + nl * 42 + 40, y0 = 240 - h / 2;
  gText("\xE2\x80\x9C", 32, y0 + 70, 110, F400, C_SUN);
  for (int i = 0; i < nl; i++) gText(lines[i].c_str(), 36, y0 + 82 + i * 42, 31, F300, C_FG);
  String by = String("\xE2\x80\x94 ") + q[1];
  gText(by.c_str(), 36, y0 + 82 + nl * 42 + 18, 17, F400, C_DIM, LEFT, 1);
}

/* ---------------- setup / info / connecting ---------------- */
static void drawQr(const char *text, float cx, float cy, int px) {
  QRCode qr;
  static uint8_t data[300];
  qrcode_initText(&qr, data, 5, ECC_LOW, text);
  int sz = qr.size * px, x0 = (int)(cx - sz / 2), y0 = (int)(cy - sz / 2);
  gRoundRect(x0 - 12, y0 - 12, sz + 24, sz + 24, 10, C_FG);
  for (int y = 0; y < qr.size; y++)
    for (int x = 0; x < qr.size; x++)
      if (qrcode_getModule(&qr, x, y)) gFillRect(x0 + x * px, y0 + y * px, px, px, C_BG);
}
static void orbitMotif(float cx, float cy) {
  gRing(cx, cy, 52, 2, C_LINE);
  gCircle(cx, cy, 12, C_SUN);
  float a = (millis() % 6000) / 6000.f * TWO_PI;
  gCircle(cx + 52 * cosf(a), cy + 52 * sinf(a), 6, C_RAIN);
}
static void sSetup() {
  gText("Connect to Wi-Fi", 240, 56, 28, F500, C_FG, CENTER);
  String qr = String("WIFI:T:WPA;S:" SETUP_AP ";P:") + apPass + ";;";
  drawQr(qr.c_str(), 240, 176, 4);
  gText("1  On your phone, join the Wi-Fi (or scan the code)", 44, 300, 16, F400, C_SOFT);
  gText(SETUP_AP, 68, 326, 19, F500, C_SUN);
  gText("password", 68, 352, 15, F400, C_DIM);
  gText(apPass, 140, 353, 21, F500, C_SUN, LEFT, 1.5f);
  gText("2  A page opens (or visit 192.168.4.1)", 44, 388, 16, F400, C_SOFT);
  gText("3  Pick your network and type its password", 44, 414, 16, F400, C_SOFT);
  if (setupFailed) {
    String m = String("Couldn't join \xE2\x80\x9C") + wifiSsid + "\xE2\x80\x9D. Try again.";
    gText(gEllipsize(m, 15, F400, 420).c_str(), 240, 450, 15, F400, C_BAD, CENTER);
  }
}
static void sInfo() {
  gText("Settings", 240, 56, 28, F500, C_FG, CENTER);
  if (!wifiOk) {
    orbitMotif(240, 200);
    gText("Turning on Wi-Fi\xE2\x80\xA6", 240, 318, 20, F400, C_SOFT, CENTER);
    gText("Settings stay reachable for 10 minutes", 240, 350, 15, F400, C_DIM, CENTER);
    return;
  }
  String url = String("http://") + ipStr + "/";
  String qr = url + "#pin=" + pairPin;   // scanning the code also pairs the phone
  drawQr(qr.c_str(), 240, 172, 4);
  gText("Scan, or open on a phone or PC on the same Wi-Fi:", 240, 294, 15, F400, C_DIM, CENTER);
  gText(url.c_str(), 240, 326, 24, F500, C_SUN, CENTER);
  gText("PAIRING CODE", 240, 366, 13, F500, C_DIM, CENTER, 1.8f);
  char pin[12]; snprintf(pin, sizeof pin, "%.3s %.3s", pairPin, pairPin + 3);
  gText(pin, 240, 408, 34, F500, C_FG, CENTER, 2, true);
  String w = String("Wi-Fi on for 10 min \xC2\xB7 v" FW_VERSION);
  gText(w.c_str(), 240, 446, 13, F400, C_DIM, CENTER);
}
static void sUpdate() {
  int pct = max(0, (int)otaPercent);
  gRing(240, 200, 90, 8, C_FAINT);
  if (pct > 0) gArc(240, 200, 90, 8, -HALF_PI, -HALF_PI + TWO_PI * pct / 100.f, C_SUN);
  char b[8]; snprintf(b, sizeof b, "%d%%", pct);
  gText(b, 240, 216, 44, F300, C_FG, CENTER, 0, true);
  String t = String(otaPhase >= 3 ? "Restarting on " : otaPhase == 2 ? "Installing " : "Downloading ") + otaNewVersion;
  gText(t.c_str(), 240, 352, 26, F500, C_FG, CENTER);
  gText(otaPhase == 2 ? "The screen turns off for a few seconds" : "Please keep the display powered", 240, 388, 17, F400, C_DIM, CENTER);
}
static void sConnecting() {
  orbitMotif(240, 180);
  gText("SkyFrame", 240, 300, 40, F300, C_FG, CENTER);
  String m = wifiSsid[0] ? String("Connecting to ") + wifiSsid + "\xE2\x80\xA6" : String("Starting\xE2\x80\xA6");
  gText(gEllipsize(m, 20, F400, 420).c_str(), 240, 342, 20, F400, C_SOFT, CENTER);
}

/* ---------------- entry points ---------------- */
static void dots(int idx, int count) {
  if (count <= 1 || idx < 0) return;
  float w = (count - 1) * 12 + 14, x = 240 - w / 2;
  for (int i = 0; i < count; i++) {
    if (i == idx) { gRoundRect(x, 463, 14, 5, 2.5f, C_DIM); x += 21; }
    else { gCircle(x + 2.5f, 465.5f, 2.5f, C_LINE); x += 12; }
  }
}

void drawScreen(int id, int dotIndex, int dotCount) {
  LOCK();
  C = CFG; W = WX; A = AQ; I = ISS; memcpy(P, PLAN, sizeof P); pOk = planOk; memcpy(E, EVENTS, sizeof E); nE = nEvents;
  UNLOCK();
  NOW = nowUtc();
  OFF = W.ok ? W.off : C.off;
  gClear(C_BG);
  switch (id) {
    case S_HOME: sHome(); break;
    case S_FORECAST: sForecast(); break;
    case S_HOURLY: sHourly(); break;
    case S_SUN: sSun(); break;
    case S_MOON: sMoon(); break;
    case S_SKY: sSky(); break;
    case S_PLANETS: sPlanets(); break;
    case S_SOLAR: sSolar(); break;
    case S_EVENTS: sEvents(); break;
    case S_ISS: sIss(); break;
    case S_CLOCK: sClock(); break;
    case S_CALENDAR: sCalendar(); break;
    case S_AIR: sAir(); break;
    case S_STARGAZE: sStargaze(); break;
    case S_COUNTDOWN: sCountdown(); break;
    case S_QUOTE: sQuote(); break;
    case S_SETUP: sSetup(); break;
    case S_INFO: sInfo(); break;
    case S_CONNECTING: sConnecting(); break;
    case S_UPDATE: sUpdate(); break;
  }
  if (id < N_SCREENS) dots(dotIndex, dotCount);
  if (id < N_SCREENS && stale()) gText("offline", 466, 20, 11, F400, RGB(68, 68, 68), RIGHT);
}

bool screenTap(int id, int x, int y) { return false; }   // no screen uses taps itself any more
