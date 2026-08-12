import json

path = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
with open(path, 'r', encoding='utf-8') as f:
    data = json.load(f)

for s in data.get("stratumLayers", []):
    print("Stratum:")
    for k, v in s.items():
        print(f"  {k}: {v} ({type(v)})")
