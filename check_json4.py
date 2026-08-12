import json

path = r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
with open(path, 'r', encoding='utf-8') as f:
    data = json.load(f)

for key, value in data.items():
    if isinstance(value, int) and not isinstance(value, bool):
        print(f"INT {key}: {value}")
    elif isinstance(value, float):
        print(f"FLOAT {key}: {value}")
    elif isinstance(value, list):
        print(f"LIST {key}: len {len(value)}")
    elif isinstance(value, dict):
        print(f"DICT {key}: len {len(value)}")
    elif isinstance(value, bool):
        print(f"BOOL {key}: {value}")
    else:
        print(f"OTHER {key}: {type(value)}")

