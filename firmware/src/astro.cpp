// Astronomy, ported from the web version: JPL approximate planet elements, low-precision Moon, star catalogue, sky events
#include "astro.h"
#include "gfx.h"
#include "util.h"

static const double RADc = PI / 180.0, DEGc = 180.0 / PI;
static double norm360(double a) { a = fmod(a, 360.0); return a < 0 ? a + 360 : a; }
static double wrap180(double a) { return norm360(a + 180) - 180; }
static double jd(double t) { return t / 86400.0 + 2440587.5; }

const PlanetInfo PLANET[N_PLANETS] = {
  {"Mercury", RGB(189, 182, 173), 3}, {"Venus", RGB(243, 223, 174), 4.5}, {"Earth", RGB(90, 168, 255), 4.5},
  {"Mars", RGB(255, 122, 89), 3.8}, {"Jupiter", RGB(230, 195, 154), 8}, {"Saturn", RGB(234, 217, 168), 6.5},
  {"Uranus", RGB(159, 227, 230), 5}, {"Neptune", RGB(111, 140, 255), 5},
};
const int NAKED[5] = {MERCURY, VENUS, MARS, JUPITER, SATURN};

static const double EL[N_PLANETS][2][6] = {
  {{0.38709927, 0.20563593, 7.00497902, 252.25032350, 77.45779628, 48.33076593}, {0.00000037, 0.00001906, -0.00594749, 149472.67411175, 0.16047689, -0.12534081}},
  {{0.72333566, 0.00677672, 3.39467605, 181.97909950, 131.60246718, 76.67984255}, {0.00000390, -0.00004107, -0.00078890, 58517.81538729, 0.00268329, -0.27769418}},
  {{1.00000261, 0.01671123, -0.00001531, 100.46457166, 102.93768193, 0}, {0.00000562, -0.00004392, -0.01294668, 35999.37244981, 0.32327364, 0}},
  {{1.52371034, 0.09339410, 1.84969142, -4.55343205, -23.94362959, 49.55953891}, {0.00001847, 0.00007882, -0.00813131, 19140.30268499, 0.44441088, -0.29257343}},
  {{5.20288700, 0.04838624, 1.30439695, 34.39644051, 14.72847983, 100.47390909}, {-0.00011607, -0.00013253, -0.00183714, 3034.74612775, 0.21252668, 0.20469106}},
  {{9.53667594, 0.05386179, 2.48599187, 49.95424423, 92.59887831, 113.66242448}, {-0.00125060, -0.00050991, 0.00193609, 1222.49362201, -0.41897216, -0.28867794}},
  {{19.18916464, 0.04725744, 0.77263783, 313.23810451, 170.95427630, 74.01692503}, {-0.00196176, -0.00004397, -0.00242939, 428.48202785, 0.40805281, 0.04240589}},
  {{30.06992276, 0.00859048, 1.77004347, -55.12002969, 44.96476227, 131.78422574}, {0.00026291, 0.00005105, 0.00035372, 218.45945325, -0.32241464, -0.01262724}},
};

V3 helio(int p, double t) {
  double T = (jd(t) - 2451545.0) / 36525.0, v[6];
  for (int i = 0; i < 6; i++) v[i] = EL[p][0][i] + EL[p][1][i] * T;
  double a = v[0], e = v[1], I = v[2] * RADc, L = v[3], wb = v[4], O = v[5];
  double w = (wb - O) * RADc, Om = O * RADc, M = wrap180(L - wb) * RADc;
  double E = M + e * sin(M);
  for (int k = 0; k < 6; k++) E -= (E - e * sin(E) - M) / (1 - e * cos(E));
  double xp = a * (cos(E) - e), yp = a * sqrt(1 - e * e) * sin(E);
  double cw = cos(w), sw = sin(w), cO = cos(Om), sO = sin(Om), cI = cos(I), sI = sin(I);
  return {(cw * cO - sw * sO * cI) * xp + (-sw * cO - cw * sO * cI) * yp,
          (cw * sO + sw * cO * cI) * xp + (-sw * sO + cw * cO * cI) * yp,
          (sw * sI) * xp + (cw * sI) * yp};
}
V3 geoPlanet(int p, double t) { V3 a = helio(p, t), e = helio(EARTH, t); return {a.x - e.x, a.y - e.y, a.z - e.z}; }
V3 sunGeo(double t) { V3 e = helio(EARTH, t); return {-e.x, -e.y, -e.z}; }
V3 moonGeo(double t, double *km) {
  double d = jd(t) - 2451545.0;
  double L = 218.316 + 13.176396 * d, M = (134.963 + 13.064993 * d) * RADc, F = (93.272 + 13.229350 * d) * RADc;
  double lon = (L + 6.289 * sin(M)) * RADc, lat = 5.128 * sin(F) * RADc, k = 385001 - 20905 * cos(M);
  if (km) *km = k;
  double r = k / 149597870.7;
  return {r * cos(lat) * cos(lon), r * cos(lat) * sin(lon), r * sin(lat)};
}
static const double OBL = 23.43928 * PI / 180.0;
RaDec eclToEq(const V3 &v) {
  double ye = v.y * cos(OBL) - v.z * sin(OBL), ze = v.y * sin(OBL) + v.z * cos(OBL);
  return {atan2(ye, v.x), atan2(ze, sqrt(v.x * v.x + ye * ye))};
}
AltAz altaz(const RaDec &e, double t, double lat, double lon) {
  double lst = norm360(280.46061837 + 360.98564736629 * (jd(t) - 2451545.0) + lon) * RADc, H = lst - e.ra, phi = lat * RADc;
  double alt = asin(sin(phi) * sin(e.dec) + cos(phi) * cos(e.dec) * cos(H));
  double az = atan2(sin(H), cos(H) * sin(phi) - tan(e.dec) * cos(phi)) + PI;
  return {alt * DEGc, norm360(az * DEGc)};
}
double vlen(const V3 &v) { return sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
double sepDeg(const V3 &a, const V3 &b) {
  double c = (a.x * b.x + a.y * b.y + a.z * b.z) / (vlen(a) * vlen(b));
  return acos(fmax(-1.0, fmin(1.0, c))) * DEGc;
}
double eclLonDeg(const V3 &v) { return atan2(v.y, v.x) * DEGc; }
const char *compass(double az) {
  static const char *D[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  return D[(int)lround(az / 45) % 8];
}
double moonPhase(double t) {
  const double syn = 29.530588853, ref = 947182440.0;   // 2000-01-06 18:14 UTC
  double p = fmod((t - ref) / 86400.0, syn);
  if (p < 0) p += syn;
  return p / syn;
}

/* ---------------- stars & constellations ---------------- */
const Star STARS[] = {
  {"Sirius", 6.752, -16.716, -1.46}, {"Canopus", 6.399, -52.696, -.74}, {"Arcturus", 14.261, 19.182, -.05}, {"Rigil Kent", 14.660, -60.834, -.27},
  {"Vega", 18.616, 38.784, .03}, {"Capella", 5.278, 45.998, .08}, {"Rigel", 5.242, -8.202, .13}, {"Procyon", 7.655, 5.225, .34},
  {"Achernar", 1.629, -57.237, .46}, {"Betelgeuse", 5.919, 7.407, .5}, {"Hadar", 14.064, -60.373, .61}, {"Altair", 19.846, 8.868, .76},
  {"Acrux", 12.443, -63.099, .76}, {"Aldebaran", 4.599, 16.509, .86}, {"Antares", 16.490, -26.432, .96}, {"Spica", 13.420, -11.161, .97},
  {"Pollux", 7.755, 28.026, 1.14}, {"Fomalhaut", 22.961, -29.622, 1.16}, {"Deneb", 20.690, 45.280, 1.25}, {"Mimosa", 12.795, -59.689, 1.25},
  {"Regulus", 10.139, 11.967, 1.35}, {"Adhara", 6.977, -28.972, 1.5}, {"Castor", 7.577, 31.888, 1.58}, {"Gacrux", 12.519, -57.113, 1.63},
  {"Shaula", 17.560, -37.104, 1.62}, {"Bellatrix", 5.419, 6.350, 1.64}, {"Elnath", 5.438, 28.608, 1.65}, {"Alnilam", 5.604, -1.202, 1.69},
  {"Alnitak", 5.679, -1.943, 1.74}, {"Mintaka", 5.533, -.299, 2.23}, {"Saiph", 5.796, -9.670, 2.06}, {"Alioth", 12.900, 55.960, 1.77},
  {"Dubhe", 11.062, 61.751, 1.79}, {"Merak", 11.031, 56.382, 2.37}, {"Phecda", 11.897, 53.695, 2.44}, {"Megrez", 12.257, 57.033, 3.31},
  {"Mizar", 13.399, 54.925, 2.23}, {"Alkaid", 13.792, 49.313, 1.86}, {"Polaris", 2.530, 89.264, 1.98}, {"Mirfak", 3.405, 49.861, 1.79},
  {"Alpheratz", .140, 29.091, 2.06}, {"Schedar", .675, 56.537, 2.24}, {"Caph", .153, 59.150, 2.28}, {"Navi", .945, 60.717, 2.47},
  {"Ruchbah", 1.430, 60.235, 2.68}, {"Segin", 1.907, 63.670, 3.38}, {"Sadr", 20.370, 40.257, 2.23}, {"Aljanah", 20.770, 33.970, 2.48},
  {"Albireo", 19.512, 27.960, 3.08}, {"Fawaris", 19.750, 45.131, 2.87}, {"Denebola", 11.818, 14.572, 2.14}, {"Algieba", 10.333, 19.842, 2.08},
  {"Zosma", 11.235, 20.524, 2.56}, {"Chertan", 11.237, 15.430, 3.3}, {"Al Jabhah", 10.122, 16.763, 3.5}, {"Adhafera", 10.278, 23.417, 3.4},
  {"Rasalas", 9.879, 26.007, 3.88}, {"Algenubi", 9.764, 23.774, 2.98}, {"Rasalhague", 17.582, 12.560, 2.08}, {"Hamal", 2.120, 23.462, 2.0},
  {"Algol", 3.136, 40.956, 2.1}, {"Markab", 23.079, 15.205, 2.49}, {"Scheat", 23.063, 28.083, 2.42}, {"Algenib", .221, 15.184, 2.83},
  {"Kaus Australis", 18.403, -34.385, 1.85}, {"Nunki", 18.921, -26.297, 2.05}, {"Acrab", 16.091, -19.806, 2.62}, {"Dschubba", 16.006, -22.622, 2.29},
  {"Paikauhale", 16.598, -28.216, 2.82}, {"Larawag", 16.836, -34.293, 2.29}, {"Xamidimura", 16.864, -38.047, 3.0}, {"Sargas", 17.622, -42.998, 1.86},
  {"Girtab", 17.708, -39.030, 2.39}, {"Imai", 12.252, -58.749, 2.8}, {"Mirach", 1.162, 35.621, 2.06}, {"Almach", 2.065, 42.330, 2.1},
  {"Alcyone", 3.791, 24.105, 2.87},
};
const int N_STARS = sizeof(STARS) / sizeof(STARS[0]);
const char *CON_NAMES[] = {"Orion", "Big Dipper", "Cassiopeia", "Cygnus", "Leo", "Scorpius", "Pegasus", "Andromeda", "Crux", "Gemini"};
const int N_CONS = 10;
const ConLine CON_LINES[] = {
  {0, "Betelgeuse", "Bellatrix"}, {0, "Betelgeuse", "Alnitak"}, {0, "Bellatrix", "Mintaka"}, {0, "Mintaka", "Alnilam"}, {0, "Alnilam", "Alnitak"}, {0, "Alnitak", "Saiph"}, {0, "Mintaka", "Rigel"},
  {1, "Dubhe", "Merak"}, {1, "Merak", "Phecda"}, {1, "Phecda", "Megrez"}, {1, "Megrez", "Dubhe"}, {1, "Megrez", "Alioth"}, {1, "Alioth", "Mizar"}, {1, "Mizar", "Alkaid"},
  {2, "Caph", "Schedar"}, {2, "Schedar", "Navi"}, {2, "Navi", "Ruchbah"}, {2, "Ruchbah", "Segin"},
  {3, "Deneb", "Sadr"}, {3, "Sadr", "Albireo"}, {3, "Fawaris", "Sadr"}, {3, "Sadr", "Aljanah"},
  {4, "Regulus", "Al Jabhah"}, {4, "Al Jabhah", "Algieba"}, {4, "Algieba", "Adhafera"}, {4, "Adhafera", "Rasalas"}, {4, "Rasalas", "Algenubi"},
  {4, "Algieba", "Zosma"}, {4, "Zosma", "Denebola"}, {4, "Denebola", "Chertan"}, {4, "Chertan", "Regulus"}, {4, "Zosma", "Chertan"},
  {5, "Acrab", "Dschubba"}, {5, "Dschubba", "Antares"}, {5, "Antares", "Paikauhale"}, {5, "Paikauhale", "Larawag"}, {5, "Larawag", "Xamidimura"},
  {5, "Xamidimura", "Sargas"}, {5, "Sargas", "Girtab"}, {5, "Girtab", "Shaula"},
  {6, "Markab", "Scheat"}, {6, "Scheat", "Alpheratz"}, {6, "Alpheratz", "Algenib"}, {6, "Algenib", "Markab"},
  {7, "Alpheratz", "Mirach"}, {7, "Mirach", "Almach"},
  {8, "Acrux", "Gacrux"}, {8, "Mimosa", "Imai"},
  {9, "Castor", "Pollux"},
};
const int N_CON_LINES = sizeof(CON_LINES) / sizeof(CON_LINES[0]);

/* ---------------- planets tonight ---------------- */
void planetReport(double t0, double lat, double lon, int32_t off, bool h12, PlanetReport out[5]) {
  for (int n = 0; n < 5; n++) {
    int k = NAKED[n];
    PlanetReport &r = out[n];
    r.p = k; r.kind = 0; r.big[0] = 0;
    V3 g = geoPlanet(k, t0);
    double elong = sepDeg(g, sunGeo(t0));
    r.distAu = vlen(g);
    AltAz p0 = altaz(eclToEq(g), t0, lat, lon), s0 = altaz(eclToEq(sunGeo(t0)), t0, lat, lon);
    if (s0.alt < -6 && p0.alt > 3) {
      double setT = 0;
      for (int i = 1; i <= 144; i++) {
        double t = t0 + i * 600;
        if (altaz(eclToEq(geoPlanet(k, t)), t, lat, lon).alt < 0) { setT = t; break; }
      }
      r.kind = 2;
      strcpy(r.status, "Visible now");
      snprintf(r.big, sizeof r.big, "%s %d\xC2\xB0", compass(p0.az), (int)lround(p0.alt));
      if (setT) snprintf(r.sub, sizeof r.sub, "Sets %s", fmtHM((int64_t)setT, off, h12, true).c_str());
      else strcpy(r.sub, "Up all night");
      continue;
    }
    double bestAlt = -99, bestT = 0, bestAz = 0;
    for (int i = 0; i <= 144; i++) {
      double t = t0 + i * 600;
      AltAz s = altaz(eclToEq(sunGeo(t)), t, lat, lon);
      if (s.alt >= -8) continue;
      AltAz p = altaz(eclToEq(geoPlanet(k, t)), t, lat, lon);
      if (p.alt > 8 && p.alt > bestAlt) { bestAlt = p.alt; bestT = t; bestAz = p.az; }
    }
    if (bestT) {
      r.kind = 1;
      strcpy(r.status, toLocal((int64_t)bestT, off).h < 7 ? "Before dawn" : "Tonight");
      snprintf(r.big, sizeof r.big, "%s", fmtHM((int64_t)bestT, off, h12, true).c_str());
      snprintf(r.sub, sizeof r.sub, "Look %s \xC2\xB7 %d\xC2\xB0 high", compass(bestAz), (int)lround(bestAlt));
    } else {
      strcpy(r.status, elong < 18 ? "Lost in sunlight" : "Daytime sky");
      snprintf(r.sub, sizeof r.sub, "%d\xC2\xB0 from the Sun", (int)lround(elong));
    }
  }
}

/* ---------------- sky events ---------------- */
static const char *MOON_NAMES[12] = {"Wolf", "Snow", "Worm", "Pink", "Flower", "Strawberry", "Buck", "Sturgeon", "Harvest", "Hunter's", "Beaver", "Cold"};
struct Ecl { int y, m, d; const char *title, *sub; };
static const Ecl ECLIPSES[] = {
  {2026, 8, 12, "Total solar eclipse", "Path: Greenland, Iceland, Spain"}, {2026, 8, 28, "Partial lunar eclipse", "Visible wherever the Moon is up"},
  {2027, 2, 6, "Annular solar eclipse", "Path: South America, West Africa"}, {2027, 2, 20, "Penumbral lunar eclipse", "Subtle shading of the Moon"},
  {2027, 7, 18, "Penumbral lunar eclipse", "Subtle shading of the Moon"}, {2027, 8, 2, "Total solar eclipse", "Path: Spain, North Africa, Egypt"},
  {2027, 8, 17, "Penumbral lunar eclipse", "Subtle shading of the Moon"}, {2028, 1, 12, "Partial lunar eclipse", "Visible wherever the Moon is up"},
  {2028, 1, 26, "Annular solar eclipse", "Path: South America, Iberia"}, {2028, 7, 6, "Partial lunar eclipse", "Visible wherever the Moon is up"},
  {2028, 7, 22, "Total solar eclipse", "Path: Australia, New Zealand"}, {2028, 12, 31, "Total lunar eclipse", "Blood Moon on New Year's Eve"},
  {2029, 6, 26, "Total lunar eclipse", "Blood Moon"}, {2029, 12, 20, "Total lunar eclipse", "Blood Moon"},
};
struct Shower { const char *name; int m, d, zhr; };
static const Shower SHOWERS[] = {{"Quadrantids", 1, 4, 120}, {"Lyrids", 4, 22, 18}, {"Eta Aquariids", 5, 6, 50}, {"Perseids", 8, 12, 100},
                                 {"Orionids", 10, 21, 20}, {"Leonids", 11, 17, 15}, {"Geminids", 12, 14, 150}, {"Ursids", 12, 22, 10}};

static int evCount, evMax;
static SkyEvent *evOut;
static double evFrom;
static void addEv(double t, uint8_t type, const char *title, const char *sub) {
  if (t < evFrom) return;
  // keep the list sorted and only the earliest evMax entries
  int pos = evCount;
  while (pos > 0 && evOut[pos - 1].t > t) pos--;
  if (pos >= evMax) return;
  int last = evCount < evMax ? evCount : evMax - 1;
  for (int i = last; i > pos; i--) evOut[i] = evOut[i - 1];
  evOut[pos].t = t; evOut[pos].type = type;
  strlcpy(evOut[pos].title, title, sizeof evOut[pos].title);
  strlcpy(evOut[pos].sub, sub, sizeof evOut[pos].sub);
  if (evCount < evMax) evCount++;
}

int buildEvents(double now, int32_t off, double lat, SkyEvent *out, int maxOut) {
  evOut = out; evMax = maxOut; evCount = 0;
  LT today = toLocal((int64_t)now, off);
  evFrom = (double)localToUtc(today.y, today.mo, today.d, 0, 0, off);
  char t1[48], t2[48];
  for (int yy = today.y; yy <= today.y + 1; yy++)
    for (const Shower &s : SHOWERS) {
      snprintf(t1, sizeof t1, "%s meteor shower", s.name);
      snprintf(t2, sizeof t2, "Peak night \xC2\xB7 up to %d meteors/hour", s.zhr);
      addEv((double)localToUtc(yy, s.m, s.d, 22, 0, off), 0, t1, t2);
    }
  const double syn = 29.530588853 * 86400, ref = 947182440.0;
  double k0 = floor((now - ref) / syn);
  for (int i = 0; i < 14; i++) {
    double tf = ref + (k0 + i) * syn + syn / 2, km;
    moonGeo(tf, &km);
    snprintf(t1, sizeof t1, "Full Moon \xC2\xB7 %s Moon", MOON_NAMES[toLocal((int64_t)tf, off).mo - 1]);
    addEv(tf, 1, t1, km < 361000 ? "Supermoon \xC2\xB7 bigger and brighter" : "Rises around sunset");
  }
  static const double SC[4][5] = {{2451623.80984, 365242.37404, .05169, -.00411, -.00057}, {2451716.56767, 365241.62603, .00325, .00888, -.00030},
                                  {2451810.21715, 365242.01767, -.11575, .00337, .00078}, {2451900.05952, 365242.74049, -.06223, -.00823, .00032}};
  static const char *SN[4][2] = {{"Spring equinox", "Day and night equal length"}, {"Summer solstice", "Longest day of the year"},
                                 {"Autumn equinox", "Day and night equal length"}, {"Winter solstice", "Shortest day of the year"}};
  for (int yy = today.y; yy <= today.y + 1; yy++)
    for (int i = 0; i < 4; i++) {
      double Y = (yy - 2000) / 1000.0, j = SC[i][0] + SC[i][1] * Y + SC[i][2] * Y * Y + SC[i][3] * Y * Y * Y + SC[i][4] * Y * Y * Y * Y;
      int s = lat < 0 ? (i + 2) % 4 : i;
      addEv((j - 2440587.5) * 86400, 3, SN[s][0], SN[s][1]);
    }
  for (const Ecl &e : ECLIPSES) addEv((double)(daysFromCivil(e.y, e.m, e.d) * 86400 + 43200), 2, e.title, e.sub);

  // planet events: scan ~13 months day by day (evening of each day)
  const int DAYS = 400;
  double base = (double)localToUtc(today.y, today.mo, today.d, 21, 0, off);
  V3 g[3][N_PLANETS], S[3];
  auto fill = [&](int slot, double t) { S[slot] = sunGeo(t); for (int p = 0; p < N_PLANETS; p++) if (p != EARTH) g[slot][p] = geoPlanet(p, t); };
  fill(0, base); fill(1, base + 86400);
  const int OUTER[5] = {MARS, JUPITER, SATURN, URANUS, NEPTUNE};
  for (int i = 1; i < DAYS; i++) {
    double tb = base + i * 86400.0;
    fill(2, tb + 86400);
    int a = 0, b = 1, c = 2;
    for (int p : OUTER) {
      double d0 = wrap180(eclLonDeg(g[a][p]) - eclLonDeg(S[a]) - 180), d1 = wrap180(eclLonDeg(g[b][p]) - eclLonDeg(S[b]) - 180);
      if (d0 < 0 && d1 >= 0 && fabs(d1) < 20) {
        snprintf(t1, sizeof t1, "%s at opposition", PLANET[p].name);
        addEv(tb, 4, t1, p <= SATURN ? "Closest and brightest \xC2\xB7 up all night" : "Best time to find it with binoculars");
      }
    }
    for (int p : {MERCURY, VENUS}) {
      double e0 = sepDeg(g[a][p], S[a]), e1 = sepDeg(g[b][p], S[b]), e2 = sepDeg(g[c][p], S[c]);
      if (e1 > e0 && e1 >= e2) {
        bool evening = wrap180(eclLonDeg(g[b][p]) - eclLonDeg(S[b])) > 0;
        snprintf(t1, sizeof t1, "%s at its best", PLANET[p].name);
        snprintf(t2, sizeof t2, "%d\xC2\xB0 from the Sun \xC2\xB7 %s sky", (int)lround(e1), evening ? "evening" : "morning");
        addEv(tb, 4, t1, t2);
      }
    }
    for (int x = 0; x < 5; x++)
      for (int z = x + 1; z < 5; z++) {
        int A = NAKED[x], B = NAKED[z];
        double s0 = sepDeg(g[a][A], g[a][B]), s1 = sepDeg(g[b][A], g[b][B]), s2 = sepDeg(g[c][A], g[c][B]);
        if (s1 < s0 && s1 <= s2 && s1 < 3 && sepDeg(g[b][A], S[b]) > 15 && sepDeg(g[b][B], S[b]) > 15) {
          bool evening = wrap180(eclLonDeg(g[b][A]) - eclLonDeg(S[b])) > 0;
          snprintf(t1, sizeof t1, "%s meets %s", PLANET[A].name, PLANET[B].name);
          snprintf(t2, sizeof t2, "%.1f\xC2\xB0 apart \xC2\xB7 %s sky", s1, evening ? "evening" : "morning");
          addEv(tb, 4, t1, t2);
        }
      }
    // slide the 3-day window
    S[0] = S[1]; S[1] = S[2];
    for (int p = 0; p < N_PLANETS; p++) { g[0][p] = g[1][p]; g[1][p] = g[2][p]; }
    if ((i & 31) == 0) vTaskDelay(1);   // stay friendly to other tasks
  }
  return evCount;
}
