#!/usr/bin/env python3
import os
import sys

def convert_image_to_rgb565(image_path, output_header_path):
    try:
        from PIL import Image
    except ImportError:
        print("Error: The 'Pillow' library is not installed. Installing it now...")
        import subprocess
        subprocess.check_call([sys.executable, "-m", "pip", "install", "Pillow"])
        from PIL import Image

    print(f"Opening image: {image_path}")
    if not os.path.exists(image_path):
        print(f"Error: File {image_path} does not exist.")
        return False

    img = Image.open(image_path)
    
    # Target size for the ESP32 CYD screen in landscape
    width, height = 320, 240
    print(f"Resizing image from {img.size} to {width}x{height} using Lanczos filter...")
    img = img.resize((width, height), Image.Resampling.LANCZOS)
    img = img.convert("RGB")

    print("Generating C++ array...")
    pixels = list(img.getdata())
    
    rgb565_values = []
    for r, g, b in pixels:
        # Scale to 5-6-5 bits
        r5 = (r >> 3) & 0x1F
        g6 = (g >> 2) & 0x3F
        b5 = (b >> 3) & 0x1F
        
        # Combine into a single 16-bit unsigned integer
        val = (r5 << 11) | (g6 << 5) | b5
        rgb565_values.append(val)

    print(f"Writing C++ header: {output_header_path}")
    os.makedirs(os.path.dirname(output_header_path), exist_ok=True)
    
    with open(output_header_path, "w") as f:
        f.write("// Clockwork Yellow World Map Asset\n")
        f.write("// Generated automatically from raw image. Do not edit manually.\n\n")
        f.write("#ifndef WORLD_MAP_H\n")
        f.write("#define WORLD_MAP_H\n\n")
        f.write("#include <pgmspace.h>\n")
        f.write("#include <stdint.h>\n\n")
        f.write("#define WORLD_MAP_WIDTH 320\n")
        f.write("#define WORLD_MAP_HEIGHT 240\n\n")
        f.write("// World map stored in Flash (PROGMEM) in RGB565 format (150 KB)\n")
        f.write("const uint16_t world_map[76800] PROGMEM = {\n")
        
        # Write values in formatted rows of 16 values for readability
        for i in range(0, len(rgb565_values), 16):
            chunk = rgb565_values[i:i+16]
            hex_strings = [f"0x{val:04X}" for val in chunk]
            line = ", ".join(hex_strings)
            if i + 16 < len(rgb565_values):
                f.write(f"    {line},\n")
            else:
                f.write(f"    {line}\n")
                
        f.write("};\n\n")
        f.write("#endif // WORLD_MAP_H\n")

    print("Success! C++ header generated successfully.")
    return True

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 image_to_c.py <input_image_path> <output_header_path>")
        sys.exit(1)
        
    input_img = sys.argv[1]
    output_h = sys.argv[2]
    convert_image_to_rgb565(input_img, output_h)
