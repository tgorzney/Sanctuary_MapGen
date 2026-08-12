import re

with open(r"D:\Projects\Sanctuary\Map Generator\core\MapImporter.cpp", 'r', encoding='utf-8') as f:
    lines = f.readlines()

for i, line in enumerate(lines):
    if "mapdef" in line and ".contains(" in line:
        pass
    if "s.contains(" in line or 'contains("name"' in line or 'contains("path"' in line:
        print(f"{i+1}: {line.strip()}")
