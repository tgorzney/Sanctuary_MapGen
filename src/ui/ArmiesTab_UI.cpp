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
DraggableListSignal DrawArmyList(const std::vector<Params::Army>& armies, int selectedArmyIndex) {
    return DraggableList<Params::Army>::Render(
        "armies", armies,
        [&](int rowIndex) {
            DraggableListRow row;
            row.label = ArmyRowLabel(armies[static_cast<std::size_t>(rowIndex)]);
            return row;
        },
        [](int) {},
        selectedArmyIndex);
}

// A removed army takes its unit rules with it and renumbers the ones above it
// (DropUnitRulesForRemovedArmy); a reordered army carries its unit rules' ownership along with it
// (RenumberUnitRuleArmyIndicesForReorder, STEP20 ruling #4, called BEFORE the armies vector itself
// moves — the renumber needs the pre-move army count) — never leaving a rule pointing at the wrong
// army or one that no longer exists. Reports whether `recipe.unitRules` moved, the only half of
// this that ever feeds the preview (SCOPE NOTE 1: an army's own fields do not).
bool ApplyArmyListSignal(Params::MapRecipe& recipe, ArmiesTabState& state,
                         const DraggableListSignal& signal) {
    if (signal.kind == DraggableListSignalKind::Select) {
        state.selectedArmyIndex = signal.sourceRowIndex;
        state.units.selectedRuleIndex = -1;
        return false;
    }
    const bool bDeleting          = signal.kind == DraggableListSignalKind::Delete;
    const bool bReordering        = signal.kind == DraggableListSignalKind::Reorder;
    const int  sourceArmyIndex    = signal.sourceRowIndex;
    const int  armyCountBeforeMove = static_cast<int>(recipe.armies.size());
    bool bUnitRulesMoved = bReordering && RenumberUnitRuleArmyIndicesForReorder(
        recipe.unitRules, sourceArmyIndex, signal.targetRowIndex, armyCountBeforeMove);
    if (!ApplyDraggableListSignal(recipe.armies, signal)) return bUnitRulesMoved;
    if (bDeleting)
        bUnitRulesMoved = DropUnitRulesForRemovedArmy(recipe.unitRules, sourceArmyIndex) || bUnitRulesMoved;
    if (state.selectedArmyIndex >= static_cast<int>(recipe.armies.size()))
        state.selectedArmyIndex = static_cast<int>(recipe.armies.size()) - 1;
    state.units.selectedRuleIndex = -1;
    return bUnitRulesMoved;
}

// The gamedata root the host loads unit blueprints from (SCOPE NOTE 2) and the Add Army button.
// Reports whether an army was added, so the caller knows to run the name-uniqueness repair.
bool DrawArmiesGlobals(std::vector<Params::Army>& armies, ArmiesTabState& state) {
    if (!DrawSectionBegin("Global", state.globalSection)) return false;
    DrawFilePathPicker("Gamedata Folder", state.gamedataDirectory, state.gamedataOptions);
    bool bArmiesMoved = false;
    if (ImGui::Button("Add Army")) {
        Params::Army army;
        army.name = NextArmyName(static_cast<int>(armies.size()));
        armies.push_back(army);
        state.selectedArmyIndex = static_cast<int>(armies.size()) - 1;
        bArmiesMoved = true;
    }
    DrawSectionEnd();
    return bArmiesMoved;
}

// The selected army's own fields: name, alias, team color, faction and starting resources. None
// of it notifies the driver (SCOPE NOTE 1) — nothing hashes or previews an army's own fields yet.
// Reports whether the NAME committed, the only field the uniqueness repair cares about.
bool DrawArmySettings(Params::Army& army, ArmiesTabState& state) {
    TextInputRules nameRules;
    nameRules.maximumLength = 48;
    nameRules.bAllowEmpty   = false;
    nameRules.fallbackText  = "Army";
    const bool bNameCommitted = DrawTextInput("Name", army.name, nameRules).bCommitted;

    TextInputRules aliasRules;
    aliasRules.maximumLength = 48;
    DrawTextInput("Alias", army.alias, aliasRules);

    DrawColorSwatch("Team Color", army.armyColor, state.armyColorOptions, state.armyColorToggle);

    // Same-frame local int mirror: a combo pick commits immediately, so it carries no RealtimeToggle
    // of its own (STEP20 ruling #6).
    int factionIndex = static_cast<int>(army.faction);
    ComboOptions factionOptions;
    factionOptions.labels = armyFactionLabels;
    factionOptions.count  = kArmyFactionCount;
    if (DrawCombo("Faction", factionIndex, factionOptions).bCommitted)
        army.faction = static_cast<Params::Faction>(factionIndex);

    DrawSliderScalar("Starting Alloys", army.alloys, state.alloysRange, state.alloysToggle,
                     WidgetStyle(), "%.0f");
    DrawSliderScalar("Starting Energy", army.energy, state.energyRange, state.energyToggle,
                     WidgetStyle(), "%.0f");
    return bNameCommitted;
}

} // namespace

void DrawArmiesTab(Params::MapRecipe& recipe, ArmiesTabState& state,
                   Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest) {
    ImGui::PushID("armiesTab");
    bool bArmiesMoved = DrawArmiesGlobals(recipe.armies, state);
    if (DrawSectionBegin("Armies", state.armySection)) {
        const DraggableListSignal signal = DrawArmyList(recipe.armies, state.selectedArmyIndex);
        if (signal.bHasSignal())
            NotifyPlacementChange(ApplyArmyListSignal(recipe, state, signal), previewDriver);
        Params::Army* const army = SelectedArmy(recipe.armies, state.selectedArmyIndex);
        if (army == nullptr) {
            ImGui::TextUnformatted("Add or select an army to edit it.");
        } else {
            bArmiesMoved = DrawArmySettings(*army, state) || bArmiesMoved;
            DrawArmyUnitList(recipe.unitRules, state.selectedArmyIndex, state.units, previewDriver,
                             iconManifest);
        }
        DrawSectionEnd();
    }
    // The export keys armies by NAME, so the duplicate repair runs on the frames a name settled —
    // not every frame, which would rename a row mid-typing (STEP20 ruling #5).
    if (bArmiesMoved) MakeNamesUnique(recipe.armies);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
