import os
import sys

# Let's inspect the actual Pandemonium Isthmus.sanmap JSON!
path = r"D:\Projects\Sanctuary\Sanctuary Maps\Map Editor\map-editor v0.16\SanctuaryMapEditor_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
if os.path.exists(path):
    with open(path, 'r') as f:
        data = f.read(2000)
    print("Found map! JSON preview:")
    print(data)
else:
    print("Map not found.")
