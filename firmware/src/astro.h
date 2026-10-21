#pragma once
#include <Arduino.h>

// Times are unix seconds (UTC). Vectors are ecliptic J2000, in AU.
struct V3 { double x, y, z; };
struct AltAz { double alt, az; };
struct RaDec { double ra, dec; };

enum Planet { MERCURY, VENUS, EARTH, MARS, JUPITER, SATURN, URANUS, NEPTUNE, N_PLANETS };
struct PlanetInfo { const char *name; uint16_t color; float size; };
extern const PlanetInfo PLANET[N_PLANETS];
extern const int NAKED[5];

V3 helio(int p, double t);
V3 geoPlanet(int p, double t);
V3 sunGeo(double t);
V3 moonGeo(double t, double *km = nullptr);
RaDec eclToEq(const V3 &v);
AltAz altaz(const RaDec &e, double t, double lat, double lon);
double sepDeg(const V3 &a, const V3 &b);
double eclLonDeg(const V3 &v);
double vlen(const V3 &v);
const char *compass(double az);
double moonPhase(double t);   // 0 new … 0.5 full … 1

struct Star { const char *name; float ra, dec, mag; };   // ra hours, dec degrees
extern const Star STARS[];
extern const int N_STARS;
struct ConLine { int8_t con; const char *a, *b; };
extern const char *CON_NAMES[];
extern const int N_CONS;
extern const ConLine CON_LINES[];
extern const int N_CON_LINES;

// Planets tonight
struct PlanetReport { int p; char status[24]; char big[16]; char sub[40]; uint8_t kind; float distAu; };   // kind 0 dim, 1 tonight, 2 now
void planetReport(double t0, double lat, double lon, int32_t utcOff, bool h12, PlanetReport out[5]);

// Sky events
struct SkyEvent { double t; uint8_t type; char title[40]; char sub[48]; };   // type: 0 meteor 1 moon 2 eclipse 3 season 4 planet
int buildEvents(double now, int32_t utcOff, double lat, SkyEvent *out, int maxOut);
