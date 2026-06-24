#ifndef SOLAR_H
#define SOLAR_H

#include <time.h>

// Struct to store the calculated astronomical solar position
struct SolarPosition {
    double declination;      // Latitude where the sun is directly overhead (degrees, -23.44 to +23.44)
    double subsolarLon;      // Longitude where the sun is directly overhead (degrees, -180.0 to +180.0)
    double equationOfTime;   // Equation of time in minutes (correction for Earth's non-circular orbit)
};

/**
 * Calculates the Sun's subsolar coordinates (latitude and longitude) and the Equation of Time
 * for a given UTC epoch time.
 * 
 * @param utcTime The current UTC time as a unix epoch timestamp.
 * @return SolarPosition struct containing declination, subsolar longitude, and Equation of Time.
 */
SolarPosition calculateSolarPosition(time_t utcTime);

#endif // SOLAR_H
