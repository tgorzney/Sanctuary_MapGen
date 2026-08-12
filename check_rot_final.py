import json
map_path = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
try:
    with open(map_path, 'r', encoding='utf-8') as f:
        data = json.load(f)
        if 'markers' in data and 'Spawn' in data['markers'] and 'transforms' in data['markers']['Spawn']:
            spawn_keys = list(data['markers']['Spawn']['transforms'].keys())
            if len(spawn_keys) > 0:
                print(f"Spawn {spawn_keys[0]} rotation: {data['markers']['Spawn']['transforms'][spawn_keys[0]].get('rotation')}")
            else:
                print("No spawn transforms found.")
        else:
            print("No Spawn markers found in the file.")
except Exception as e:
    print(f"Error: {e}")
