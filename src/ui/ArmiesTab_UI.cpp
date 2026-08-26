// ArmiesTab_UI.cpp — the imgui composition of the armies tab. Layer: UI.
// Shared widgets only: DraggableList for the army stack, ColorSwatch / Combo / SliderScalar /
// TextInput / FilePathPicker / Section for the per-army settings, and ArmiesTab_Units_UI for the
// unit rules. No ImGui::SliderFloat / DragFloat / VSliderFloat in this file.
#include "ArmiesTab_UI.h"
#include "Combo_UI.h"
#include "DraggableListWidget_UI.h"
#include "TextInput_UI.h"
#include "../io/Sanmap_ArmyIdentity_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

void DrawArmySettings(std::vector<Params::Army>& armies, int armyIndex, ArmiesTabState& state);

// The army stack. STEP110: each row's own settings render directly below its own header (via
// drawRowBody), operating on that row's own army — never on whatever `selectedArmyIndex` happened
// to point at — so an expanded row can never show another row's fields. `selectedArmyIndex` is
// still threaded through only to drive the CollapsingHeader's `_Selected` highlight and the strip-
// click Select signal (ArmiesTabState.units mirrors it below).
DraggableListSignal DrawArmyList(std::vector<Params::Army>& armies, ArmiesTabState& state) {
    return DraggableList<Params::Army>::Render(
        "armies", armies,
        [&](int rowIndex) {
            DraggableListRow row;
            row.label = ArmyRowLabel(armies[static_cast<std::size_t>(rowIndex)]);
            return row;
        },
        [&](int rowIndex) { DrawArmySettings(armies, rowIndex, state); },
        state.selectedArmyIndex);
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
        army.displayName = NextArmyDisplayName(static_cast<int>(armies.size()));
        // ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-D: a real default color, not the field's own
        // plain-white default — roster position is armies.size() (0-based, unambiguous single call
        // site) BEFORE the push_back below.
        Params::SeedDefaultArmyColor(army.armyColor, static_cast<int>(armies.size()));
        armies.push_back(army);
        state.selectedArmyIndex = static_cast<int>(armies.size()) - 1;
        bArmiesMoved = true;
    }
    DrawSectionEnd();
    return bArmiesMoved;
}

// One row's own fields (SCOPE NOTE 1: none of it notifies the driver). STEP76: "Name" binds
// `displayName`, never machine-owned `name` (ruling 2). STEP75: also draws the mirror-onto-next-
// army button (ruling 1); its confirm dialog is drawn separately by DrawArmiesTab so it stays
// reachable on a frame this function does not run (DrawPendingDeleteRuleLayerDialog's pattern).
// STEP110: called once per EXPANDED row (armyIndex is that row's own index, not necessarily
// `state.selectedArmyIndex`) rather than once at the bottom for whatever was selected. The Mirror
// button is safe to draw unconditionally per row, unlike LayerEditor_Group_UI's import-path picker
// (STEP104): it owns no persisted per-frame edit buffer, only a single click that stamps
// `armyIndex` into the shared pending-mirror state and opens the (still singly-drawn) confirm
// dialog, so two expanded rows can never fight over a live text edit the way a picker could.
void DrawArmySettings(std::vector<Params::Army>& armies, int armyIndex, ArmiesTabState& state) {
    Params::Army& army = armies[static_cast<std::size_t>(armyIndex)];
    ImGui::TextDisabled("Engine ID: %s", army.name.c_str());   // machine-owned, ruling 2 — no input

    TextInputRules displayNameRules;
    displayNameRules.maximumLength = 48;
    displayNameRules.bAllowEmpty   = true;    // display-only; blank is legal, ArmyRowLabel falls back
    DrawTextInput("Name", army.displayName, displayNameRules);

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

    DrawMirrorArmyButton(armies, armyIndex, state.pendingMirrorSourceArmyIndex,
                         state.mirrorConfirmDialogState);
}

} // namespace

void DrawArmiesTab(Params::MapRecipe& recipe, ArmiesTabState& state,
                   Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                   const Io::TemplateIngestReport* templateIngestReport) {
    ImGui::PushID("armiesTab");
    bool bRosterMutated = DrawArmiesGlobals(recipe.armies, state);
    if (DrawSectionBegin("Armies", state.armySection)) {
        const DraggableListSignal signal = DrawArmyList(recipe.armies, state);
        if (signal.bHasSignal()) {
            NotifyPlacementChange(ApplyArmyListSignal(recipe, state, signal), previewDriver);
            // STEP76: Reorder AND Delete both change at least one army's roster position (Select
            // does not, but re-minting on a Select frame is a free no-op — AssignArmyIdentities is
            // idempotent), so every signal, not just Add, must reach the re-mint below.
            bRosterMutated = true;
        }
        ImGui::Separator();
        // STEP110: an army's own fields now render inline in DrawArmyList's row body above; only
        // the unit roster (a genuinely separate, per-selected-army sub-editor, not this ticket's
        // "settings hidden below" antipattern) still needs the selection to know which army's units
        // to show.
        Params::Army* const army = SelectedArmy(recipe.armies, state.selectedArmyIndex);
        if (army == nullptr) {
            ImGui::TextUnformatted("Add or select an army to edit its unit roster.");
        } else {
            DrawArmyUnitList(recipe.unitRules, state.selectedArmyIndex, state.units, previewDriver,
                             iconManifest, templateIngestReport);
        }
        // Ruling 5: not threaded into NotifyPlacementChange/bRosterMutated — Army.groups has no reader.
        DrawPendingMirrorArmyConfirmDialog(recipe.armies, state.pendingMirrorSourceArmyIndex,
                                           state.mirrorConfirmDialogState, recipe.geometry);
        DrawSectionEnd();
    }
    // STEP76: the export keys armies by an identity SanGen now owns outright (ARMY_XX), not a
    // human-authored name, so this re-mints rather than de-duplicates — the identity must stay
    // correct across Add/Reorder/Delete with no window where a stale ARMY_XX could reach export.
    if (bRosterMutated) Io::AssignArmyIdentities(recipe.armies);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
