import json
with open(r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\The_Forge\The_Forge.sanmap", 'r', encoding='utf-8') as f:
    data = json.load(f)
    if 'markers' in data:
        print(data['markers'].keys())
