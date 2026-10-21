// Small anti-aliased 2D renderer into an RGB565 480x480 buffer, with TrueType text (stb_truetype + glyph cache)
#include "gfx.h"
#include "hw.h"
#include <unordered_map>
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

extern const uint8_t inter300[] asm("_binary_data_inter300_ttf_start");
extern const uint8_t inter400[] asm("_binary_data_inter400_ttf_start");
extern const uint8_t inter500[] asm("_binary_data_inter500_ttf_start");

static uint16_t *T = nullptr;
static bool clipOn = false;
static float clipX, clipY, clipR;

void gTarget(uint16_t *buf) { T = buf; }
uint16_t *gBuf() { return T; }
void gClear(uint16_t c) {
  if (c == 0) { memset(T, 0, LCD_W * LCD_H * 2); return; }
  for (int i = 0; i < LCD_W * LCD_H; i++) T[i] = c;
}
void gClipCircle(float cx, float cy, float r) { clipOn = true; clipX = cx; clipY = cy; clipR = r; }
void gClipNone() { clipOn = false; }

uint16_t gBlend(uint16_t d, uint16_t s, uint8_t a) {
  if (a >= 252) return s;
  if (a < 4) return d;
  uint32_t al = (a + 4) >> 3;
  uint32_t bg = (d | ((uint32_t)d << 16)) & 0x07E0F81F;
  uint32_t fg = (s | ((uint32_t)s << 16)) & 0x07E0F81F;
  uint32_t r = ((((fg - bg) * al) >> 5) + bg) & 0x07E0F81F;
  return (uint16_t)(r | (r >> 16));
}

void gPixel(int x, int y, uint16_t c, uint8_t a) {
  if ((unsigned)x >= LCD_W || (unsigned)y >= LCD_H || !a) return;
  if (clipOn) {
    float dx = x + .5f - clipX, dy = y + .5f - clipY, d2 = dx * dx + dy * dy;
    if (d2 > clipR * clipR) {
      float cov = clipR - sqrtf(d2) + .5f;
      if (cov <= 0) return;
      a = (uint8_t)(a * cov);
    }
  }
  uint16_t &p = T[y * LCD_W + x];
  p = gBlend(p, c, a);
}

static inline uint8_t covA(float cov, uint8_t a) {
  if (cov <= 0) return 0;
  if (cov >= 1) return a;
  return (uint8_t)(cov * a);
}

void gFillRect(int x, int y, int w, int h, uint16_t c, uint8_t a) {
  int x0 = max(0, x), y0 = max(0, y), x1 = min(LCD_W, x + w), y1 = min(LCD_H, y + h);
  for (int yy = y0; yy < y1; yy++) {
    uint16_t *row = T + yy * LCD_W;
    if (a >= 252 && !clipOn) for (int xx = x0; xx < x1; xx++) row[xx] = c;
    else for (int xx = x0; xx < x1; xx++) gPixel(xx, yy, c, a);
  }
}

static float sdRoundBox(float px, float py, float cx, float cy, float hw, float hh, float r) {
  float qx = fabsf(px - cx) - hw + r, qy = fabsf(py - cy) - hh + r;
  float ox = fmaxf(qx, 0), oy = fmaxf(qy, 0);
  return sqrtf(ox * ox + oy * oy) + fminf(fmaxf(qx, qy), 0) - r;
}
void gRoundRect(float x, float y, float w, float h, float r, uint16_t c, uint8_t a) {
  float cx = x + w / 2, cy = y + h / 2;
  for (int yy = (int)y - 1; yy <= (int)(y + h) + 1; yy++)
    for (int xx = (int)x - 1; xx <= (int)(x + w) + 1; xx++)
      gPixel(xx, yy, c, covA(.5f - sdRoundBox(xx + .5f, yy + .5f, cx, cy, w / 2, h / 2, r), a));
}
void gRoundRectStroke(float x, float y, float w, float h, float r, float t, uint16_t c, uint8_t a) {
  float cx = x + w / 2, cy = y + h / 2;
  for (int yy = (int)y - 2; yy <= (int)(y + h) + 2; yy++)
    for (int xx = (int)x - 2; xx <= (int)(x + w) + 2; xx++) {
      float d = fabsf(sdRoundBox(xx + .5f, yy + .5f, cx, cy, w / 2, h / 2, r));
      gPixel(xx, yy, c, covA(t / 2 - d + .5f, a));
    }
}

void gCircle(float cx, float cy, float r, uint16_t c, uint8_t a) {
  int y0 = (int)floorf(cy - r - 1), y1 = (int)ceilf(cy + r + 1), x0 = (int)floorf(cx - r - 1), x1 = (int)ceilf(cx + r + 1);
  float in2 = (r - .7f) > 0 ? (r - .7f) * (r - .7f) : 0;
  for (int y = y0; y <= y1; y++) {
    float dy = y + .5f - cy;
    for (int x = x0; x <= x1; x++) {
      float dx = x + .5f - cx, d2 = dx * dx + dy * dy;
      if (d2 <= in2) gPixel(x, y, c, a);
      else gPixel(x, y, c, covA(r - sqrtf(d2) + .5f, a));
    }
  }
}

void gRing(float cx, float cy, float r, float t, uint16_t c, uint8_t a) {
  float ro = r + t / 2 + 1, ri = fmaxf(0, r - t / 2 - 1);
  int y0 = (int)floorf(cy - ro), y1 = (int)ceilf(cy + ro), x0 = (int)floorf(cx - ro), x1 = (int)ceilf(cx + ro);
  for (int y = y0; y <= y1; y++) {
    float dy = y + .5f - cy;
    for (int x = x0; x <= x1; x++) {
      float dx = x + .5f - cx, d2 = dx * dx + dy * dy;
      if (d2 > ro * ro || d2 < ri * ri) continue;
      gPixel(x, y, c, covA(t / 2 - fabsf(sqrtf(d2) - r) + .5f, a));
    }
  }
}

void gArc(float cx, float cy, float r, float t, float a0, float a1, uint16_t c, uint8_t a) {
  if (a1 < a0) { float s = a0; a0 = a1; a1 = s; }
  if (a1 - a0 >= TWO_PI - 1e-4f) { gRing(cx, cy, r, t, c, a); return; }
  float ro = r + t / 2 + 1, ri = fmaxf(0, r - t / 2 - 1), h = t / 2;
  float e0x = cx + r * cosf(a0), e0y = cy + r * sinf(a0), e1x = cx + r * cosf(a1), e1y = cy + r * sinf(a1);
  // bounding box of the arc
  float bx0 = fminf(e0x, e1x), bx1 = fmaxf(e0x, e1x), by0 = fminf(e0y, e1y), by1 = fmaxf(e0y, e1y);
  for (int k = -4; k <= 8; k++) {
    float q = k * HALF_PI;
    if (q > a0 && q < a1) { bx0 = fminf(bx0, cx + r * cosf(q)); bx1 = fmaxf(bx1, cx + r * cosf(q)); by0 = fminf(by0, cy + r * sinf(q)); by1 = fmaxf(by1, cy + r * sinf(q)); }
  }
  for (int y = (int)floorf(by0 - h - 1); y <= (int)ceilf(by1 + h + 1); y++) {
    float dy = y + .5f - cy;
    for (int x = (int)floorf(bx0 - h - 1); x <= (int)ceilf(bx1 + h + 1); x++) {
      float dx = x + .5f - cx, d2 = dx * dx + dy * dy;
      if (d2 > ro * ro || d2 < ri * ri) continue;
      float ang = atan2f(dy, dx);
      ang = a0 + fmodf(ang - a0 + 4 * TWO_PI, TWO_PI);
      float cov;
      if (ang <= a1) cov = h - fabsf(sqrtf(d2) - r) + .5f;
      else {
        float d0 = hypotf(x + .5f - e0x, y + .5f - e0y), d1 = hypotf(x + .5f - e1x, y + .5f - e1y);
        cov = h - fminf(d0, d1) + .5f;
      }
      gPixel(x, y, c, covA(cov, a));
    }
  }
}

void gDashedArc(float cx, float cy, float r, float t, float a0, float a1, float dash, float gap, uint16_t c, uint8_t a) {
  float da = dash / r, ga = gap / r;
  for (float s = a0; s < a1; s += da + ga) gArc(cx, cy, r, t, s, fminf(s + da, a1), c, a);
}

void gLine(float x0, float y0, float x1, float y1, float t, uint16_t c, uint8_t a) {
  float h = t / 2, vx = x1 - x0, vy = y1 - y0, len2 = vx * vx + vy * vy;
  int bx0 = (int)floorf(fminf(x0, x1) - h - 1), bx1 = (int)ceilf(fmaxf(x0, x1) + h + 1);
  int by0 = (int)floorf(fminf(y0, y1) - h - 1), by1 = (int)ceilf(fmaxf(y0, y1) + h + 1);
  for (int y = by0; y <= by1; y++)
    for (int x = bx0; x <= bx1; x++) {
      float px = x + .5f - x0, py = y + .5f - y0;
      float u = len2 > 0 ? fmaxf(0, fminf(1, (px * vx + py * vy) / len2)) : 0;
      float dx = px - u * vx, dy = py - u * vy;
      float d = sqrtf(dx * dx + dy * dy);
      gPixel(x, y, c, covA(h - d + .5f, a));
    }
}

void gDashedLine(float x0, float y0, float x1, float y1, float t, float dash, float gap, uint16_t c, uint8_t a) {
  float len = hypotf(x1 - x0, y1 - y0);
  if (len < .5f) return;
  float ux = (x1 - x0) / len, uy = (y1 - y0) / len;
  for (float s = 0; s < len; s += dash + gap) {
    float e = fminf(s + dash, len);
    gLine(x0 + ux * s, y0 + uy * s, x0 + ux * e, y0 + uy * e, t, c, a);
  }
}

static bool inPoly(const float *xy, int n, float px, float py) {
  bool in = false;
  for (int i = 0, j = n - 1; i < n; j = i++) {
    float xi = xy[2 * i], yi = xy[2 * i + 1], xj = xy[2 * j], yj = xy[2 * j + 1];
    if ((yi > py) != (yj > py) && px < (xj - xi) * (py - yi) / (yj - yi) + xi) in = !in;
  }
  return in;
}
void gPolygon(const float *xy, int n, uint16_t c, uint8_t a) {
  float bx0 = 1e9, bx1 = -1e9, by0 = 1e9, by1 = -1e9;
  for (int i = 0; i < n; i++) { bx0 = fminf(bx0, xy[2 * i]); bx1 = fmaxf(bx1, xy[2 * i]); by0 = fminf(by0, xy[2 * i + 1]); by1 = fmaxf(by1, xy[2 * i + 1]); }
  for (int y = (int)floorf(by0); y <= (int)ceilf(by1); y++)
    for (int x = (int)floorf(bx0); x <= (int)ceilf(bx1); x++) {
      int hits = 0;
      for (int sy = 0; sy < 4; sy++)
        for (int sx = 0; sx < 4; sx++) hits += inPoly(xy, n, x + (sx + .5f) / 4, y + (sy + .5f) / 4);
      if (hits) gPixel(x, y, c, (uint8_t)(a * hits / 16));
    }
}

/* ---------------- text ---------------- */
struct Glyph { int16_t w, h, xo, yo; float adv; uint8_t *bmp; };
static stbtt_fontinfo FI[3];
static std::unordered_map<uint32_t, Glyph> cache;
static std::unordered_map<uint32_t, float> tabCache;

void gInit() {
  const uint8_t *data[3] = {inter300, inter400, inter500};
  for (int i = 0; i < 3; i++) stbtt_InitFont(&FI[i], data[i], stbtt_GetFontOffsetForIndex(data[i], 0));
}

static const Glyph &glyph(Font f, int size, uint32_t cp) {
  uint32_t key = ((uint32_t)f << 25) | ((uint32_t)size << 17) | (cp & 0x1FFFF);
  auto it = cache.find(key);
  if (it != cache.end()) return it->second;
  float sc = stbtt_ScaleForMappingEmToPixels(&FI[f], size);
  int adv, lsb, w = 0, h = 0, xo = 0, yo = 0;
  stbtt_GetCodepointHMetrics(&FI[f], cp, &adv, &lsb);
  uint8_t *b = stbtt_GetCodepointBitmap(&FI[f], sc, sc, cp, &w, &h, &xo, &yo);
  Glyph g = {(int16_t)w, (int16_t)h, (int16_t)xo, (int16_t)yo, adv * sc, nullptr};
  if (b && w * h) {
    g.bmp = (uint8_t *)ps_malloc(w * h);
    if (g.bmp) memcpy(g.bmp, b, w * h);
  }
  if (b) stbtt_FreeBitmap(b, nullptr);
  return cache.emplace(key, g).first->second;
}

static float tabAdv(Font f, int size) {
  uint32_t key = (f << 16) | size;
  auto it = tabCache.find(key);
  if (it != tabCache.end()) return it->second;
  float m = 0;
  for (char d = '0'; d <= '9'; d++) m = fmaxf(m, glyph(f, size, d).adv);
  tabCache[key] = m;
  return m;
}

static uint32_t nextCp(const char *&s) {
  uint8_t c = *s++;
  if (c < 0x80) return c;
  int n = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : (c >= 0xC0) ? 1 : 0;
  uint32_t cp = c & (0x3F >> n);
  while (n-- && (*s & 0xC0) == 0x80) cp = (cp << 6) | (*s++ & 0x3F);
  return cp;
}

int gTextW(const char *s, int size, Font f, float tracking, bool tabular) {
  float w = 0;
  int n = 0;
  while (*s) {
    uint32_t cp = nextCp(s);
    w += (tabular && cp >= '0' && cp <= '9') ? tabAdv(f, size) : glyph(f, size, cp).adv;
    w += tracking;
    n++;
  }
  if (n) w -= tracking;
  return (int)roundf(w);
}

int gText(const char *s, float x, float baseline, int size, Font f, uint16_t c, Align al, float tracking, bool tabular, uint8_t a) {
  int w = gTextW(s, size, f, tracking, tabular);
  float pen = al == CENTER ? x - w / 2.f : al == RIGHT ? x - w : x;
  int by = (int)roundf(baseline);
  while (*s) {
    uint32_t cp = nextCp(s);
    const Glyph &g = glyph(f, size, cp);
    float cell = (tabular && cp >= '0' && cp <= '9') ? tabAdv(f, size) : g.adv;
    float gx = pen + (cell - g.adv) / 2;
    if (g.bmp) {
      int ox = (int)roundf(gx) + g.xo, oy = by + g.yo;
      for (int j = 0; j < g.h; j++)
        for (int i = 0; i < g.w; i++) {
          uint8_t v = g.bmp[j * g.w + i];
          if (v) gPixel(ox + i, oy + j, c, a == 255 ? v : (uint8_t)(v * a / 255));
        }
    }
    pen += cell + tracking;
  }
  return w;
}

String gEllipsize(const String &s, int size, Font f, int maxW) {
  if (gTextW(s.c_str(), size, f) <= maxW) return s;
  String t = s;
  while (t.length() > 1) {
    t.remove(t.length() - 1);
    while (t.length() && (t[t.length() - 1] & 0xC0) == 0x80) t.remove(t.length() - 1);   // don't split UTF-8
    String u = t;
    u.trim();
    u += "\xE2\x80\xA6";
    if (gTextW(u.c_str(), size, f) <= maxW) return u;
  }
  return t;
}

int gWrap(const String &s, int size, Font f, int maxW, String *lines, int maxLines) {
  int n = 0;
  String cur;
  int i = 0;
  while (i < (int)s.length() && n < maxLines) {
    int sp = s.indexOf(' ', i);
    String word = sp < 0 ? s.substring(i) : s.substring(i, sp);
    i = sp < 0 ? s.length() : sp + 1;
    String trial = cur.length() ? cur + " " + word : word;
    if (gTextW(trial.c_str(), size, f) <= maxW || !cur.length()) cur = trial;
    else { lines[n++] = cur; cur = word; }
  }
  if (cur.length() && n < maxLines) lines[n++] = cur;
  return n;
}
