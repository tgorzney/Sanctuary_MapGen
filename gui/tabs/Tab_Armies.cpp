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
        std::string newName = "Army_1";
        while (params.Armies.find(newName) != params.Armies.end()) {
            idx++;
            newName = "Army_" + std::to_string(idx);
        }
        Army newArmy;
        newArmy.Faction = 0;
        newArmy.Alloys = 100.0f;
        newArmy.Energy = 1000.0f;
        params.Armies[newName] = newArmy;
        bNeedsMapUpdate = true;
    }
    
    ImGui::Spacing();
    
    std::string toRemove = "";
    for (auto& [armyName, army] : params.Armies) {
        ImGui::PushID(armyName.c_str());
        if (ImGui::CollapsingHeader(armyName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();
            
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
