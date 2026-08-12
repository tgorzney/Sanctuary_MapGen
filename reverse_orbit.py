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

# The bad pivot I used
cx = 512.0
cz = 512.0

# +90 degrees on Y axis to reverse the orientation rotation
y2 = math.sin(math.radians(45))
w2 = math.cos(math.radians(45))
rot_q = {'x': 0.0, 'y': y2, 'z': 0.0, 'w': w2}

def reverse_rotate_transform(transform, is_spawn=False):
    if 'position' in transform:
        pos = transform['position']
        x, y, z = pos['x'], pos['y'], pos['z']
        
        # Reverse Orbital rotation +90 degrees around center
        new_x = cx - (z - cz)
        new_z = cz + (x - cx)
        
        transform['position']['x'] = new_x
        transform['position']['z'] = new_z
        
    if 'rotation' in transform:
        if not is_spawn:
            old_rot = transform['rotation']
            new_rot = multiply_quaternion(old_rot, rot_q)
            transform['rotation'] = new_rot

if 'markers' in data:
    for marker_type, marker_data in data['markers'].items():
        transforms = marker_data.get('transforms', {})
        for key, transform in transforms.items():
            reverse_rotate_transform(transform, is_spawn=(marker_type == 'Spawn'))

if 'mapdef' in data and 'props' in data['mapdef']:
    for prop in data['mapdef']['props']:
        reverse_rotate_transform(prop, is_spawn=False)

if 'units' in data:
    for key, unit in data['units'].items():
        reverse_rotate_transform(unit, is_spawn=False)

with open(map_path, 'w', encoding='utf-8') as f:
    json.dump(data, f, indent=4)
print("Reversed the bad 512-pivot rotation.")
