import os

with open("gui/EnvironmentTabs.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Update ProceduralMarkerLayer
if "bNeedsMapUpdate\n            );" in content:
    content = content.replace("bNeedsMapUpdate\n            );", "bNeedsMapUpdate, nullptr, &bNeedsPreviewRender\n            );")

# Update PlacedMarkerLayer
if "bNeedsMapUpdate, &params.SelectedPlacedLayerIndex\n        );" in content:
    content = content.replace("bNeedsMapUpdate, &params.SelectedPlacedLayerIndex\n        );", "bNeedsMapUpdate, &params.SelectedPlacedLayerIndex, &bNeedsPreviewRender\n        );")

with open("gui/EnvironmentTabs.cpp", "w", encoding="utf-8") as f:
    f.write(content)

with open("gui/main.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Make Placed markers respect layer visibility
old_loop = """                for (auto& [key, marker] : params.MarkersList) {
                    if (marker.IsHidden) continue;"""

new_loop = """                std::unordered_set<std::string> hiddenMarkers;
                for (const auto& layer : params.PlacedMarkerLayers) {
                    if (!layer.Enabled) {
                        for (const auto& k : layer.MarkerKeys) hiddenMarkers.insert(k);
                    }
                }
                
                for (auto& [key, marker] : params.MarkersList) {
                    if (marker.IsHidden || hiddenMarkers.count(key)) continue;"""

content = content.replace(old_loop, new_loop)
if "#include <unordered_set>" not in content:
    content = "#include <unordered_set>\n" + content

with open("gui/main.cpp", "w", encoding="utf-8") as f:
    f.write(content)
