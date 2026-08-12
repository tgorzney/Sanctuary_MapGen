import json
import math

def round_floats(obj, decimals=3):
    if isinstance(obj, float):
        return round(obj, decimals)
    elif isinstance(obj, dict):
        return {k: round_floats(v, decimals) for k, v in obj.items()}
    elif isinstance(obj, list):
        return [round_floats(elem, decimals) for elem in obj]
    return obj

file1 = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
file2 = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap.bak"

with open(file1, 'r', encoding='utf-8') as f1:
    data1 = round_floats(json.load(f1))
with open(file2, 'r', encoding='utf-8') as f2:
    data2 = round_floats(json.load(f2))

from deepdiff import DeepDiff
diff = DeepDiff(data2, data1, ignore_order=True)

if not diff:
    print("No structural differences found after rounding floats to 3 decimal places.")
else:
    for key, changes in diff.items():
        print(f"--- {key} ---")
        if isinstance(changes, dict):
            for path, value in changes.items():
                if "props" not in path:
                    print(f"{path}: {value}")
        else:
            print(changes)
