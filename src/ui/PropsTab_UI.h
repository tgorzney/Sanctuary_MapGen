// PropsTab_UI.h — the prop tab: the manual prop layers, the procedural prop stack and the decal
// stack. Layer: UI. Accuracy class: Visual. It edits two recipe slices — `recipe.propRules`
// (`Params::PropRule`: density + slope/height gates + Poisson spacing + the water/cliff gates)
// and, through PropsTab_Decals_UI, `recipe.decalRules`. TAB_REBUILD_PLAN "§ Props"; extended by
// tab-rebuild WO C4.
//
// Both procedural stacks are DraggableLists — rule ORDER decides which rule claims a contested
// position first, so every row is a drop target — while the read-only resolved transform list is
// a VirtualList and the template pickers are IconGrids. The tier of a committed edit is derived
// by Pipeline::PreviewDriver from the stage parameter hashes, never here.
//
// ICON PICKER SCOPE (ARCH §8.4): the manifest belongs to the app shell (M5-7) and is passed in,
// nullable; it carries an `iconId` and nothing maps that id back to a game `tpId`, so the grid
// reports the selection while the tpId itself is typed. Wiring the two needs a manifest that
// carries the tpId — a work-order this one does not own.
//
// FURTHER SCOPE NOTES (ARCH §8.4 — reported, not invented):
//  1. `Params::PropRule` has NO separate "physics simulate" flag and NO collision TAG: the one
//     gameplay flag the tree models is `ScatterTransform::bCollidable`, drawn once in the shared
//     Instance Transform block. A second control over it would be a rival toggle (ARCH §4).
//  2. Manual prop GROUPS have no PARAMS home — see PropsTab_Manual_UI.h.
#pragma once
#include "IconGridWidget_UI.h"
#include "LabelledDialWidget_UI.h"
#include "PropsTab_Decals_UI.h"
#include "PropsTab_Manual_UI.h"
#include "PropsTab_Rules_UI.h"
#include "RangeSliderWidget_UI.h"
#include "../params/ScatterRule_PARAMS.h"

namespace SanmapGen {
namespace Data { class PlacementInstances; }
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct PropsTabState {
    // Every limit and metric is a setting, never a literal at a use site (Constitution §8).
    float ruleRowHeight  = 22.0f;
    float ruleListHeight = 140.0f;
    float iconGridHeight = 180.0f;
    RangeSliderBounds slopeBounds{ 0.0f, 89.9f, 0.1f };
    RangeSliderBounds heightBounds{ 0.0f, 1.0f, 0.001f };
    DialRange densityRange{ 0.0f, 1.0f, 0.0f, 400.0f };
    DialRange spacingRange{ 0.0f, 64.0f, 0.0f, 600.0f };
    DialRange obstacleDistanceRange{ 0.0f, 128.0f, 0.0f, 600.0f };
    DialRange nearCliffDistanceRange{ 0.0f, 64.0f, 0.0f, 600.0f };

    RealtimeToggle slopeToggle;
    RealtimeToggle heightToggle;
    RealtimeToggle densityToggle;
    RealtimeToggle spacingToggle;
    RealtimeToggle obstacleDistanceToggle;
    RealtimeToggle nearCliffDistanceToggle;

    IconGridState iconGridState;
    int selectedRuleIndex = 0;
    RangeSliderValues slopeValues{ 0.0f, 89.9f };
    RangeSliderValues heightValues{ 0.0f, 1.0f };

    // WO C4 additions: the two other stacks the tab hosts, and the blocks shared with every
    // other placement tab (PlacementRuleSections_UI.h).
    SectionState            ruleStackSection;
    PropRuleDetailState     ruleDetail;
    PlacementGateState      gate;
    PlacementTransformState transform;
    ManualPropLayersState   manualLayers;
    DecalRuleStackState     decalStack;
};

// rule -> widget mirrors (the paired min/max fields the range sliders edit).
inline void LoadPropRuleValues(const Params::PropRule& rule, PropsTabState& state) {
    state.slopeValues.minimumValue  = rule.minSlope;
    state.slopeValues.maximumValue  = rule.maxSlope;
    state.heightValues.minimumValue = rule.minHeight;
    state.heightValues.maximumValue = rule.maxHeight;
}

// widget mirrors -> rule. Reports whether the recipe actually moved.
inline bool StorePropRuleValues(const PropsTabState& state, Params::PropRule& rule) {
    const bool bMoved = state.slopeValues.minimumValue  != rule.minSlope
                     || state.slopeValues.maximumValue  != rule.maxSlope
                     || state.heightValues.minimumValue != rule.minHeight
                     || state.heightValues.maximumValue != rule.maxHeight;
    rule.minSlope  = state.slopeValues.minimumValue;
    rule.maxSlope  = state.slopeValues.maximumValue;
    rule.minHeight = state.heightValues.minimumValue;
    rule.maxHeight = state.heightValues.maximumValue;
    return bMoved;
}

// The rule the detail controls edit, or null when the selection points at nothing.
Params::PropRule* SelectedPropRule(std::vector<Params::PropRule>& propRules,
                                   const PropsTabState& state);

// `iconManifest` and `placedProps` are both nullable: with no resident atlas the picker degrades
// to the typed tpId, and before the first generation the transform list simply says so.
void DrawPropsTab(Params::MapRecipe& recipe, PropsTabState& state,
                  Pipeline::PreviewDriver* previewDriver,
                  const IconAtlasManifest* iconManifest = nullptr,
                  const Data::PlacementInstances* placedProps = nullptr);

} // namespace Ui
} // namespace SanmapGen
