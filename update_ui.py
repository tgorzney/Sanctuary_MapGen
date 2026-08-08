import os

# We will completely replace the Procedural and Placed Marker section
# of EnvironmentTabs.cpp

with open("gui/EnvironmentTabs.cpp", "r", encoding="utf-8") as f:
    content = f.read()

# Find the start of Procedural Marker block
start_idx = content.find('ImGui::Text("Procedural Marker Generation (Masks & Rules)");')
# Find the start of Props block (end of our target section)
end_idx = content.find('void RenderPropsTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {')

if start_idx == -1 or end_idx == -1:
    print("Could not find blocks in EnvironmentTabs.cpp")
    exit(1)

new_ui_code = """ImGui::Text("Procedural Marker Generation (Masks & Rules)");
        ImGui::Separator();
        if (params.EnableProceduralMarkers) {
            if (ImGui::Button("Add Procedural Layer", ImVec2(-1, 30))) {
                ProceduralMarkerLayer newLayer;
                newLayer.Name = "Procedural Layer " + std::to_string(params.ProceduralMarkerLayers.size() + 1);
                params.ProceduralMarkerLayers.push_back(newLayer);
                bNeedsMapUpdate = true;
            }
            ImGui::Spacing();

            RenderDraggableLayerList<ProceduralMarkerLayer>(
                "PROCEDURAL_LAYER_DRAG",
                params.ProceduralMarkerLayers,
                [&](ProceduralMarkerLayer& layer, size_t layerIdx, bool& bUpdate) {
                    if (ImGui::Button(("Add Rule##" + std::to_string(layerIdx)).c_str(), ImVec2(-1, 25))) {
                        MarkerRule rule;
                        rule.Name = "New Rule " + std::to_string(layer.Rules.size());
                        layer.Rules.push_back(rule);
                        bNeedsPreviewRender = true;
                        bUpdate = true;
                    }
                    ImGui::Spacing();

                    int ruleToDelete = -1;
                    for (int i = 0; i < (int)layer.Rules.size(); ++i) {
                        ImGui::PushID((int)layerIdx * 1000 + i + 5000);
                        auto& rule = layer.Rules[i];
                        char label[128];
                        snprintf(label, sizeof(label), "Rule: %s", rule.Name.c_str());
                        if (ImGui::CollapsingHeader(label)) {
                            char nameBuf[128];
                            strncpy(nameBuf, rule.Name.c_str(), sizeof(nameBuf));
                            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
                                rule.Name = nameBuf;
                            }
                            if (ImGui::Checkbox("Enabled", &rule.Enabled)) {
                                bUpdate = true;
                                bNeedsPreviewRender = true;
                            }
                            
                            // Type Selection
                            if (!params.KnownMarkerTypes.empty()) {
                                int currentType = -1;
                                for (int j = 0; j < (int)params.KnownMarkerTypes.size(); ++j) {
                                    if (params.KnownMarkerTypes[j] == rule.Type) {
                                        currentType = j;
                                        break;
                                    }
                                }
                                std::vector<const char*> types;
                                for (const auto& t : params.KnownMarkerTypes) types.push_back(t.c_str());
                                if (ImGui::Combo("Marker Type", &currentType, types.data(), (int)types.size())) {
                                    if (currentType >= 0 && currentType < (int)params.KnownMarkerTypes.size()) {
                                        rule.Type = params.KnownMarkerTypes[currentType];
                                        bUpdate = true;
                                    }
                                }
                            }

                            if (ImGui::ColorEdit4("Base Color Override", rule.Color)) bNeedsPreviewRender = true;
                            
                            ImGui::Text("Placement Logic");
                            ImGui::Separator();
                            if (ImGui::SliderInt("Count", &rule.Count, 1, 1000)) bUpdate = true;
                            if (ImGui::Checkbox("Use All Positions", &rule.UseAllPositions)) bUpdate = true;
                            if (ImGui::Checkbox("Use Density", &rule.UseDensity)) bUpdate = true;
                            if (rule.UseDensity) {
                                if (ImGui::SliderFloat("Density", &rule.Density, 0.0f, 1.0f)) bUpdate = true;
                            }
                            if (ImGui::Checkbox("Random Selection", &rule.RandomSelection)) bUpdate = true;
                            
                            ImGui::Text("Spatial & Tolerance");
                            if (ImGui::SliderFloat("Min Height", &rule.MinHeight, 0.0f, 128.0f)) bNeedsPreviewRender = true;
                            if (ImGui::SliderFloat("Max Height", &rule.MaxHeight, 0.0f, 128.0f)) bNeedsPreviewRender = true;
                            if (ImGui::SliderFloat("Min Slope", &rule.MinSlope, 0.0f, 90.0f)) bNeedsPreviewRender = true;
                            if (ImGui::SliderFloat("Max Slope", &rule.MaxSlope, 0.0f, 90.0f)) bNeedsPreviewRender = true;
                            
                            if (ImGui::SliderFloat("Area Radius Min", &rule.AreaRadiusMin, 1.0f, 200.0f)) bUpdate = true;
                            if (ImGui::Checkbox("Check Max Radius", &rule.CheckMaxRadius)) bUpdate = true;
                            if (rule.CheckMaxRadius) {
                                if (ImGui::SliderFloat("Area Radius Max", &rule.AreaRadiusMax, 1.0f, 500.0f)) bUpdate = true;
                            }
                            if (ImGui::SliderFloat("Area Height Range", &rule.AreaHeightRange, 0.1f, 10.0f)) bUpdate = true;
                            if (ImGui::SliderFloat("Clearance Spacing", &rule.ClearanceSpacing, 0.0f, 500.0f)) bUpdate = true;
                            
                            ImGui::Text("Avoidance & Gradients");
                            if (ImGui::SliderFloat("Map Edge Padding", &rule.MapEdgePadding, 0.0f, 200.0f)) bUpdate = true;
                            
                            const char* gradTypes[] = { "None", "Center Focus", "Edge Focus", "Torus" };
                            int gradIdx = rule.FocusGradient;
                            if (ImGui::Combo("Focus Gradient", &gradIdx, gradTypes, 4)) {
                                rule.FocusGradient = gradIdx;
                                bUpdate = true;
                            }
                            if (rule.FocusGradient != Gradient_None) {
                                if (ImGui::SliderFloat("Gradient Radius", &rule.FocusGradientRadius, 0.1f, 1000.0f)) bUpdate = true;
                                if (ImGui::SliderFloat("Gradient Strength", &rule.FocusGradientStrength, 0.1f, 5.0f)) bUpdate = true;
                                if (ImGui::SliderFloat("Gradient Contrast", &rule.FocusGradientContrast, 0.1f, 5.0f)) bUpdate = true;
                            }
                            
                            if (ImGui::Button("Delete Rule", ImVec2(-1, 25))) {
                                ruleToDelete = i;
                            }
                        }
                        ImGui::PopID();
                    }
                    if (ruleToDelete >= 0) {
                        layer.Rules.erase(layer.Rules.begin() + ruleToDelete);
                        bUpdate = true;
                        bNeedsPreviewRender = true;
                    }
                },
                bNeedsMapUpdate
            );
        } else {
            ImGui::TextDisabled("Procedural marker generation is globally disabled.");
            ImGui::TextDisabled("Check 'Enable Procedural Markers' at the top to edit rules.");
        }

        ImGui::Spacing();
        ImGui::Text("Placed Markers");
        ImGui::Separator();
        
        if (ImGui::Button("Add Placed Marker Layer", ImVec2(-1, 30))) {
            PlacedMarkerLayer newLayer;
            newLayer.Name = "Placed Markers " + std::to_string(params.PlacedMarkerLayers.size() + 1);
            params.PlacedMarkerLayers.push_back(newLayer);
            bNeedsMapUpdate = true;
        }
        ImGui::Spacing();

        std::string keyToDelete = "";
        
        auto triggerSymmetryDeltaUpdate = [&](const MarkerTransform& changedMarker) {
            if (changedMarker.SymmetryId == 0) return;
            for (auto& [symKey, symMarker] : params.MarkersList) {
                if (symMarker.SymmetryId == changedMarker.SymmetryId && &symMarker != &changedMarker) {
                    symMarker.Type = changedMarker.Type;
                    symMarker.Color[0] = changedMarker.Color[0]; symMarker.Color[1] = changedMarker.Color[1];
                    symMarker.Color[2] = changedMarker.Color[2]; symMarker.Color[3] = changedMarker.Color[3];
                    symMarker.IconOverride = changedMarker.IconOverride;
                    symMarker.SymmetryMask = changedMarker.SymmetryMask;
                    symMarker.SymmetryUseGlobal = changedMarker.SymmetryUseGlobal;
                    
                    // Duplicate Delta Translations
                    float mapSize = static_cast<float>(params.MapSize);
                    int mask = changedMarker.SymmetryUseGlobal ? params.GlobalSymmetryMask : changedMarker.SymmetryMask;
                    
                    if (params.SnapImperfectSymmetry) {
                        if (mask & Symmetry_Point) {
                            symMarker.Position[0] = mapSize - changedMarker.Position[0];
                            symMarker.Position[2] = mapSize - changedMarker.Position[2];
                        } else if (mask & Symmetry_X) {
                            symMarker.Position[0] = mapSize - changedMarker.Position[0];
                            symMarker.Position[2] = changedMarker.Position[2];
                        } else if (mask & Symmetry_Z) {
                            symMarker.Position[0] = changedMarker.Position[0];
                            symMarker.Position[2] = mapSize - changedMarker.Position[2];
                        }
                        symMarker.Position[1] = changedMarker.Position[1];
                    }
                }
            }
        };

        RenderDraggableLayerList<PlacedMarkerLayer>(
            "PLACED_LAYER_DRAG",
            params.PlacedMarkerLayers,
            [&](PlacedMarkerLayer& layer, size_t layerIdx, bool& bUpdate) {
                if (ImGui::Button(("Add Marker##" + std::to_string(layerIdx)).c_str(), ImVec2(-1, 25))) {
                    SanmapGen::MarkerTransform newMarker;
                    newMarker.Type = "Spawn";
                    newMarker.CustomName = "New_Marker_" + std::to_string(params.MarkersList.size() + 1);
                    newMarker.IsManual = true;
                    params.MarkersList[newMarker.CustomName] = newMarker;
                    layer.MarkerKeys.push_back(newMarker.CustomName);
                    bUpdate = true;
                    bNeedsPreviewRender = true;
                }
                ImGui::Spacing();
                
                std::map<std::string, std::vector<std::string>> groupedMarkers;
                for (const auto& key : layer.MarkerKeys) {
                    if (params.MarkersList.count(key)) {
                        groupedMarkers[params.MarkersList[key].Type].push_back(key);
                    }
                }
                
                for (auto& groupPair : groupedMarkers) {
                    if (ImGui::CollapsingHeader((groupPair.first + " (" + std::to_string(groupPair.second.size()) + ")##" + std::to_string(layerIdx)).c_str())) {
                        ImGui::Indent();
                        for (const auto& key : groupPair.second) {
                            auto& marker = params.MarkersList[key];
                            
                            ImGui::PushID(key.c_str());
                            char label[128];
                            snprintf(label, sizeof(label), "%s%s", marker.CustomName.empty() ? key.c_str() : marker.CustomName.c_str(), marker.SymmetryId != 0 ? (" [Sym " + std::to_string(marker.SymmetryId) + "]").c_str() : "");
                            
                            if (ImGui::CollapsingHeader(label)) {
                                bool localUpdate = false;
                                char nameBuf[128]; 
                                strncpy(nameBuf, marker.CustomName.c_str(), sizeof(nameBuf));
                                if (ImGui::InputText("Name/ID", nameBuf, sizeof(nameBuf))) {
                                    marker.CustomName = nameBuf;
                                    localUpdate = true;
                                }
                                
                                if (ImGui::DragFloat3("Position (X,Y,Z)", marker.Position, 1.0f, 0.0f, 4096.0f)) localUpdate = true;
                                if (ImGui::ColorEdit4("Color Override", marker.Color)) localUpdate = true;
                                
                                std::string iconToDisplay = marker.IconOverride.empty() ? marker.Type : marker.IconOverride;
                                RenderIconPicker("MarkerIcon", iconToDisplay, params, bNeedsPreviewRender);
                                if (iconToDisplay != marker.Type) { marker.IconOverride = iconToDisplay; localUpdate = true; }
                                else if (!marker.IconOverride.empty()) { marker.IconOverride = ""; localUpdate = true; }
                                
                                ImGui::Spacing();
                                ImGui::Text("Symmetry Settings");
                                if (ImGui::Checkbox("Use Global Symmetry", &marker.SymmetryUseGlobal)) localUpdate = true;
                                if (!marker.SymmetryUseGlobal) {
                                    bool symPoint  = (marker.SymmetryMask & Symmetry_Point);
                                    bool symX      = (marker.SymmetryMask & Symmetry_X);
                                    bool symZ      = (marker.SymmetryMask & Symmetry_Z);
                                    bool symXY     = (marker.SymmetryMask & Symmetry_XY);
                                    bool symRadial = (marker.SymmetryMask & Symmetry_Radial);
                                    
                                    if (ImGui::Checkbox("Point (Origin)", &symPoint)) localUpdate = true;
                                    if (ImGui::Checkbox("X-Axis (Left/Right)", &symX)) localUpdate = true;
                                    if (ImGui::Checkbox("Z-Axis (Top/Bottom)", &symZ)) localUpdate = true;
                                    if (ImGui::Checkbox("XY-Axis (Diagonal)", &symXY)) localUpdate = true;
                                    if (ImGui::Checkbox("Radial", &symRadial)) localUpdate = true;
                                    
                                    int newMask = 0;
                                    if (symPoint)  newMask |= Symmetry_Point;
                                    if (symX)      newMask |= Symmetry_X;
                                    if (symZ)      newMask |= Symmetry_Z;
                                    if (symXY)     newMask |= Symmetry_XY;
                                    if (symRadial) newMask |= Symmetry_Radial;
                                    
                                    marker.SymmetryMask = newMask;
                                }

                                if (!params.KnownMarkerTypes.empty()) {
                                    int currentType = -1;
                                    for (int j = 0; j < (int)params.KnownMarkerTypes.size(); ++j) {
                                        if (params.KnownMarkerTypes[j] == marker.Type) {
                                            currentType = j;
                                            break;
                                        }
                                    }
                                    std::vector<const char*> types;
                                    for (const auto& t : params.KnownMarkerTypes) types.push_back(t.c_str());
                                    
                                    if (ImGui::Combo("Type", &currentType, types.data(), (int)types.size())) {
                                        if (currentType >= 0 && currentType < (int)params.KnownMarkerTypes.size()) {
                                            marker.Type = params.KnownMarkerTypes[currentType];
                                            localUpdate = true;
                                        }
                                    }
                                }
                                
                                if (localUpdate) {
                                    bUpdate = true;
                                    triggerSymmetryDeltaUpdate(marker);
                                }
                                
                                if (ImGui::Button("Delete Marker", ImVec2(-1, 20))) {
                                    keyToDelete = key;
                                }
                            }
                            ImGui::PopID();
                        }
                        ImGui::Unindent();
                    }
                }
            },
            bNeedsMapUpdate
        );
        
        if (!keyToDelete.empty()) {
            if (selectedMarkerKey == keyToDelete) selectedMarkerKey = "";
            
            // Delete from all layers
            for (auto& layer : params.PlacedMarkerLayers) {
                layer.MarkerKeys.erase(std::remove(layer.MarkerKeys.begin(), layer.MarkerKeys.end(), keyToDelete), layer.MarkerKeys.end());
            }
            
            // Delete from master list
            params.MarkersList.erase(keyToDelete);
            bNeedsMapUpdate = true;
            bNeedsPreviewRender = true;
        }
    }

    """

content = content[:start_idx] + new_ui_code + content[end_idx:]

with open("gui/EnvironmentTabs.cpp", "w", encoding="utf-8") as f:
    f.write(content)
