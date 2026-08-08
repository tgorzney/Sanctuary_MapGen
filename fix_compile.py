import os

# Fix Parameters.h
with open("core/Parameters.h", "r", encoding="utf-8") as f:
    content = f.read()

content = content.replace('std::string Name = "Imported";`n        LayerType Type = LayerType::Manual;', 'std::string Name = "Imported";\n        LayerType Type = LayerType::Manual;')

with open("core/Parameters.h", "w", encoding="utf-8") as f:
    f.write(content)

# Fix EnvironmentTabs.cpp
with open("gui/EnvironmentTabs.cpp", "r", encoding="utf-8") as f:
    content = f.read()

content = content.replace("bNeedsMapUpdate, &params.SelectedProceduralLayerIndex", "bNeedsMapUpdate")

# Find the end of the PlacedMarkerLayer lambda and make sure it has &params.SelectedPlacedLayerIndex
# It should be around line 430
# The call to RenderDraggableLayerList<PlacedMarkerLayer> ends with:
#                },
#                bNeedsMapUpdate

if "RenderDraggableLayerList<PlacedMarkerLayer>" in content:
    idx = content.find("RenderDraggableLayerList<PlacedMarkerLayer>")
    end_idx = content.find("bNeedsMapUpdate", idx)
    content = content[:end_idx] + "bNeedsMapUpdate, &params.SelectedPlacedLayerIndex" + content[end_idx+len("bNeedsMapUpdate"):]

with open("gui/EnvironmentTabs.cpp", "w", encoding="utf-8") as f:
    f.write(content)
