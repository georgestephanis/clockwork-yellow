#include "solar.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper function to convert degrees to radians
static inline double rad(double deg) {
    return deg * M_PI / 180.0;
}

// Helper function to convert radians to degrees
static inline double deg(double rad) {
    return rad * 180.0 / M_PI;
}

// Calculates the Julian Date according to Meeus' Astronomical Algorithms
static double getJulianDate(int year, int month, int day, int hour, int minute, int second) {
    if (month <= 2) {
        year -= 1;
        month += 12;
    }
    double A = std::floor(year / 100.0);
    double B = 2.0 - A + std::floor(A / 4.0);
    double JD = std::floor(365.25 * (year + 4716)) + std::floor(30.6001 * (month + 1)) + day + B - 1524.5;
    double day_fraction = (hour + minute / 60.0 + second / 3600.0) / 24.0;
    return JD + day_fraction;
}

SolarPosition calculateSolarPosition(time_t utcTime) {
    // Break down the epoch time into UTC components
    struct tm *tm_utc = gmtime(&utcTime);
    
    int year = tm_utc->tm_year + 1900;
    int month = tm_utc->tm_mon + 1;
    int day = tm_utc->tm_mday;
    int hour = tm_utc->tm_hour;
    int minute = tm_utc->tm_min;
    int second = tm_utc->tm_sec;

    // 1. Calculate Julian Date (JD)
    double jd = getJulianDate(year, month, day, hour, minute, second);
    double utc_hour_decimal = hour + minute / 60.0 + second / 3600.0;

    // 2. Julian centuries since J2000.0
    double T = (jd - 2451545.0) / 36525.0;

    // 3. Geometric Mean Longitude of the Sun (degrees)
    double L0 = 280.46646 + T * (36000.76983 + T * 0.0003032);
    L0 = std::fmod(L0, 360.0);
    if (L0 < 0) L0 += 360.0;

    // 4. Geometric Mean Anomaly of the Sun (degrees)
    double M = 357.52911 + T * (35999.05029 - T * 0.0001537);

    // 5. Eccentricity of Earth's orbit
    double e = 0.016708634 - T * (0.000042037 + T * 0.0000001267);

    // 6. Sun's Equation of the Center (degrees)
    double C = std::sin(rad(M)) * (1.914602 - T * (0.004817 + T * 0.000014))
             + std::sin(rad(2.0 * M)) * (0.019993 - T * 0.000101)
             + std::sin(rad(3.0 * M)) * 0.000289;

    // 7. Sun's True Longitude (degrees)
    double sunTrueLon = L0 + C;

    // 8. Sun's Apparent Longitude (degrees, corrected for aberration and nutation)
    double omega = 125.04 - 1934.136 * T;
    double lambda = sunTrueLon - 0.00569 - 0.00478 * std::sin(rad(omega));

    // 9. Obliquity of the Ecliptic (degrees)
    double obliquity0 = 23.439291 - T * (46.8150 + T * (0.00059 - T * 0.001813)) / 3600.0;
    double obliquity = obliquity0 + 0.00256 * std::cos(rad(omega));

    // 10. Sun's Declination (degrees)
    double declination = deg(std::asin(std::sin(rad(obliquity)) * std::sin(rad(lambda))));

    // 11. Equation of Time (minutes of time)
    double y = std::pow(std::tan(rad(obliquity) / 2.0), 2.0);
    double eot = 4.0 * deg(y * std::sin(2.0 * rad(L0)) 
                           - 2.0 * e * std::sin(rad(M)) 
                           + 4.0 * e * y * std::sin(rad(M)) * std::cos(2.0 * rad(L0)) 
                           - 0.5 * y * y * std::sin(4.0 * rad(L0)) 
                           - 1.25 * e * e * std::sin(2.0 * rad(M)));

    // 12. Subsolar Longitude (degrees)
    // Solar noon at Greenwich occurs when UTC = 12h - EoT.
    // The subsolar longitude shifts by 15 degrees per hour relative to Greenwich.
    double subsolarLon = -(utc_hour_decimal - 12.0 + eot / 60.0) * 15.0;
    
    // Normalize to [-180.0, +180.0]
    subsolarLon = std::fmod(subsolarLon + 180.0, 360.0);
    if (subsolarLon < 0) subsolarLon += 360.0;
    subsolarLon -= 180.0;

    return {declination, subsolarLon, eot};
}
