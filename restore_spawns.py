import json

map_path = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
with open(map_path, 'r', encoding='utf-8') as f:
    data = json.load(f)

# Restore all spawn rotations to pure 0 degrees (w=1, x=0, y=0, z=0)
rot_q = {'x': 0.0, 'y': 0.0, 'z': 0.0, 'w': 1.0}
if 'markers' in data and 'Spawn' in data['markers']:
    spawns = data['markers']['Spawn'].get('transforms', {})
    for key, transform in spawns.items():
        if 'rotation' in transform:
            transform['rotation'] = rot_q

with open(map_path, 'w', encoding='utf-8') as f:
    json.dump(data, f, indent=4)
