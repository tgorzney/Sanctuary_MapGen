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

# Get map size
map_size = 1024.0
if 'mapdef' in data and 'mapDimensions' in data['mapdef']:
    map_size = float(data['mapdef']['mapDimensions']['x'])

cx = map_size / 2.0
cz = map_size / 2.0

# -90 degrees on Y axis (left-handed)
y2 = math.sin(math.radians(-45))
w2 = math.cos(math.radians(-45))
rot_q = {'x': 0.0, 'y': y2, 'z': 0.0, 'w': w2}

def rotate_transform(transform, is_spawn=False):
    if 'position' in transform:
        pos = transform['position']
        x, y, z = pos['x'], pos['y'], pos['z']
        
        # Orbital rotation -90 degrees around center
        # (x, z) -> (y, -x) around origin
        # new_x = cx + (z - cz)
        # new_z = cz - (x - cx)
        new_x = cx + (z - cz)
        new_z = cz - (x - cx)
        
        transform['position']['x'] = new_x
        transform['position']['z'] = new_z
        
    if 'rotation' in transform:
        if is_spawn:
            # I previously hardcoded spawns to -90, so they are already oriented correctly.
            # No need to multiply again.
            pass
        else:
            old_rot = transform['rotation']
            new_rot = multiply_quaternion(old_rot, rot_q)
            transform['rotation'] = new_rot

# Rotate all markers
if 'markers' in data:
    for marker_type, marker_data in data['markers'].items():
        transforms = marker_data.get('transforms', {})
        for key, transform in transforms.items():
            rotate_transform(transform, is_spawn=(marker_type == 'Spawn'))

# Rotate all props
if 'mapdef' in data and 'props' in data['mapdef']:
    for prop in data['mapdef']['props']:
        rotate_transform(prop, is_spawn=False)

# Rotate all units (if any)
if 'units' in data:
    for key, unit in data['units'].items():
        rotate_transform(unit, is_spawn=False)

with open(map_path, 'w', encoding='utf-8') as f:
    json.dump(data, f, indent=4)
print("Finished orbital -90 degree rotation of all map elements.")
