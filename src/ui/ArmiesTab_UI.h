// ArmiesTab_UI.h — the armies tab: the army stack and, per army, its unit rules.
// Layer: UI. Accuracy class: Visual/Exact. TAB_REBUILD_PLAN "§ Armies"; tab-rebuild WO C4;
// retyped onto the real `Params::Army` by STEP20 (`ENTITY_AUTHORING_PARAMS_SPEC.md`).
//
// It edits two recipe slices — `recipe.armies` (`Params::Army`, name/alias/faction/color/
// resources) and `recipe.unitRules` (`Params::UnitRule`, whose `armyIndex` is a POSITIONAL index
// into `recipe.armies` — ScatterRule_PARAMS.h). The army stack is a DraggableList (ordered, tens
// of rows, every row a drop target); the per-army unit rules are a VirtualList; the unit picker is
// an IconGrid (ArmiesTab_Units_UI.h).
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing type; reported, not invented):
//  1. Hand-placed `Army.groups`/`UnitGroup`/`UnitTransform` authoring has NO widget here (STEP20
//     ruling #8, out of scope) — it stays reachable only by hand-editing/import until a dedicated
//     ticket designs that canvas/manual-entry UI. `DrawArmySettings` binds the army's own fields
//     only, and none of them notify `Pipeline::PreviewDriver`: no stage hashes a team color, alias
//     or resource total (SCOPE NOTE was "no home"; now it has one, `Army_PARAMS.h`, but nothing
//     downstream reads it yet — asking for a regeneration a tint cannot affect is the "cheap tweak
//     triggers a full regen" defect).
//  2. The GAMEDATA ROOT is reported to the host, never read here: loading gamedata is the IO
//     layer's job (ASSET_LOADING_SPEC) and the atlas manifest belongs to the app shell.
//  3. `Ui::IconGridState` carries ONE selected id, so the plan's multi-select Add Units modal
//     appends one rule per confirm; multi-select is a shared-widget work-order.
#pragma once
#include <string>
#include <vector>
#include "ArmiesTab_Units_UI.h"
#include "ColorSwatch_UI.h"
#include "FilePathPicker_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/Army_PARAMS.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

// The factions the ratified enum names, in enum order (UNIT_PROP_MARKER_DATA_SPEC "Factions").
// STEP20 fix: was `{"UEF","Cybran","Aeon"}`, Supreme Commander names left over from v1 — harmless
// while `factionIndex` was a meaningless UI-only int, but a real lie once cast into `Params::
// Faction` and round-tripped.
enum : int { kArmyFactionCount = 3 };
inline const char* const armyFactionLabels[kArmyFactionCount] = { "Chosen", "Guard", "EDA" };

struct ArmiesTabState {
    SectionState          globalSection;
    SectionState          armySection;
    std::string           gamedataDirectory;                 // SCOPE NOTE 2
    FilePathPickerOptions gamedataOptions;
    ColorSwatchOptions    armyColorOptions;
    ScalarSliderRange     alloysRange{ 0.0f, 100000.0f, 1.0f };
    ScalarSliderRange     energyRange{ 0.0f, 1000000.0f, 1.0f };

    int               selectedArmyIndex = -1;
    // ONE shared toggle set for the currently-selected army's detail section — not per-row: only
    // the selected army's settings ever draw, the same posture `ArmyUnitListState`/`LayerEditorState`
    // already use for a single-selection editor over a real PARAMS vector (STEP20 ruling #1).
    RealtimeToggle    armyColorToggle;
    RealtimeToggle    alloysToggle;
    RealtimeToggle    energyToggle;
    ArmyUnitListState units;
};

// The army the per-army controls edit, or null when the selection points at nothing.
inline Params::Army* SelectedArmy(std::vector<Params::Army>& armies, int selectedArmyIndex) {
    if (selectedArmyIndex < 0 || selectedArmyIndex >= static_cast<int>(armies.size())) return nullptr;
    return &armies[static_cast<std::size_t>(selectedArmyIndex)];
}

// The label an army row shows — never empty (Constitution §6).
inline const char* ArmyRowLabel(const Params::Army& army) {
    return army.name.empty() ? "Army" : army.name.c_str();
}

// The name "Add Army" seeds a fresh row with, before the shared uniqueness repair runs — a thin
// domain wrapper over the shared cross-entity template (UniqueNameList_UI.h, STEP20 ruling #5),
// mirroring AreasTab_List_UI.h's `NextAreaName`.
inline std::string NextArmyName(int armyCount) { return NextUniqueLabel("Army", armyCount); }

// Repairs `recipe.unitRules` after an army row is removed: rules that spawned for the removed
// army are DROPPED (they can no longer name an owner) and every rule above it shifts down one, so
// no rule is silently re-pointed at a different army. Pure, and the reason removing an army is
// testable without a window. Reports whether the recipe moved.
inline bool DropUnitRulesForRemovedArmy(std::vector<Params::UnitRule>& unitRules, int removedArmyIndex) {
    if (removedArmyIndex < 0) return false;
    bool bRecipeMoved = false;
    for (std::size_t ruleIndex = unitRules.size(); ruleIndex > 0u; --ruleIndex) {
        Params::UnitRule& rule = unitRules[ruleIndex - 1u];
        if (rule.armyIndex == removedArmyIndex) {
            unitRules.erase(unitRules.begin() + static_cast<std::ptrdiff_t>(ruleIndex - 1u));
            bRecipeMoved = true;
        } else if (rule.armyIndex > removedArmyIndex) {
            --rule.armyIndex;
            bRecipeMoved = true;
        }
    }
    return bRecipeMoved;
}

// Keeps every rule's armyIndex correct after `recipe.armies` is reordered from source to target
// (the exact same erase-then-insert move ApplyDraggableListSignal performs on the armies vector
// itself) — the Reorder-signal counterpart to DropUnitRulesForRemovedArmy's Delete-signal repair.
// STEP20 ruling #4: a real, pre-existing bug (dragging an army row never renumbered armyIndex),
// exact implementation UI-Expert-provided.
inline bool RenumberUnitRuleArmyIndicesForReorder(std::vector<Params::UnitRule>& unitRules,
                                                   int sourceArmyIndex, int targetArmyIndex,
                                                   int armyCount) {
    if (sourceArmyIndex < 0 || sourceArmyIndex >= armyCount) return false;
    int clampedTarget = targetArmyIndex;
    if (clampedTarget < 0) clampedTarget = 0;
    if (clampedTarget > armyCount - 1) clampedTarget = armyCount - 1;
    if (clampedTarget == sourceArmyIndex) return false;
    bool bRecipeMoved = false;
    for (Params::UnitRule& rule : unitRules) {
        if (rule.armyIndex == sourceArmyIndex) { rule.armyIndex = clampedTarget; bRecipeMoved = true; }
        else if (sourceArmyIndex < clampedTarget && rule.armyIndex > sourceArmyIndex
                 && rule.armyIndex <= clampedTarget) { --rule.armyIndex; bRecipeMoved = true; }
        else if (sourceArmyIndex > clampedTarget && rule.armyIndex >= clampedTarget
                 && rule.armyIndex < sourceArmyIndex) { ++rule.armyIndex; bRecipeMoved = true; }
    }
    return bRecipeMoved;
}

// `iconManifest` is nullable: with no resident atlas the unit picker degrades to the typed tpId.
void DrawArmiesTab(Params::MapRecipe& recipe, ArmiesTabState& state,
                   Pipeline::PreviewDriver* previewDriver,
                   const IconAtlasManifest* iconManifest = nullptr);

} // namespace Ui
} // namespace SanmapGen
