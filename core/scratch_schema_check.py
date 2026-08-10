import json
import glob
import os

maps_dir = r'E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps'
map_files = glob.glob(os.path.join(maps_dir, '*', '*.sanmap'))

stats = {
    'total_maps': 0,
    'has_armies': 0,
    'armies_format': set(),
    'marker_keys': {},
    'root_keys': set(),
    'props_has_empty_transforms': False
}

for mf in map_files:
    stats['total_maps'] += 1
    with open(mf, 'r', encoding='utf-8') as f:
        try:
            d = json.load(f)
        except:
            continue
            
    for k in d.keys():
        stats['root_keys'].add(k)
        
    armies = d.get('armies')
    if armies:
        stats['has_armies'] += 1
        if isinstance(armies, dict):
            for k, v in armies.items():
                stats['armies_format'].add(f"Dict key: {k.split('_')[0]}")
                
    markers = d.get('markers', {})
    for mtype, mdata in markers.items():
        if mtype not in stats['marker_keys']:
            stats['marker_keys'][mtype] = set()
        for k in mdata.get('transforms', {}).keys():
            stats['marker_keys'][mtype].add(k.split(' ')[0])

    for p in d.get('props', []):
        if len(p.get('transforms', [])) == 0:
            stats['props_has_empty_transforms'] = True

print(f"Total maps: {stats['total_maps']}")
print(f"Maps with populated armies: {stats['has_armies']}")
print(f"Armies format examples: {stats['armies_format']}")
print(f"Root keys across all maps: {sorted(list(stats['root_keys']))}")
print(f"Props empty transforms allowed: {stats['props_has_empty_transforms']}")
for k, v in stats['marker_keys'].items():
    print(f"Marker type {k} prefixes: {sorted(list(v))[:5]}")
