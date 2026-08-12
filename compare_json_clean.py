import json
from deepdiff import DeepDiff

file1 = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
file2 = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap.bak"

with open(file1, 'r', encoding='utf-8') as f1:
    data1 = json.load(f1)
with open(file2, 'r', encoding='utf-8') as f2:
    data2 = json.load(f2)
    
diff = DeepDiff(data2, data1, ignore_order=True, significant_digits=4)

with open(r"D:\Projects\Sanctuary\Map Generator\compare_summary.txt", "w") as out:
    if not diff:
        out.write("The files are structurally identical (ignoring minor float precision).")
    else:
        for key, changes in diff.items():
            out.write(f"--- {key} ---\n")
            if isinstance(changes, dict):
                for path, value in changes.items():
                    out.write(f"{path}: {value}\n")
            elif isinstance(changes, set):
                for value in changes:
                    out.write(f"{value}\n")
            else:
                out.write(f"{changes}\n")
