// ArmiesTab_UI.cpp — the imgui composition of the armies tab. Layer: UI.
// Shared widgets only: DraggableList for the army stack, ColorSwatch / Combo / SliderScalar /
// TextInput / FilePathPicker / Section for the per-army settings, and ArmiesTab_Units_UI for the
// unit rules. No ImGui::SliderFloat / DragFloat / VSliderFloat in this file.
#include "ArmiesTab_UI.h"
#include "Combo_UI.h"
#include "DraggableListWidget_UI.h"
#include "TextInput_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The army stack. MUTATES NOTHING while drawing: the signal is applied after the list closes.
DraggableListSignal DrawArmyList(const ArmiesTabState& state) {
    return DraggableList<ArmyPresentation>::Render(
        "armies", state.armies,
        [&](int rowIndex) {
            DraggableListRow row;
            row.label = ArmyRowLabel(state.armies[static_cast<std::size_t>(rowIndex)]);
            return row;
        },
        [](int) {},
        state.selectedArmyIndex);
}

// A removed army takes its unit rules with it and renumbers the ones above it
// (DropUnitRulesForRemovedArmy) — never leaving a rule pointing at an army that no longer exists.
bool ApplyArmyListSignal(std::vector<Params::UnitRule>& unitRules, ArmiesTabState& state,
                         const DraggableListSignal& signal) {
    if (signal.kind == DraggableListSignalKind::Select) {
        state.selectedArmyIndex = signal.sourceRowIndex;
        state.units.selectedRuleIndex = -1;
        return false;
    }
    const bool bDeleting = signal.kind == DraggableListSignalKind::Delete;
    const int removedArmyIndex = signal.sourceRowIndex;
    if (!ApplyDraggableListSignal(state.armies, signal)) return false;
    const bool bRecipeMoved = bDeleting && DropUnitRulesForRemovedArmy(unitRules, removedArmyIndex);
    if (state.selectedArmyIndex >= static_cast<int>(state.armies.size()))
        state.selectedArmyIndex = static_cast<int>(state.armies.size()) - 1;
    state.units.selectedRuleIndex = -1;
    return bRecipeMoved;
}

// The gamedata root the host loads unit blueprints from (SCOPE NOTE 2) and the Add Army button.
void DrawArmiesGlobals(ArmiesTabState& state) {
    if (!DrawSectionBegin("Global", state.globalSection)) return;
    DrawFilePathPicker("Gamedata Folder", state.gamedataDirectory, state.gamedataOptions);
    if (ImGui::Button("Add Army")) {
        state.armies.push_back(ArmyPresentation());
        state.selectedArmyIndex = static_cast<int>(state.armies.size()) - 1;
    }
    DrawSectionEnd();
}

// The selected army's presentation. None of it notifies the driver: no stage hashes a team color
// (SCOPE NOTE 1), and asking for a regeneration a tint cannot affect is the "cheap tweak triggers
// a full regen" defect.
void DrawArmySettings(ArmyPresentation& army, ArmiesTabState& state) {
    TextInputRules nameRules;
    nameRules.maximumLength = 48;
    nameRules.bAllowEmpty   = false;
    nameRules.fallbackText  = "Army";
    DrawTextInput("Name", army.name, nameRules);
    DrawColorSwatch("Team Color", army.teamColor, state.teamColorOptions, army.teamColorToggle);
    ComboOptions factionOptions;
    factionOptions.labels = armyFactionLabels;
    factionOptions.count  = kArmyFactionCount;
    DrawCombo("Faction", army.factionIndex, factionOptions);
    DrawSliderScalar("Starting Alloys", army.startingAlloys, state.startingAlloysRange,
                     army.startingAlloysToggle, WidgetStyle(), "%.0f");
    DrawSliderScalar("Starting Energy", army.startingEnergy, state.startingEnergyRange,
                     army.startingEnergyToggle, WidgetStyle(), "%.0f");
}

} // namespace

void DrawArmiesTab(Params::MapRecipe& recipe, ArmiesTabState& state,
                   Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest) {
    ImGui::PushID("armiesTab");
    DrawArmiesGlobals(state);
    if (!DrawSectionBegin("Armies", state.armySection)) { ImGui::PopID(); return; }
    const DraggableListSignal signal = DrawArmyList(state);
    if (signal.bHasSignal())
        NotifyPlacementChange(ApplyArmyListSignal(recipe.unitRules, state, signal), previewDriver);
    ArmyPresentation* const army = SelectedArmy(state);
    if (army == nullptr) {
        ImGui::TextUnformatted("Add or select an army to edit it.");
        DrawSectionEnd();
        ImGui::PopID();
        return;
    }
    DrawArmySettings(*army, state);
    DrawArmyUnitList(recipe.unitRules, state.selectedArmyIndex, state.units, previewDriver,
                     iconManifest);
    DrawSectionEnd();
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
