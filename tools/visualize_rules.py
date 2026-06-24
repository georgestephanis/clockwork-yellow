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

def print_ascii_map(rule_func, title):
    print(f"\n=== {title} ===")
    # Downsample to 24 rows by 80 columns
    dy = 240 // 24
    dx = 320 // 80
    for y_ascii in range(24):
        line = ""
        for x_ascii in range(80):
            # Sample a pixel in this block
            y = y_ascii * dy
            x = x_ascii * dx
            idx = y * 320 + x
            val = pixels[idx]
            r, g, b = get_rgb565(val)
            if rule_func(r, g, b):
                line += " " # Water is space
            else:
                line += "#" # Land is #
        print(line)

# Current rule: R < 6
print_ascii_map(lambda r, g, b: r < 6, "Current Rule: R < 6")

# Proposed Rule 1: R < 6 and B > G
print_ascii_map(lambda r, g, b: r < 6 and b > g, "Proposed Rule 1: R < 6 and B > G")

# Proposed Rule 2: B > G (since water is blue, land is not)
print_ascii_map(lambda r, g, b: b > g, "Proposed Rule 2: B > G")

# Proposed Rule 3: B >= 11 (since water is highly blue)
print_ascii_map(lambda r, g, b: b >= 11, "Proposed Rule 3: B >= 11")

# Proposed Rule 4: B > G and B > R (water is bluer than green or red)
print_ascii_map(lambda r, g, b: b > g and b > r, "Proposed Rule 4: B > G and B > R")
