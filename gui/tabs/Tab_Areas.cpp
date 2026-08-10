#include "../UITabs.h"
#include <imgui.h>

namespace SanmapGen {
namespace UI {
    void RenderAreasTab(GenerationParams& params) {
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
            params.Areas[newName] = a;
        }

        ImGui::Spacing();
        ImGui::Separator();

        std::string areaToRemove = "";

        for (auto& [name, area] : params.Areas) {
            if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                
                // Name editor
                char nameBuf[256];
                strncpy_s(nameBuf, area.Name.c_str(), sizeof(nameBuf));
                if (ImGui::InputText(("Name##" + name).c_str(), nameBuf, sizeof(nameBuf))) {
                    area.Name = nameBuf;
                }
                
                ImGui::DragFloat(("X Position##" + name).c_str(), &area.X, 1.0f);
                ImGui::DragFloat(("Y Position (Z-axis)##" + name).c_str(), &area.Y, 1.0f);
                ImGui::DragFloat(("Width##" + name).c_str(), &area.Width, 1.0f, 1.0f, params.MapSize * 2.0f);
                ImGui::DragFloat(("Length##" + name).c_str(), &area.Length, 1.0f, 1.0f, params.MapSize * 2.0f);
                
                if (name != "PlayableArea") {
                    if (ImGui::Button(("Remove Area##" + name).c_str())) {
                        areaToRemove = name;
                    }
                } else {
                    ImGui::TextDisabled("PlayableArea is required by the engine and cannot be removed.");
                }
                ImGui::Spacing();
            }
        }
        
        // Safety: update keys if names changed
        std::map<std::string, MapArea> newMap;
        for (auto& [oldKey, area] : params.Areas) {
            if (oldKey == areaToRemove) continue;
            // PlayableArea key cannot change
            if (oldKey == "PlayableArea") {
                area.Name = "PlayableArea";
                newMap["PlayableArea"] = area;
            } else {
                // Ensure unique key
                std::string k = area.Name;
                int suffix = 1;
                while (newMap.find(k) != newMap.end()) {
                    k = area.Name + "_" + std::to_string(suffix++);
                }
                area.Name = k;
                newMap[k] = area;
            }
        }
        params.Areas = newMap;
    }
}
}
