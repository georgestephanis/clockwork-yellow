# Clockwork Yellow - Development & Validation Tools

This directory contains a suite of Python scripts designed to generate assets, validate astronomical equations, and optimize image processing/water-detection algorithms for the Clockwork Yellow world clock.

---

## 🛠️ Tool Index

### 1. [image_to_c.py](file:///Users/georgestephanis/code/clockwork-yellow/tools/image_to_c.py)
* **Purpose**: Converts any standard image format (PNG, JPEG, etc.) into a resized, 16-bit RGB565 C++ header file (`world_map.h`) stored in flash memory (`PROGMEM`) for display rendering.
* **Key Features**:
  - Automatically resizes input images to the target Cheap Yellow Display (CYD) landscape resolution of 320x240 pixels using high-quality Lanczos downsampling.
  - Automatically installs the Python `Pillow` library via pip if it is not already present on the host system.
  - Formats the output array in neat rows of 16 hexadecimal values for readability.
* **Usage**:
  ```bash
  python3 tools/image_to_c.py <path_to_input_image> <path_to_output_header>
  ```
  *Example*:
  ```bash
  python3 tools/image_to_c.py assets/world_physical_map.png src/world_map.h
  ```

### 2. [verify_seasons.py](file:///Users/georgestephanis/code/clockwork-yellow/tools/verify_seasons.py)
* **Purpose**: Simulates and validates the astronomical algorithms implemented in `src/solar.cpp` (based on Jean Meeus' *Astronomical Algorithms*) for the solstices and equinoxes.
* **Key Features**:
  - Computes the precise Julian Date, geometric mean longitude, anomaly, eccentricity, apparent longitude, and obliquity.
  - Outputs a verification table displaying the exact solar declination (axial tilt angle), subsolar longitude, and Equation of Time (EoT) in minutes.
  - Mathematically proves that the shadow declination correctly limits between $+23.44^\circ$ (Summer Solstice) and $-23.44^\circ$ (Winter Solstice), and crosses $0^\circ$ at the equinoxes.
* **Usage**:
  ```bash
  python3 tools/verify_seasons.py
  ```

### 3. [analyze_map.py](file:///Users/georgestephanis/code/clockwork-yellow/tools/analyze_map.py)
* **Purpose**: Inspects the raw 16-bit pixel values and extracts the 5-bit Red, 6-bit Green, and 5-bit Blue components of the map at specific geographic coordinates.
* **Key Features**:
  - Provides a diagnostic readout of known land/desert, ice, ocean, and dense forest regions.
  - Prints statistics and raw values for pixels that fall below specific color thresholds to troubleshoot classification issues.
* **Usage**:
  ```bash
  python3 tools/analyze_map.py
  ```

### 4. [visualize_rules.py](file:///Users/georgestephanis/code/clockwork-yellow/tools/visualize_rules.py)
* **Purpose**: Generates downsampled 24x80 ASCII text world maps using different water-detection rules.
* **Key Features**:
  - Draws `#` for land and spaces for water.
  - Allows rapid visual inspection of candidate thresholds to ensure continents are well-defined and that no inland bleeding or coastline expansions occur.
* **Usage**:
  ```bash
  python3 tools/visualize_rules.py
  ```

### 5. [test_lakes_rules.py](file:///Users/georgestephanis/code/clockwork-yellow/tools/test_lakes_rules.py)
* **Purpose**: Evaluates candidate water-detection rules against a targeted set of test pixels representing the Great Lakes (water) and the Amazon rainforest (land).
* **Key Features**:
  - Outputs success rates for lake detection versus land misclassification.
  - Renders visual ASCII maps of the world for each tested rule.
* **Usage**:
  ```bash
  python3 tools/test_lakes_rules.py
  ```

### 6. [count_global_pixels.py](file:///Users/georgestephanis/code/clockwork-yellow/tools/count_global_pixels.py)
* **Purpose**: Counts the global water pixel totals and percentages across the entire 76,800-pixel map for different classification rules.
* **Key Features**:
  - Helps check if a rule is globally balanced (a correct rule should yield roughly $64.5\%$ water once land misclassifications are removed).
* **Usage**:
  ```bash
  python3 tools/count_global_pixels.py
  ```

### 7. [compare_rules_rigorous.py](file:///Users/georgestephanis/code/clockwork-yellow/tools/compare_rules_rigorous.py)
* **Purpose**: Runs a rigorous side-by-side quantitative comparison of all candidate rules, tracking water pixel counts in the South America box, the Great Lakes region, and the global map.
* **Key Features**:
  - Proves mathematically that the rule `(b > r + 3) && (b >= g - 12)` preserves the Great Lakes (26/29 pixels) and keeps the Amazon rainforest as land (reducing South American false positives by over 110 pixels).
* **Usage**:
  ```bash
  python3 tools/compare_rules_rigorous.py
  ```

---

## 📋 Requirements & Setup

All scripts are written in Python 3 and have zero external dependencies, with the exception of `image_to_c.py` which requires the `Pillow` image library.

1. **Verify Python 3 is installed**:
   ```bash
   python3 --version
   ```
2. **Run any validation script** directly from the project root:
   ```bash
   python3 tools/compare_rules_rigorous.py
   ```
