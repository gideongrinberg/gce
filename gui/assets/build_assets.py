import glob
import pathlib
import os


def sanitize_symbol(name):
    return name.replace('.', '_').replace('-', '_').lower()


res_dir = pathlib.Path(__file__).resolve().parent
all_files = lambda ext: glob.glob((res_dir / f"*.{ext}").as_posix())
files = all_files('png') + all_files('ttf')
header_path = res_dir.parent / "assets.h"
source_path = res_dir.parent / "assets.c"

with open(header_path, "w") as header:
    header.write("// Auto-generated header with embedded image and font data\n\n")
    header.write("#ifndef ASSETS_H\n#define ASSETS_H\n\n")

    for file in files:
        filename = os.path.basename(file)
        symbol_base = sanitize_symbol(filename)

        header.write(f"// {filename}\n")
        header.write(f"extern unsigned char {symbol_base}[];\n")
        header.write(f"extern unsigned int {symbol_base}_len;\n\n")

    header.write("#endif // ASSETS_H\n")

with open(source_path, "w") as source:
    source.write("// Auto-generated source file with embedded image and font data\n\n")

    for file in files:
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
