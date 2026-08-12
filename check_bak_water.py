import json
with open(r"E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap.bak", 'r', encoding='utf-8') as f:
    data = json.load(f)
    print("waterLevelMax:", data.get("waterLevelMax"))
    print("waterLevelMin:", data.get("waterLevelMin"))
    print("waterLevel:", data.get("waterLevel"))
    print("waterDepth:", data.get("waterDepth"))
    print("deepWaterDepthMax:", data.get("deepWaterDepthMax"))
    print("water object:", {k: v for k, v in data.items() if 'water' in k.lower()})
