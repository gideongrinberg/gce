import glob
import pathlib
import os

def sanitize_symbol(name):
    return name.replace('.', '_').replace('-', '_')

image_dir = pathlib.Path(__file__).resolve().parent
png_files = glob.glob((image_dir / "*.png").as_posix())

header_path = image_dir.parent / "images.h"
source_path = image_dir.parent / "images.c"

with open(header_path, "w") as header:
    header.write("// Auto-generated header with embedded PNG image data\n\n")
    header.write("#ifndef IMAGES_H\n#define IMAGES_H\n\n")

    for file in png_files:
        filename = os.path.basename(file)
        symbol_base = sanitize_symbol(filename)

        header.write(f"// {filename}\n")
        header.write(f"extern unsigned char {symbol_base}[];\n")
        header.write(f"extern unsigned int {symbol_base}_len;\n\n")

    header.write("#endif // IMAGES_H\n")

with open(source_path, "w") as source:
    source.write("// Auto-generated source file with embedded PNG image data\n\n")

    for file in png_files:
        filename = os.path.basename(file)
        symbol_base = sanitize_symbol(filename)

        with open(file, "rb") as f:
            data = f.read()

        source.write(f"// {filename}\n")
        source.write(f"unsigned char {symbol_base}[] = {{\n")

        for i in range(0, len(data), 12):
            line = ', '.join(f'0x{b:02x}' for b in data[i:i + 12])
            source.write(f"    {line},\n")

        source.write("};\n")
        source.write(f"unsigned int {symbol_base}_len = {len(data)};\n\n")
