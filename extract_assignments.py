import re

with open(r"D:\Projects\Sanctuary\Map Generator\core\MapImporter.cpp", 'r', encoding='utf-8') as f:
    lines = f.readlines()

for i, line in enumerate(lines):
    if "mapdef[" in line and "=" in line:
        print(f"Line {i+1}: {line.strip()}")
