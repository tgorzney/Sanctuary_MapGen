#include "../UITabs.h"
#include "../UIHelpers.h"
#include "FileDialog.h"
#include "imgui.h"
#include "imgui.h"
#include <string>
#include <unordered_map>

namespace SanmapGen {
namespace UI {

void RenderArmiesTab(GenerationParams& params, bool& bNeedsMapUpdate) {
    ImGui::Text("ARMIES CONFIGURATION");
    ImGui::Separator();
    ImGui::Spacing();
    
    if (ImGui::Button("Browse Gamedata...")) {
        std::string outPath;
        if (FileDialog::SelectFolder(outPath)) {
            params.GamedataPath = outPath;
            bNeedsMapUpdate = true;
        }
    }
    ImGui::SameLine();
    ImGui::TextWrapped("Current: %s", params.GamedataPath.empty() ? "None" : params.GamedataPath.c_str());
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
            
            ImGui::SameLine();
            if (ImGui::Button("Add Units")) {
                params.ActiveArmyForUnits = armyName;
                params.SelectedUnitsToSpawn.clear();
                params.UnitsToSpawnCount = 1;
                ImGui::OpenPopup("Add Units##Modal");
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
    
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("Add Units##Modal", NULL, ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::Text("Select Units to add to %s", params.ActiveArmyForUnits.c_str());
        ImGui::Separator();
        
        ImGui::BeginChild("##UnitSelection", ImVec2(0, -60), true);
        
        // Show all parsed unit definitions
        int columns = 4;
        if (ImGui::BeginTable("##UnitGrid", columns)) {
            for (const auto& [typeId, def] : params.UnitDefinitions) {
                ImGui::TableNextColumn();
                
                bool isSelected = std::find(params.SelectedUnitsToSpawn.begin(), params.SelectedUnitsToSpawn.end(), typeId) != params.SelectedUnitsToSpawn.end();
                
                // Optional: Render thumbnail here
                // ImGui::Image(...);
                
                if (ImGui::Selectable(def.DisplayName.empty() ? typeId.c_str() : def.DisplayName.c_str(), &isSelected, ImGuiSelectableFlags_DontClosePopups, ImVec2(120, 120))) {
                    if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift) {
                        params.SelectedUnitsToSpawn.clear();
                    }
                    if (isSelected) {
                        params.SelectedUnitsToSpawn.push_back(typeId);
                    } else {
                        params.SelectedUnitsToSpawn.erase(std::remove(params.SelectedUnitsToSpawn.begin(), params.SelectedUnitsToSpawn.end(), typeId), params.SelectedUnitsToSpawn.end());
                    }
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
        
        ImGui::Separator();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Count:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::InputInt("##UnitCount", &params.UnitsToSpawnCount);
        if (params.UnitsToSpawnCount < 1) params.UnitsToSpawnCount = 1;
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 160);
        if (ImGui::Button("Cancel", ImVec2(70, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Confirm", ImVec2(70, 0))) {
            // State is already saved in params. Just close.
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}


} // namespace UI
} // namespace SanmapGen
