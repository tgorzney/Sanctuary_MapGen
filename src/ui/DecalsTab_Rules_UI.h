// DecalsTab_Rules_UI.h — the Decal Rules stack, part of the standalone Decals tab (ARCH §20).
// Layer: UI. Accuracy class: Visual. It edits exactly one recipe slice — `recipe.decalRules`.
// TAB_REBUILD_PLAN "§ Props · Decal Rules stack" (Decals split out of the Props tab per §20).
//
// A decal rule is a prop rule without the water/cliff affinities
// (`Params::DecalRule` carries neither field), so the stack is drawn from the same shared blocks —
// DraggableList for the ordered stack, RangeSlider/Dial for the gates, and the shared gate,
// transform and template blocks from PlacementRuleSections_UI.h.
#pragma once
#include "LabelledDialWidget_UI.h"
#include "PlacementRuleSections_UI.h"
#include "../params/ScatterRule_PARAMS.h"
#include <vector>

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct DecalRuleStackState {
    SectionState      stackSection;
    SectionState      gateSection;
    RangeSliderBounds slopeBounds{ 0.0f, 89.9f, 0.1f };
    RangeSliderBounds heightBounds{ 0.0f, 1.0f, 0.001f };
    DialRange         densityRange{ 0.0f, 1.0f, 0.0f, 400.0f };
    DialRange         spacingRange{ 0.0f, 64.0f, 0.0f, 600.0f };

    RealtimeToggle slopeToggle;
    RealtimeToggle heightToggle;
    RealtimeToggle densityToggle;
    RealtimeToggle spacingToggle;

    PlacementGateState      gate;
    PlacementTransformState transform;
    IconGridState           iconGridState;
    float iconGridHeight    = 180.0f;
    int   selectedRuleIndex = 0;
    RangeSliderValues slopeValues{ 0.0f, 89.9f };
    RangeSliderValues heightValues{ 0.0f, 1.0f };
};

// rule -> widget mirrors (the paired min/max fields the range sliders edit).
inline void LoadDecalRuleValues(const Params::DecalRule& rule, DecalRuleStackState& state) {
    state.slopeValues.minimumValue  = rule.minSlope;
    state.slopeValues.maximumValue  = rule.maxSlope;
    state.heightValues.minimumValue = rule.minHeight;
    state.heightValues.maximumValue = rule.maxHeight;
}

// widget mirrors -> rule. Reports whether the recipe actually moved.
inline bool StoreDecalRuleValues(const DecalRuleStackState& state, Params::DecalRule& rule) {
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

// The rule the detail sections edit, or null when the selection points at nothing.
Params::DecalRule* SelectedDecalRule(std::vector<Params::DecalRule>& decalRules,
                                     const DecalRuleStackState& state);

// `iconManifest` is nullable: with no resident atlas the picker degrades to the typed tpId.
void DrawDecalRuleStack(std::vector<Params::DecalRule>& decalRules, DecalRuleStackState& state,
                        Pipeline::PreviewDriver* previewDriver,
                        const IconAtlasManifest* iconManifest);

} // namespace Ui
} // namespace SanmapGen
