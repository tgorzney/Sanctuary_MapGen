import json

path = r"D:\Projects\Sanctuary\Sanctuary Maps\Map Editor\map-editor v0.16\SanctuaryMapEditor_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
with open(path, 'r', encoding='utf-8') as f:
    data = json.load(f)

print(f"width type: {type(data.get('width'))}")
print(f"height type: {type(data.get('height'))}")
print(f"length type: {type(data.get('length'))}")
print(f"areas type: {type(data.get('areas'))}")
print(f"armies type: {type(data.get('armies'))}")

