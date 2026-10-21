#pragma once
#include <Arduino.h>

struct LT { int y, mo, d, h, mi, s, wd, yd; };   // mo 1-12, wd 0 = Sunday, yd 0-365

int64_t daysFromCivil(int y, int m, int d);
LT toLocal(int64_t utc, int32_t off);
int64_t localToUtc(int y, int mo, int d, int h, int mi, int32_t off);
String fmtHM(int64_t utc, int32_t off, bool h12, bool withAmPm = false);
String deg(float v, bool ok = true);
extern const char *MONTHS_SHORT[12];
extern const char *MONTHS_LONG[12];
extern const char *WEEKDAYS_SHORT[7];
