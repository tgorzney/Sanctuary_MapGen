import json
with open(r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap", 'r', encoding='utf-8') as f:
    data = json.load(f)
    if 'markers' in data:
        print("Pandemonium marker keys:", data['markers'].keys())
        if 'Spawn' in data['markers']:
            print("Spawn markers:", len(data['markers']['Spawn'].get('transforms', {})))
