import json

file2 = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\The_Forge\The_Forge.sanmap"

with open(file2, 'r', encoding='utf-8') as f2:
    data2 = json.load(f2)

print("THE FORGE AREAS:", data2.get("areas"))
