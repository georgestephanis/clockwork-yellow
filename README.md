# Clockwork Yellow

A premium, highly optimized firmware for the **ESP32 Cheap Yellow Display (CYD)** that displays a real-time **GeoChron-style world map** with a dynamically calculated day/night terminator and twilight blending. It automatically synchronizes and updates itself using internet NTP servers via WiFi.

Named as an homage to *A Clockwork Orange*, the fact that GeoChrons are a sort of clock, and its target hardware (the CYD).

---

## 🗺️ Visual Assets

The firmware compiles a high-contrast physical world map directly into the ESP32's flash memory. The day/night shadow and twilight zones are calculated and blended onto this base map in real-time.

---

## 🚀 Key Features

* **High-Precision Solar Position:** Implements self-contained Meeus/NOAA solar algorithms to compute the Sun's declination and the Equation of Time (correcting for Earth's orbital eccentricity).
* **Ultra-Fast Rendering (Row-by-Row):** Uses trigonometric precalculations to render the entire screen in **under 2 milliseconds** of CPU time per frame, avoiding slow runtime trig functions.
* **Soft Twilight Blending:** Supports a beautifully blended twilight zone (nautical twilight, down to $-12^\circ$ solar elevation) for a smooth day-to-night gradient.
* **Interactive Dashboard:**
  * **Local Time Clock:** Tap the local clock to increment/decrement the timezone offset (persists automatically in ESP32 non-volatile Preferences storage).
  * **UTC Clock:** Standard in GeoChrons, in signature Clockwork Gold color.
  * **[G] Grid Button:** Toggles a 30-degree coordinate grid overlay.
  * **[T] Twilight Button:** Toggles between a sharp day/night line and a soft blended twilight zone.
  * **[M] Map Mode Button:** Represented by a custom folded vector map icon, this button toggles between the original full-color physical map and a clean, basic flat dual-color map (deep navy blue oceans and sage green continents) dynamically at runtime (zero flash overhead).
  * **Auto-Hiding Dashboard & Full-Screen Expansion:** Auto-hides the control banner after 10 seconds of touch inactivity to expand the map to the full 320x240 screen, showing the southern hemisphere down to the South Pole. Tapping the screen safely wakes the banner back up without triggering accidental button presses.
  * **Piecewise Latitude Calibration:** Features a mathematically calibrated latitude mapping function `latToY()` (RMS error = 1.11 pixels) to perfectly plot customized home location reference dots (like Marietta, PA) onto the AI-generated map's warped geography.
  * **Manual Sync:** Tapping the UTC clock area forces an immediate WiFi reconnection and NTP sync.
* **Network & Sync Indicators:** Two green status dots monitor WiFi connection and NTP time sync states.
* **SPI Bus Separation:** Runs the touchscreen on a dedicated hardware SPI bus (`VSPI`), eliminating bus conflicts and preventing electrical screen flickering.

---

## 🔌 Hardware Configuration (ESP32-2432S028R)

The CYD board is hardwired as follows (automatically configured in `platformio.ini` and `main.cpp`):

### TFT Display (SPI HSPI Bus)
* **Driver:** `ILI9341_2_DRIVER` (with panel color inversion enabled)
* **MOSI:** GPIO 13
* **MISO:** GPIO 12
* **SCLK:** GPIO 14
* **CS:** GPIO 15
* **DC:** GPIO 2
* **RST:** GPIO -1 (uses ESP32 reset)
* **Backlight (BL):** GPIO 21 (kept permanently ON at 100% active-high duty cycle to support all CYD board revisions)

### Touchscreen (SPI VSPI Bus)
* **CS:** GPIO 33
* **MOSI:** GPIO 32
* **MISO:** GPIO 39
* **SCLK:** GPIO 25
* **IRQ:** GPIO 36

---

## 🛠️ Software Setup & Build

This project is structured for **PlatformIO**. To compile and flash:

1. **Install PlatformIO Core:** Ensure you have PlatformIO installed (via VS Code or CLI).
2. **Configure WiFi Credentials:** Copy the template configuration file `src/config.template.h` to `src/config.h` (which is automatically gitignored to protect your private credentials):
   ```bash
   cp src/config.template.h src/config.h
   ```
   Open `src/config.h` and enter your network details:
   ```cpp
   #define WIFI_SSID "Your_Actual_SSID"
   #define WIFI_PASSWORD "Your_Actual_Password"
   ```
3. **Compile the Firmware:**
   ```bash
   pio run
   ```
4. **Flash to the CYD:**
   Connect the CYD via micro-USB/USB-C and run:
   ```bash
   pio run --target upload
   ```
   *Note: If the upload utility fails to connect and displays `Connecting...`, hold down the physical **BOOT** button on the back of the Cheap Yellow Display board to force it into bootloader mode.*
5. **Open Serial Monitor:**
   ```bash
   pio device monitor
   ```

---

## 📜 License

This project is open-source and free to use. Portions of the solar equations are based on standard Jean Meeus algorithms.
