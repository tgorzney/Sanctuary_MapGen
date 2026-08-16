// MarkersTab_UI.h — the marker-rule tab: the rule list plus the selected rule's gates.
// Layer: UI. Accuracy class: Visual. Edits exactly one recipe slice — `recipe.markerRules`.
// The list is the shared VirtualList (M5-2), so a rule set of any length costs O(visible rows);
// every scalar is a shared dial or range slider; the template picker is the shared IconGrid.
// The tier of a committed edit is derived by Pipeline::PreviewDriver from the stage parameter
// hashes — this tab contains no flag mapping.
//
// ICON PICKER SCOPE (ARCH §8.4): the atlas manifest is OWNED BY THE APP SHELL (M5-7) — it holds
// the Io::AssetAtlasCache / Sys::AtlasResidency pair — and is passed in, nullable; a tab never
// invents an atlas source. The manifest carries `iconId` only, and nothing in the tree maps an
// icon id back to a game `tpId`, so the grid reports the SELECTION and the tpId itself is typed;
// wiring the two together needs the manifest to carry the tpId, which is a work-order this one
// does not own.
#pragma once
#include "IconGridWidget_UI.h"
#include "LabelledDialWidget_UI.h"
#include "RangeSliderWidget_UI.h"
#include "../params/MarkerRule_PARAMS.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct MarkersTabState {
    // Every limit and metric is a setting, never a literal at a use site (Constitution §8).
    float ruleRowHeight   = 22.0f;
    float ruleListHeight  = 140.0f;
    float iconGridHeight  = 180.0f;
    RangeSliderBounds slopeBounds{ 0.0f, 89.9f, 0.1f };
    RangeSliderBounds heightBounds{ 0.0f, 1.0f, 0.001f };
    DialRange densityRange{ 0.0f, 1.0f, 0.0f, 400.0f };
    DialRange countRange{ 0.0f, 64.0f, 1.0f, 400.0f };
    DialRange clearanceSpacingRange{ 0.0f, 128.0f, 0.0f, 600.0f };
    DialRange obstacleDistanceRange{ 0.0f, 128.0f, 0.0f, 600.0f };

    RealtimeToggle slopeToggle;
    RealtimeToggle heightToggle;
    RealtimeToggle densityToggle;
    RealtimeToggle countToggle;
    RealtimeToggle clearanceSpacingToggle;
    RealtimeToggle obstacleDistanceToggle;

    IconGridState iconGridState;
    int   selectedRuleIndex = 0;
    float countValue        = 4.0f;        // int mirror
    RangeSliderValues slopeValues{ 0.0f, 89.9f };
    RangeSliderValues heightValues{ 0.0f, 1.0f };
};

// rule -> widget mirrors (the paired min/max fields the range sliders edit, and the int count).
inline void LoadMarkerRuleValues(const Params::MarkerRule& rule, MarkersTabState& state) {
    state.countValue = static_cast<float>(rule.count);
    state.slopeValues.minimumValue  = rule.minSlope;
    state.slopeValues.maximumValue  = rule.maxSlope;
    state.heightValues.minimumValue = rule.minHeight;
    state.heightValues.maximumValue = rule.maxHeight;
}

// widget mirrors -> rule. Reports whether the recipe actually moved.
inline bool StoreMarkerRuleValues(const MarkersTabState& state, Params::MarkerRule& rule) {
    const int count = static_cast<int>(ClampDialValue(state.countValue, state.countRange) + 0.5f);
    const bool bMoved = count != rule.count
                     || state.slopeValues.minimumValue  != rule.minSlope
                     || state.slopeValues.maximumValue  != rule.maxSlope
                     || state.heightValues.minimumValue != rule.minHeight
                     || state.heightValues.maximumValue != rule.maxHeight;
    rule.count     = count;
    rule.minSlope  = state.slopeValues.minimumValue;
    rule.maxSlope  = state.slopeValues.maximumValue;
    rule.minHeight = state.heightValues.minimumValue;
    rule.maxHeight = state.heightValues.maximumValue;
    return bMoved;
}

// The rule the detail controls edit, or null when the selection points at nothing.
Params::MarkerRule* SelectedMarkerRule(std::vector<Params::MarkerRule>& markerRules,
                                       const MarkersTabState& state);

// `iconManifest` is nullable: with no resident atlas the picker degrades to the typed tpId.
void DrawMarkersTab(Params::MapRecipe& recipe, MarkersTabState& state,
                    Pipeline::PreviewDriver* previewDriver,
                    const IconAtlasManifest* iconManifest = nullptr);

} // namespace Ui
} // namespace SanmapGen
