import json
import math

file1 = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
file2 = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\The_Forge\The_Forge.sanmap"

with open(file1, 'r', encoding='utf-8') as f1:
    data1 = json.load(f1)
with open(file2, 'r', encoding='utf-8') as f2:
    data2 = json.load(f2)

def print_scale_vars(d, name):
    print(f"\n--- {name} ---")
    print("heightmapResolution:", d.get("heightmapResolution"))
    print("width:", d.get("width"))
    print("length:", d.get("length"))
    print("height:", d.get("height"))
    print("sunCookieSize:", d.get("sunCookieSize"))
    print("tileSize (stratum 0):", d.get("stratumLayers", [{}])[0].get("tileSize"))
    
print_scale_vars(data1, "Pandemonium")
print_scale_vars(data2, "The Forge")

