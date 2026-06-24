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

# Let's define the regions:
# Great Lakes region: x: 78 to 92, y: 65 to 80
# Amazon rainforest land region (which we want to be land, i.e., NOT water):
# Let's collect coordinates of known Amazon land pixels that were misclassified (R < 6)
# e.g., (99, 110) [R=5, G=22, B=8], (101, 113) [R=5, G=23, B=0], (97, 117) [R=5, G=19, B=2]
# And known Great Lakes water pixels, e.g., (85, 65) [R=3, G=17, B=13], (84, 66) [R=3, G=18, B=12], (85, 66) [R=5, G=19, B=10]

great_lakes_test = [
    (84, 65, 1, 16, 16),
    (85, 65, 3, 17, 13),
    (84, 66, 3, 18, 12),
    (85, 66, 5, 19, 10),
    (78, 70, 2, 17, 12),
    (78, 72, 2, 15, 12)
]

amazon_land_test = [
    (99, 110, 5, 22, 8),
    (101, 113, 5, 23, 0),
    (105, 114, 5, 21, 8),
    (106, 114, 5, 22, 9),
    (97, 117, 5, 19, 2),
    (94, 118, 5, 22, 1),
    (98, 118, 5, 20, 2)
]

rules = {
    "b > r + 3": lambda r, g, b: b > r + 3,
    "b > r + 4": lambda r, g, b: b > r + 4,
    "b > r + 2": lambda r, g, b: b > r + 2,
    "b > r + 1": lambda r, g, b: b > r + 1,
    "b > r and b > g - 6": lambda r, g, b: b > r and b >= g - 6,
    "b > r + 2 and b >= g - 8": lambda r, g, b: b > r + 2 and b >= g - 8,
    "b > r + 3 and b >= g - 10": lambda r, g, b: b > r + 3 and b >= g - 10,
    "b > r + 4 and b >= g - 12": lambda r, g, b: b > r + 4 and b >= g - 12,
}

print("Testing candidate rules on test pixels:")
for name, rule in rules.items():
    lakes_passed = sum(1 for _, _, r, g, b in great_lakes_test if rule(r, g, b))
    amazon_passed = sum(1 for _, _, r, g, b in amazon_land_test if rule(r, g, b))
    print(f"Rule: {name:<30} | Lakes detected: {lakes_passed}/{len(great_lakes_test)} | Amazon misclassified as water: {amazon_passed}/{len(amazon_land_test)}")

# Let's also print ASCII maps for the top candidates to see how they look globally!
def print_ascii_map(rule_func, title):
    print(f"\n=== {title} ===")
    dy = 240 // 24
    dx = 320 // 80
    for y_ascii in range(24):
        line = ""
        for x_ascii in range(80):
            y = y_ascii * dy
            x = x_ascii * dx
            idx = y * 320 + x
            val = pixels[idx]
            r, g, b = get_rgb565(val)
            if rule_func(r, g, b):
                line += " " # Water
            else:
                line += "#" # Land
        print(line)

# Let's visualize the best candidates
print_ascii_map(lambda r, g, b: b > r + 3, "Rule: b > r + 3")
print_ascii_map(lambda r, g, b: b > r + 4, "Rule: b > r + 4")
print_ascii_map(lambda r, g, b: b > r + 2, "Rule: b > r + 2")
print_ascii_map(lambda r, g, b: b > r + 4 and b >= g - 12, "Rule: b > r + 4 and b >= g - 12")
