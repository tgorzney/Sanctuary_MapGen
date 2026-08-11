#include "Widget_LayerManager.h"
#include "../core/FileDialog.h"
#include "../UIHelpers.h"
#include <fstream>
#include <string>

namespace SanmapGen {

    void Widget_LayerManager::RenderSingleLayerSettings(GenerationParams& params, size_t i, NoiseLayer& layer, std::vector<NoiseLayer>& layerArray, bool& bNeedsMapUpdate, LayerType type) {
        float headerWidth = ImGui::GetContentRegionAvail().x;
        ImVec2 headerPos = ImGui::GetCursorScreenPos();
        float headerHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;

        ImGui::GetWindowDrawList()->AddRectFilled(
            headerPos, ImVec2(headerPos.x + headerWidth, headerPos.y + headerHeight), IM_COL32(45, 45, 48, 255)
        );

        if (ImGui::Checkbox("##enabled", &layer.Enabled)) bNeedsMapUpdate = true;
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.18f, 0.18f, 0.19f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,  ImVec4(0.25f, 0.25f, 0.27f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,   ImVec4(0.30f, 0.30f, 0.33f, 1.0f));
        bool expanded = ImGui::CollapsingHeader(layer.Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowOverlap);
        ImGui::PopStyleColor(3);

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload("LAYER_DRAG", &i, sizeof(size_t));
            ImGui::Text("Moving: %s", layer.Name.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LAYER_DRAG")) {
                size_t source_i = *(const size_t*)payload->Data;
                if (source_i < layerArray.size() && source_i != i) {
                    // Reorder
                    NoiseLayer movingLayer = layerArray[source_i];
                    layerArray.erase(layerArray.begin() + source_i);
                    // Adjust insertion index if moving downwards
                    size_t insert_i = (source_i < i) ? i - 1 : i;
                    layerArray.insert(layerArray.begin() + insert_i, movingLayer);
                    bNeedsMapUpdate = true;
                }
            }
            ImGui::EndDragDropTarget();
        }

        const float btnWidth = 70.0f;
        const float btnGap = 4.0f;
        // Expanded to 4 buttons (Import, Duplicate, Bake/Clear, Delete)
        float buttonsWidth = btnWidth * 4.0f + btnGap * 3.0f;
        ImGui::SameLine(headerWidth - buttonsWidth);

        if (ImGui::Button("Import RAW...", ImVec2(btnWidth, 0))) {
            std::string path;
            if (FileDialog::OpenFile("RAW Heightmaps\0*.raw\0", path)) {
                std::ifstream inFile(path, std::ios::binary | std::ios::ate);
                if (inFile) {
                    std::streamsize size = inFile.tellg();
                    inFile.seekg(0, std::ios::beg);
                    
                    std::vector<uint16_t> rawData(size / sizeof(uint16_t));
                    if (inFile.read(reinterpret_cast<char*>(rawData.data()), size)) {
                        layer.ImageData.resize(rawData.size());
                        for (size_t k = 0; k < rawData.size(); ++k) {
                            layer.ImageData[k] = static_cast<float>(rawData[k]) / 65535.0f;
                        }
                        
                        int dim = static_cast<int>(std::sqrt(rawData.size()));
                        layer.ImageWidth = dim;
                        layer.ImageHeight = dim;
                        layer.UseImage = true;
                        
                        bNeedsMapUpdate = true;
                    }
                }
            }
        }
        ImGui::SameLine(0, btnGap);

        if (ImGui::Button("Duplicate", ImVec2(btnWidth, 0))) {
            NoiseLayer copiedLayer = layer;
            copiedLayer.Name = copiedLayer.Name + " (Copy)";
            layerArray.insert(layerArray.begin() + i + 1, copiedLayer);
            bNeedsMapUpdate = true; 
            return;
        }
        ImGui::SameLine(0, btnGap);
        
        if (layer.IsBaked) {
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
            if (ImGui::Button("Unbake", ImVec2(btnWidth, 0))) {
                layer.IsBaked = false;
                // Add TerrainGenerator::ClearBakedLayer call hook here eventually
                bNeedsMapUpdate = true;
            }
            ImGui::PopStyleColor(2);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.2f, 0.2f, 0.6f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.3f, 0.3f, 0.7f, 1.0f));
            if (ImGui::Button("Bake", ImVec2(btnWidth, 0))) {
                layer.IsBaked = true;
                layer.BakeRequested = true; // Triggers the async compute pass
                bNeedsMapUpdate = true;
            }
            ImGui::PopStyleColor(2);
        }
        ImGui::SameLine(0, btnGap);

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Delete", ImVec2(btnWidth, 0))) {
            layerArray.erase(layerArray.begin() + i);
            bNeedsMapUpdate = true; 
            ImGui::PopStyleColor(2); 
            return;
        }
        ImGui::PopStyleColor(2);

        if (expanded) {
            ImGui::Indent();
            
            // Disable all procedural settings if the layer is currently baked
            if (layer.IsBaked) {
                ImGui::BeginDisabled(true);
            }
            
            char nameBuf[128];
            strncpy(nameBuf, layer.Name.c_str(), sizeof(nameBuf));
            if (ImGui::InputText("Layer Name", nameBuf, IM_ARRAYSIZE(nameBuf))) {
                layer.Name = nameBuf;
            }
            
            if (ImGui::SliderInt("Stratum Material Index", &layer.StratumIndex, 0, 8)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Opacity", &layer.Opacity, 0.0f, 1.0f)) bNeedsMapUpdate = true;

            if (ImGui::Button("Levels Adjustment...", ImVec2(-1, 24))) {
                ImGui::OpenPopup("LevelsPopup");
            }
            if (ImGui::BeginPopup("LevelsPopup")) {
                ImGui::Text("Input Levels");
                
                static float dummyHist[64] = {0};
                ImGui::PlotHistogram("##hist", dummyHist, 64, 0, NULL, 0.0f, 1.0f, ImVec2(256, 80));
                
                ImGui::SetNextItemWidth(80);
                if (ImGui::DragFloat("##Shadows", &layer.LevelsShadows, 0.01f, 0.0f, layer.LevelsHighlights)) bNeedsMapUpdate = true;
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80);
                if (ImGui::DragFloat("##Midtones", &layer.LevelsMidtones, 0.01f, 0.01f, 9.99f)) bNeedsMapUpdate = true;
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80);
                if (ImGui::DragFloat("##Highlights", &layer.LevelsHighlights, 0.01f, layer.LevelsShadows, 1.0f)) bNeedsMapUpdate = true;
                
                ImGui::Spacing();
                ImGui::Text("Output Levels");
                ImVec2 p = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilledMultiColor(p, ImVec2(p.x + 256, p.y + 20), IM_COL32(0,0,0,255), IM_COL32(255,255,255,255), IM_COL32(255,255,255,255), IM_COL32(0,0,0,255));
                ImGui::Dummy(ImVec2(256, 20));
                
                ImGui::SetNextItemWidth(80);
                if (ImGui::DragFloat("##OutBlack", &layer.LevelsOutputBlack, 0.01f, 0.0f, 1.0f)) bNeedsMapUpdate = true;
                ImGui::SameLine(184); 
                ImGui::SetNextItemWidth(80);
                if (ImGui::DragFloat("##OutWhite", &layer.LevelsOutputWhite, 0.01f, 0.0f, 1.0f)) bNeedsMapUpdate = true;
                
                ImGui::EndPopup();
            }

            if (ImGui::TreeNodeEx("Height Blending", ImGuiTreeNodeFlags_DefaultOpen)) {
                const char* blend_labels[] = { "Add", "Subtract", "Multiply", "Overlay", "Max", "Min" };
                int current_blend = (int)layer.Blend;
                if (ImGui::Combo("Blend Mode", &current_blend, blend_labels, IM_ARRAYSIZE(blend_labels))) {
                    layer.Blend = (BlendMode)current_blend;
                    bNeedsMapUpdate = true;
                }
                if (ImGui::SliderFloat("Blend Sharpness", &layer.HeightBlendContrast, 0.1f, 5.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Image Contrast", &layer.ImageContrast, 0.0f, 3.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Image Brightness", &layer.ImageBrightness, -1.0f, 1.0f)) bNeedsMapUpdate = true;
                if (UI::RangeSliderFloat("Height Mask", &layer.HeightBlendMin, &layer.HeightBlendMax, 0.0f, 1.0f)) {
                    bNeedsMapUpdate = true;
                }
                ImGui::TreePop();
            }

            if (ImGui::Checkbox("Use Global Symmetry", &layer.SymmetryUseGlobal)) bNeedsMapUpdate = true;
            if (!layer.SymmetryUseGlobal) {
                ImGui::Text("Local Symmetry:"); ImGui::SameLine();
                bool symPoint = (layer.SymmetryMask & Symmetry_Point);
                bool symX = (layer.SymmetryMask & Symmetry_X);
                bool symZ = (layer.SymmetryMask & Symmetry_Z);
                bool symXY = (layer.SymmetryMask & Symmetry_XY);
                bool symRadial = (layer.SymmetryMask & Symmetry_Radial);
                if (ImGui::Checkbox("Point", &symPoint)) { layer.SymmetryMask ^= Symmetry_Point; bNeedsMapUpdate = true; } ImGui::SameLine();
                if (ImGui::Checkbox("X", &symX)) { layer.SymmetryMask ^= Symmetry_X; bNeedsMapUpdate = true; } ImGui::SameLine();
                if (ImGui::Checkbox("Z", &symZ)) { layer.SymmetryMask ^= Symmetry_Z; bNeedsMapUpdate = true; } ImGui::SameLine();
                if (ImGui::Checkbox("XY", &symXY)) { layer.SymmetryMask ^= Symmetry_XY; bNeedsMapUpdate = true; } ImGui::SameLine();
                if (ImGui::Checkbox("Radial", &symRadial)) { layer.SymmetryMask ^= Symmetry_Radial; bNeedsMapUpdate = true; }
            }

            if (!layer.UseImage) {
                if (ImGui::TreeNodeEx("Noise Properties", ImGuiTreeNodeFlags_None)) {
                    const char* noise_labels[] = { "OpenSimplex2", "OpenSimplex2S", "Cellular", "Perlin", "ValueCubic", "Value", "None" };
                    int current_noise = (int)layer.Type;
                    if (ImGui::Combo("Type", &current_noise, noise_labels, IM_ARRAYSIZE(noise_labels))) { layer.Type = (NoiseType)current_noise; bNeedsMapUpdate = true; }
                    
                    const char* fractal_labels[] = { "None", "FBm", "Ridged", "PingPong" };
                    int current_fractal = (int)layer.Fractal;
                    if (ImGui::Combo("Fractal", &current_fractal, fractal_labels, IM_ARRAYSIZE(fractal_labels))) { layer.Fractal = (FractalType)current_fractal; bNeedsMapUpdate = true; }

                    if (ImGui::DragFloat("Frequency", &layer.Frequency, 0.001f, 0.0001f, 0.5f, "%.4f")) bNeedsMapUpdate = true;
                    if (ImGui::SliderInt("Octaves", &layer.Octaves, 1, 10)) bNeedsMapUpdate = true;
                    if (ImGui::SliderFloat("Gain", &layer.Gain, 0.1f, 5.0f)) bNeedsMapUpdate = true;
                    ImGui::TreePop();
                }
                if (ImGui::TreeNodeEx("Density Shaping", ImGuiTreeNodeFlags_None)) {
                    if (ImGui::SliderFloat("Land", &layer.LandDensity, 0.0f, 1.0f)) bNeedsMapUpdate = true;
                    if (ImGui::SliderFloat("Plateau", &layer.PlateauDensity, 0.0f, 1.0f)) bNeedsMapUpdate = true;
                    if (ImGui::SliderFloat("Mountain", &layer.MountainDensity, 0.0f, 1.0f)) bNeedsMapUpdate = true;
                    if (ImGui::SliderFloat("Ramp", &layer.RampDensity, 0.0f, 1.0f)) bNeedsMapUpdate = true;
                    ImGui::TreePop();
                }
            }

            bool soilPhysicsOpen = ImGui::TreeNodeEx("Soil Physics", ImGuiTreeNodeFlags_None);
            ImGui::SameLine();
            ImGui::PushID("SoilPresets");
            if (ImGui::Button("Presets \xE2\x96\xBC")) { 
                ImGui::OpenPopup("SoilPresetsPopup");
            }
            if (ImGui::BeginPopup("SoilPresetsPopup")) {
                auto& strat = params.Stratums[layer.StratumIndex];
                if (ImGui::MenuItem("Bedrock")) { strat.hardness = 1.0f; strat.friction = 0.8f; strat.cohesion = 1.0f; strat.capacityMult = 0.1f; bNeedsMapUpdate = true; }
                if (ImGui::MenuItem("Rock")) { strat.hardness = 0.8f; strat.friction = 0.7f; strat.cohesion = 0.8f; strat.capacityMult = 0.5f; bNeedsMapUpdate = true; }
                if (ImGui::MenuItem("Clay")) { strat.hardness = 0.5f; strat.friction = 0.4f; strat.cohesion = 0.9f; strat.capacityMult = 1.0f; bNeedsMapUpdate = true; }
                if (ImGui::MenuItem("Dirt")) { strat.hardness = 0.4f; strat.friction = 0.5f; strat.cohesion = 0.5f; strat.capacityMult = 1.5f; bNeedsMapUpdate = true; }
                if (ImGui::MenuItem("Mud")) { strat.hardness = 0.2f; strat.friction = 0.2f; strat.cohesion = 0.7f; strat.capacityMult = 2.0f; bNeedsMapUpdate = true; }
                if (ImGui::MenuItem("Sand")) { strat.hardness = 0.1f; strat.friction = 0.6f; strat.cohesion = 0.1f; strat.capacityMult = 2.5f; bNeedsMapUpdate = true; }
                ImGui::EndPopup();
            }
            ImGui::PopID();

            if (soilPhysicsOpen) {
                auto& strat = params.Stratums[layer.StratumIndex];
                if (ImGui::SliderFloat("Hardness", &strat.hardness, 0.01f, 1.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Friction", &strat.friction, 0.01f, 1.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Cohesion", &strat.cohesion, 0.01f, 1.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Capacity Mult", &strat.capacityMult, 0.1f, 5.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Absorption Rate", &strat.absorptionRate, 0.001f, 0.5f)) bNeedsMapUpdate = true;
                ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx("Hydraulic Erosion", ImGuiTreeNodeFlags_None)) {
                if (ImGui::Checkbox("Enable##ero", &layer.Erosion.Enabled)) bNeedsMapUpdate = true;
                if (layer.Erosion.Enabled) {
                    if (ImGui::Checkbox("Erode Beneath", &layer.ErodeBeneath)) bNeedsMapUpdate = true;
                    if (ImGui::DragInt("Droplet Count", &layer.Erosion.DropletCount, 1000.0f, 1000, 5000000)) bNeedsMapUpdate = true;
                    if (ImGui::SliderInt("Max Lifetime", &layer.Erosion.MaxLifetime, 5, 200)) bNeedsMapUpdate = true;
                    if (ImGui::SliderFloat("Evaporation", &layer.Erosion.EvaporationRate, 0.001f, 0.2f)) bNeedsMapUpdate = true;
                    
                    ImGui::Spacing();
                    ImGui::Text("Scientific Fluid Mechanics");
                    if (ImGui::SliderFloat("Viscosity", &layer.Erosion.FluidViscosity, 0.1f, 10.0f)) bNeedsMapUpdate = true;
                    if (ImGui::SliderFloat("Base Absorption", &layer.Erosion.BaseAbsorptionRate, 0.001f, 0.5f)) bNeedsMapUpdate = true;
                    if (ImGui::SliderFloat("Capacity Scale", &layer.Erosion.CarryingCapacityScale, 0.1f, 10.0f)) bNeedsMapUpdate = true;
                    ImGui::Spacing();
                    
                    if (ImGui::Checkbox("Use Global Gravity", &layer.Erosion.GravityUseGlobal)) bNeedsMapUpdate = true;
                    if (!layer.Erosion.GravityUseGlobal) {
                        if (ImGui::SliderFloat("Gravity", &layer.Erosion.Gravity, 0.5f, 20.0f)) bNeedsMapUpdate = true;
                    }

                    if (ImGui::TreeNodeEx("Precipitation", ImGuiTreeNodeFlags_None)) {
                        if (ImGui::Checkbox("Rain Noise", &layer.Erosion.UseRainNoise)) bNeedsMapUpdate = true;
                        if (layer.Erosion.UseRainNoise) {
                            if (ImGui::SliderFloat("Freq", &layer.Erosion.RainNoiseFreq, 0.001f, 0.1f)) bNeedsMapUpdate = true;
                            if (ImGui::SliderInt("Octaves", &layer.Erosion.RainNoiseOctaves, 1, 8)) bNeedsMapUpdate = true;
                        }
                        if (ImGui::Checkbox("Orographic Rain", &layer.Erosion.UseOrographicRain)) bNeedsMapUpdate = true;
                        if (layer.Erosion.UseOrographicRain) {
                            if (ImGui::SliderFloat("Wind Angle", &layer.Erosion.WindAngle, 0.0f, 360.0f)) bNeedsMapUpdate = true;
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
            
            if (ImGui::TreeNodeEx("Deposition (Soil Drop)", ImGuiTreeNodeFlags_None)) {
                if (ImGui::Checkbox("Enable Deposition", &layer.Erosion.DepositionMode)) {
                    bNeedsMapUpdate = true;
                    if (layer.Erosion.DepositionMode) layer.Erosion.Enabled = true;
                }
                if (layer.Erosion.DepositionMode) {
                    if (ImGui::SliderFloat("Initial Load", &layer.Erosion.InitialSedimentLoad, 0.01f, 5.0f)) bNeedsMapUpdate = true;
                    if (UI::RangeSliderFloat("Spawn Height", &layer.Erosion.SpawnMinHeight, &layer.Erosion.SpawnMaxHeight, params.TerrainMinHeight, params.TerrainMaxHeight)) bNeedsMapUpdate = true;
                }
                ImGui::TreePop();
            }

            if (type == LayerType::Prop) {
                if (ImGui::TreeNodeEx("Prop Placement Rules", ImGuiTreeNodeFlags_DefaultOpen)) {
                    char bpBuf[256]; strncpy(bpBuf, layer.BlueprintPath.c_str(), sizeof(bpBuf));
                    if (ImGui::InputText("Blueprint", bpBuf, IM_ARRAYSIZE(bpBuf))) layer.BlueprintPath = bpBuf;
                    
                    if (UI::RangeSliderFloat("Slope Range", &layer.MinSlope, &layer.MaxSlope, 0.0f, 90.0f)) bNeedsMapUpdate = true;
                    if (UI::RangeSliderFloat("Height Range", &layer.MinHeight, &layer.MaxHeight, params.TerrainMinHeight, params.TerrainMaxHeight)) bNeedsMapUpdate = true;
                    
                    if (ImGui::Checkbox("Avoid Water", &layer.AvoidWater)) bNeedsMapUpdate = true;
                    if (ImGui::Checkbox("Near Cliffs", &layer.NearCliffs)) bNeedsMapUpdate = true;
                    
                    ImGui::Separator();
                    if (ImGui::Checkbox("Physics Simulate", &layer.PhysicsTagSimulate)) bNeedsMapUpdate = true;
                    char tagBuf[128]; strncpy(tagBuf, layer.PhysicsTagCollision.c_str(), sizeof(tagBuf));
                    if (ImGui::InputText("Collision Tag", tagBuf, IM_ARRAYSIZE(tagBuf))) layer.PhysicsTagCollision = tagBuf;
                    ImGui::TreePop();
                }
            }

            if (layer.IsBaked) {
                ImGui::EndDisabled();
            }
            
            ImGui::Unindent();
        }
    }

    void Widget_LayerManager::RenderLayerStack(GenerationParams& params, std::vector<NoiseLayer>& flatLayers, std::vector<GeoLayerDef>* geoLayers, bool useGeoLayers, bool& bNeedsMapUpdate, LayerType filterType) {
        if (useGeoLayers && geoLayers) {
            // Render Nested GeoLayers
            if (ImGui::Button(filterType == LayerType::Prop ? "Add Prop Group" : "Add GeoLayer", ImVec2(-1, 30))) {
                GeoLayerDef newGeoLayer;
                newGeoLayer.Name = (filterType == LayerType::Prop ? "Prop Group " : "GeoLayer ") + std::to_string(geoLayers->size());
                newGeoLayer.Type = filterType;
                geoLayers->push_back(newGeoLayer);
                bNeedsMapUpdate = true;
            }
            ImGui::Spacing();

            for (int g = (int)geoLayers->size() - 1; g >= 0; --g) {
                GeoLayerDef& geoLayer = (*geoLayers)[g];
                if (geoLayer.Type != filterType) continue; // FILTER

                ImGui::PushID(g * 1000);
                bool geoExpanded = ImGui::CollapsingHeader(geoLayer.Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
                
                if (geoExpanded) {
                    ImGui::Indent();
                    if (ImGui::Button("Add Layer")) {
                        NoiseLayer nl;
                        nl.Name = "New Layer " + std::to_string(geoLayer.Layers.size() + 1);
                        geoLayer.Layers.push_back(nl);
                        bNeedsMapUpdate = true;
                    }
                    for (int i = (int)geoLayer.Layers.size() - 1; i >= 0; --i) {
                        ImGui::PushID(i);
                        RenderSingleLayerSettings(params, i, geoLayer.Layers[i], geoLayer.Layers, bNeedsMapUpdate, filterType);
                        // Safety break handles array size changes
                        ImGui::PopID();
                        if (bNeedsMapUpdate) break; 
                    }
                    ImGui::Unindent();
                }
                ImGui::PopID();
            }
        } else {
            // Render Flat Layer Stack
            if (ImGui::Button("Add Layer", ImVec2(-1, 30))) {
                NoiseLayer nl;
                nl.Name = "New Mask Layer " + std::to_string(flatLayers.size() + 1);
                flatLayers.push_back(nl);
                bNeedsMapUpdate = true;
            }
            for (size_t i = 0; i < flatLayers.size(); ++i) {
                ImGui::PushID((int)i);
                RenderSingleLayerSettings(params, i, flatLayers[i], flatLayers, bNeedsMapUpdate, filterType);
                ImGui::PopID();
                if (bNeedsMapUpdate && i >= flatLayers.size()) break; // Safety break
            }
        }
    }

}
