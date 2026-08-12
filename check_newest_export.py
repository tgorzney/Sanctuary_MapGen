import json
with open(r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap", 'r', encoding='utf-8') as f:
    data = json.load(f)
    print("WATER:")
    for k, v in data.items():
        if 'water' in k.lower():
            print(f"  {k}: {v}")
            
    print("\nSPAWNS:")
    for k, v in data.items():
        if 'spawn' in k.lower():
            print(f"  {k}: {v}")
    
    # Let's also check the markers to see if spawns are in there
    if 'markers' in data:
        spawns = data['markers'].get('SpawnPoint', {})
        print(f"\nMARKERS (SpawnPoint):")
        if 'transforms' in spawns:
            for k, v in spawns['transforms'].items():
                print(f"  {k}: {v}")
