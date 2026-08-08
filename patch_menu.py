import os

with open("gui/main.cpp", "r", encoding="utf-8") as f:
    content = f.read()

old_menu = """                if (ImGui::BeginPopup("AddMarkerMenu")) {
                    if (ImGui::BeginMenu("Add Marker")) {
                        if (ImGui::MenuItem("Alloy")) {
                            std::string newKey = "Alloys_" + std::to_string(params.MarkersList.size() + 1);
                            SanmapGen::MarkerTransform m;
                            m.Type = "Alloy";
                            m.IsManual = true;
                            float screenU_clk = (mousePos.x - p0.x) / renderSize;
                            float screenV_clk = (mousePos.y - p0.y) / renderSize;
                            m.Position[0] = (uv0.x + screenU_clk * (uv1.x - uv0.x)) * params.MapSize;
                            m.Position[2] = (uv0.y + screenV_clk * (uv1.y - uv0.y)) * params.MapSize;
                            m.Scale[0] = 1.0f; m.Scale[1] = 1.0f; m.Scale[2] = 1.0f;
                            params.MarkersList[newKey] = m;
                        }
                        if (ImGui::MenuItem("Plasma")) {
                            std::string newKey = "Plasmas_" + std::to_string(params.MarkersList.size() + 1);
                            SanmapGen::MarkerTransform m;
                            m.Type = "Plasma";
                            m.IsManual = true;
                            float screenU_clk = (mousePos.x - p0.x) / renderSize;
                            float screenV_clk = (mousePos.y - p0.y) / renderSize;
                            m.Position[0] = (uv0.x + screenU_clk * (uv1.x - uv0.x)) * params.MapSize;
                            m.Position[2] = (uv0.y + screenV_clk * (uv1.y - uv0.y)) * params.MapSize;
                            m.Scale[0] = 1.0f; m.Scale[1] = 1.0f; m.Scale[2] = 1.0f;
                            params.MarkersList[newKey] = m;
                        }
                        if (ImGui::MenuItem("Spawn")) {
                            std::string newKey = "Spawns_" + std::to_string(params.MarkersList.size() + 1);
                            SanmapGen::MarkerTransform m;
                            m.Type = "Spawn";
                            m.IsManual = true;
                            float screenU_clk = (mousePos.x - p0.x) / renderSize;
                            float screenV_clk = (mousePos.y - p0.y) / renderSize;
                            m.Position[0] = (uv0.x + screenU_clk * (uv1.x - uv0.x)) * params.MapSize;
                            m.Position[2] = (uv0.y + screenV_clk * (uv1.y - uv0.y)) * params.MapSize;
                            m.Scale[0] = 1.0f; m.Scale[1] = 1.0f; m.Scale[2] = 1.0f;
                            params.MarkersList[newKey] = m;
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndPopup();
                }"""

new_menu = """                if (ImGui::BeginPopup("AddMarkerMenu")) {
                    int selIdx = params.SelectedPlacedLayerIndex;
                    if (selIdx >= 0 && selIdx < (int)params.PlacedMarkerLayers.size()) {
                        auto& layer = params.PlacedMarkerLayers[selIdx];
                        if (layer.Type == SanmapGen::LayerType::Manual) {
                            if (ImGui::BeginMenu("Add Marker to Selected Layer")) {
                                auto placeMarker = [&](const std::string& type, const std::string& prefix) {
                                    std::string newKey = prefix + "_" + std::to_string(params.MarkersList.size() + 1);
                                    SanmapGen::MarkerTransform m;
                                    m.Type = type;
                                    m.IsManual = true;
                                    float screenU_clk = (mousePos.x - p0.x) / renderSize;
                                    float screenV_clk = (mousePos.y - p0.y) / renderSize;
                                    m.Position[0] = (uv0.x + screenU_clk * (uv1.x - uv0.x)) * params.MapSize;
                                    m.Position[2] = (uv0.y + screenV_clk * (uv1.y - uv0.y)) * params.MapSize;
                                    m.Scale[0] = 1.0f; m.Scale[1] = 1.0f; m.Scale[2] = 1.0f;
                                    params.MarkersList[newKey] = m;
                                    layer.MarkerKeys.push_back(newKey);
                                    bNeedsMapUpdate = true;
                                    bNeedsPreviewRender = true;
                                };
                                
                                if (ImGui::MenuItem("Alloy")) placeMarker("Alloy", "Alloys");
                                if (ImGui::MenuItem("Plasma")) placeMarker("Plasma", "Plasmas");
                                if (ImGui::MenuItem("Spawn")) placeMarker("Spawn", "Spawns");
                                ImGui::EndMenu();
                            }
                        } else if (layer.Type == SanmapGen::LayerType::Fixed) {
                            ImGui::TextDisabled("Selected layer is Fixed (Imported).");
                            ImGui::TextDisabled("Cannot manually place markers here.");
                        }
                    } else {
                        ImGui::TextDisabled("No Placed Marker Layer selected.");
                        ImGui::TextDisabled("Select a Manual layer in the UI first.");
                    }
                    ImGui::EndPopup();
                }"""

if old_menu in content:
    content = content.replace(old_menu, new_menu)
else:
    print("WARNING: Could not find old menu in main.cpp")

with open("gui/main.cpp", "w", encoding="utf-8") as f:
    f.write(content)
