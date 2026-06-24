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

rules = {
    "Original R < 6": lambda r, g, b: r < 6,
    "Last attempt b > g": lambda r, g, b: b > g,
    "b > r + 2": lambda r, g, b: b > r + 2,
    "b > r + 3": lambda r, g, b: b > r + 3,
    "b > r + 4": lambda r, g, b: b > r + 4,
    "b > r + 3 and b >= g - 10": lambda r, g, b: b > r + 3 and b >= g - 10,
    "b > r + 4 and b >= g - 12": lambda r, g, b: b > r + 4 and b >= g - 12,
}

print("Global Water Pixel Count for each rule:")
for name, rule in rules.items():
    count = sum(1 for val in pixels if rule(*get_rgb565(val)))
    print(f"{name:<30}: {count} water pixels ({count/76800*100:.2f}%)")
