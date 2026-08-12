import json
map_path = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
with open(map_path, 'r', encoding='utf-8') as f:
    data = json.load(f)

if 'markers' in data:
    for marker_type in data['markers']:
        if 'Alloy' in marker_type or 'Plasma' in marker_type:
            print(f"Found marker type: {marker_type}")
            transforms = data['markers'][marker_type].get('transforms', {})
            keys = list(transforms.keys())
            print(f"  Count: {len(keys)}")
            if keys:
                print(f"  Example keys: {keys[:5]}")
                print(f"  Example transform: {transforms[keys[0]]}")
