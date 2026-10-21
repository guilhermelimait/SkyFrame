#pragma once
#include <Arduino.h>

constexpr uint16_t RGB(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }

// Palette (same as the web design)
constexpr uint16_t C_BG = RGB(0, 0, 0), C_FG = RGB(244, 245, 247), C_SOFT = RGB(201, 214, 230), C_DIM = RGB(125, 133, 144),
                   C_FAINT = RGB(28, 31, 36), C_LINE = RGB(43, 47, 54), C_SUN = RGB(246, 201, 69), C_CLOUD = RGB(168, 198, 230),
                   C_CLOUD2 = RGB(143, 169, 196), C_RAIN = RGB(90, 168, 255), C_MOON = RGB(236, 231, 214), C_SNOW = RGB(232, 241, 251),
                   C_GOOD = RGB(76, 211, 138), C_MOD = RGB(246, 201, 69), C_USG = RGB(255, 159, 67), C_BAD = RGB(255, 94, 87),
                   C_VBAD = RGB(181, 108, 255);

enum Font : uint8_t { F300 = 0, F400 = 1, F500 = 2 };
enum Align : uint8_t { LEFT, CENTER, RIGHT };

void gInit();
void gTarget(uint16_t *buf);
uint16_t *gBuf();
void gClear(uint16_t c);
uint16_t gBlend(uint16_t dst, uint16_t src, uint8_t a);
void gPixel(int x, int y, uint16_t c, uint8_t a = 255);
void gFillRect(int x, int y, int w, int h, uint16_t c, uint8_t a = 255);
void gRoundRect(float x, float y, float w, float h, float r, uint16_t c, uint8_t a = 255);
void gRoundRectStroke(float x, float y, float w, float h, float r, float t, uint16_t c, uint8_t a = 255);
void gCircle(float cx, float cy, float r, uint16_t c, uint8_t a = 255);
void gRing(float cx, float cy, float r, float t, uint16_t c, uint8_t a = 255);
// arc from angle a0 to a1 (radians, screen space: 0 = right, +pi/2 = down), round caps
void gArc(float cx, float cy, float r, float t, float a0, float a1, uint16_t c, uint8_t a = 255);
void gDashedArc(float cx, float cy, float r, float t, float a0, float a1, float dash, float gap, uint16_t c, uint8_t a = 255);
void gLine(float x0, float y0, float x1, float y1, float t, uint16_t c, uint8_t a = 255);
void gDashedLine(float x0, float y0, float x1, float y1, float t, float dash, float gap, uint16_t c, uint8_t a = 255);
void gPolygon(const float *xy, int n, uint16_t c, uint8_t a = 255);   // small filled polygons, 4x4 supersampled
void gClipCircle(float cx, float cy, float r);
void gClipNone();

int gText(const char *s, float x, float baseline, int size, Font f, uint16_t c, Align al = LEFT, float tracking = 0, bool tabular = false, uint8_t a = 255);
int gTextW(const char *s, int size, Font f, float tracking = 0, bool tabular = false);
String gEllipsize(const String &s, int size, Font f, int maxW);
int gWrap(const String &s, int size, Font f, int maxW, String *lines, int maxLines);
