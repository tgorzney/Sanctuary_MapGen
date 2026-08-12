import json

file1 = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
file2 = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\The_Forge\The_Forge.sanmap"

try:
    with open(file1, 'r', encoding='utf-8') as f1:
        data1 = json.load(f1)
    with open(file2, 'r', encoding='utf-8') as f2:
        data2 = json.load(f2)
        
    keys_to_compare = ['width', 'length', 'heightmapResolution', 'height', 'areas', 'mapVersion', 'fileVersion']
    
    print("--- Pandemonium Isthmus ---")
    for k in keys_to_compare:
        print(f"{k}: {data1.get(k)}")
        
    print("\n--- The Forge ---")
    for k in keys_to_compare:
        print(f"{k}: {data2.get(k)}")
        
except Exception as e:
    print(f"Error: {e}")
