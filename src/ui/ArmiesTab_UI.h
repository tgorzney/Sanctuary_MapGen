// ArmiesTab_UI.h — the armies tab: the army stack and, per army, its unit rules.
// Layer: UI. Accuracy class: Visual. TAB_REBUILD_PLAN "§ Armies"; tab-rebuild WO C4.
//
// It edits exactly one recipe slice — `recipe.unitRules`, whose `armyIndex` is the ONLY army
// identity the tree models (ScatterRule_PARAMS.h). The army stack is a DraggableList (ordered,
// tens of rows, every row a drop target); the per-army unit rules are a VirtualList; the unit
// picker is an IconGrid (ArmiesTab_Units_UI.h).
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing type; reported, not invented):
//  1. AN ARMY HAS NO `_PARAMS` HOME. There is no `Params::Army` and no `MapRecipe` slice to hold
//     one, so the name, TEAM COLOR, FACTION, STARTING ALLOYS and STARTING ENERGY the plan lists
//     are CALLER-OWNED UI state the app shell (WO E) fills — the same standing HeightmapTab_UI's
//     global gravity and SystemTab_UI's asset-cache directory already have. They are NOT
//     serialized and they do NOT notify Pipeline::PreviewDriver, because no stage hashes them.
//     What IS recipe content — which army a unit rule spawns for, and how many — is edited
//     against `Params::UnitRule` and does notify. A durable `Army_PARAMS` is its own work-order.
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

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

// The factions the plan names, in its order.
enum : int { kArmyFactionCount = 3 };
inline const char* const armyFactionLabels[kArmyFactionCount] = { "UEF", "Cybran", "Aeon" };

// SCOPE NOTE 1: presentation only. Every field here is host state, not recipe content.
struct ArmyPresentation {
    std::string    name;
    float          teamColor[kColorSwatchChannelCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
    int            factionIndex   = 0;
    float          startingAlloys = 0.0f;
    float          startingEnergy = 0.0f;
    RealtimeToggle teamColorToggle;
    RealtimeToggle startingAlloysToggle;
    RealtimeToggle startingEnergyToggle;
};

struct ArmiesTabState {
    SectionState          globalSection;
    SectionState          armySection;
    std::string           gamedataDirectory;                 // SCOPE NOTE 2
    FilePathPickerOptions gamedataOptions;
    ColorSwatchOptions    teamColorOptions;
    ScalarSliderRange     startingAlloysRange{ 0.0f, 100000.0f, 1.0f };
    ScalarSliderRange     startingEnergyRange{ 0.0f, 1000000.0f, 1.0f };

    std::vector<ArmyPresentation> armies;
    int               selectedArmyIndex = -1;
    ArmyUnitListState units;
};

// The army the per-army controls edit, or null when the selection points at nothing.
inline ArmyPresentation* SelectedArmy(ArmiesTabState& state) {
    if (state.selectedArmyIndex < 0
        || state.selectedArmyIndex >= static_cast<int>(state.armies.size())) return nullptr;
    return &state.armies[static_cast<std::size_t>(state.selectedArmyIndex)];
}

// The label an army row shows — never empty (Constitution §6).
inline const char* ArmyRowLabel(const ArmyPresentation& army) {
    return army.name.empty() ? "Army" : army.name.c_str();
}

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

// `iconManifest` is nullable: with no resident atlas the unit picker degrades to the typed tpId.
void DrawArmiesTab(Params::MapRecipe& recipe, ArmiesTabState& state,
                   Pipeline::PreviewDriver* previewDriver,
                   const IconAtlasManifest* iconManifest = nullptr);

} // namespace Ui
} // namespace SanmapGen
