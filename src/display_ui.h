#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <TFT_eSPI.h>

// Expose the global TFT instance for hardware reference if needed
extern TFT_eSPI tft;

/**
 * Initializes the TFT display hardware, sets rotation, byte-swapping, 
 * and IPS color inversion. Also initializes the screen backlight.
 */
void initDisplay();

/**
 * Sets the screen backlight level.
 * 
 * @param level Backlight level.
 */
void setBacklight(int level);

/**
 * Highly optimized rendering of the day/night terminator map.
 * Blends the base map with solar declination, twilight, coordinates grid, and home locations.
 * 
 * @param bannerVisible Whether the bottom banner is currently visible.
 * @param mapMode Active map mode (0 = Full Color, 1 = Flat).
 * @param twilightMode Active twilight mode (0 = Sharp, 1 = Blended).
 * @param gridEnabled Whether the coordinate grid is enabled.
 * @param homeEnabled Whether home location dots are enabled.
 */
void drawMap(bool bannerVisible, int mapMode, int twilightMode, bool gridEnabled, bool homeEnabled);

/**
 * Renders the bottom dashboard status banner, clocks, custom vector icons, and network indicators.
 * 
 * @param forceRedraw If true, forces a full redraw of the banner background and borders.
 * @param bannerVisible Whether the banner is currently visible.
 * @param timezoneOffset Current local timezone offset in hours.
 * @param gridEnabled Whether the coordinate grid is active.
 * @param homeEnabled Whether home location dots are active.
 * @param twilightMode Active twilight mode (0 = Sharp, 1 = Blended).
 * @param mapMode Active map mode (0 = Color, 1 = Flat).
 */
void drawBanner(bool forceRedraw, bool bannerVisible, int timezoneOffset, bool gridEnabled, bool homeEnabled, int twilightMode, int mapMode);

#endif // DISPLAY_UI_H
