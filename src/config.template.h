#ifndef CONFIG_TEMPLATE_H
#define CONFIG_TEMPLATE_H

// WiFi Configuration - Replace these with your actual network credentials!
#define WIFI_SSID "WIFISSID"
#define WIFI_PASSWORD "PASSWORD"

// WiFi connection timeout in milliseconds
#define WIFI_TIMEOUT_MS 15000

// NTP Server Configuration
#define NTP_SERVER "pool.ntp.org"
#define NTP_SYNC_INTERVAL_SEC 3600 // Sync time every hour

// Display Refresh Configuration
#define MAP_UPDATE_INTERVAL_MS 300000 // Update the day/night terminator every 5 minutes (300,000 ms)
#define BANNER_TIMEOUT_MS 10000       // Auto-hide bottom banner after 10 seconds of inactivity

// Default timezone offset from UTC in hours (user can adjust this via touch controls)
#define DEFAULT_TIMEZONE_OFFSET 0

// Default User Preferences
#define DEFAULT_GRID_ENABLED false // Show latitude/longitude grid by default
#define DEFAULT_TWILIGHT_MODE 1    // 0 = Sharp (no twilight), 1 = Blended (nautical twilight)
#define DEFAULT_BACKLIGHT_LEVEL 3  // 0 = Off, 1 = Low, 2 = Medium, 3 = High

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

#endif // CONFIG_TEMPLATE_H
