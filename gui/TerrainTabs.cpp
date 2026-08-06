#include "UITabs.h"
#include "imgui.h"
#include "FileDialog.h"
#include <string>
#include <algorithm>
#include <fstream>
#include <cmath>

namespace SanmapGen {
namespace UI {

    // Internal helper to render the UI for a single NoiseLayer
    static void RenderSingleLayerSettings(GenerationParams& params, size_t i, NoiseLayer& layer, std::vector<NoiseLayer>& layerArray, bool& bNeedsMapUpdate) {
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
        bool expanded = ImGui::CollapsingHeader(layer.Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth);
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
        float buttonsWidth = btnWidth * 3.0f + btnGap * 2.0f;
        ImGui::SameLine(headerWidth - buttonsWidth);

        if (ImGui::Button("Import RAW...", ImVec2(btnWidth, 0))) {
            std::string path;
            if (FileDialog::OpenFile("RAW Heightmaps\0*.raw\0", path)) {
                std::ifstream inFile(path, std::ios::binary | std::ios::ate);
                if (inFile) {
                    std::streamsize size = inFile.tellg();
                    inFile.seekg(0, std::ios::beg);
                    
                    // We assume 16-bit uint raw maps that match MapSize+1.
                    // If the user selects a random raw file, we still just read floats.
                    std::vector<uint16_t> rawData(size / sizeof(uint16_t));
                    if (inFile.read(reinterpret_cast<char*>(rawData.data()), size)) {
                        layer.ImageData.resize(rawData.size());
                        for (size_t k = 0; k < rawData.size(); ++k) {
                            layer.ImageData[k] = static_cast<float>(rawData[k]) / 65535.0f;
                        }
                        
                        // Guess dimensions (assume square)
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
                if (ImGui::SliderFloat("Mask Min", &layer.HeightBlendMin, 0.0f, 1.0f)) {
                    if (layer.HeightBlendMin >= layer.HeightBlendMax) layer.HeightBlendMax = layer.HeightBlendMin + 0.001f;
                    if (layer.HeightBlendMax > 1.0f) { layer.HeightBlendMax = 1.0f; layer.HeightBlendMin = 0.999f; }
                    bNeedsMapUpdate = true;
                }
                if (ImGui::SliderFloat("Mask Max", &layer.HeightBlendMax, 0.0f, 1.0f)) {
                    if (layer.HeightBlendMax <= layer.HeightBlendMin) layer.HeightBlendMin = layer.HeightBlendMax - 0.001f;
                    if (layer.HeightBlendMin < 0.0f) { layer.HeightBlendMin = 0.0f; layer.HeightBlendMax = 0.001f; }
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
                if (ImGui::MenuItem("Bedrock")) { layer.Hardness = 1.0f; layer.Friction = 0.8f; layer.Cohesion = 1.0f; layer.CapacityMult = 0.1f; bNeedsMapUpdate = true; }
                if (ImGui::MenuItem("Rock")) { layer.Hardness = 0.8f; layer.Friction = 0.7f; layer.Cohesion = 0.8f; layer.CapacityMult = 0.5f; bNeedsMapUpdate = true; }
                if (ImGui::MenuItem("Clay")) { layer.Hardness = 0.5f; layer.Friction = 0.4f; layer.Cohesion = 0.9f; layer.CapacityMult = 1.0f; bNeedsMapUpdate = true; }
                if (ImGui::MenuItem("Dirt")) { layer.Hardness = 0.4f; layer.Friction = 0.5f; layer.Cohesion = 0.5f; layer.CapacityMult = 1.5f; bNeedsMapUpdate = true; }
                if (ImGui::MenuItem("Mud")) { layer.Hardness = 0.2f; layer.Friction = 0.2f; layer.Cohesion = 0.7f; layer.CapacityMult = 2.0f; bNeedsMapUpdate = true; }
                if (ImGui::MenuItem("Sand")) { layer.Hardness = 0.1f; layer.Friction = 0.6f; layer.Cohesion = 0.1f; layer.CapacityMult = 2.5f; bNeedsMapUpdate = true; }
                ImGui::EndPopup();
            }
            ImGui::PopID();

            if (soilPhysicsOpen) {
                if (ImGui::SliderFloat("Hardness", &layer.Hardness, 0.01f, 1.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Friction", &layer.Friction, 0.01f, 1.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Cohesion", &layer.Cohesion, 0.01f, 1.0f)) bNeedsMapUpdate = true;
                if (ImGui::SliderFloat("Capacity Mult", &layer.CapacityMult, 0.1f, 5.0f)) bNeedsMapUpdate = true;
                ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx("Hydraulic Erosion", ImGuiTreeNodeFlags_None)) {
                if (ImGui::Checkbox("Enable##ero", &layer.Erosion.Enabled)) bNeedsMapUpdate = true;
                if (layer.Erosion.Enabled) {
                    if (ImGui::Checkbox("Erode Beneath", &layer.ErodeBeneath)) bNeedsMapUpdate = true;
                    if (ImGui::DragInt("Droplet Count", &layer.Erosion.DropletCount, 1000.0f, 1000, 5000000)) bNeedsMapUpdate = true;
                    if (ImGui::SliderInt("Max Lifetime", &layer.Erosion.MaxLifetime, 5, 200)) bNeedsMapUpdate = true;
                    if (ImGui::SliderFloat("Evaporation", &layer.Erosion.EvaporationRate, 0.001f, 0.2f)) bNeedsMapUpdate = true;
                    
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
                    if (ImGui::SliderFloat("Spawn Min H", &layer.Erosion.SpawnMinHeight, 0.0f, 1.0f)) bNeedsMapUpdate = true;
                    if (ImGui::SliderFloat("Spawn Max H", &layer.Erosion.SpawnMaxHeight, 0.0f, 1.0f)) bNeedsMapUpdate = true;
                }
                ImGui::TreePop();
            }

            ImGui::Unindent();
        }
    }

    // Reusable Global Layer Framework
    void RenderLayerStack(GenerationParams& params, std::vector<NoiseLayer>& flatLayers, std::vector<GeoLayerDef>* geoLayers, bool useGeoLayers, bool& bNeedsMapUpdate) {
        if (useGeoLayers && geoLayers) {
            // Render Nested GeoLayers
            if (ImGui::Button("Add GeoLayer", ImVec2(-1, 30))) {
                GeoLayerDef newGeoLayer;
                newGeoLayer.Name = "GeoLayer " + std::to_string(geoLayers->size());
                geoLayers->push_back(newGeoLayer);
                bNeedsMapUpdate = true;
            }
            ImGui::Spacing();

            for (int g = (int)geoLayers->size() - 1; g >= 0; --g) {
                ImGui::PushID(g * 1000);
                GeoLayerDef& geoLayer = (*geoLayers)[g];
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
                        RenderSingleLayerSettings(params, i, geoLayer.Layers[i], geoLayer.Layers, bNeedsMapUpdate);
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
                RenderSingleLayerSettings(params, i, flatLayers[i], flatLayers, bNeedsMapUpdate);
                ImGui::PopID();
                if (bNeedsMapUpdate && i >= flatLayers.size()) break; // Safety break
            }
        }
    }

    void RenderHeightmapTab(GenerationParams& params, bool& bNeedsMapUpdate) {
        ImGui::Checkbox("##showHeightmap", &params.ShowHeightmap); ImGui::SameLine();
        ImGui::Text("Global Settings");
        ImGui::Separator();

        if (ImGui::InputInt("Map Seed", &params.Seed)) bNeedsMapUpdate = true;
        
        const int sizes[] = { 256, 512, 1024, 2048, 4096 };
        const char* size_labels[] = { "256", "512", "1024", "2048", "4096" };
        
        int size_idx = 1;
        for (int i = 0; i < IM_ARRAYSIZE(sizes); ++i) {
            if (sizes[i] == params.MapSize) size_idx = i;
        }
        
        ImGui::Checkbox("Scale features to Map Size", &params.ScaleFeaturesToMapSize);
        if (ImGui::BeginCombo("Map Size", size_labels[size_idx])) {
            for (int n = 0; n < IM_ARRAYSIZE(sizes); n++) {
                if (ImGui::Selectable(size_labels[n], size_idx == n)) {
                    if (size_idx != n) {
                        int oldS = sizes[size_idx];
                        size_idx = n;
                        params.MapSize = sizes[size_idx];
                        bNeedsMapUpdate = true;
                        if (params.ScaleFeaturesToMapSize) {
                            float scale = static_cast<float>(oldS) / sizes[n];
                            for (auto& gl : params.GeoLayers) {
                                for (auto& l : gl.Layers) l.Frequency *= scale;
                            }
                        }
                    }
                }
            }
            ImGui::EndCombo();
        }
        
        if (ImGui::SliderFloat("Global Gravity", &params.GlobalGravity, 1.0f, 20.0f)) bNeedsMapUpdate = true;

        ImGui::Spacing();
        ImGui::Text("Global Symmetry");
        
        int current_alg = static_cast<int>(params.SymAlgorithm);
        const char* alg_names[] = { "Fold", "Blur", "Cross Fade", "Cylinder3D", "Torus3D", "Native Hash", "Superposition" };
        if (ImGui::Combo("Symmetry Algorithm", &current_alg, alg_names, IM_ARRAYSIZE(alg_names))) {
            params.SymAlgorithm = static_cast<SanmapGen::SymmetryAlgorithm>(current_alg);
            bNeedsMapUpdate = true;
        }
        
        if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::Blur) {
            if (ImGui::SliderFloat("Blur Radius", &params.SymmetryBlurRadius, 1.0f, 50.0f)) bNeedsMapUpdate = true;
        } else if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::CrossFade) {
            if (ImGui::SliderFloat("Cross-Fade Width", &params.CrossFadeWidth, 0.0f, 0.5f)) bNeedsMapUpdate = true;
        } else if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::Superposition) {
            int current_blend = static_cast<int>(params.SymSuperpositionBlend);
            const char* blend_names[] = { "Add", "Subtract", "Multiply", "Overlay", "Max", "Min" };
            if (ImGui::Combo("Blend Mode", &current_blend, blend_names, IM_ARRAYSIZE(blend_names))) {
                params.SymSuperpositionBlend = static_cast<SanmapGen::BlendMode>(current_blend);
                bNeedsMapUpdate = true;
            }
        } else if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::Cylinder3D) {
            if (ImGui::SliderFloat("Z Scale", &params.CylinderZScale, 0.1f, 10.0f)) bNeedsMapUpdate = true;
        } else if (params.SymAlgorithm == SanmapGen::SymmetryAlgorithm::Torus3D) {
            if (ImGui::SliderFloat("Major Radius", &params.TorusMajorRadius, 1.0f, 20.0f)) bNeedsMapUpdate = true;
            if (ImGui::SliderFloat("Minor Radius", &params.TorusMinorRadius, 0.1f, 5.0f)) bNeedsMapUpdate = true;
        }
        
        bool symPoint  = (params.GlobalSymmetryMask & Symmetry_Point);
        bool symX      = (params.GlobalSymmetryMask & Symmetry_X);
        bool symZ      = (params.GlobalSymmetryMask & Symmetry_Z);
        bool symXY     = (params.GlobalSymmetryMask & Symmetry_XY);
        bool symRadial = (params.GlobalSymmetryMask & Symmetry_Radial);
        if (ImGui::Checkbox("Point",  &symPoint))  { params.GlobalSymmetryMask ^= Symmetry_Point;  bNeedsMapUpdate = true; } ImGui::SameLine();
        if (ImGui::Checkbox("X",      &symX))      { params.GlobalSymmetryMask ^= Symmetry_X;      bNeedsMapUpdate = true; } ImGui::SameLine();
        if (ImGui::Checkbox("Z",      &symZ))      { params.GlobalSymmetryMask ^= Symmetry_Z;      bNeedsMapUpdate = true; } ImGui::SameLine();
        if (ImGui::Checkbox("XY",     &symXY))     { params.GlobalSymmetryMask ^= Symmetry_XY;     bNeedsMapUpdate = true; } ImGui::SameLine();
        if (ImGui::Checkbox("Radial", &symRadial)) { params.GlobalSymmetryMask ^= Symmetry_Radial; bNeedsMapUpdate = true; }
        

        ImGui::Spacing();
        ImGui::Text("GeoLayers Hierarchy");
        ImGui::Separator();
        
        std::vector<NoiseLayer> dummy;
        RenderLayerStack(params, dummy, &params.GeoLayers, true, bNeedsMapUpdate);
    }

    bool GradientEditor(const char* label, GradientSettings& gradient, float maxLocation) {
        bool changed = false;
        ImGui::PushID(label);
        
        if (ImGui::Checkbox("Smooth Interpolation", &gradient.SmoothInterpolation)) changed = true;
        
        ImGui::Spacing();
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        float width = ImGui::GetContentRegionAvail().x;
        float height = 30.0f;
        ImVec2 p1 = ImVec2(p0.x + width, p0.y + height);
        
        // Draw the gradient bar
        if (gradient.Stops.empty()) {
            draw_list->AddRectFilled(p0, p1, IM_COL32(0, 0, 0, 255));
        } else if (gradient.Stops.size() == 1) {
            auto& c = gradient.Stops[0].Color;
            draw_list->AddRectFilled(p0, p1, ImGui::ColorConvertFloat4ToU32(ImVec4(c[0], c[1], c[2], c[3])));
        } else {
            // Sort a copy for drawing
            std::vector<GradientStop> drawStops = gradient.Stops;
            std::sort(drawStops.begin(), drawStops.end());
            
            for (size_t i = 0; i < drawStops.size() - 1; ++i) {
                float loc0 = std::clamp(drawStops[i].Location / maxLocation, 0.0f, 1.0f);
                float loc1 = std::clamp(drawStops[i+1].Location / maxLocation, 0.0f, 1.0f);
                
                ImVec2 rectMin(p0.x + loc0 * width, p0.y);
                ImVec2 rectMax(p0.x + loc1 * width, p1.y);
                
                auto& c0 = drawStops[i].Color;
                auto& c1 = drawStops[i+1].Color;
                ImU32 col0 = ImGui::ColorConvertFloat4ToU32(ImVec4(c0[0], c0[1], c0[2], c0[3]));
                ImU32 col1 = ImGui::ColorConvertFloat4ToU32(ImVec4(c1[0], c1[1], c1[2], c1[3]));
                
                if (gradient.SmoothInterpolation) {
                    draw_list->AddRectFilledMultiColor(rectMin, rectMax, col0, col1, col1, col0);
                } else {
                    draw_list->AddRectFilled(rectMin, rectMax, col0);
                }
            }
        }
        
        draw_list->AddRect(p0, p1, IM_COL32(255, 255, 255, 100)); // Border
        
        // Handle clicking on bar to add stop
        ImGui::InvisibleButton("GradientBar", ImVec2(width, height));
        if (ImGui::IsItemClicked()) {
            float t = (ImGui::GetIO().MousePos.x - p0.x) / width;
            float newLoc = t * maxLocation;
            GradientStop ns;
            ns.Location = newLoc;
            gradient.Stops.push_back(ns);
            changed = true;
        }
        
        static int selectedStop = -1;
        
        // Draw Stops
        for (int i = 0; i < (int)gradient.Stops.size(); ++i) {
            float loc = std::clamp(gradient.Stops[i].Location / maxLocation, 0.0f, 1.0f);
            ImVec2 center(p0.x + loc * width, p1.y + 5.0f);
            
            // Triangle bounds
            ImVec2 tp0(center.x - 5.0f, center.y + 10.0f);
            ImVec2 tp1(center.x + 5.0f, center.y + 10.0f);
            ImVec2 tp2(center.x, center.y);
            
            ImU32 outlineColor = (selectedStop == i) ? IM_COL32(255, 255, 255, 255) : IM_COL32(150, 150, 150, 255);
            auto& c = gradient.Stops[i].Color;
            ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(c[0], c[1], c[2], c[3]));
            
            draw_list->AddTriangleFilled(tp0, tp1, tp2, fillColor);
            draw_list->AddTriangle(tp0, tp1, tp2, outlineColor);
            
            // Interaction
            ImGui::SetCursorScreenPos(ImVec2(tp0.x, tp2.y));
            ImGui::PushID(i);
            ImGui::InvisibleButton("Stop", ImVec2(10.0f, 10.0f));
            if (ImGui::IsItemClicked()) {
                selectedStop = i;
            }
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                selectedStop = i;
                float delta = ImGui::GetIO().MouseDelta.x;
                gradient.Stops[i].Location += (delta / width) * maxLocation;
                gradient.Stops[i].Location = std::clamp(gradient.Stops[i].Location, 0.0f, maxLocation);
                changed = true;
            }
            ImGui::PopID();
        }
        
        ImGui::SetCursorScreenPos(ImVec2(p0.x, p1.y + 20.0f));
        ImGui::Spacing();
        
        // Editor for selected stop
        if (selectedStop >= 0 && selectedStop < (int)gradient.Stops.size()) {
            ImGui::Text("Selected Stop:");
            if (ImGui::DragFloat("Location", &gradient.Stops[selectedStop].Location, 0.1f, 0.0f, maxLocation)) changed = true;
            if (ImGui::ColorEdit4("Color", gradient.Stops[selectedStop].Color)) changed = true;
            if (ImGui::Button("Delete Stop")) {
                gradient.Stops.erase(gradient.Stops.begin() + selectedStop);
                selectedStop = -1;
                changed = true;
            }
            
            // Keep array sorted internally if location changed
            if (changed) {
                std::sort(gradient.Stops.begin(), gradient.Stops.end());
            }
        }
        
        ImGui::PopID();
        return changed;
    }

    void RenderSlopeMapTab(GenerationParams& params, bool& bNeedsPreviewRender) {
        ImGui::Text("Slope Settings");
        if (ImGui::Checkbox("Show Slope Map Overlay", &params.ShowSlopeMap)) bNeedsPreviewRender = true;
        ImGui::Separator();
        
        ImGui::TextDisabled("Degrees (0 - 90)");
        if (GradientEditor("SlopeGradient", params.SlopeSettingsParams.Gradient, 90.0f)) {
            bNeedsPreviewRender = true;
        }
    }

    void RenderFlowMapTab(GenerationParams& params, bool& bNeedsMapUpdate, bool& bNeedsPreviewRender) {
        ImGui::Text("Flow (Velocity) Settings");
        if (ImGui::Checkbox("Show Flow Map Overlay", &params.ShowFlowMap)) bNeedsPreviewRender = true;
        ImGui::Separator();
        
        ImGui::TextDisabled("Flow Simulation affects both Flow & Accumulation Maps.");
        ImGui::Spacing();
        if (ImGui::DragFloat("Precipitation Rate", &params.FlowSettingsParams.Precipitation, 0.01f, 0.0f, 10.0f)) bNeedsMapUpdate = true;
        if (ImGui::SliderInt("Iterations (Time)", &params.FlowSettingsParams.Iterations, 1, 100)) bNeedsMapUpdate = true;
        if (ImGui::Checkbox("Use GPU Compute (Flow)", &params.FlowSettingsParams.UseGPU)) bNeedsMapUpdate = true;
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Flow Gradient");
        if (GradientEditor("FlowGradient", params.FlowSettingsParams.Gradient, 100.0f)) {
            bNeedsPreviewRender = true;
        }
    }

    void RenderAccumulationMapTab(GenerationParams& params, bool& bNeedsPreviewRender) {
        ImGui::Text("Accumulation Map Settings");
        if (ImGui::Checkbox("Show Accumulation Overlay", &params.ShowAccumulationMap)) bNeedsPreviewRender = true;
        ImGui::Separator();
        
        ImGui::Text("Accumulation Gradient");
        // Reuse Flow gradient settings or add new one? I'll reuse the Flow Settings Gradient for now, or add Accumulation Gradient if needed.
        if (GradientEditor("AccGradient", params.FlowSettingsParams.Gradient, 100.0f)) {
            bNeedsPreviewRender = true;
        }
    }

} // namespace UI
} // namespace SanmapGen
