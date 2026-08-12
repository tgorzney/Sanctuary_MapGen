import json
from deepdiff import DeepDiff

file1 = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
file2 = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap.bak"

try:
    with open(file1, 'r', encoding='utf-8') as f1:
        data1 = json.load(f1)
    with open(file2, 'r', encoding='utf-8') as f2:
        data2 = json.load(f2)
        
    diff = DeepDiff(data2, data1, ignore_order=True)
    if not diff:
        print("The files are identical.")
    else:
        for key, changes in diff.items():
            print(f"--- {key} ---")
            if isinstance(changes, dict):
                for path, value in changes.items():
                    print(f"{path}: {value}")
            elif isinstance(changes, set):
                for value in changes:
                    print(value)
            else:
                print(changes)
except Exception as e:
    print(f"Error: {e}")
