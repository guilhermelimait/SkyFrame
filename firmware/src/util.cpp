#include "util.h"

const char *MONTHS_SHORT[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char *MONTHS_LONG[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
const char *WEEKDAYS_SHORT[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Howard Hinnant's civil date algorithms
int64_t daysFromCivil(int y, int m, int d) {
  y -= m <= 2;
  int64_t era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int64_t)doe - 719468;
}

LT toLocal(int64_t utc, int32_t off) {
  int64_t t = utc + off;
  int64_t days = t >= 0 ? t / 86400 : (t - 86399) / 86400;
  int64_t secs = t - days * 86400;
  LT r;
  r.h = secs / 3600; r.mi = (secs % 3600) / 60; r.s = secs % 60;
  r.wd = (int)(((days % 7) + 11) % 7);   // 1970-01-01 was a Thursday
  int64_t z = days + 719468;
  int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  unsigned doe = (unsigned)(z - era * 146097);
  unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int y = (int)(yoe + era * 400);
  unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  unsigned mp = (5 * doy + 2) / 153;
  r.d = doy - (153 * mp + 2) / 5 + 1;
  r.mo = mp < 10 ? mp + 3 : mp - 9;
  r.y = y + (r.mo <= 2);
  r.yd = (int)(days - daysFromCivil(r.y, 1, 1));
  return r;
}

int64_t localToUtc(int y, int mo, int d, int h, int mi, int32_t off) {
  return daysFromCivil(y, mo, d) * 86400 + h * 3600 + mi * 60 - off;
}

String fmtHM(int64_t utc, int32_t off, bool h12, bool withAmPm) {
  LT l = toLocal(utc, off);
  char b[16];
  if (h12) {
    int h = l.h % 12 == 0 ? 12 : l.h % 12;
    snprintf(b, sizeof b, withAmPm ? "%d:%02d %s" : "%d:%02d", h, l.mi, l.h < 12 ? "AM" : "PM");
  } else snprintf(b, sizeof b, "%02d:%02d", l.h, l.mi);
  return b;
}

String deg(float v, bool ok) {
  if (!ok || isnan(v)) return "--\xC2\xB0";
  return String((int)lroundf(v)) + "\xC2\xB0";
}
