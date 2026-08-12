import json
with open(r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\The_Forge\The_Forge.sanmap", 'r', encoding='utf-8') as f:
    data = json.load(f)
    if 'markers' in data:
        spawns = data['markers'].get('SpawnPoint', {})
        print(f"\nMARKERS (SpawnPoint) in The Forge:")
        if 'transforms' in spawns:
            for k, v in spawns['transforms'].items():
                print(f"  {k}: {v}")
