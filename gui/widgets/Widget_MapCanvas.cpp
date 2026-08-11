#include "Widget_MapCanvas.h"
#include "Widget_AreaEditor.h"
#include <unordered_set>
#include <algorithm>
#include <string>

extern GLuint GetMarkerIcon(const std::string& typeName, SanmapGen::GenerationParams& params, void* openZipArchive);

namespace SanmapGen {

    bool Widget_MapCanvas::isDraggingMarker = false;
    std::string Widget_MapCanvas::draggingMarker = "";
    ImVec2 Widget_MapCanvas::dragOffset(0, 0);
    static float mapZoom = 1.0f;
    static ImVec2 mapOffset(0.0f, 0.0f);

    void Widget_MapCanvas::Render(GenerationParams& params, GLuint previewTexture, bool& bNeedsMapUpdate, 
                                  int& activeTab, std::string& selectedMarkerKey, bool& bNeedsPreviewRender, bool& bResetPreviewTransform, FloatMask& dummyMap) {
                // --- MAP PREVIEW WINDOW ---
        ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_FirstUseEver);
        ImGui::Begin("Map Preview");
        static bool showCompositeSettings = false;
        
        if (ImGui::Button("[ View ]")) {
            ImGui::OpenPopup("ViewPopup");
        }
        ImGui::SameLine();
        if (ImGui::Button("[ Order ]")) showCompositeSettings = true;
        
        ImGui::SameLine();
        if (ImGui::Checkbox("Auto-Level", &params.AutoLevelPreview)) {
            bNeedsPreviewRender = true;
        }
        
        const char* blendModeNames[] = { "None", "Normal", "Add", "Subtract", "Multiply", "Divide", "Overlay", "Screen", "Soft Light", "Hard Light" };
        
        if (ImGui::BeginPopup("ViewPopup")) {
            ImGui::Text("Map Layers");
            ImGui::Separator();
            for (int i = (int)params.PreviewLayers.size() - 1; i >= 0; --i) {
                auto& layer = params.PreviewLayers[i];
                ImGui::PushID(i);
                
                ImGui::AlignTextToFramePadding();
                ImGui::Selectable(layer.Name.c_str(), false, 0, ImVec2(120, 0));
                
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("PREVIEW_LAYER_POPUP_DRAG", &i, sizeof(int));
                    ImGui::Text("Moving %s", layer.Name.c_str());
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREVIEW_LAYER_POPUP_DRAG")) {
                        int source_i = *(const int*)payload->Data;
                        if (source_i != i) {
                            auto movingLayer = params.PreviewLayers[source_i];
                            params.PreviewLayers.erase(params.PreviewLayers.begin() + source_i);
                            int insert_i = (source_i < i) ? i - 1 : i;
                            params.PreviewLayers.insert(params.PreviewLayers.begin() + insert_i, movingLayer);
                            bNeedsPreviewRender = true;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                
                ImGui::SameLine(135);
                
                ImGui::SetNextItemWidth(100);
                int current_blend = (int)layer.Blend;
                if (ImGui::Combo("##blend", &current_blend, blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
                    layer.Blend = (SanmapGen::GenerationParams::LayerBlendMode)current_blend;
                    
                    bool enabled = (layer.Blend != SanmapGen::GenerationParams::LayerBlendMode::None);
                    switch (layer.Type) {
                        case SanmapGen::GenerationParams::PreviewLayerType::Heightmap: params.ShowHeightmap = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Slope: params.ShowSlopeMap = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Flow: params.ShowFlowMap = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Accumulation: params.ShowAccumulationMap = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Stratums: params.ShowStratums = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Water: params.ShowWater = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Markers: params.ShowMarkers = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Props: params.ShowProps = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Areas: params.ShowAreas = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::DetailNormal: params.ShowDetailNormal = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Tint: params.ShowTint = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Holes: params.ShowHoles = enabled; break;
                        case SanmapGen::GenerationParams::PreviewLayerType::Smoothness: params.ShowSmoothness = enabled; break;
                    }
                    bNeedsPreviewRender = true;
                }
                
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }
        ImGui::Separator();
        
        if (showCompositeSettings) {
            ImGui::Begin("Composite Layer Settings", &showCompositeSettings);
            ImGui::Text("Composite Layers (Drag to Reorder, Top-to-Bottom)");
            for (int i = (int)params.PreviewLayers.size() - 1; i >= 0; --i) {
                ImGui::PushID(i);
                ImGui::Selectable(params.PreviewLayers[i].Name.c_str(), false, 0, ImVec2(0, 20));
                
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("PREVIEW_LAYER_DRAG", &i, sizeof(int));
                    ImGui::Text("Moving %s", params.PreviewLayers[i].Name.c_str());
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREVIEW_LAYER_DRAG")) {
                        int source_i = *(const int*)payload->Data;
                        if (source_i != i) {
                            auto movingLayer = params.PreviewLayers[source_i];
                            params.PreviewLayers.erase(params.PreviewLayers.begin() + source_i);
                            int insert_i = (source_i < i) ? i - 1 : i;
                            params.PreviewLayers.insert(params.PreviewLayers.begin() + insert_i, movingLayer);
                            bNeedsPreviewRender = true;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::PopID();
            }
            ImGui::End();
        }
        
        if (previewTexture == 0) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Preview texture not generated yet.");
        } else {
            ImVec2 availSize = ImGui::GetContentRegionAvail();
            float renderSize = availSize.x < availSize.y ? availSize.x : availSize.y;
            
            static ImVec2 mapOffset(0.0f, 0.0f);
            static float mapZoom = 1.0f;
            
            // Reset preview transform when a new sanmap is loaded
            if (bResetPreviewTransform) {
                mapZoom = 1.0f;
                mapOffset = ImVec2(0.0f, 0.0f);
                bResetPreviewTransform = false;
            }
            
            // Interaction logic
            ImGui::InvisibleButton("MapCanvas", ImVec2(renderSize, renderSize));
            bool isHovered = ImGui::IsItemHovered();
            bool isActive = ImGui::IsItemActive();
            ImVec2 p0 = ImGui::GetItemRectMin();
            ImVec2 p1 = ImGui::GetItemRectMax();
            
            if (isHovered && ImGui::GetIO().MouseWheel != 0.0f) {
                // Zoom
                float mouseWheel = ImGui::GetIO().MouseWheel;
                float zoomFactor = powf(1.1f, mouseWheel);
                
                // Keep the mouse position fixed in world space while zooming
                ImVec2 mousePos = ImGui::GetIO().MousePos;
                ImVec2 uvMouse = ImVec2((mousePos.x - p0.x) / renderSize, (mousePos.y - p0.y) / renderSize);
                
                ImVec2 oldCenterOffset = ImVec2(uvMouse.x - 0.5f, uvMouse.y - 0.5f);
                
                mapZoom *= zoomFactor;
                if (mapZoom < 1.0f) { mapZoom = 1.0f; mapOffset = ImVec2(0,0); }
                if (mapZoom > 50.0f) mapZoom = 50.0f;
                
                // Adjust offset to keep mouse point still
                mapOffset.x += oldCenterOffset.x * (1.0f / (mapZoom / zoomFactor) - 1.0f / mapZoom);
                mapOffset.y += oldCenterOffset.y * (1.0f / (mapZoom / zoomFactor) - 1.0f / mapZoom);
            }
            
                        static std::string draggingMarker = "";
            static bool isDraggingMarker = false;
            static int draggingPropIndex = -1;
            static bool isDraggingProp = false;
            static ImVec2 dragOffset(0, 0);
            
            if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !isDraggingMarker) {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                mapOffset.x -= delta.x / renderSize / mapZoom;
                mapOffset.y -= delta.y / renderSize / mapZoom;
            }
            
            // Constrain offset
            float maxOffset = 0.5f - 0.5f / mapZoom;
            mapOffset.x = std::clamp(mapOffset.x, -maxOffset, maxOffset);
            mapOffset.y = std::clamp(mapOffset.y, -maxOffset, maxOffset);
            
            ImVec2 uv0 = ImVec2(0.5f - 0.5f / mapZoom + mapOffset.x, 0.5f - 0.5f / mapZoom + mapOffset.y);
            ImVec2 uv1 = ImVec2(0.5f + 0.5f / mapZoom + mapOffset.x, 0.5f + 0.5f / mapZoom + mapOffset.y);
            
            ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)previewTexture, p0, p1, uv0, uv1);


            ImVec2 mousePos = ImGui::GetIO().MousePos;
            bool isHoveringPreview = isHovered;

            if (params.ShowMarkers && dummyMap.GetWidth() == params.MapSize + 1) {

                
                // Determine screen position for each marker
                std::unordered_set<std::string> hiddenMarkers;
                for (const auto& layer : params.PlacedMarkerLayers) {
                    if (!layer.Enabled) {
                        for (const auto& k : layer.MarkerKeys) hiddenMarkers.insert(k);
                    }
                }
                
                for (auto& [key, marker] : params.MarkersList) {
                    if (marker.IsHidden || hiddenMarkers.count(key)) continue;
                    
                    float worldU = marker.Position[0] / (float)params.MapSize;
                    float worldV = marker.Position[2] / (float)params.MapSize;
                    
                    
                    float screenU = (worldU - uv0.x) / (uv1.x - uv0.x);
                    float screenV = (worldV - uv0.y) / (uv1.y - uv0.y);
                    
                    ImVec2 screenPos;
                    screenPos.x = p0.x + screenU * renderSize;
                    screenPos.y = p0.y + screenV * renderSize;
                    
                    // Base size is ~32 pixels for zoom 1
                    float baseScale = 32.0f;
                    if (marker.Type == "Alloy" || marker.Type == "Alloys") baseScale *= params.MarkerScaleAlloy;
                    else if (marker.Type == "Spawn" || marker.Type == "Spawns") baseScale *= params.MarkerScaleSpawn;
                    else if (marker.Type == "Plasma" || marker.Type == "Plasmas") baseScale *= params.MarkerScalePlasma;
                    
                    ImVec2 iconP0(screenPos.x - baseScale/2.0f, screenPos.y - baseScale/2.0f);
                    ImVec2 iconP1(screenPos.x + baseScale/2.0f, screenPos.y + baseScale/2.0f);
                    
                    float globalColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                    std::string iconName = marker.Type;
                    
                    if (marker.Type == "Alloy" || marker.Type == "Alloys") {
                        iconName = params.GlobalIconAlloy;
                        globalColor[0] = params.MarkerColorAlloy[0]; globalColor[1] = params.MarkerColorAlloy[1]; globalColor[2] = params.MarkerColorAlloy[2]; globalColor[3] = params.MarkerColorAlloy[3];
                    } else if (marker.Type == "Spawn" || marker.Type == "Spawns") {
                        iconName = params.GlobalIconSpawn;
                        
                        // Use exactly the army color, default to white if not found
                        std::string armyId = "";
                        if (marker.CustomName.find("Spawn_") == 0) {
                            armyId = marker.CustomName.substr(6);
                        } else {
                            armyId = marker.CustomName;
                        }
                        if (params.Armies.find(armyId) != params.Armies.end()) {
                            globalColor[0] = params.Armies.at(armyId).Color[0];
                            globalColor[1] = params.Armies.at(armyId).Color[1];
                            globalColor[2] = params.Armies.at(armyId).Color[2];
                            globalColor[3] = params.Armies.at(armyId).Color[3];
                        } else {
                            globalColor[0] = 1.0f; globalColor[1] = 1.0f; globalColor[2] = 1.0f; globalColor[3] = 1.0f;
                        }
                    } else if (marker.Type == "Plasma" || marker.Type == "Plasmas") {
                        iconName = params.GlobalIconPlasma;
                        globalColor[0] = params.MarkerColorPlasma[0]; globalColor[1] = params.MarkerColorPlasma[1]; globalColor[2] = params.MarkerColorPlasma[2]; globalColor[3] = params.MarkerColorPlasma[3];
                    }
                    
                    if (!marker.IconOverride.empty()) {
                        iconName = marker.IconOverride;
                    }
                    
                    GLuint tex = GetMarkerIcon(iconName, params, nullptr);
                    if (tex == 0) tex = GetMarkerIcon(marker.Type, params, nullptr); // Fallback to base type name
                    if (tex == 0 && !params.IconCache.empty()) tex = params.IconCache.begin()->second; // Ultimate fallback
                    ImU32 tintCol = IM_COL32(
                        std::clamp((int)(globalColor[0] * marker.Color[0] * 255.0f), 0, 255),
                        std::clamp((int)(globalColor[1] * marker.Color[1] * 255.0f), 0, 255),
                        std::clamp((int)(globalColor[2] * marker.Color[2] * 255.0f), 0, 255),
                        std::clamp((int)(globalColor[3] * marker.Color[3] * 255.0f), 0, 255)
                    );
                    
                    // Is the mouse over this marker?
                    bool hit = (mousePos.x >= iconP0.x && mousePos.x <= iconP1.x &&
                                mousePos.y >= iconP0.y && mousePos.y <= iconP1.y);
                                
                    if (hit && isHoveringPreview) {
                        if (ImGui::IsMouseClicked(0) && !isDraggingMarker) {
                            draggingMarker = key;
                            selectedMarkerKey = key;
                            activeTab = 9; // Switch to Markers tab
                            isDraggingMarker = true;
                            // Record relative offset to icon center
                            dragOffset.x = mousePos.x - screenPos.x;
                            dragOffset.y = mousePos.y - screenPos.y;
                        }
                        if (ImGui::IsMouseClicked(1)) {
                            ImGui::OpenPopup(("MarkerContext_" + key).c_str());
                        }
                    }
                    
                    if (tex != 0) {
                        if (!marker.IsValid) {
                            ImGui::GetWindowDrawList()->AddRectFilled(iconP0, iconP1, IM_COL32(255, 0, 0, 150));
                        }
                        ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)tex, iconP0, iconP1, ImVec2(0,0), ImVec2(1,1), tintCol);
                    } else {
                        ImU32 col = IM_COL32(255, 255, 0, 255);
                        if (!marker.IsValid) col = IM_COL32(255, 0, 0, 255);
                        else if (marker.Type == "Spawn") col = tintCol;
                        else if (marker.Type == "Plasma") col = IM_COL32(255, 0, 255, 255);
                        
                        if (marker.Type == "Alloy") {
                            ImVec2 p1(screenPos.x, iconP0.y);
                            ImVec2 p2(iconP1.x, iconP1.y);
                            ImVec2 p3(iconP0.x, iconP1.y);
                            ImGui::GetWindowDrawList()->AddTriangleFilled(p1, p2, p3, col);
                        } else {
                            ImGui::GetWindowDrawList()->AddRectFilled(iconP0, iconP1, col);
                        }
                    }
                                        if (ImGui::BeginPopup(("MarkerContext_" + key).c_str())) {
                          if (ImGui::MenuItem("Delete Marker")) {
                              if (params.MarkersList[key].Type == "Spawn") {
                                  params.Armies.erase(key);
                                  float px = params.MarkersList[key].Position[0];
                                  float py = params.MarkersList[key].Position[2];
                                  params.MarkersList.erase(key);
                                  
                                  int symMask = params.GlobalSymmetryMask;
                                  float halfSize = params.MapSize / 2.0f;
                                  std::vector<std::pair<float, float>> symPoints;
                                  
                                  if (symMask & SanmapGen::Symmetry_Point) {
                                      symPoints.push_back({params.MapSize - px, params.MapSize - py});
                                  } else if (symMask & SanmapGen::Symmetry_X) {
                                      symPoints.push_back({params.MapSize - px, py});
                                  } else if (symMask & SanmapGen::Symmetry_Z) {
                                      symPoints.push_back({px, params.MapSize - py});
                                  } else if (symMask & SanmapGen::Symmetry_XY) {
                                      symPoints.push_back({py, px});
                                  } else if (symMask & SanmapGen::Symmetry_Radial && params.SpawnPointCount > 1) {
                                      float dx = px - halfSize;
                                      float dy = py - halfSize;
                                      float angleStep = (2.0f * 3.14159265f) / static_cast<float>(params.SpawnPointCount);
                                      for (int i = 1; i < params.SpawnPointCount; ++i) {
                                          float cosA = std::cos(i * angleStep);
                                          float sinA = std::sin(i * angleStep);
                                          symPoints.push_back({dx * cosA - dy * sinA + halfSize, dx * sinA + dy * cosA + halfSize});
                                      }
                                  }
                                  
                                  for (const auto& sp : symPoints) {
                                      for (auto it = params.MarkersList.begin(); it != params.MarkersList.end(); ) {
                                          if (it->second.Type == "Spawn") {
                                              float dx = it->second.Position[0] - sp.first;
                                              float dy = it->second.Position[2] - sp.second;
                                              if (dx*dx + dy*dy < 100.0f) { // 10 units squared tolerance
                                                  params.Armies.erase(it->first);
                                                  it = params.MarkersList.erase(it);
                                                  continue;
                                              }
                                          }
                                          ++it;
                                      }
                                  }
                              } else {
                                  params.MarkersList.erase(key);
                              }
                              ImGui::EndPopup();
                              break; // Stop iteration as map was modified
                          }
                          ImGui::EndPopup();
                      }
                }
                
                // --- O(1) Prop Click Detection ---
                if (isHoveringPreview && params.ShowProps && !isDraggingMarker && !isDraggingProp) {
                    float screenU_clk = (mousePos.x - p0.x) / renderSize;
                    float screenV_clk = (mousePos.y - p0.y) / renderSize;
                    
                    float uvX = uv0.x + screenU_clk * (uv1.x - uv0.x);
                    float uvY = uv0.y + screenV_clk * (uv1.y - uv0.y);
                    
                    if (uvX >= 0.0f && uvX <= 1.0f && uvY >= 0.0f && uvY <= 1.0f && params.EntityIDBufferWidth > 0) {
                        int px = static_cast<int>(uvX * params.EntityIDBufferWidth);
                        int py = static_cast<int>(uvY * params.EntityIDBufferHeight);
                        int bufIdx = py * params.EntityIDBufferWidth + px;
                        
                        if (bufIdx >= 0 && bufIdx < (int)params.EntityIDBuffer.size()) {
                            uint32_t hitId = params.EntityIDBuffer[bufIdx];
                            if (hitId != 0xFFFFFFFF && hitId < params.StaticPropsList.size()) {
                                if (ImGui::IsMouseClicked(0)) {
                                    draggingPropIndex = hitId;
                                    isDraggingProp = true;
                                    
                                    float worldU = params.StaticPropsList[hitId].X / (float)params.MapSize;
                                    float worldV = params.StaticPropsList[hitId].Z / (float)params.MapSize;
                                    float screenU = (worldU - uv0.x) / (uv1.x - uv0.x);
                                    float screenV = (worldV - uv0.y) / (uv1.y - uv0.y);
                                    ImVec2 screenPos(p0.x + screenU * renderSize, p0.y + screenV * renderSize);
                                    
                                    dragOffset.x = mousePos.x - screenPos.x;
                                    dragOffset.y = mousePos.y - screenPos.y;
                                    
                                    activeTab = 8; // Switch to Props tab
                                }
                            }
                        }
                    }
                }
                
                // Handle Prop Dragging
                if (isDraggingProp) {
                    if (ImGui::IsMouseDragging(0, 0.0f) && draggingPropIndex >= 0 && draggingPropIndex < (int)params.StaticPropsList.size()) {
                        float screenU_drag = (mousePos.x - dragOffset.x - p0.x) / renderSize;
                        float screenV_drag = (mousePos.y - dragOffset.y - p0.y) / renderSize;
                        
                        float worldU_drag = uv0.x + screenU_drag * (uv1.x - uv0.x);
                        float worldV_drag = uv0.y + screenV_drag * (uv1.y - uv0.y);
                        
                        auto& pi = params.StaticPropsList[draggingPropIndex];
                        pi.X = std::clamp(worldU_drag * params.MapSize, 0.0f, (float)params.MapSize);
                        pi.Z = std::clamp(worldV_drag * params.MapSize, 0.0f, (float)params.MapSize);
                        
                        if (pi.LayerIndex >= 0 && pi.LayerIndex < (int)params.ManualPropLayers.size()) {
                            auto& layer = params.ManualPropLayers[pi.LayerIndex];
                            if (pi.GroupIndex >= 0 && pi.GroupIndex < (int)layer.Groups.size()) {
                                auto& group = layer.Groups[pi.GroupIndex];
                                if (pi.TransformIndex >= 0 && pi.TransformIndex < (int)group.Transforms.size()) {
                                    group.Transforms[pi.TransformIndex].Position[0] = pi.X;
                                    group.Transforms[pi.TransformIndex].Position[2] = pi.Z;
                                }
                            }
                        }
                        
                        bNeedsPreviewRender = true; // Constantly re-render to see drag
                    }
                    if (ImGui::IsMouseReleased(0)) {
                        bNeedsMapUpdate = true;
                        isDraggingProp = false;
                        draggingPropIndex = -1;
                    }
                }
                
                // Handle Marker dragging
                if (isDraggingMarker) {
                    if (ImGui::IsMouseDragging(0, 0.0f)) {
                        auto it = params.MarkersList.find(draggingMarker);
                        if (it != params.MarkersList.end()) {
                            float screenU_drag = (mousePos.x - dragOffset.x - p0.x) / renderSize;
                            float screenV_drag = (mousePos.y - dragOffset.y - p0.y) / renderSize;
                            
                            float worldU_drag = uv0.x + screenU_drag * (uv1.x - uv0.x);
                            float worldV_drag = uv0.y + screenV_drag * (uv1.y - uv0.y);
                            
                            it->second.Position[0] = std::clamp(worldU_drag * params.MapSize, 0.0f, (float)params.MapSize);
                            it->second.Position[2] = std::clamp(worldV_drag * params.MapSize, 0.0f, (float)params.MapSize);
                        }
                    }
                    if (ImGui::IsMouseReleased(0)) {
                        bNeedsMapUpdate = true; // Trigger JSON save update or logic if necessary upon drop
                        isDraggingMarker = false;
                        draggingMarker = "";
                    }
                }
                
                // Context Menu on the map itself
                if (isHoveringPreview && ImGui::IsMouseClicked(1) && !isDraggingMarker) {
                    ImGui::OpenPopup("AddMarkerMenu");
                }
                
                if (ImGui::BeginPopup("AddMarkerMenu")) {
                    int selIdx = params.SelectedPlacedLayerIndex;
                    if (selIdx >= 0 && selIdx < (int)params.PlacedMarkerLayers.size()) {
                        auto& layer = params.PlacedMarkerLayers[selIdx];
                        if (layer.Type == SanmapGen::LayerType::Manual) {
                              if (ImGui::BeginMenu("Add Marker to Selected Layer")) {
                                  auto placeMarker = [&](const std::string& type, const std::string& prefix) {
                                      float screenU_clk = (mousePos.x - p0.x) / renderSize;
                                      float screenV_clk = (mousePos.y - p0.y) / renderSize;
                                      float posX = (uv0.x + screenU_clk * (uv1.x - uv0.x)) * params.MapSize;
                                      float posY = (uv0.y + screenV_clk * (uv1.y - uv0.y)) * params.MapSize;

                                      if (type == "Spawn") {
                                          auto addSymmetricSpawn = [&](float x, float y) {
                                              int armyIdx = 1;
                                              std::string newKey = "Army_1";
                                              while (params.MarkersList.find(newKey) != params.MarkersList.end()) {
                                                  armyIdx++;
                                                  newKey = "Army_" + std::to_string(armyIdx);
                                              }
                                              SanmapGen::MarkerTransform m;
                                              m.Type = type;
                                              m.IsManual = true;
                                              m.Position[0] = x;
                                              m.Position[2] = y;
                                              m.Scale[0] = 1.0f; m.Scale[1] = 1.0f; m.Scale[2] = 1.0f;
                                              params.MarkersList[newKey] = m;
                                              layer.MarkerKeys.push_back(newKey);
                                              
                                              if (params.Armies.find(newKey) == params.Armies.end()) {
                                                  SanmapGen::Army newArmy;
                                                  newArmy.Faction = 0;
                                                  newArmy.Alloys = 100.0f;
                                                  newArmy.Energy = 1000.0f;
                                                  params.Armies[newKey] = newArmy;
                                              }
                                          };

                                          addSymmetricSpawn(posX, posY);

                                          float halfSize = params.MapSize / 2.0f;
                                          int symMask = params.GlobalSymmetryMask;

                                          if (symMask & SanmapGen::Symmetry_Point) {
                                              addSymmetricSpawn(params.MapSize - posX, params.MapSize - posY);
                                          } else if (symMask & SanmapGen::Symmetry_X) {
                                              addSymmetricSpawn(params.MapSize - posX, posY);
                                          } else if (symMask & SanmapGen::Symmetry_Z) {
                                              addSymmetricSpawn(posX, params.MapSize - posY);
                                          } else if (symMask & SanmapGen::Symmetry_XY) {
                                              addSymmetricSpawn(posY, posX);
                                          } else if (symMask & SanmapGen::Symmetry_Radial && params.SpawnPointCount > 1) {
                                              float dx = posX - halfSize;
                                              float dy = posY - halfSize;
                                              float angleStep = (2.0f * 3.14159265f) / static_cast<float>(params.SpawnPointCount);
                                              for (int i = 1; i < params.SpawnPointCount; ++i) {
                                                  float cosA = std::cos(i * angleStep);
                                                  float sinA = std::sin(i * angleStep);
                                                  addSymmetricSpawn(dx * cosA - dy * sinA + halfSize, dx * sinA + dy * cosA + halfSize);
                                              }
                                          }
                                      } else {
                                          std::string newKey = prefix + "_" + std::to_string(params.MarkersList.size() + 1);
                                          SanmapGen::MarkerTransform m;
                                          m.Type = type;
                                          m.IsManual = true;
                                          m.Position[0] = posX;
                                          m.Position[2] = posY;
                                          m.Scale[0] = 1.0f; m.Scale[1] = 1.0f; m.Scale[2] = 1.0f;
                                          params.MarkersList[newKey] = m;
                                          layer.MarkerKeys.push_back(newKey);
                                      }
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
                }
            }
            
            // Draw Area Editor Overlay
            Widget_AreaEditor::RenderOverlay(params, p0, p1, uv0, uv1, renderSize, mapZoom, mapOffset, bNeedsPreviewRender);
        }
        
        ImGui::End(); // Map Preview Window
    }
}
