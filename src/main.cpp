#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>

#include "config.h"
#include "solar.h"
#include "world_map.h"
#include "map_projection.h"
#include "network_time.h"
#include "display_ui.h"

// Hardware instances
SPIClass touchscreenSPI(VSPI); // Dedicated SPI bus for the touchscreen
XPT2046_Touchscreen ts(TOUCH_CS);
Preferences prefs;

// Global state variables (restored from flash)
int timezone_offset = DEFAULT_TIMEZONE_OFFSET; // Local timezone offset in hours
bool grid_enabled = DEFAULT_GRID_ENABLED;      // Draw lat/lon grid
bool home_enabled = true;                      // Draw home location reference dots
int twilight_mode = DEFAULT_TWILIGHT_MODE;     // 0 = Sharp, 1 = Blended
int map_mode = 0;                              // 0 = Full Color, 1 = Flat, 2 = Outline

// Network & Time state
uint32_t last_map_update = 0;
uint32_t last_touch_time = 0;
int last_rendered_minute = -1;

// Banner visibility & auto-hide timing
bool banner_visible = true;
uint32_t last_activity_time = 0;

// Trigonometric tables and projection helpers are declared in map_projection.h

// Local wrappers to delegate rendering to the display_ui module
void drawMap() {
    drawMap(banner_visible, map_mode, twilight_mode, grid_enabled, home_enabled);
}

void drawBanner(bool forceRedraw) {
    drawBanner(forceRedraw, banner_visible, timezone_offset, grid_enabled, home_enabled, twilight_mode, map_mode);
}

// Handle Touch Inputs
void handleTouch() {
    if (!ts.touched()) return;

    // Read the touch point immediately to clear the hardware controller's registers/buffer
    TS_Point p = ts.getPoint();

    // Debounce touch inputs (minimum 200ms between registered touches)
    uint32_t now = millis();
    if (now - last_touch_time < 200) {
        return;
    }
    last_touch_time = now;



    // WAKE UP banner behavior: If the banner was hidden, this touch brings it back
    if (!banner_visible) {
        banner_visible = true;
        last_activity_time = now;
        drawMap();        // Redraw map (200 rows)
        drawBanner(true); // Redraw buttons
        Serial.println("Banner woke up from touch!");
        return;
    }

    // Reset inactivity timer since there was activity while the banner was visible
    last_activity_time = now;

    // Map raw touch coordinate values to 320x240 screen coordinates
    int touch_x = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, 320);
    int touch_y = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, 240);

    // Constrain to valid screen coordinates
    touch_x = constrain(touch_x, 0, 319);
    touch_y = constrain(touch_y, 0, 239);

    Serial.printf("Touch registered at: X=%d, Y=%d (Raw X=%d, Y=%d)\n", touch_x, touch_y, p.x, p.y);

    // Touch event in the interactive banner area (y: 200 - 240)
    if (touch_y >= 200) {
        
        // 1. Timezone Adjustment (Left side of banner)
        if (touch_x < 115) {
            if (touch_x < 55) {
                // Decrement timezone (Tap left half)
                timezone_offset--;
                if (timezone_offset < -12) timezone_offset = 14; // Wrap around
            } else {
                // Increment timezone (Tap right half)
                timezone_offset++;
                if (timezone_offset > 14) timezone_offset = -12; // Wrap around
            }
            prefs.putInt("timezone", timezone_offset);
            Serial.printf("Timezone adjusted to: UTC%+d\n", timezone_offset);
            drawBanner(true); // Force redraw banner to update clock instantly
        }
        
        // 2. Grid Toggle [G] (x: 115 - 138)
        else if (touch_x >= 115 && touch_x < 138) {
            grid_enabled = !grid_enabled;
            prefs.putBool("grid", grid_enabled);
            Serial.printf("Grid toggled: %s\n", grid_enabled ? "ON" : "OFF");
            drawMap(); // Redraw map to apply/remove grid lines
            drawBanner(true);
        }
        
        // 3. Home Location Toggle [H] (x: 138 - 162)
        else if (touch_x >= 138 && touch_x < 162) {
            home_enabled = !home_enabled;
            prefs.putBool("home", home_enabled);
            Serial.printf("Home markers toggled: %s\n", home_enabled ? "ON" : "OFF");
            drawMap(); // Redraw map to apply/remove home dots
            drawBanner(true);
        }
        
        // 4. Twilight Mode Toggle [T] (x: 162 - 186)
        else if (touch_x >= 162 && touch_x < 186) {
            twilight_mode = twilight_mode == 1 ? 0 : 1;
            prefs.putInt("twilight", twilight_mode);
            Serial.printf("Twilight mode toggled to: %s\n", twilight_mode == 1 ? "Blended" : "Sharp");
            drawMap(); // Redraw map to apply terminator style
            drawBanner(true);
        }
        
        // 5. Map Mode Control [M] (x: 186 - 210)
        else if (touch_x >= 186 && touch_x < 210) {
            map_mode = (map_mode + 1) % 2; // Cycle: 0 (Color) -> 1 (Flat)
            prefs.begin("clockwork", false);
            prefs.putInt("map_mode", map_mode);
            prefs.end();
            Serial.printf("Map mode set to: %d\n", map_mode);
            drawMap();        // Redraw map to apply the new mode
            drawBanner(true); // Redraw banner to update the button icon
        }
        
        // 5. Manual NTP Sync (Right side of banner, UTC area)
        else if (touch_x >= 215) {
            triggerManualSync();
            drawMap();
            drawBanner(true);
        }
    }
    
    // Touch event on the map area (y < 200) - immediately toggles into full screen mode
    else {
        Serial.println("Map tap registered. Toggling into full screen mode.");
        banner_visible = false;
        drawMap(); // Redraws all 240 rows, hiding the banner
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Clockwork Yellow starting up ===");

    // Restore settings from ESP32 Flash Preferences (non-volatile memory)
    prefs.begin("clockwork", false);
    timezone_offset = prefs.getInt("timezone", DEFAULT_TIMEZONE_OFFSET);
    grid_enabled = prefs.getBool("grid", DEFAULT_GRID_ENABLED);
    home_enabled = prefs.getBool("home", true);
    twilight_mode = prefs.getInt("twilight", DEFAULT_TWILIGHT_MODE);
    map_mode = prefs.getInt("map_mode", DEFAULT_MAP_MODE);
    prefs.end();

    // 1. Initialize TFT Display & Backlight
    initDisplay();

    // 2. Initialize Touchscreen on a dedicated SPI bus
    touchscreenSPI.begin(25, 39, 32, 33); // SCLK, MISO, MOSI, CS
    ts.begin(touchscreenSPI);
    ts.setRotation(DISPLAY_ROTATION); // Match TFT display rotation

    // 4. Precalculate Trigonometric Tables
    initTrigTables();

    // 5. Connect to WiFi asynchronously (non-blocking)
    initNetwork();

    // 6. Perform initial drawing
    banner_visible = true;
    last_activity_time = millis();
    drawMap();
    drawBanner(true);
    last_map_update = millis();
}

void loop() {
    // Update non-blocking network state machine (WiFi and NTP)
    updateNetworkState();

    // Poll touch input every 80ms to reduce SPI bus traffic and eliminate electrical screen flicker
    static uint32_t last_touch_poll = 0;
    uint32_t now = millis();
    if (now - last_touch_poll >= 80) {
        last_touch_poll = now;
        handleTouch();
    }

    // Check if the banner has timed out due to touch inactivity
    if (banner_visible && (now - last_activity_time >= BANNER_TIMEOUT_MS)) {
        banner_visible = false;
        Serial.println("Banner timed out. Hiding banner and expanding map.");
        drawMap(); // Redraw map to full 240 rows, overwriting the banner
    }

    // Check if it is time to refresh the day/night terminator map
    if (now - last_map_update >= MAP_UPDATE_INTERVAL_MS) {
        last_map_update = now;
        
        // Refresh the map overlay
        drawMap();
    }

    // Periodic background NTP sync (every hour, non-blocking)
    checkNtpPeriodic(NTP_SYNC_INTERVAL_SEC);

    // Update the clocks in the bottom banner (handles its own min-change filter)
    drawBanner(false);

    // Minimal delay to keep ESP32 watchdog and task scheduler happy
    delay(20);
}
