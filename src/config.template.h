#ifndef CONFIG_TEMPLATE_H
#define CONFIG_TEMPLATE_H

#include <stdint.h>

// WiFi Configuration - Replace these with your actual network credentials!
#define WIFI_SSID "WIFISSID"
#define WIFI_PASSWORD "PASSWORD"

// WiFi connection timeout in milliseconds
#define WIFI_TIMEOUT_MS 15000

// NTP Server Configuration
#define NTP_SERVER "pool.ntp.org"
#define NTP_SYNC_INTERVAL_SEC 3600 // Sync time every hour

// Display and Touchscreen rotation:
// 3 = Landscape (USB port on the right side - default)
// 1 = Landscape (USB port on the left side - rotated 180 degrees)
#define DISPLAY_ROTATION 3

// Display Refresh Configuration
#define MAP_UPDATE_INTERVAL_MS 300000 // Update the day/night terminator every 5 minutes (300,000 ms)
#define BANNER_TIMEOUT_MS 10000       // Auto-hide bottom banner after 10 seconds of inactivity

// Default timezone offset from UTC in hours (user can adjust this via touch controls)
#define DEFAULT_TIMEZONE_OFFSET 0

// Default User Preferences
#define DEFAULT_GRID_ENABLED false // Show latitude/longitude grid by default
#define DEFAULT_TWILIGHT_MODE 1    // 0 = Sharp (no twilight), 1 = Blended (nautical twilight)
#define DEFAULT_MAP_MODE 0  // 0 = Full Color, 1 = Flat Basic

// Pin definition for the touchscreen CS (shared SPI bus with TFT)
#define TOUCH_CS 33

// Touchscreen calibration parameters for standard CYD
#define TOUCH_MIN_X 200
#define TOUCH_MAX_X 3800
#define TOUCH_MIN_Y 200
#define TOUCH_MAX_Y 3800

// Pin definitions for CYD peripherals (optional use)
#define CYD_LDR_PIN 34   // Light Dependent Resistor (Analog input)
#define CYD_RGB_RED 4    // RGB LED Red channel
#define CYD_RGB_GREEN 16 // RGB LED Green channel
#define CYD_RGB_BLUE 17  // RGB LED Blue channel

// OpenHamClock Backend (OHB) Configuration
// Points to the public community backend server by default. Can be customized if self-hosting.
#define HAMCLOCK_BACKEND_URL "http://ohb.hamclock.app"

// Home Location Reference Dots
struct HomeLocation {
    float latitude;   // -90.0 to +90.0 (Southern hemisphere is negative)
    float longitude;  // -180.0 to +180.0 (Western hemisphere is negative)
    const char* name; // Label for reference
    uint16_t color;   // RGB565 color for the dot center
};

#define HOME_LOCATIONS_COUNT 1
const HomeLocation HOME_LOCATIONS[HOME_LOCATIONS_COUNT] = {
    {42.3601, -71.0589, "Boston", 0xFDA0} // Boston in Clockwork Gold (High contrast on any background)
};

#endif // CONFIG_TEMPLATE_H
