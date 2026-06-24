#include "display_ui.h"
#include <Arduino.h>
#include <time.h>
#include "config.h"
#include "solar.h"
#include "world_map.h"
#include "map_projection.h"
#include "network_time.h"

// Global TFT instance definition
TFT_eSPI tft = TFT_eSPI();

// Internal state for banner rendering optimization
static int last_rendered_minute = -1;

// Helper: Convert degrees to radians
static inline double degToRad(double deg) {
    return deg * M_PI / 180.0;
}

// Helper to classify a map pixel as water based on its red channel
static inline bool isWaterPixel(uint16_t pix) {
    return ((pix >> 11) & 0x1F) < 6;
}

void setBacklight(int level) {
    ledcWrite(0, 255); // Always fully ON (255 is active-high maximum brightness)
}

void initDisplay() {
    // 1. Initialize and turn off the RGB LED on the back of the device
    // (Common anode RGB LED is active-low: writing HIGH turns it OFF)
    pinMode(CYD_RGB_RED, OUTPUT);
    pinMode(CYD_RGB_GREEN, OUTPUT);
    pinMode(CYD_RGB_BLUE, OUTPUT);
    digitalWrite(CYD_RGB_RED, HIGH);
    digitalWrite(CYD_RGB_GREEN, HIGH);
    digitalWrite(CYD_RGB_BLUE, HIGH);

    // 2. Initialize Screen Backlight (using ESP32 PWM on channel 0)
    ledcSetup(0, 5000, 8);
    ledcAttachPin(TFT_BL, 0);
    setBacklight(2); // Keep backlight fully ON

    // 3. Initialize TFT Display
    tft.init();
    tft.setRotation(3); // Landscape: USB port on the right side
    tft.setSwapBytes(true); // Swap bytes for correct RGB565 color mapping
    tft.invertDisplay(true); // Invert colors to correct IPS/panel inversion (white = white, blue = blue)
    tft.fillScreen(TFT_BLACK);
}

void drawMap(bool bannerVisible, int mapMode, int twilightMode, bool gridEnabled, bool homeEnabled) {
    time_t now_utc;
    time(&now_utc);
    
    // Calculate current position of the Sun
    SolarPosition sun = calculateSolarPosition(now_utc);
    
    double phi_s = degToRad(sun.declination);
    double lambda_s = degToRad(sun.subsolarLon);

    // Precalculate subsolar terms
    double sin_phi_s = std::sin(phi_s);
    double cos_phi_s = std::cos(phi_s);
    double sin_lambda_s = std::sin(lambda_s);
    double cos_lambda_s = std::cos(lambda_s);

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
    int num_rows = bannerVisible ? 200 : 240;
    for (int y = 0; y < num_rows; y++) {
        // Read raw physical map row from Flash memory into RAM buffer
        memcpy_P(row_buffer, &world_map[y * 320], 320 * sizeof(uint16_t));

        // Check if this row is a latitude grid line
        bool is_lat_grid = false;
        if (gridEnabled) {
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

            // Apply dynamic map transformations based on mapMode
            if (mapMode == 1) {
                // Mode 1: Basic Flat Map (navy blue oceans, sage green land)
                if (isWaterPixel(raw_pixel)) {
                    pixel = 0x0911; // Flat deep navy blue
                } else {
                    pixel = 0x5CE9; // Flat sage green
                }
            }

            // 1. Calculate Solar Elevation Angle (sin_a)
            double sin_a = A + C * cos_lambda[x] + D * sin_lambda[x];

            // 2. Determine Day/Night/Twilight Dimming Factor
            double factor = 1.0;
            if (sin_a < 0.0) {
                if (twilightMode == 0) {
                    // Sharp terminator: immediate jump to night dimming
                    factor = 0.25;
                } else {
                    // Blended twilight: smooth transition down to -12 degrees (nautical twilight)
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
            if (gridEnabled && !is_grid_pixel) {
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
    if (homeEnabled) {
        for (int i = 0; i < HOME_LOCATIONS_COUNT; i++) {
            float lat = HOME_LOCATIONS[i].latitude;
            float lon = HOME_LOCATIONS[i].longitude;
            
            // Map longitude (-180 to 180) to screen x (0 to 319)
            int x = (int)((lon + 180.0f) * (320.0f / 360.0f) + 0.5f);
            // Map latitude using our calibrated curve
            int y = latToY(lat);
            
            // Constrain to screen boundaries
            x = constrain(x, 0, 319);
            y = constrain(y, 0, 239);
            
            // Only draw if the pixel is in the currently visible map region
            if (!bannerVisible || y < 200) {
                // Draw a black 1-pixel outer border for contrast on any background color
                tft.drawCircle(x, y, 3, TFT_BLACK);
                // Draw the solid colored center dot
                tft.fillCircle(x, y, 2, HOME_LOCATIONS[i].color);
            }
        }
    }
}

void drawBanner(bool forceRedraw, bool bannerVisible, int timezoneOffset, bool gridEnabled, bool homeEnabled, int twilightMode, int mapMode) {
    if (!bannerVisible) return;

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
        // Draw a thin divider in Clockwork Yellow theme (gold/yellow)
        tft.drawFastHLine(0, 200, 320, 0xFDA0);
    }

    // 1. Calculate Local Time
    time_t now_local = now_utc + (timezoneOffset * 3600);
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
    if (timezoneOffset == 0) {
        sprintf(tz_lbl, "UTC  %s", date_str);
    } else {
        sprintf(tz_lbl, "UTC%+d  %s", timezoneOffset, date_str);
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
    uint16_t grid_color = gridEnabled ? TFT_GREEN : TFT_WHITE;
    tft.drawRoundRect(116, 210, 22, 20, 4, grid_color);
    uint16_t grid_line_color = gridEnabled ? TFT_GREEN : 0x528A;
    tft.drawFastVLine(123, 212, 16, grid_line_color);
    tft.drawFastVLine(130, 212, 16, grid_line_color);
    tft.drawFastHLine(118, 216, 18, grid_line_color);
    tft.drawFastHLine(118, 224, 18, grid_line_color);

    // --- BUTTON 2: Home Location Toggle (x: 140-162) ---
    uint16_t home_btn_color = homeEnabled ? 0xFDA0 : TFT_WHITE; // Gold vs White
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
    uint16_t twi_color = twilightMode == 1 ? 0x5DFF : TFT_WHITE; // Sky Blue vs White
    tft.drawRoundRect(164, 210, 22, 20, 4, twi_color);
    tft.drawCircle(175, 220, 5, twi_color);
    if (twilightMode == 1) {
        for (int dx = -4; dx <= 0; dx++) {
            int h = (int)sqrt(25 - dx * dx);
            tft.drawFastVLine(175 + dx, 220 - h, 2 * h + 1, 0x5DFF);
        }
    } else {
        tft.drawFastVLine(175, 215, 11, TFT_WHITE);
    }

    // --- BUTTON 4: Map Mode Button (x: 188-210) ---
    uint16_t map_btn_colors[] = {TFT_ORANGE, 0x5CE9}; // Color (Orange), Flat (Sage Green)
    uint16_t map_btn_color = map_btn_colors[mapMode];
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
    uint16_t wifi_color = isWiFiConnected() ? TFT_GREEN : TFT_RED;
    tft.fillCircle(150, 235, 2, wifi_color);
    
    uint16_t ntp_color = isTimeSynced() ? TFT_GREEN : TFT_RED;
    tft.fillCircle(175, 235, 2, ntp_color);
}
