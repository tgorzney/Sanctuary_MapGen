import json
map_path = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
with open(map_path, 'r', encoding='utf-8') as f:
    data = json.load(f)
print(f"Width: {data.get('width')} Length: {data.get('length')}")
