import json
try:
    with open(r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap", 'r', encoding='utf-8') as f:
        data = json.load(f)
        print("JSON is valid.")
        
        print("\nWater values:")
        for k, v in data.items():
            if 'water' in k.lower():
                print(f"  {k}: {v}")
                
        print("\nMarkers (Spawn):", len(data.get('markers', {}).get('Spawn', {}).get('transforms', {})))
        print("Props count:", len(data.get('props', [])))
except Exception as e:
    print(f"Error: {e}")
