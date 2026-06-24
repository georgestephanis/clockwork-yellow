#ifndef MAP_PROJECTION_H
#define MAP_PROJECTION_H

#include <Arduino.h>

// Trigonometric precalculations for the 320x240 map rendering area
extern double sin_phi[240];
extern double cos_phi[240];
extern double sin_lambda[320];
extern double cos_lambda[320];

/**
 * Initializes the trigonometric tables to avoid slow transcendental function
 * calculations in the inner pixel rendering loops.
 */
void initTrigTables();

/**
 * Maps latitude (-90.0 to +90.0) to screen Y-coordinate (0 to 239)
 * using a calibrated piecewise linear interpolation to fit the warped geography of the AI map.
 * 
 * @param lat Latitude in degrees (positive for North, negative for South).
 * @return Screen Y-coordinate (0 to 239).
 */
int latToY(float lat);

/**
 * Maps longitude (-180.0 to +180.0) to screen X-coordinate (0 to 319).
 * 
 * @param lon Longitude in degrees (positive for East, negative for West).
 * @return Screen X-coordinate (0 to 319).
 */
static inline int lonToX(float lon) {
    return (int)((lon + 180.0f) * (320.0f / 360.0f) + 0.5f);
}

#endif // MAP_PROJECTION_H
