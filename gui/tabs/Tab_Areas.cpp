#include "../UITabs.h"
#include <imgui.h>
#include <vector>
#include <string>

namespace SanmapGen {
namespace UI {
    void RenderAreasTab(GenerationParams& params, bool& bNeedsPreviewRender) {
        ImGui::Text("Map Areas");
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::TextWrapped("Areas define physical boundaries on the map. You can drag and resize them directly on the Map Preview canvas, or adjust them below.");
        ImGui::Spacing();

        if (ImGui::Button("Add New Area")) {
            std::string newName = "NewArea_" + std::to_string(params.Areas.size());
            MapArea a;
            a.Name = newName;
            a.X = 0; a.Y = 0; a.Width = 100; a.Length = 100;
            params.Areas.push_back(a);
            bNeedsPreviewRender = true;
        }
        ImGui::SameLine();
        ImGui::Checkbox("Lock Areas", &params.AreasLocked);

        ImGui::Spacing();
        ImGui::Separator();

        int areaToRemove = -1;

        for (int i = (int)params.Areas.size() - 1; i >= 0; i--) {
            auto& area = params.Areas[i];
            
            ImGui::PushID(i);
            bool expanded = ImGui::CollapsingHeader(area.Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            
            // Drag and drop for reordering
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("AREA_DRAG", &i, sizeof(int));
                ImGui::Text("Moving: %s", area.Name.c_str());
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AREA_DRAG")) {
                    int source_i = *(const int*)payload->Data;
                    if (source_i >= 0 && source_i < params.Areas.size() && source_i != i) {
                        MapArea movingArea = params.Areas[source_i];
                        params.Areas.erase(params.Areas.begin() + source_i);
                        // If we are dragging downwards (lower index to higher index), the target index shifts.
                        int insert_i = i;
                        if (source_i < i) {
                            insert_i--; 
                        }
                        params.Areas.insert(params.Areas.begin() + insert_i, movingArea);
                        bNeedsPreviewRender = true;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (expanded) {
                // Name editor
                char nameBuf[256];
                strncpy_s(nameBuf, area.Name.c_str(), sizeof(nameBuf));
                if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
                    area.Name = nameBuf;
                }
                
                if (ImGui::DragFloat("X Position", &area.X, 1.0f)) bNeedsPreviewRender = true;
                if (ImGui::DragFloat("Y Position (Z-axis)", &area.Y, 1.0f)) bNeedsPreviewRender = true;
                if (ImGui::DragFloat("Width", &area.Width, 1.0f, 1.0f, params.MapSize * 2.0f)) bNeedsPreviewRender = true;
                if (ImGui::DragFloat("Length", &area.Length, 1.0f, 1.0f, params.MapSize * 2.0f)) bNeedsPreviewRender = true;
                
                if (ImGui::ColorEdit4("Color", area.Color, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf)) bNeedsPreviewRender = true;
                
                if (ImGui::Button("Set to Map Size")) {
                    area.X = 0.0f;
                    area.Y = 0.0f;
                    area.Width = (float)params.MapSize;
                    area.Length = (float)params.MapSize;
                    bNeedsPreviewRender = true;
                }
                
                if (area.Name != "PlayableArea") {
                    if (ImGui::Button("Remove Area")) {
                        areaToRemove = i;
                    }
                } else {
                    ImGui::TextDisabled("PlayableArea is required by the engine and cannot be removed.");
                }
                ImGui::Spacing();
            }
            ImGui::PopID();
        }
        
        if (areaToRemove != -1) {
            params.Areas.erase(params.Areas.begin() + areaToRemove);
            bNeedsPreviewRender = true;
        }
        
        // Safety: Ensure unique keys for JSON export
        std::vector<std::string> seenNames;
        for (auto& area : params.Areas) {
            std::string base = area.Name;
            int suffix = 1;
            while (std::find(seenNames.begin(), seenNames.end(), area.Name) != seenNames.end()) {
                area.Name = base + "_" + std::to_string(suffix++);
            }
            seenNames.push_back(area.Name);
        }
    }
}
}
