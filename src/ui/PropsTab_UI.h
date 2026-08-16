// PropsTab_UI.h — the prop-scatter tab: the rule list plus the selected rule's gates.
// Layer: UI. Accuracy class: Visual. Edits exactly one recipe slice — `recipe.propRules`
// (`Params::PropRule`: density + slope/height gates + Poisson spacing + the water/cliff gates).
// Same composition as MarkersTab_UI: shared VirtualList for the list, shared RangeSlider /
// LabelledDial for the scalars, shared IconGrid for the template picker. The tier of a committed
// edit is derived by Pipeline::PreviewDriver from the stage parameter hashes, never here.
//
// ICON PICKER SCOPE (ARCH §8.4): the manifest belongs to the app shell (M5-7) and is passed in,
// nullable; it carries an `iconId` and nothing maps that id back to a game `tpId`, so the grid
// reports the selection while the tpId itself is typed. Wiring the two needs a manifest that
// carries the tpId — a work-order this one does not own.
#pragma once
#include "IconGridWidget_UI.h"
#include "LabelledDialWidget_UI.h"
#include "RangeSliderWidget_UI.h"
#include "../params/ScatterRule_PARAMS.h"

namespace SanmapGen {
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

// `iconManifest` is nullable: with no resident atlas the picker degrades to the typed tpId.
void DrawPropsTab(Params::MapRecipe& recipe, PropsTabState& state,
                  Pipeline::PreviewDriver* previewDriver,
                  const IconAtlasManifest* iconManifest = nullptr);

} // namespace Ui
} // namespace SanmapGen
