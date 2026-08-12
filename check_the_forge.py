import json
map_path = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\The_Forge\The_Forge.sanmap"
with open(map_path, 'r', encoding='utf-8') as f:
    data = json.load(f)

if 'markers' in data:
    for marker_type in data['markers']:
        if 'Alloy' in marker_type or 'Mass' in marker_type or 'Resource' in marker_type or 'Plasma' in marker_type:
            print(f"Found marker type in The_Forge: {marker_type}")
            transforms = data['markers'][marker_type].get('transforms', {})
            keys = list(transforms.keys())
            if keys:
                print(f"  Example keys: {keys[:5]}")
