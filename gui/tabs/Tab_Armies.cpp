#include "../UITabs.h"
#include "../UIHelpers.h"
#include "FileDialog.h"
#include "imgui.h"
#include "imgui.h"
#include <string>
#include <unordered_map>
#include <algorithm>
#include <filesystem>
#include "TextureLoader.h"

namespace SanmapGen {
namespace UI {

static std::string activeArmyForUnits = "";
static std::unordered_map<std::string, bool> selectedUnits;
static int unitsToAddCount = 1;

static GLuint GetUnitThumbnail(const std::string& typeName, GenerationParams& params) {
    if (typeName.empty()) return 0;
    std::string cacheKey = "UNIT_" + typeName;
    auto it = params.IconCache.find(cacheKey);
    if (it != params.IconCache.end()) return it->second;
    
    // Not loaded yet, request it asynchronously and return 0
    AsyncTextureManager::RequestUnitIcon(typeName, cacheKey);
    return 0;
}

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
    
    ImGui::Spacing();
    
    std::string toRemove = "";
    bool openUnitModal = false;
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
                openUnitModal = true;
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
            ImGui::Separator();
            ImGui::Text("Unit Groups & Placements");
            
            std::string groupToRemove = "";
            for (auto& [groupName, group] : army.Groups) {
                ImGui::PushID(groupName.c_str());
                if (ImGui::TreeNode(groupName.c_str(), "%s (Units: %zu)", groupName.c_str(), group.Units.size())) {
                    
                    if (ImGui::Button("Delete Entire Group")) {
                        groupToRemove = groupName;
                    }
                    
                    ImGui::Spacing();
                    
                    // Group units by Type for clean UI
                    std::map<std::string, std::vector<std::string>> unitsByType;
                    for (auto& [tpid, u] : group.Units) {
                        unitsByType[u.Type].push_back(tpid);
                    }
                    
                    for (auto& [typeId, tpids] : unitsByType) {
                        std::string typeLabel = typeId + " (" + std::to_string(tpids.size()) + ")";
                        if (ImGui::TreeNode(typeId.c_str(), "%s", typeLabel.c_str())) {
                            if (ImGui::Button("Delete All of this Type")) {
                                for (const auto& id : tpids) group.Units.erase(id);
                                bNeedsMapUpdate = true;
                            }
                            
                            for (const auto& tpid : tpids) {
                                ImGui::Text("%s", tpid.c_str());
                                ImGui::SameLine();
                                if (ImGui::Button(("Delete##" + tpid).c_str())) {
                                    group.Units.erase(tpid);
                                    bNeedsMapUpdate = true;
                                    break; // Break inner loop since map was modified
                                }
                            }
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            
            if (!groupToRemove.empty()) {
                army.Groups.erase(groupToRemove);
                bNeedsMapUpdate = true;
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            
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
    
    if (openUnitModal) {
        ImGui::OpenPopup("Add Units##Modal");
    }
    
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("Add Units##Modal", NULL, ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::Text("Select Units to add to %s", params.ActiveArmyForUnits.c_str());
        ImGui::Separator();
        
        ImGui::BeginChild("##UnitSelection", ImVec2(0, -60), true);
        
        static std::vector<std::string> unitKeys;
        if (ImGui::IsWindowAppearing()) {
            unitKeys.clear();
            for (const auto& [typeId, def] : params.UnitDefinitions) {
                unitKeys.push_back(typeId);
            }
            std::sort(unitKeys.begin(), unitKeys.end());
        }
        
        // Show all parsed unit definitions using Clipper so we don't load 200 icons instantly
        int columns = 4;
        if (ImGui::BeginTable("##UnitGrid", columns)) {
            ImGuiListClipper clipper;
            int rows = (unitKeys.size() + columns - 1) / columns;
            clipper.Begin(rows);
            while (clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                    ImGui::TableNextRow();
                    for (int col = 0; col < columns; col++) {
                        int idx = row * columns + col;
                        if (idx >= (int)unitKeys.size()) break;
                        ImGui::TableSetColumnIndex(col);
                        
                        std::string typeId = unitKeys[idx];
                        auto& def = params.UnitDefinitions[typeId];
                        
                        bool isSelected = std::find(params.SelectedUnitsToSpawn.begin(), params.SelectedUnitsToSpawn.end(), typeId) != params.SelectedUnitsToSpawn.end();
                        
                        ImGui::PushID(typeId.c_str());
                        
                        GLuint tex = GetUnitThumbnail(typeId, params);
                        if (tex != 0) {
                            ImGui::Image((void*)(intptr_t)tex, ImVec2(80, 80));
                        } else {
                            ImGui::Button("?", ImVec2(80, 80)); // Placeholder if icon fails to load
                        }
                        
                        if (ImGui::Selectable(def.DisplayName.empty() ? typeId.c_str() : def.DisplayName.c_str(), &isSelected, ImGuiSelectableFlags_DontClosePopups, ImVec2(120, 30))) {
                            if (!ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift) {
                                params.SelectedUnitsToSpawn.clear();
                            }
                            if (isSelected) {
                                params.SelectedUnitsToSpawn.push_back(typeId);
                            } else {
                                params.SelectedUnitsToSpawn.erase(std::remove(params.SelectedUnitsToSpawn.begin(), params.SelectedUnitsToSpawn.end(), typeId), params.SelectedUnitsToSpawn.end());
                            }
                        }
                        
                        ImGui::PopID();
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
