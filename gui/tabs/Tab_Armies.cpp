#include "../UITabs.h"
#include "../UIHelpers.h"
#include "imgui.h"
#include <string>

namespace SanmapGen {
namespace UI {

void RenderArmiesTab(GenerationParams& params, bool& bNeedsMapUpdate) {
    ImGui::Text("ARMIES CONFIGURATION");
    ImGui::Separator();
    ImGui::Spacing();
    
    if (ImGui::Button("Add Army")) {
        int idx = 1;
        std::string newName = "ARMY_1";
        while (params.Armies.find(newName) != params.Armies.end()) {
            idx++;
            newName = "ARMY_" + std::to_string(idx);
        }
        Army newArmy;
        newArmy.Faction = 0;
        newArmy.Alloys = 100.0f;
        newArmy.Energy = 1000.0f;
        
        const float defaultColors[8][4] = {
            {1.0f, 0.0f, 0.0f, 1.0f}, // Red
            {1.0f, 0.4f, 0.7f, 1.0f}, // Pink
            {1.0f, 0.5f, 0.0f, 1.0f}, // Orange
            {0.5f, 0.0f, 0.5f, 1.0f}, // Purple
            {0.0f, 0.0f, 1.0f, 1.0f}, // Blue
            {0.0f, 0.5f, 0.5f, 1.0f}, // Teal
            {0.0f, 0.5f, 0.0f, 1.0f}, // Green
            {0.2f, 0.8f, 0.2f, 1.0f}  // Lime Green
        };
        int cIdx = (idx - 1) % 8;
        newArmy.Color[0] = defaultColors[cIdx][0];
        newArmy.Color[1] = defaultColors[cIdx][1];
        newArmy.Color[2] = defaultColors[cIdx][2];
        newArmy.Color[3] = defaultColors[cIdx][3];
        
        params.Armies[newName] = newArmy;
        bNeedsMapUpdate = true;
    }
    
    ImGui::Spacing();
    
    std::string toRemove = "";
    for (auto& [armyName, army] : params.Armies) {
        ImGui::PushID(armyName.c_str());
        if (ImGui::CollapsingHeader(armyName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            
            // Team Color
            if (ImGui::ColorEdit4("Team Color", army.Color, ImGuiColorEditFlags_NoInputs)) {
                bNeedsMapUpdate = true;
            }
            
            // Faction dropdown
            const char* factions[] = { "UEF (0)", "Cybran (1)", "Aeon (2)" };
            int currentFaction = army.Faction;
            if (currentFaction < 0 || currentFaction > 2) currentFaction = 0;
            if (ImGui::Combo("Faction", &currentFaction, factions, IM_ARRAYSIZE(factions))) {
                army.Faction = currentFaction;
                bNeedsMapUpdate = true;
            }
            
            // Resources
            if (ImGui::DragFloat("Starting Alloys", &army.Alloys, 10.0f, 0.0f, 100000.0f)) bNeedsMapUpdate = true;
            if (ImGui::DragFloat("Starting Energy", &army.Energy, 100.0f, 0.0f, 1000000.0f)) bNeedsMapUpdate = true;
            
            ImGui::Spacing();
            if (ImGui::Button("Remove Army")) {
                toRemove = armyName;
            }
            
            ImGui::Unindent();
        }
        ImGui::PopID();
    }
    
    if (!toRemove.empty()) {
        params.Armies.erase(toRemove);
        params.MarkersList.erase(toRemove);
        bNeedsMapUpdate = true;
    }
}


} // namespace UI
} // namespace SanmapGen
