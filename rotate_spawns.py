import json
import math

def multiply_quaternion(q1, q2):
    w1, x1, y1, z1 = q1['w'], q1['x'], q1['y'], q1['z']
    w2, x2, y2, z2 = q2['w'], q2['x'], q2['y'], q2['z']
    
    return {
        'w': w1*w2 - x1*x2 - y1*y2 - z1*z2,
        'x': x1*w2 + w1*x2 - z1*y2 + y1*z2,
        'y': y1*w2 + z1*x2 + w1*y2 - x1*z2,
        'z': z1*w2 - y1*x2 + x1*y2 + w1*z2
    }

map_path = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"

with open(map_path, 'r', encoding='utf-8') as f:
    data = json.load(f)

# -90 degrees on Y axis (left-handed)
# half angle = -45 deg
y2 = math.sin(math.radians(-45))
w2 = math.cos(math.radians(-45))
rot_q = {'x': 0, 'y': y2, 'z': 0, 'w': w2}

if 'markers' in data and 'Spawn' in data['markers']:
    spawns = data['markers']['Spawn'].get('transforms', {})
    for key, transform in spawns.items():
        if 'rotation' in transform:
            old_rot = transform['rotation']
            new_rot = multiply_quaternion(old_rot, rot_q)
            transform['rotation'] = new_rot
            print(f"Rotated spawn {key}: {old_rot} -> {new_rot}")

with open(map_path, 'w', encoding='utf-8') as f:
    json.dump(data, f, indent=4)
print("Finished rotating spawns in map file.")
