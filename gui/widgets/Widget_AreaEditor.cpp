#include "Widget_AreaEditor.h"
#include <imgui_internal.h>
#include <algorithm>
#include <iostream>

namespace SanmapGen {
    
    std::string Widget_AreaEditor::draggingArea = "";
    int Widget_AreaEditor::dragCorner = 0;
    ImVec2 Widget_AreaEditor::dragStartMouse = ImVec2(0,0);
    MapArea Widget_AreaEditor::dragStartArea = MapArea();

    void Widget_AreaEditor::RenderOverlay(GenerationParams& params, ImVec2 p0, ImVec2 p1, ImVec2 uv0, ImVec2 uv1, float renderSize, float mapZoom, ImVec2 mapOffset, bool& bNeedsMapUpdate) {
        
        if (!params.ShowAreas) return;
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 mousePos = ImGui::GetIO().MousePos;
        bool isHoveringPreview = ImGui::IsItemHovered();
        bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        bool isMouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        
        // Helper lambda to convert world coordinates to screen coords
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

        for (auto& [name, area] : params.Areas) {
            
            ImVec2 tl = WorldToScreen(area.X, area.Y);
            ImVec2 br = WorldToScreen(area.X + area.Width, area.Y + area.Length);
            
            // Draw rectangle
            drawList->AddRect(tl, br, IM_COL32(255, 255, 0, 200), 0.0f, 0, 2.0f);
            
            // Draw corners
            ImVec2 tr(br.x, tl.y);
            ImVec2 bl(tl.x, br.y);
            
            ImU32 cornerColor = IM_COL32(255, 255, 255, 255);
            drawList->AddCircleFilled(tl, cornerRadius, cornerColor);
            drawList->AddCircleFilled(tr, cornerRadius, cornerColor);
            drawList->AddCircleFilled(br, cornerRadius, cornerColor);
            drawList->AddCircleFilled(bl, cornerRadius, cornerColor);
            
            // Draw name in center
            ImVec2 center((tl.x + br.x) / 2.0f, (tl.y + br.y) / 2.0f);
            ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
            drawList->AddText(ImVec2(center.x - textSize.x / 2.0f, center.y - textSize.y / 2.0f), IM_COL32(255, 255, 255, 255), name.c_str());

            // Check interactions
            if (isHoveringPreview && draggingArea.empty()) {
                if (ImLengthSqr(ImVec2(mousePos.x - tl.x, mousePos.y - tl.y)) < cornerRadius * cornerRadius) { hoveredArea = name; hoveredCorner = 1; }
                else if (ImLengthSqr(ImVec2(mousePos.x - tr.x, mousePos.y - tr.y)) < cornerRadius * cornerRadius) { hoveredArea = name; hoveredCorner = 2; }
                else if (ImLengthSqr(ImVec2(mousePos.x - br.x, mousePos.y - br.y)) < cornerRadius * cornerRadius) { hoveredArea = name; hoveredCorner = 3; }
                else if (ImLengthSqr(ImVec2(mousePos.x - bl.x, mousePos.y - bl.y)) < cornerRadius * cornerRadius) { hoveredArea = name; hoveredCorner = 4; }
                else if (mousePos.x > tl.x && mousePos.x < br.x && mousePos.y > tl.y && mousePos.y < br.y) { hoveredArea = name; hoveredCorner = 5; } // Center
            }
        }
        
        // Handle Drag Start
        if (isHoveringPreview && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !hoveredArea.empty()) {
            draggingArea = hoveredArea;
            dragCorner = hoveredCorner;
            dragStartMouse = mousePos;
            dragStartArea = params.Areas[draggingArea];
        }
        
        // Handle Dragging
        if (isMouseDown && !draggingArea.empty()) {
            if (params.Areas.find(draggingArea) != params.Areas.end()) {
                MapArea& area = params.Areas[draggingArea];
                
                ImVec2 deltaScreen = ImVec2(mousePos.x - dragStartMouse.x, mousePos.y - dragStartMouse.y);
                
                // Convert delta to world delta
                ImVec2 worldStart = ScreenToWorld(dragStartMouse);
                ImVec2 worldCurrent = ScreenToWorld(mousePos);
                float dx = worldCurrent.x - worldStart.x;
                float dy = worldCurrent.y - worldStart.y;
                
                if (dragCorner == 5) { // Center Drag
                    area.X = dragStartArea.X + dx;
                    area.Y = dragStartArea.Y + dy;
                } else if (dragCorner == 1) { // Top Left
                    area.X = std::min(dragStartArea.X + dx, dragStartArea.X + dragStartArea.Width - 1.0f);
                    area.Y = std::min(dragStartArea.Y + dy, dragStartArea.Y + dragStartArea.Length - 1.0f);
                    area.Width = dragStartArea.X + dragStartArea.Width - area.X;
                    area.Length = dragStartArea.Y + dragStartArea.Length - area.Y;
                } else if (dragCorner == 2) { // Top Right
                    area.Width = std::max(1.0f, dragStartArea.Width + dx);
                    area.Y = std::min(dragStartArea.Y + dy, dragStartArea.Y + dragStartArea.Length - 1.0f);
                    area.Length = dragStartArea.Y + dragStartArea.Length - area.Y;
                } else if (dragCorner == 3) { // Bottom Right
                    area.Width = std::max(1.0f, dragStartArea.Width + dx);
                    area.Length = std::max(1.0f, dragStartArea.Length + dy);
                } else if (dragCorner == 4) { // Bottom Left
                    area.X = std::min(dragStartArea.X + dx, dragStartArea.X + dragStartArea.Width - 1.0f);
                    area.Width = dragStartArea.X + dragStartArea.Width - area.X;
                    area.Length = std::max(1.0f, dragStartArea.Length + dy);
                }
                
                bNeedsMapUpdate = true;
            }
        }
        
        // Handle Drag End
        if (isMouseReleased) {
            draggingArea = "";
            dragCorner = 0;
        }
    }
}
