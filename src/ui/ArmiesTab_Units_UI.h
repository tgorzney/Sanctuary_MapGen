// ArmiesTab_Units_UI.h — one army's unit rules, and the Add Units picker that appends them.
// Layer: UI. Accuracy class: Visual. It edits exactly one recipe slice — `recipe.unitRules`
// (`Params::UnitRule`, which already carries `armyIndex`). TAB_REBUILD_PLAN "§ Armies".
//
// Unit rules for ONE army are not contiguous in the recipe's array, so the list is virtualized
// over an INDEX list rebuilt each frame (VirtualListWidget_UI.h: "rows are addressed by index, so
// a struct-of-arrays column set virtualizes exactly as well as a contiguous array"). Collecting
// the indices is pure, so the filtering is testable with no imgui frame.
#pragma once
#include "PlacementRuleSections_UI.h"
#include "../params/ScatterRule_PARAMS.h"
#include <vector>

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct ArmyUnitListState {
    SectionState      section;
    SectionState      gateSection;
    float             rowHeight  = 22.0f;   // the TRUE row height: the clipper scrolls with it
    float             listHeight = 140.0f;
    RangeSliderBounds slopeBounds{ 0.0f, 89.9f, 0.1f };
    RangeSliderBounds heightBounds{ 0.0f, 1.0f, 0.001f };
    ScalarSliderRange countRange{ 0.0f, 512.0f, 1.0f };
    ScalarSliderRange spacingRange{ 0.0f, 64.0f, 0.0f };

    RealtimeToggle slopeToggle;
    RealtimeToggle heightToggle;
    RealtimeToggle countToggle;
    RealtimeToggle spacingToggle;

    PlacementGateState      gate;
    PlacementTransformState transform;
    IconGridState           iconGridState;
    float iconGridHeight = 180.0f;

    std::vector<int>  armyRuleIndices;      // rebuilt each frame from the selected army
    RangeSliderValues slopeValues{ 0.0f, 89.9f };
    RangeSliderValues heightValues{ 0.0f, 1.0f };
    int  selectedRuleIndex     = -1;        // an index into `recipe.unitRules`, not into the filter
    int  pendingUnitCount      = 1;         // the Add Units picker's count field
    bool bAddUnitsPickerOpen   = false;
    ScalarSliderRange pendingUnitCountRange{ 1.0f, 512.0f, 1.0f };
    RealtimeToggle    pendingUnitCountToggle;
};

// Every rule belonging to `armyIndex`, in recipe order. Pure: the filter the virtualized list
// walks, assertable without a window.
inline void CollectUnitRuleIndicesForArmy(const std::vector<Params::UnitRule>& unitRules,
                                          int armyIndex, std::vector<int>& outRuleIndices) {
    outRuleIndices.clear();
    if (armyIndex < 0) return;
    const int ruleCount = static_cast<int>(unitRules.size());
    for (int ruleIndex = 0; ruleIndex < ruleCount; ++ruleIndex)
        if (unitRules[static_cast<std::size_t>(ruleIndex)].armyIndex == armyIndex)
            outRuleIndices.push_back(ruleIndex);
}

// rule -> widget mirrors (the paired min/max fields the range sliders edit).
inline void LoadUnitRuleValues(const Params::UnitRule& rule, ArmyUnitListState& state) {
    state.slopeValues.minimumValue  = rule.minSlope;
    state.slopeValues.maximumValue  = rule.maxSlope;
    state.heightValues.minimumValue = rule.minHeight;
    state.heightValues.maximumValue = rule.maxHeight;
}

// widget mirrors -> rule. Reports whether the recipe actually moved.
inline bool StoreUnitRuleValues(const ArmyUnitListState& state, Params::UnitRule& rule) {
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

// The rule the detail sections edit, or null when the selection points outside the array or at a
// rule that belongs to a different army (Constitution §6 — an index is validated, never trusted).
Params::UnitRule* SelectedUnitRule(std::vector<Params::UnitRule>& unitRules, int armyIndex,
                                   const ArmyUnitListState& state);

void DrawArmyUnitList(std::vector<Params::UnitRule>& unitRules, int armyIndex,
                      ArmyUnitListState& state, Pipeline::PreviewDriver* previewDriver,
                      const IconAtlasManifest* iconManifest);

} // namespace Ui
} // namespace SanmapGen
