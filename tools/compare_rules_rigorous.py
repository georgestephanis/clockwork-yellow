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

# Coordinates:
# South America land box: x from 90 to 140, y from 100 to 180
# In this region, we want to see how many land pixels are misclassified as water.
# How do we know if a pixel in this box is land in the original map?
# Original map water was R < 6, but we know the Amazon rainforest had R < 6 too.
# Let's define "actual land" in South America as pixels that are land in the physical world.
# Actually, we can just look at the visual quality or we can count how many pixels are classified
# as water in the South America box.
# In the original map:
# Total pixels in SA box with R < 6 (current water detection) was 2860.
# We know this included the Amazon River mouth and coastal waters, but also a lot of forest.
# Let's test the candidate rules on the SA box and print the water pixel counts.
# We want the count to be around the actual water pixels (coast + river), which is probably around 1500-2000.
# Let's also count the Great Lakes pixels (x: 78-92, y: 65-80) that are detected as water.
# We want this count to be as high as possible (original R < 6 had 51 blue-ish pixels).

rules = {
    "Original R < 6": lambda r, g, b: r < 6,
    "Last attempt b > g": lambda r, g, b: b > g,
    "b > r + 2": lambda r, g, b: b > r + 2,
    "b > r + 3": lambda r, g, b: b > r + 3,
    "b > r + 4": lambda r, g, b: b > r + 4,
    "b > r + 3 and b >= g - 10": lambda r, g, b: b > r + 3 and b >= g - 10,
    "b > r + 4 and b >= g - 12": lambda r, g, b: b > r + 4 and b >= g - 12,
    "b > r + 3 and b >= g - 12": lambda r, g, b: b > r + 3 and b >= g - 12,
    "b > r + 2 and b >= g - 10": lambda r, g, b: b > r + 2 and b >= g - 10,
}

print(f"{'Rule':<30} | {'SA Box Water Pixels':<20} | {'Great Lakes Pixels':<20} | {'Global Water %':<15}")
print("-" * 92)

for name, rule in rules.items():
    # Count in South America box
    sa_water = 0
    for y in range(100, 180):
        for x in range(90, 140):
            idx = y * 320 + x
            val = pixels[idx]
            if rule(*get_rgb565(val)):
                sa_water += 1
                
    # Count in Great Lakes box
    lakes_water = 0
    for y in range(65, 80):
        for x in range(78, 92):
            idx = y * 320 + x
            val = pixels[idx]
            r, g, b = get_rgb565(val)
            # Only count if it's a blue-ish pixel (b > 5)
            if b > 5 and rule(r, g, b):
                lakes_water += 1
                
    # Global count
    global_water = sum(1 for val in pixels if rule(*get_rgb565(val)))
    global_pct = global_water / 76800 * 100
    
    print(f"{name:<30} | {sa_water:<20} | {lakes_water:<20} | {global_pct:.2f}%")
