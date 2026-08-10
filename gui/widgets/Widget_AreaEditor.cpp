#include "Widget_AreaEditor.h"
#include <imgui_internal.h>
#include <algorithm>

namespace SanmapGen {
    
    std::string Widget_AreaEditor::draggingArea = "";
    int Widget_AreaEditor::dragCorner = 0; // 0=none, 1=N, 2=NE, 3=E, 4=SE, 5=S, 6=SW, 7=W, 8=NW, 9=Center
    ImVec2 Widget_AreaEditor::dragStartMouse = ImVec2(0,0);
    MapArea Widget_AreaEditor::dragStartArea = MapArea();

    void Widget_AreaEditor::RenderOverlay(GenerationParams& params, ImVec2 p0, ImVec2 p1, ImVec2 uv0, ImVec2 uv1, float renderSize, float mapZoom, ImVec2 mapOffset, bool& bNeedsPreviewRender) {
        
        bool isAreaLayerEnabled = false;
        for (const auto& l : params.PreviewLayers) {
            if (l.Type == GenerationParams::PreviewLayerType::Areas && l.Blend != GenerationParams::LayerBlendMode::None) {
                isAreaLayerEnabled = true;
                break;
            }
        }
        
        if (!params.ShowAreas && !isAreaLayerEnabled) return;
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 mousePos = ImGui::GetIO().MousePos;
        bool isHoveringPreview = ImGui::IsItemHovered();
        bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        bool isMouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        
        auto WorldToScreen = [&](float worldX, float worldZ) -> ImVec2 {
            float worldU = worldX / (float)params.MapSize;
            float worldV = worldZ / (float)params.MapSize;
            float screenU = (worldU - uv0.x) / (uv1.x - uv0.x);
            float screenV = (worldV - uv0.y) / (uv1.y - uv0.y);
            return ImVec2(p0.x + screenU * renderSize, p0.y + screenV * renderSize);
        };
        
        auto ScreenToWorld = [&](ImVec2 screen) -> ImVec2 {
            float screenU = (screen.x - p0.x) / renderSize;
            float screenV = (screen.y - p0.y) / renderSize;
            float worldU = uv0.x + screenU * (uv1.x - uv0.x);
            float worldV = uv0.y + screenV * (uv1.y - uv0.y);
            return ImVec2(worldU * params.MapSize, worldV * params.MapSize);
        };

        float cornerRadius = 8.0f;
        std::string hoveredArea = "";
        int hoveredCorner = 0;

        // Draw in reverse Z-order (first in vector is bottom layer, drawn first)
        for (auto& area : params.Areas) {
            ImVec2 tl = WorldToScreen(area.X, area.Y);
            ImVec2 br = WorldToScreen(area.X + area.Width, area.Y + area.Length);
            
            // Draw border
            drawList->AddRect(tl, br, IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);
            
            // Draw 8-way handles
            ImVec2 tr(br.x, tl.y);
            ImVec2 bl(tl.x, br.y);
            ImVec2 tCenter((tl.x + tr.x) / 2.0f, tl.y);
            ImVec2 bCenter((bl.x + br.x) / 2.0f, br.y);
            ImVec2 lCenter(tl.x, (tl.y + bl.y) / 2.0f);
            ImVec2 rCenter(tr.x, (tr.y + br.y) / 2.0f);
            
            ImU32 cornerColor = IM_COL32(255, 255, 255, 255);
            drawList->AddCircleFilled(tl, cornerRadius, cornerColor); // NW
            drawList->AddCircleFilled(tCenter, cornerRadius, cornerColor); // N
            drawList->AddCircleFilled(tr, cornerRadius, cornerColor); // NE
            drawList->AddCircleFilled(rCenter, cornerRadius, cornerColor); // E
            drawList->AddCircleFilled(br, cornerRadius, cornerColor); // SE
            drawList->AddCircleFilled(bCenter, cornerRadius, cornerColor); // S
            drawList->AddCircleFilled(bl, cornerRadius, cornerColor); // SW
            drawList->AddCircleFilled(lCenter, cornerRadius, cornerColor); // W
            
            // Draw name in center
            ImVec2 center((tl.x + br.x) / 2.0f, (tl.y + br.y) / 2.0f);
            ImVec2 textSize = ImGui::CalcTextSize(area.Name.c_str());
            
            // Add slight shadow to text for readability
            drawList->AddText(ImVec2(center.x - textSize.x / 2.0f + 1, center.y - textSize.y / 2.0f + 1), IM_COL32(0, 0, 0, 255), area.Name.c_str());
            drawList->AddText(ImVec2(center.x - textSize.x / 2.0f, center.y - textSize.y / 2.0f), IM_COL32(255, 255, 255, 255), area.Name.c_str());

            // Check interactions
            if (isHoveringPreview && draggingArea.empty()) {
                float thresholdSq = cornerRadius * cornerRadius * 2.0f; // slightly larger hit area
                
                if (ImLengthSqr(ImVec2(mousePos.x - tCenter.x, mousePos.y - tCenter.y)) < thresholdSq) { hoveredArea = area.Name; hoveredCorner = 1; }
                else if (ImLengthSqr(ImVec2(mousePos.x - tr.x, mousePos.y - tr.y)) < thresholdSq) { hoveredArea = area.Name; hoveredCorner = 2; }
                else if (ImLengthSqr(ImVec2(mousePos.x - rCenter.x, mousePos.y - rCenter.y)) < thresholdSq) { hoveredArea = area.Name; hoveredCorner = 3; }
                else if (ImLengthSqr(ImVec2(mousePos.x - br.x, mousePos.y - br.y)) < thresholdSq) { hoveredArea = area.Name; hoveredCorner = 4; }
                else if (ImLengthSqr(ImVec2(mousePos.x - bCenter.x, mousePos.y - bCenter.y)) < thresholdSq) { hoveredArea = area.Name; hoveredCorner = 5; }
                else if (ImLengthSqr(ImVec2(mousePos.x - bl.x, mousePos.y - bl.y)) < thresholdSq) { hoveredArea = area.Name; hoveredCorner = 6; }
                else if (ImLengthSqr(ImVec2(mousePos.x - lCenter.x, mousePos.y - lCenter.y)) < thresholdSq) { hoveredArea = area.Name; hoveredCorner = 7; }
                else if (ImLengthSqr(ImVec2(mousePos.x - tl.x, mousePos.y - tl.y)) < thresholdSq) { hoveredArea = area.Name; hoveredCorner = 8; }
                else if (mousePos.x > tl.x && mousePos.x < br.x && mousePos.y > tl.y && mousePos.y < br.y) { hoveredArea = area.Name; hoveredCorner = 9; } // Center
            }
        }
        
        // Change cursor based on hover
        if (hoveredArea != "") {
            if (hoveredCorner == 1 || hoveredCorner == 5) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            else if (hoveredCorner == 3 || hoveredCorner == 7) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            else if (hoveredCorner == 2 || hoveredCorner == 6) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
            else if (hoveredCorner == 4 || hoveredCorner == 8) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
            else if (hoveredCorner == 9) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        }
        
        // Handle Drag Start
        if (isHoveringPreview && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !hoveredArea.empty()) {
            draggingArea = hoveredArea;
            dragCorner = hoveredCorner;
            dragStartMouse = mousePos;
            
            // Find the area to copy state
            for (auto& a : params.Areas) {
                if (a.Name == draggingArea) {
                    dragStartArea = a;
                    break;
                }
            }
        }
        
        // Handle Dragging
        if (isMouseDown && !draggingArea.empty()) {
            MapArea* activeArea = nullptr;
            for (auto& a : params.Areas) {
                if (a.Name == draggingArea) {
                    activeArea = &a;
                    break;
                }
            }
            
            if (activeArea) {
                ImVec2 deltaScreen = ImVec2(mousePos.x - dragStartMouse.x, mousePos.y - dragStartMouse.y);
                ImVec2 worldStart = ScreenToWorld(dragStartMouse);
                ImVec2 worldCurrent = ScreenToWorld(mousePos);
                float dx = worldCurrent.x - worldStart.x;
                float dy = worldCurrent.y - worldStart.y;
                
                bool shiftDown = ImGui::GetIO().KeyShift;
                bool ctrlDown  = ImGui::GetIO().KeyCtrl;
                
                // 1=N, 2=NE, 3=E, 4=SE, 5=S, 6=SW, 7=W, 8=NW, 9=Center
                if (dragCorner == 9) { // Center Drag
                    activeArea->X = dragStartArea.X + dx;
                    activeArea->Y = dragStartArea.Y + dy;
                } else {
                    float startW = dragStartArea.Width;
                    float startL = dragStartArea.Length;
                    float aspect = startW / startL;
                    
                    float deltaW = 0, deltaL = 0;
                    
                    if (dragCorner == 3 || dragCorner == 2 || dragCorner == 4) deltaW = dx; // East
                    if (dragCorner == 7 || dragCorner == 8 || dragCorner == 6) deltaW = -dx; // West
                    
                    if (dragCorner == 5 || dragCorner == 4 || dragCorner == 6) deltaL = dy; // South
                    if (dragCorner == 1 || dragCorner == 2 || dragCorner == 8) deltaL = -dy; // North
                    
                    if (ctrlDown) {
                        deltaW *= 2.0f;
                        deltaL *= 2.0f;
                    }
                    
                    float newW = startW + deltaW;
                    float newL = startL + deltaL;
                    
                    if (shiftDown) {
                        if (dragCorner == 1 || dragCorner == 5) {
                            newW = newL * aspect;
                        } else if (dragCorner == 3 || dragCorner == 7) {
                            newL = newW / aspect;
                        } else {
                            if (std::abs(deltaW) > std::abs(deltaL)) {
                                newL = newW / aspect;
                            } else {
                                newW = newL * aspect;
                            }
                        }
                    }
                    
                    if (newW < 1.0f) newW = 1.0f;
                    if (newL < 1.0f) newL = 1.0f;
                    
                    float cx = dragStartArea.X + startW / 2.0f;
                    float cy = dragStartArea.Y + startL / 2.0f;
                    
                    float newX = dragStartArea.X;
                    float newY = dragStartArea.Y;
                    
                    if (ctrlDown) {
                        newX = cx - newW / 2.0f;
                        newY = cy - newL / 2.0f;
                    } else {
                        if (dragCorner == 3 || dragCorner == 2 || dragCorner == 4) newX = dragStartArea.X; 
                        if (dragCorner == 7 || dragCorner == 8 || dragCorner == 6) newX = dragStartArea.X + startW - newW;
                        if (dragCorner == 1 || dragCorner == 5) newX = cx - newW / 2.0f;
                        
                        if (dragCorner == 5 || dragCorner == 4 || dragCorner == 6) newY = dragStartArea.Y;
                        if (dragCorner == 1 || dragCorner == 2 || dragCorner == 8) newY = dragStartArea.Y + startL - newL;
                        if (dragCorner == 3 || dragCorner == 7) newY = cy - newL / 2.0f;
                    }
                    
                    activeArea->X = newX;
                    activeArea->Y = newY;
                    activeArea->Width = newW;
                    activeArea->Length = newL;
                }
                
                // Draw a temporary filled rect while dragging so the user sees immediate feedback
                ImVec2 tl = WorldToScreen(activeArea->X, activeArea->Y);
                ImVec2 br = WorldToScreen(activeArea->X + activeArea->Width, activeArea->Y + activeArea->Length);
                ImU32 dragColor = IM_COL32((int)(activeArea->Color[0]*255), (int)(activeArea->Color[1]*255), (int)(activeArea->Color[2]*255), 100);
                drawList->AddRectFilled(tl, br, dragColor);
            }
        }
        
        if (isMouseReleased) {
            if (!draggingArea.empty()) {
                bNeedsPreviewRender = true; // ONLY update the expensive composite texture on release!
            }
            draggingArea = "";
            dragCorner = 0;
        }
    }
}
