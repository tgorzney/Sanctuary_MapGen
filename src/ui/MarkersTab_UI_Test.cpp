// MarkersTab_UI_Test.cpp — tab-rebuild WO C4 acceptance, part 1: the Markers tab. Every check
// drives the tab's PURE logic — the rule<->widget mirrors, the enum mirrors, the label fallbacks,
// the global scale rows and the placed-list selection fence — so the binary needs no imgui frame,
// no window and no GL context, exactly like the M5-6 parameter-tab tests.
// Owns main() and `Check`/`failureCount`, shared with the sibling TU MarkersTab_RuleLayers_UI_Test.cpp
// (STEP80's two-level list acceptance) — ARCH §1.5's "one binary, split translation units", the same
// pattern ParameterTabs_UI_Test.cpp's four files already use.
#include "MarkersTab_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

void RunMarkerRuleLayerAcceptanceChecks();   // MarkersTab_RuleLayers_UI_Test.cpp
void RunGlobalMarkerScaleRowFieldsAcceptanceChecks();   // MarkersTab_GlobalScaleRowFields_UI_Test.cpp

namespace {

// The mirrors are the whole contract between a range slider and PARAMS: a load followed by a store
// must be the identity, and a store must report honestly whether the recipe actually moved —
// a lie either way is a lost edit or a needless regeneration.
void RunRuleMirrorChecks() {
    Params::MarkerRule rule;
    rule.count = 12; rule.minSlope = 5.0f; rule.maxSlope = 40.0f;
    rule.minHeight = 0.25f; rule.maxHeight = 0.75f;

    MarkersTabState state;
    LoadMarkerRuleValues(rule, state);
    Check(state.countValue == 12.0f && state.slopeValues.minimumValue == 5.0f
          && state.slopeValues.maximumValue == 40.0f
          && state.heightValues.minimumValue == 0.25f
          && state.heightValues.maximumValue == 0.75f,
          "every gate field reaches its widget mirror");
    Check(!StoreMarkerRuleValues(state, rule),
          "storing back what was loaded reports no move, so it costs no regeneration");

    state.slopeValues.maximumValue = 41.0f;
    Check(StoreMarkerRuleValues(state, rule) && rule.maxSlope == 41.0f,
          "and a real edit reports the move and lands on the rule");

    // The count mirror is a float the dial edits; the store is what puts it back on the lattice.
    state.countValue = 7.6f;
    StoreMarkerRuleValues(state, rule);
    Check(rule.count == 8, "the float count mirror rounds onto a whole marker count");
    state.countValue = -50.0f;
    StoreMarkerRuleValues(state, rule);
    Check(rule.count == 0, "and a value below the dial's range is clamped, never stored raw");
}

// TAB_REBUILD_PLAN "§ Markers": Count 1-1000, Clearance Spacing 0-500, Area Radius Min 1-200 /
// Max 1-500, Map Edge Padding 0-200, Density 0-1, Gradient Radius 0.1-1000, Strength/Contrast to 5.
void RunPlanLimitChecks() {
    MarkersTabState state;
    Check(state.countRange.maximumValue >= 1000.0f, "the count dial reaches the plan's 1000");
    Check(state.clearanceSpacingRange.maximumValue >= 500.0f,
          "clearance spacing reaches the plan's 500");
    Check(state.densityRange.minimumValue == 0.0f && state.densityRange.maximumValue == 1.0f,
          "density carries the plan's 0-1");
    Check(state.ruleDetail.areaRadiusMinimumRange.maximumValue >= 200.0f
          && state.ruleDetail.areaRadiusMaximumRange.maximumValue >= 500.0f,
          "both area radius sliders reach the plan's limits");
    Check(state.gate.edgePaddingRange.maximumValue >= 200.0f,
          "map edge padding reaches the plan's 200");
    Check(state.ruleDetail.focusRadiusRange.maximumValue >= 1000.0f
          && state.ruleDetail.focusStrengthRange.maximumValue >= 5.0f
          && state.ruleDetail.focusContrastRange.maximumValue >= 5.0f,
          "the focus gradient shaping sliders reach theirs");
    Check(state.globals.iconScaleRange.minimumValue == 0.1f
          && state.globals.iconScaleRange.maximumValue == 10.0f,
          "the global marker scale rows carry the plan's 0.1-10");
}

// A label table is read with an index that came from a file, so every lookup is fenced.
void RunEnumMirrorChecks() {
    Params::MarkerRule rule;
    rule.category      = Params::MarkerCategory::Alloys;
    rule.priority      = Params::MarkerPriority::LeastVariance;
    rule.focusGradient = Params::FocusGradient::Torus;

    MarkerRuleDetailState detail;
    LoadMarkerRuleEnumIndices(rule, detail);
    Check(detail.categoryIndex == 2 && detail.priorityIndex == 2 && detail.focusGradientIndex == 3,
          "all three enum fields reach their combo mirrors");
    Check(MarkerCategoryLabel(Params::MarkerCategory::Spawn) == markerCategoryLabels[1],
          "a category in range reads its own label");
    Check(MarkerCategoryLabel(static_cast<Params::MarkerCategory>(99)) != nullptr,
          "and a category from a longer table falls back rather than reading off the end");
}

// The read-only placed list is indexed by a selection that survives a regeneration, so the fence
// is asserted rather than assumed.
void RunSelectionFenceChecks() {
    MarkersTabGlobals globals;
    Check(!globals.bIconScanRequested, "the tab opens without a pending icon scan");

    Check(ResolvedPlacedMarkerSelection(3, 10) == 3, "a row inside the buffer stays picked");
    Check(ResolvedPlacedMarkerSelection(10, 10) == -1,
          "a selection left over from a longer generation is dropped, not read off the end");
    Check(ResolvedPlacedMarkerSelection(0, 0) == -1, "and an empty buffer picks nothing");
}

// STEP118: RT enabled by default for the Markers-domain toggles, scoped to these five structs
// only — RealtimeToggle's own class default stays off (RtToggleWidget_UI_Test.cpp).
void RunRealtimeDefaultChecks() {
    MarkersTabState state;
    Check(state.slopeToggle.IsRealtimeEnabled() && state.heightToggle.IsRealtimeEnabled()
          && state.densityToggle.IsRealtimeEnabled() && state.countToggle.IsRealtimeEnabled()
          && state.clearanceSpacingToggle.IsRealtimeEnabled()
          && state.obstacleDistanceToggle.IsRealtimeEnabled(),
          "MarkersTabState's six rule-stack toggles default to realtime ON (STEP118)");
    Check(state.ruleDetail.areaRadiusMinimumToggle.IsRealtimeEnabled()
          && state.ruleDetail.areaRadiusMaximumToggle.IsRealtimeEnabled()
          && state.ruleDetail.areaHeightRangeToggle.IsRealtimeEnabled()
          && state.ruleDetail.focusRadiusToggle.IsRealtimeEnabled()
          && state.ruleDetail.focusStrengthToggle.IsRealtimeEnabled()
          && state.ruleDetail.focusContrastToggle.IsRealtimeEnabled(),
          "MarkerRuleDetailState's six toggles default to realtime ON (STEP118)");
    Check(state.globals.scaleRows[0].iconScaleToggle.IsRealtimeEnabled()
          && state.globals.scaleRows[0].previewColorToggle.IsRealtimeEnabled()
          && state.globals.scaleRows[0].selectColorToggle.IsRealtimeEnabled(),
          "MarkerGlobalScaleRow's three toggles default to realtime ON (STEP118, +selectColorToggle STEP134)");
}

} // namespace

int main() {
    RunRuleMirrorChecks();
    RunPlanLimitChecks();
    RunEnumMirrorChecks();
    RunSelectionFenceChecks();
    RunRealtimeDefaultChecks();
    RunMarkerRuleLayerAcceptanceChecks();
    RunGlobalMarkerScaleRowFieldsAcceptanceChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
