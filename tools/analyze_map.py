import re

# Read the world_map.h file
with open('/Users/georgestephanis/code/clockwork-yellow/src/world_map.h', 'r') as f:
    content = f.read()

# Find all hex numbers
hex_values = re.findall(r'0x[0-9a-fA-F]+', content)
pixels = [int(val, 16) for val in hex_values]

def get_rgb565(val):
    r = (val >> 11) & 0x1F
    g = (val >> 5) & 0x3F
    b = val & 0x1F
    return r, g, b

# Let's inspect the Great Lakes region:
# x: 78 to 92, y: 65 to 80
print("Analyzing Great Lakes region:")
lake_pixels = 0
for y in range(65, 80):
    for x in range(78, 92):
        idx = y * 320 + x
        val = pixels[idx]
        r, g, b = get_rgb565(val)
        # Let's print pixels in this region that have some blue component
        # We can see what their R, G, B values are
        if b > 5:
            print(f"Lake pixel at ({x}, {y}): hex={hex(val)}, R={r}, G={g}, B={b} | b/g={b/(g or 1):.2f} | r={r}")
            lake_pixels += 1
print(f"Total blue-ish pixels in Great Lakes region: {lake_pixels}")

# Let's inspect the Amazon rainforest again:
# x: 100 to 120, y: 110 to 130
print("\nAnalyzing Amazon rainforest region:")
forest_pixels = 0
for y in range(110, 130):
    for x in range(100, 120):
        idx = y * 320 + x
        val = pixels[idx]
        r, g, b = get_rgb565(val)
        # Let's see what the dark green forest pixels look like:
        if r < 8:
            if forest_pixels < 20:
                print(f"Amazon pixel at ({x}, {y}): hex={hex(val)}, R={r}, G={g}, B={b} | b/g={b/(g or 1):.2f}")
            forest_pixels += 1
print(f"Total dark pixels (R < 8) in Amazon: {forest_pixels}")

# Let's test different candidate thresholds!
# We want a threshold that:
# 1. Classifies Great Lakes as water.
# 2. Classifies Amazon as land.
# 3. Classifies coastal antialiased pixels as water if they are mostly water.
#
# Let's write a loop to test different formulas on both regions.
# A candidate formula could be:
# - b > g - delta (e.g. b >= g - 4, or b >= g - 6? Wait, remember g is 6-bit and b is 5-bit!)
#   Wait, if g is 6-bit and b is 5-bit, to compare them on the same scale, we should do:
#   b_scaled = b * 2 (or b << 1)
#   Then compare b_scaled with g!
#   Let's check:
#   - For ocean: b=17, g=16. b_scaled = 34. 34 is much larger than 16!
#   - For Amazon: b=1, g=25. b_scaled = 2. 2 is much smaller than 25!
#   - For Great Lakes: let's see what their values are first.
