import json
import math

map_path = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"

with open(map_path, 'r', encoding='utf-8') as f:
    data = json.load(f)

# Explicitly set exactly -90 degrees (Y axis)
rot_q = {'x': 0.0, 'y': -0.7071067811865476, 'z': 0.0, 'w': 0.7071067811865476}

if 'markers' in data and 'Spawn' in data['markers']:
    spawns = data['markers']['Spawn'].get('transforms', {})
    for key, transform in spawns.items():
        if 'rotation' in transform:
            transform['rotation'] = rot_q
            print(f"Set spawn {key} rotation to -90 degrees")

with open(map_path, 'w', encoding='utf-8') as f:
    json.dump(data, f, indent=4)
