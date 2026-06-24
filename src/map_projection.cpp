#include "map_projection.h"
#include <cmath>

// Global trigonometric lookup tables
double sin_phi[240];
double cos_phi[240];
double sin_lambda[320];
double cos_lambda[320];

// Helper: Convert degrees to radians
static inline double degToRad(double deg) {
    return deg * M_PI / 180.0;
}

void initTrigTables() {
    // Map latitude: y from 0 to 239 corresponds to +90 degrees (top) to -90 degrees (bottom)
    for (int y = 0; y < 240; y++) {
        double lat_deg = 90.0 - y * (180.0 / 240.0);
        sin_phi[y] = std::sin(degToRad(lat_deg));
        cos_phi[y] = std::cos(degToRad(lat_deg));
    }

    // Map longitude: x from 0 to 319 corresponds to -180 degrees (left) to +180 degrees (right)
    for (int x = 0; x < 320; x++) {
        double lon_deg = -180.0 + x * (360.0 / 320.0);
        sin_lambda[x] = std::sin(degToRad(lon_deg));
        cos_lambda[x] = std::cos(degToRad(lon_deg));
    }
}

int latToY(float lat) {
    if (lat >= 90.0f) return 0;
    if (lat <= -90.0f) return 239;
    
    if (lat >= 62.0f) {
        return (int)(0.0f + (90.0f - lat) * (15.0f / 28.0f) + 0.5f);
    } else if (lat >= 43.5f) {
        return (int)(15.0f + (62.0f - lat) * ((51.0f - 15.0f) / (62.0f - 43.5f)) + 0.5f);
    } else if (lat >= 40.0572f) {
        // Precise Pennsylvania / Marietta transition to sit perfectly south of the Great Lakes
        return (int)(51.0f + (43.5f - lat) * ((73.0f - 51.0f) / (43.5f - 40.0572f)) + 0.5f);
    } else if (lat >= 27.5f) {
        return (int)(73.0f + (40.0572f - lat) * ((75.0f - 73.0f) / (40.0572f - 27.5f)) + 0.5f);
    } else if (lat >= 20.0f) {
        return (int)(75.0f + (27.5f - lat) * ((101.0f - 75.0f) / (27.5f - 20.0f)) + 0.5f);
    } else if (lat >= 0.0f) {
        return (int)(101.0f + (20.0f - lat) * ((115.0f - 101.0f) / 20.0f) + 0.5f);
    } else if (lat >= -56.0f) {
        return (int)(115.0f + (-lat) * ((152.0f - 115.0f) / 56.0f) + 0.5f);
    } else if (lat >= -70.0f) {
        return (int)(152.0f + (-56.0f - lat) * ((200.0f - 152.0f) / 14.0f) + 0.5f);
    } else {
        return (int)(200.0f + (-70.0f - lat) * ((239.0f - 200.0f) / 20.0f) + 0.5f);
    }
}
