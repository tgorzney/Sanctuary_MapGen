import json

path = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
with open(path, 'r', encoding='utf-8') as f:
    data = json.load(f)

def check_dict(d, prefix=""):
    for key, value in d.items():
        if value is None:
            print(f"NULL {prefix}{key}")
        elif isinstance(value, str):
            print(f"STRING {prefix}{key}: {value}")
        elif isinstance(value, dict):
            check_dict(value, prefix + key + ".")
        elif isinstance(value, list):
            check_list(value, prefix + key)
            
def check_list(l, prefix=""):
    for i, value in enumerate(l):
        if value is None:
            print(f"NULL {prefix}[{i}]")
        elif isinstance(value, str):
            print(f"STRING {prefix}[{i}]: {value}")
        elif isinstance(value, dict):
            check_dict(value, f"{prefix}[{i}].")
        elif isinstance(value, list):
            check_list(value, f"{prefix}[{i}]")

check_dict(data)
