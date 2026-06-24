#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>

#include "config.h"
#include "solar.h"
#include "world_map.h"

// Hardware instances
TFT_eSPI tft = TFT_eSPI();
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
bool wifi_connected = false;
bool time_synced = false;
uint32_t last_map_update = 0;
uint32_t last_touch_time = 0;
int last_rendered_minute = -1;

// Banner visibility & auto-hide timing
bool banner_visible = true;
uint32_t last_activity_time = 0;

// Trigonometric precalculations for the 320x240 map rendering area
double sin_phi[240];
double cos_phi[240];
double sin_lambda[320];
double cos_lambda[320];

// Helper: Convert degrees to radians
static inline double degToRad(double deg) {
    return deg * PI / 180.0;
}

// Initialize the trigonometric tables to avoid slow trig calculations in the inner loops
void initTrigTables() {
    // Map latitude: y from 0 to 239 corresponds to +90 degrees (top) to -90 degrees (bottom)
    for (int y = 0; y < 240; y++) {
        double lat_deg = 90.0 - y * (180.0 / 240.0);
        sin_phi[y] = sin(degToRad(lat_deg));
        cos_phi[y] = cos(degToRad(lat_deg));
    }

    // Map longitude: x from 0 to 319 corresponds to -180 degrees (left) to +180 degrees (right)
    for (int x = 0; x < 320; x++) {
        double lon_deg = -180.0 + x * (360.0 / 320.0);
        sin_lambda[x] = sin(degToRad(lon_deg));
        cos_lambda[x] = cos(degToRad(lon_deg));
    }
}

// Configure screen backlight level using ESP32 PWM
// (Inverted for active-low backlight transistor on standard CYD hardware)
void setBacklight(int level) {
    ledcWrite(0, 255); // Always fully ON (255 is active-high maximum brightness)
}

// Connect to WiFi network
void connectWiFi() {
    tft.fillRect(0, 200, 320, 40, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Connecting to WiFi...", 160, 220, 2);

    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    uint32_t start_time = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start_time < WIFI_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        wifi_connected = true;
        Serial.println("\nWiFi Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        wifi_connected = false;
        Serial.println("\nWiFi Connection Failed (Timeout).");
    }
}

// Sync time with NTP server
void syncNTP() {
    if (!wifi_connected) return;

    tft.fillRect(0, 200, 320, 40, TFT_BLACK);
    tft.setTextColor(TFT_GOLD, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Syncing Time via NTP...", 160, 220, 2);

    Serial.println("Syncing time via NTP...");
    configTime(0, 0, NTP_SERVER);

    // Wait for time to sync (up to 10 seconds)
    struct tm timeinfo;
    int retry = 0;
    while (!getLocalTime(&timeinfo) && retry < 20) {
        delay(500);
        retry++;
    }

    if (retry < 20) {
        time_synced = true;
        Serial.println("Time synchronized successfully!");
    } else {
        time_synced = false;
        Serial.println("Time synchronization failed.");
    }
}

// Helper to classify a map pixel as water based on its red channel
inline bool isWaterPixel(uint16_t pix) {
    return ((pix >> 11) & 0x1F) < 6;
}

// Map latitude to screen y using calibrated piecewise linear interpolation to fit the AI map geography
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



// Highly optimized, row-by-row rendering of the day/night terminator map
void drawMap() {
    time_t now_utc;
    time(&now_utc);
    
    // Calculate current position of the Sun
    SolarPosition sun = calculateSolarPosition(now_utc);
    
    double phi_s = degToRad(sun.declination);
    double lambda_s = degToRad(sun.subsolarLon);

    // Precalculate subsolar terms
    double sin_phi_s = sin(phi_s);
    double cos_phi_s = cos(phi_s);
    double sin_lambda_s = sin(lambda_s);
    double cos_lambda_s = cos(lambda_s);

    // Grid lines spacing (every 30 degrees)
    // Longitude indices to draw lines at
    const int lon_lines[] = {27, 53, 80, 107, 133, 160, 187, 213, 240, 267, 293};
    const int lon_lines_count = 11;
    
    // Latitude indices to draw lines at
    const int lat_lines[] = {40, 80, 120, 160, 200};
    const int lat_lines_count = 5;

    // Direct buffer for writing a line of 320 pixels to the display via SPI
    uint16_t row_buffer[320];

    // Loop through each row in the map area (y from 0 to 199 or 239)
    int num_rows = banner_visible ? 200 : 240;
    for (int y = 0; y < num_rows; y++) {
        // Read raw physical map row from Flash memory into RAM buffer
        memcpy_P(row_buffer, &world_map[y * 320], 320 * sizeof(uint16_t));

        // Check if this row is a latitude grid line
        bool is_lat_grid = false;
        if (grid_enabled) {
            for (int i = 0; i < lat_lines_count; i++) {
                if (y == lat_lines[i]) {
                    is_lat_grid = true;
                    break;
                }
            }
        }

        // Row-specific solar terms to avoid redundant calculations in the inner loop
        double A = sin_phi[y] * sin_phi_s;
        double B = cos_phi[y] * cos_phi_s;
        double C = B * cos_lambda_s;
        double D = B * sin_lambda_s;

        // Loop through each pixel in the row
        for (int x = 0; x < 320; x++) {
            uint16_t raw_pixel = row_buffer[x];
            uint16_t pixel = raw_pixel;

            // Apply dynamic map transformations based on map_mode
            if (map_mode == 1) {
                // Mode 1: Basic Flat Map (navy blue oceans, sage green land)
                if (isWaterPixel(raw_pixel)) {
                    pixel = 0x0911; // Flat deep navy blue
                } else {
                    pixel = 0x5CE9; // Flat sage green
                }
            }

            // 1. Calculate Solar Elevation Angle (sin_a)
            // sin(a) = sin(phi)*sin(phi_s) + cos(phi)*cos(phi_s)*cos(lambda - lambda_s)
            // Using identity: cos(L - L_s) = cos(L)*cos(L_s) + sin(L)*sin(L_s)
            double sin_a = A + C * cos_lambda[x] + D * sin_lambda[x];

            // 2. Determine Day/Night/Twilight Dimming Factor
            double factor = 1.0;
            if (sin_a < 0.0) {
                if (twilight_mode == 0) {
                    // Sharp terminator: immediate jump to night dimming
                    factor = 0.25;
                } else {
                    // Blended twilight: smooth transition down to -12 degrees (nautical twilight)
                    // sin(-12 deg) approx -0.20791
                    const double sin_limit = -0.20791;
                    if (sin_a <= sin_limit) {
                        factor = 0.25; // Full night dimming
                    } else {
                        // Linear interpolation between 1.0 (day) and 0.25 (night)
                        factor = 0.25 + 0.75 * (sin_a - sin_limit) / (-sin_limit);
                    }
                }
            }

            // 3. Apply lat/lon grid line if enabled (50% blend with white)
            bool is_grid_pixel = is_lat_grid;
            if (grid_enabled && !is_grid_pixel) {
                for (int i = 0; i < lon_lines_count; i++) {
                    if (x == lon_lines[i]) {
                        is_grid_pixel = true;
                        break;
                    }
                }
            }

            if (is_grid_pixel) {
                // Extract RGB565 components
                uint16_t r = (pixel >> 11) & 0x1F;
                uint16_t g = (pixel >> 5) & 0x3F;
                uint16_t b = pixel & 0x1F;
                
                // Blend with white (value 31 for 5-bit, 63 for 6-bit)
                r = (r + 31) >> 1;
                g = (g + 63) >> 1;
                b = (b + 31) >> 1;
                
                pixel = (r << 11) | (g << 5) | b;
            }

            // 4. Apply day/night dimming factor
            if (factor < 1.0) {
                uint16_t r = (pixel >> 11) & 0x1F;
                uint16_t g = (pixel >> 5) & 0x3F;
                uint16_t b = pixel & 0x1F;
                
                r = (uint16_t)(r * factor);
                g = (uint16_t)(g * factor);
                b = (uint16_t)(b * factor);
                
                pixel = (r << 11) | (g << 5) | b;
            }

            row_buffer[x] = pixel;
        }

        // Push the fully compiled, blended row directly to the TFT display
        tft.pushImage(0, y, 320, 1, row_buffer);
    }

    // Draw defined home locations as high-contrast reference dots on the map if enabled
    if (home_enabled) {
        for (int i = 0; i < HOME_LOCATIONS_COUNT; i++) {
            float lat = HOME_LOCATIONS[i].latitude;
            float lon = HOME_LOCATIONS[i].longitude;
            
            // Map longitude (-180 to 180) to screen x (0 to 319)
            int x = (int)((lon + 180.0) * (320.0 / 360.0) + 0.5);
            // Map latitude using our calibrated curve to align with the AI-generated map's geography
            int y = latToY(lat);
            
            // Constrain to screen boundaries
            x = constrain(x, 0, 319);
            y = constrain(y, 0, 239);
            
            // Only draw if the pixel is in the currently visible map region
            if (!banner_visible || y < 200) {
                // Draw a black 1-pixel outer border for contrast on any background color
                tft.drawCircle(x, y, 3, TFT_BLACK);
                // Draw the solid colored center dot
                tft.fillCircle(x, y, 2, HOME_LOCATIONS[i].color);
            }
        }
    }
}

// Draw the interactive bottom dashboard banner containing clocks and settings
void drawBanner(bool forceRedraw) {
    if (!banner_visible) return;

    time_t now_utc;
    time(&now_utc);
    struct tm tm_utc = *gmtime(&now_utc); // Copy struct to local variable to avoid static buffer overwrite

    // Only redraw the banner when minutes change, or if explicitly forced
    if (!forceRedraw && tm_utc.tm_min == last_rendered_minute) {
        return;
    }
    last_rendered_minute = tm_utc.tm_min;

    // Draw solid black background for the banner area (y: 200-239)
    if (forceRedraw) {
        tft.fillRect(0, 200, 320, 40, TFT_BLACK);
        // Draw a thin dividers in Clockwork Yellow theme (gold/yellow)
        tft.drawFastHLine(0, 200, 320, 0xFDA0);
    }

    // 1. Calculate Local Time
    time_t now_local = now_utc + (timezone_offset * 3600);
    struct tm tm_local = *gmtime(&now_local); // Copy struct to local variable to avoid static buffer overwrite

    char time_str[16];
    char date_str[16];
    sprintf(time_str, "%02d:%02d", tm_local.tm_hour, tm_local.tm_min);
    
    // Format Month Day
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    sprintf(date_str, "%s %02d", months[tm_local.tm_mon], tm_local.tm_mday);

    // Render Local Clock (Left side, white)
    tft.fillRect(5, 204, 110, 32, TFT_BLACK); // Clear local time bounding box
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(time_str, 5, 204, 4); // Large elegant font (Font 4)
    
    // Render Local Timezone and Date (Smaller, below or next to clock)
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setTextDatum(BL_DATUM);
    char tz_lbl[16];
    if (timezone_offset == 0) {
        sprintf(tz_lbl, "UTC  %s", date_str);
    } else {
        sprintf(tz_lbl, "UTC%+d  %s", timezone_offset, date_str);
    }
    tft.drawString(tz_lbl, 5, 238, 1);

    // 2. Render UTC Clock (Right side, Gold)
    char utc_time_str[16];
    sprintf(utc_time_str, "%02d:%02d", tm_utc.tm_hour, tm_utc.tm_min);
    
    tft.fillRect(215, 204, 100, 32, TFT_BLACK); // Clear UTC time bounding box
    tft.setTextColor(0xFDA0, TFT_BLACK); // Clockwork Gold color
    tft.setTextDatum(TR_DATUM);
    tft.drawString(utc_time_str, 315, 204, 4); // Large Font 4
    
    tft.setTextColor(TFT_GOLD, TFT_BLACK);
    tft.setTextDatum(BR_DATUM);
    char utc_date_lbl[16];
    sprintf(utc_date_lbl, "UTC  %s %02d", months[tm_utc.tm_mon], tm_utc.tm_mday);
    tft.drawString(utc_date_lbl, 315, 238, 1);

    // 3. Render Dashboard Indicators & Buttons (Center area: x 115-210)
    tft.fillRect(115, 204, 95, 32, TFT_BLACK); // Clear middle area

    // --- BUTTON 1: Grid Status Button (x: 116-138) ---
    uint16_t grid_color = grid_enabled ? TFT_GREEN : TFT_WHITE;
    tft.drawRoundRect(116, 210, 22, 20, 4, grid_color);
    uint16_t grid_line_color = grid_enabled ? TFT_GREEN : 0x528A;
    tft.drawFastVLine(123, 212, 16, grid_line_color);
    tft.drawFastVLine(130, 212, 16, grid_line_color);
    tft.drawFastHLine(118, 216, 18, grid_line_color);
    tft.drawFastHLine(118, 224, 18, grid_line_color);

    // --- BUTTON 2: Home Location Toggle (x: 140-162) ---
    uint16_t home_btn_color = home_enabled ? 0xFDA0 : TFT_WHITE; // Gold vs White
    tft.drawRoundRect(140, 210, 22, 20, 4, home_btn_color);
    // Draw house roof
    tft.drawLine(145, 219, 151, 213, home_btn_color);
    tft.drawLine(151, 213, 157, 219, home_btn_color);
    tft.drawFastHLine(146, 219, 11, home_btn_color);
    // Draw house body
    tft.drawRect(147, 219, 9, 8, home_btn_color);
    // Draw door
    tft.fillRect(150, 223, 3, 4, home_btn_color);

    // --- BUTTON 3: Twilight Style Button (x: 164-186) ---
    uint16_t twi_color = twilight_mode == 1 ? 0x5DFF : TFT_WHITE; // Sky Blue vs White
    tft.drawRoundRect(164, 210, 22, 20, 4, twi_color);
    tft.drawCircle(175, 220, 5, twi_color);
    if (twilight_mode == 1) {
        for (int dx = -4; dx <= 0; dx++) {
            int h = (int)sqrt(25 - dx * dx);
            tft.drawFastVLine(175 + dx, 220 - h, 2 * h + 1, 0x5DFF);
        }
    } else {
        tft.drawFastVLine(175, 215, 11, TFT_WHITE);
    }

    // --- BUTTON 4: Map Mode Button (x: 188-210) ---
    uint16_t map_btn_colors[] = {TFT_ORANGE, 0x5CE9}; // Color (Orange), Flat (Sage Green)
    uint16_t map_btn_color = map_btn_colors[map_mode];
    tft.drawRoundRect(188, 210, 22, 20, 4, map_btn_color);
    
    // Draw folded map icon inside the box (x: 193 to 208, y: 213 to 227)
    tft.drawLine(193, 215, 193, 227, map_btn_color);
    tft.drawLine(198, 213, 198, 225, map_btn_color);
    tft.drawLine(203, 215, 203, 227, map_btn_color);
    tft.drawLine(208, 213, 208, 225, map_btn_color);
    
    tft.drawLine(193, 215, 198, 213, map_btn_color);
    tft.drawLine(198, 213, 203, 215, map_btn_color);
    tft.drawLine(203, 215, 208, 213, map_btn_color);
    
    tft.drawLine(193, 227, 198, 225, map_btn_color);
    tft.drawLine(198, 225, 203, 227, map_btn_color);
    tft.drawLine(203, 227, 208, 225, map_btn_color);

    // 4. Render small WiFi & Sync status icons below the buttons
    uint16_t wifi_color = wifi_connected ? TFT_GREEN : TFT_RED;
    tft.fillCircle(150, 235, 2, wifi_color);
    
    uint16_t ntp_color = time_synced ? TFT_GREEN : TFT_RED;
    tft.fillCircle(175, 235, 2, ntp_color);
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
    int touch_x = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, 320); // Normal mapping for landscape orientation 3
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
            Serial.println("Manual sync triggered via touch.");
            if (WiFi.status() != WL_CONNECTED) {
                connectWiFi();
            }
            if (wifi_connected) {
                syncNTP();
            }
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
    map_mode = prefs.getInt("map_mode", 0);
    prefs.end();

    // 1. Initialize Screen Backlight (using ESP32 PWM on channel 0)
    ledcSetup(0, 5000, 8);
    ledcAttachPin(TFT_BL, 0);
    setBacklight(2); // Keep backlight fully ON

    // 2. Initialize TFT Display
    tft.init();
    tft.setRotation(3); // Landscape: USB port on the right side
    tft.setSwapBytes(true); // Swap bytes for correct RGB565 color mapping
    tft.invertDisplay(true); // Invert colors to correct IPS/panel inversion (white = white, blue = blue)
    tft.fillScreen(TFT_BLACK);

    // 3. Initialize Touchscreen on a dedicated SPI bus
    touchscreenSPI.begin(25, 39, 32, 33); // SCLK, MISO, MOSI, CS
    ts.begin(touchscreenSPI);
    ts.setRotation(3); // Match TFT display rotation

    // 4. Precalculate Trigonometric Tables
    initTrigTables();

    // 5. Connect to WiFi & Sync Time
    connectWiFi();
    if (wifi_connected) {
        syncNTP();
    }

    // 6. Perform initial drawing
    banner_visible = true;
    last_activity_time = millis();
    drawMap();
    drawBanner(true);
    last_map_update = millis();
}

void loop() {
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
        
        // Trigger NTP sync periodically (every hour)
        static uint32_t last_ntp_sync = 0;
        if (wifi_connected && (now - last_ntp_sync >= NTP_SYNC_INTERVAL_SEC * 1000 || last_ntp_sync == 0)) {
            last_ntp_sync = now;
            syncNTP();
        }
    }

    // Update the clocks in the bottom banner (handles its own min-change filter)
    drawBanner(false);

    // Minimal delay to keep ESP32 watchdog and task scheduler happy
    delay(20);
}
