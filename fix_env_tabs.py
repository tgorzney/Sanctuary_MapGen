import os

with open("gui/EnvironmentTabs.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Replace Procedural Layer Lambda
content = content.replace(
    "[&](ProceduralMarkerLayer& layer, size_t layerIdx, bool& bUpdate) {",
    "[&](ProceduralMarkerLayer& layer, size_t layerIdx, bool& bUpdate, bool& bDeleteLayer) {"
)
# Add delete layer button at the bottom of the Procedural Layer lambda (before "if (ruleToDelete >= 0)")
# Wait, it's easier to find the end of the lambda block
proc_add = """
                    ImGui::Separator();
                    if (ImGui::Button(("Delete Layer##" + std::to_string(layerIdx)).c_str(), ImVec2(-1, 25))) {
                        bDeleteLayer = true;
                    }
                },
                bNeedsMapUpdate"""
content = content.replace(
    "                },\n                bNeedsMapUpdate",
    proc_add,
    1
)

# Replace Placed Layer Lambda
content = content.replace(
    "[&](PlacedMarkerLayer& layer, size_t layerIdx, bool& bUpdate) {",
    "[&](PlacedMarkerLayer& layer, size_t layerIdx, bool& bUpdate, bool& bDeleteLayer) {"
)
placed_add = """
                    ImGui::Separator();
                    if (ImGui::Button(("Delete Layer##" + std::to_string(layerIdx)).c_str(), ImVec2(-1, 25))) {
                        bDeleteLayer = true;
                    }
                },
                bNeedsMapUpdate"""
# We have to be careful not to replace the first one again if it matched
# Actually, the replacement for the end of lambda is exactly the same string, but we can do a replace from the right or we can just replace all (max 2).
content = content.replace(
    "                },\n                bNeedsMapUpdate",
    proc_add
)

with open("gui/EnvironmentTabs.cpp", "w", encoding="utf-8") as f:
    f.write(content)
