import os

# Update SupComImporter.cpp
with open("core/SupComImporter.cpp", "r", encoding="utf-8") as f:
    content = f.read()

content = content.replace("outParams.MarkersList.clear();\n    outParams.StaticPropsList.clear();", 
                          "outParams.MarkersList.clear();\n    outParams.StaticPropsList.clear();\n    PlacedMarkerLayer importedLayer;\n    importedLayer.Name = \"Imported SupCom Markers\";")
content = content.replace("outParams.MarkersList[mt.CustomName] = mt;", 
                          "outParams.MarkersList[mt.CustomName] = mt;\n                    importedLayer.MarkerKeys.push_back(mt.CustomName);")
content = content.replace("outDebugLog += \"Successfully imported \" + std::to_string(outParams.MarkersList.size()) + \" markers from Lua.\\n\";", 
                          "outParams.PlacedMarkerLayers.clear();\n    outParams.PlacedMarkerLayers.push_back(importedLayer);\n    outDebugLog += \"Successfully imported \" + std::to_string(outParams.MarkersList.size()) + \" markers from Lua.\\n\";")

with open("core/SupComImporter.cpp", "w", encoding="utf-8") as f:
    f.write(content)


# Update MapImporter.cpp
with open("core/MapImporter.cpp", "r", encoding="utf-8") as f:
    content2 = f.read()

content2 = content2.replace("outParams.ProceduralMarkerLayers.clear();", 
                            "outParams.ProceduralMarkerLayers.clear();\n    PlacedMarkerLayer importedSanmapLayer;\n    importedSanmapLayer.Name = \"Imported Markers\";")
content2 = content2.replace("outParams.MarkersList[transformName] = mt;", 
                            "outParams.MarkersList[transformName] = mt;\n                        importedSanmapLayer.MarkerKeys.push_back(transformName);")
content2 = content2.replace("if (mapdef.contains(\"trees\") && mapdef[\"trees\"].is_array()) {", 
                            "outParams.PlacedMarkerLayers.clear();\n    outParams.PlacedMarkerLayers.push_back(importedSanmapLayer);\n\n    if (mapdef.contains(\"trees\") && mapdef[\"trees\"].is_array()) {")

with open("core/MapImporter.cpp", "w", encoding="utf-8") as f:
    f.write(content2)
