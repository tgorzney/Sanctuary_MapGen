// MapCanvas_ScenarioEditMode_Chrome_UI.cpp — the legend strip + "Preview As" toggle row (reusing
// STEP74 §3's own DrawSlotPatternToggleRow, per the ticket's own instruction), and the right-click
// context-menu popup. Layer: UI.
#include "MapCanvas_ScenarioEditMode_Ops_UI.h"
#include "ArmiesTab_UI.h"
#include "ScenariosTab_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

void DrawScenarioEditModeChrome(ScenarioEditModeState& state, const std::vector<Params::Army>& armies,
                                int maxArmySlotCount) {
    if (!state.IsActive()) return;
    ImGui::TextUnformatted("Scenario Edit Mode is ON \xE2\x80\x94 left-drag a spawn; right-click an "
                           "alloy marker or empty ground.");
    ImGui::SeparatorText("Preview As");
    DrawSlotPatternToggleRow(state.previewAsSlotPattern, armies, maxArmySlotCount);
    ImGui::SeparatorText("Legend");
    for (std::size_t index = 0; index < armies.size(); ++index) {
        if (index > 0) ImGui::SameLine();
        const ImVec4 armyColor(armies[index].armyColor[0], armies[index].armyColor[1],
                               armies[index].armyColor[2], armies[index].armyColor[3]);
        ImGui::TextColored(armyColor, "%s", ArmyRowLabel(armies[index]));
    }
    ImGui::TextWrapped("Hollow + ! = spawn not yet explicit for this scenario. Solid = explicit. "
                       "Plain = baseline alloy kept. Grey + line = deleted. '+' = added. "
                       "Ghost + red X = marked for removal.");
}

void DrawScenarioEditModeContextMenuPopup(ScenarioEditModeState& state) {
    using RequestKind = ScenarioEditModeState::ContextMenuRequest::Kind;
    const char* const popupIdentifier = "ScenarioEditModeContextMenu";
    if (state.bContextMenuJustRequested && state.pendingContextMenu.kind != RequestKind::None)
        ImGui::OpenPopup(popupIdentifier);
    if (!ImGui::BeginPopup(popupIdentifier)) return;
    if (state.editedBody == nullptr) { ImGui::CloseCurrentPopup(); ImGui::EndPopup(); return; }

    const Params::ScenarioAlloyMode alloyMode = state.editedBody->alloyMode;
    if (state.pendingContextMenu.kind == RequestKind::RemoveBaselineAlloy) {
        const bool bCanCommit = CanRemoveBaselineAlloyForScenario(alloyMode);
        ImGui::BeginDisabled(!bCanCommit);
        const bool bClicked = ImGui::MenuItem("Remove for this scenario");
        ImGui::EndDisabled();
        if (!bCanCommit && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", RemoveBaselineAlloyDisabledReason(alloyMode));
        if (bClicked && bCanCommit) {
            CommitScenarioEditModeContextMenu(*state.editedBody, state.pendingContextMenu);
            ImGui::CloseCurrentPopup();
        }
    } else if (state.pendingContextMenu.kind == RequestKind::AddAlloyForArmy) {
        const bool bCanCommit = CanAddAlloyMarkerForScenario(alloyMode);
        const std::string label = "Add Alloy Marker for " + state.pendingContextMenu.armyName;
        ImGui::BeginDisabled(!bCanCommit);
        const bool bClicked = ImGui::MenuItem(label.c_str());
        ImGui::EndDisabled();
        if (!bCanCommit && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", AddAlloyMarkerDisabledReason(alloyMode));
        if (bClicked && bCanCommit) {
            CommitScenarioEditModeContextMenu(*state.editedBody, state.pendingContextMenu);
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::EndPopup();
}

} // namespace Ui
} // namespace SanmapGen
