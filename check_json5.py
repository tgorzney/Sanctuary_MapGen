import json

path = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
with open(path, 'r', encoding='utf-8') as f:
    data = json.load(f)

def check_dict(d, prefix=""):
    for key, value in d.items():
        if isinstance(value, int) and not isinstance(value, bool):
            print(f"INT {prefix}{key}: {value}")
        elif isinstance(value, float):
            print(f"FLOAT {prefix}{key}: {value}")
        elif isinstance(value, dict):
            check_dict(value, prefix + key + ".")
        elif isinstance(value, list):
            check_list(value, prefix + key)
            
def check_list(l, prefix=""):
    for i, value in enumerate(l):
        if isinstance(value, int) and not isinstance(value, bool):
            print(f"INT {prefix}[{i}]: {value}")
        elif isinstance(value, float):
            print(f"FLOAT {prefix}[{i}]: {value}")
        elif isinstance(value, dict):
            check_dict(value, f"{prefix}[{i}].")
        elif isinstance(value, list):
            check_list(value, f"{prefix}[{i}]")

check_dict(data)
