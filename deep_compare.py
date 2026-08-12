import json

file1 = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
file2 = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\The_Forge\The_Forge.sanmap"

with open(file1, 'r', encoding='utf-8') as f1:
    data1 = json.load(f1)
with open(file2, 'r', encoding='utf-8') as f2:
    data2 = json.load(f2)

def print_type_and_structure(d, path=""):
    if isinstance(d, dict):
        return {k: print_type_and_structure(v, f"{path}.{k}") for k, v in d.items()}
    elif isinstance(d, list):
        if len(d) > 0:
            return [print_type_and_structure(d[0], f"{path}[0]")]
        else:
            return []
    else:
        return type(d).__name__

def extract_keys(d):
    return set(d.keys())

print("--- PANDEMONIUM ---")
print("ROOT KEYS:", extract_keys(data1))
print("WATER KEYS:", {k: type(v).__name__ for k, v in data1.items() if 'water' in k.lower()})
print("MARKERS TYPE:", type(data1.get('markers')).__name__)
if 'markers' in data1 and isinstance(data1['markers'], dict):
    print("MARKER KEYS:", data1['markers'].keys())
    if 'Spawn' in data1['markers']:
        print("Spawn item structure:", print_type_and_structure(data1['markers']['Spawn']))
print("PROPS TYPE:", type(data1.get('props')).__name__)
if 'props' in data1 and isinstance(data1['props'], list) and len(data1['props']) > 0:
    print("Props[0] structure:", print_type_and_structure(data1['props'][0]))


print("\n--- THE FORGE ---")
print("ROOT KEYS:", extract_keys(data2))
print("WATER KEYS:", {k: type(v).__name__ for k, v in data2.items() if 'water' in k.lower()})
print("MARKERS TYPE:", type(data2.get('markers')).__name__)
if 'markers' in data2 and isinstance(data2['markers'], dict):
    print("MARKER KEYS:", data2['markers'].keys())
    if 'Spawn' in data2['markers']:
        print("Spawn item structure:", print_type_and_structure(data2['markers']['Spawn']))
print("PROPS TYPE:", type(data2.get('props')).__name__)
if 'props' in data2 and isinstance(data2['props'], list) and len(data2['props']) > 0:
    print("Props[0] structure:", print_type_and_structure(data2['props'][0]))

