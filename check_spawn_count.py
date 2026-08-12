import json
with open(r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap", 'r', encoding='utf-8') as f:
    data = json.load(f)
    print("spawnPointCount:", data.get("spawnPointCount"))
