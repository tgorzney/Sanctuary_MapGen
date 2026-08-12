import json

path = r"D:\Projects\Sanctuary\Sanctuary Maps\Map Editor\map-editor v0.16\SanctuaryMapEditor_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
with open(path, 'r', encoding='utf-8') as f:
    data = json.load(f)

for key, value in data.items():
    if isinstance(value, int) and not isinstance(value, bool):
        print(f"{key}: {value}")
