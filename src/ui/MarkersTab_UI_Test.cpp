// MarkersTab_UI_Test.cpp — tab-rebuild WO C4 acceptance, part 1: the Markers tab. Every check
// drives the tab's PURE logic — the rule<->widget mirrors, the enum mirrors, the label fallbacks,
// the global scale rows and the placed-list selection fence — so the binary needs no imgui frame,
// no window and no GL context, exactly like the M5-6 parameter-tab tests.
// Owns main() and `Check`/`failureCount`, shared with the sibling TU MarkersTab_RuleLayers_UI_Test.cpp
// (STEP80's two-level list acceptance) — ARCH §1.5's "one binary, split translation units", the same
// pattern ParameterTabs_UI_Test.cpp's four files already use.
#include "MarkersTab_UI.h"
#include "ListWidget_TestFrame_UI.h"
#include "../params/MapRecipe_PARAMS.h"
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
    Check(state.ruleDetail.areaRadiusBounds.upperLimit >= 500.0f,
          "the combined area radius range slider reaches the plan's limits (both the old"
          " 200 minimum-only and 500 maximum-only ceilings)");
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
    Check(state.ruleDetail.areaRadiusToggle.IsRealtimeEnabled()
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

// Human's own bug report (STEP152 correction) — "+ Instance" for a non-Alloy type was silently
// landing under an ALLOY layer: ResolveAddInstanceLayerIndex used to fall back to a bare `0` (i.e.
// markerLayers[0], whichever Layer happens to sit at global index 0, roster-wide) instead of a real
// "no specific layer". Both fallback branches must now report -1, never a position.
void RunResolveAddInstanceLayerIndexChecks() {
    std::vector<Params::MarkerInstanceLayer> markerLayers(3);
    markerLayers[0].markerTypeName = "Alloy";
    markerLayers[1].markerTypeName = "Plasma";
    markerLayers[2].markerTypeName = "Alloy";

    Check(ResolveAddInstanceLayerIndex(markerLayers, 1, "Plasma") == 1,
         "a selected Layer typed to THIS Type-section is used as-is");
    Check(ResolveAddInstanceLayerIndex(markerLayers, 0, "Plasma") == -1,
         "a selected Layer typed to a DIFFERENT Type-section falls back to -1, never that Layer's own "
         "position (the reported cross-type contamination: markerLayers[0] happening to be Alloy)");
    Check(ResolveAddInstanceLayerIndex(markerLayers, -1, "Plasma") == -1,
         "no selection at all (-1) falls back to -1, not markerLayers[0]");
    Check(ResolveAddInstanceLayerIndex(markerLayers, 99, "Plasma") == -1,
         "an out-of-range selection falls back to -1, not markerLayers[0]");
    Check(ResolveAddInstanceLayerIndex({}, 0, "Plasma") == -1,
         "an empty markerLayers vector falls back to -1 too — there is no markerLayers[0] to guess at");
}

// Human's own bug report — Alloy markers vanishing from the Markers tab on a real map import: a
// real (non-SanGen) `.sanmap` names its groups in the PLURAL ("Alloys"/"Plasmas"), but every
// Type-section is keyed by the singular form. `CanonicalMarkerTypeSectionName` is the fold every
// Type-section-membership comparison now goes through (DrawBaseSectionManualInstanceList's
// `group.name` filter, FindOrCreateMarkerInstanceGroupByName's lookup,
// Io::ReconcileMarkerLayers's markerTypeName — MarkersTab_UI.cpp / MapImporter_MarkerLayerReconcile_IO.cpp).
void RunCanonicalMarkerTypeSectionNameChecks() {
    Check(Params::CanonicalMarkerTypeSectionName("Alloys") == "Alloy",
         "a real import's plural Alloy group name folds to the singular Type-section name");
    Check(Params::CanonicalMarkerTypeSectionName("Plasmas") == "Plasma",
         "and likewise for Plasma");
    Check(Params::CanonicalMarkerTypeSectionName("Spawns") == Params::kSpawnMarkerGroupName,
         "and Spawn's plural form, though no real map has been seen to use it");
    Check(Params::CanonicalMarkerTypeSectionName("Alloy") == "Alloy"
         && Params::CanonicalMarkerTypeSectionName("Plasma") == "Plasma"
         && Params::CanonicalMarkerTypeSectionName(Params::kSpawnMarkerGroupName) == Params::kSpawnMarkerGroupName,
         "SanGen's own already-singular names pass through unchanged");
    Check(Params::CanonicalMarkerTypeSectionName("Generic") == "Generic",
         "an unrelated/freeform group name passes through unchanged — this is alias resolution, not a taxonomy");
}

// STEP208 — the Type-section header's own "+ Layer" -> "Procedural" click (buttons.
// bAddProceduralLayerClicked, DrawMarkersTab's own handler, MarkersTab_UI.cpp), driven through the
// REAL `DrawMarkersTab` rather than a small isolated widget, since that handler is inline in that
// one function with no smaller exported seam. The "+ Layer" SmallButton opens a popup
// ("addLayerTypePopup") holding "Manual"/"Procedural" MenuItems buried arbitrarily deep in the
// tab's own layout (Globals + up to 3 Type-sections) — rather than hunting for the SmallButton's
// own screen rect, this exploits imgui's own popup contract instead: `ImGui::OpenPopup` records
// ONLY the mouse position at the moment it is called (FindBestWindowPosForPopup) and a POPUP is a
// window entirely DETACHED from its opener's layout, so priming the SAME id (matching the "markersTab"
// -> "Alloy" PushID stack DrawMarkersTab's own loop pushes, by string content, not call site) open at
// a chosen mouse position makes the REAL `DrawRightAlignedTypeSectionHeaderButtons`'s own
// `if (ImGui::BeginPopup("addLayerTypePopup"))` render the SAME two real MenuItems at a position this
// test can predict WITHOUT ever locating "+ Layer" itself: measured once via a throwaway reference
// popup (mirroring FilePathPicker_UI_Test.cpp's own "reference sequence, drawn directly, to learn
// coordinates a wrapped call won't expose" technique), opened at that exact same mouse position, with
// the exact same two-item content, so imgui's position-from-mouse popup placement lands identically.
struct AddProceduralLayerPopupGeometry {
    ImVec2 proceduralItemCenter;
};

AddProceduralLayerPopupGeometry MeasureAddLayerPopupGeometry(ImVec2 openMousePosition) {
    AddProceduralLayerPopupGeometry geometry;
    HeadlessImguiSession session;   // its own context — never shared with the real drive below
    HeadlessMouseState openMouse; openMouse.position = openMousePosition;
    RunHeadlessFrame(openMouse, ImVec2(400.0f, 200.0f), [&] {
        ImGui::OpenPopup("referenceAddLayerTypePopup");
        if (ImGui::BeginPopup("referenceAddLayerTypePopup")) {
            ImGui::MenuItem("Manual");
            ImGui::MenuItem("Procedural");
            const ImVec2 itemMin = ImGui::GetItemRectMin();
            const ImVec2 itemMax = ImGui::GetItemRectMax();
            geometry.proceduralItemCenter =
                ImVec2((itemMin.x + itemMax.x) * 0.5f, (itemMin.y + itemMax.y) * 0.5f);
            ImGui::EndPopup();
        }
    });
    return geometry;
}

void RunAddProceduralLayerHeaderButtonClickThroughChecks() {
    HeadlessImguiSession session;
    const ImVec2 openMousePosition(300.0f, 300.0f);
    const AddProceduralLayerPopupGeometry geometry = MeasureAddLayerPopupGeometry(openMousePosition);

    Params::MapRecipe recipe;
    MarkersTabState state;
    const ImVec2 windowSize(1200.0f, 400.0f);

    // Frame 1: prime the REAL popup open at the reference measurement's own mouse position — same
    // "markersTab" -> "Alloy" PushID stack DrawMarkersTab's own Type-section loop pushes for the
    // "Alloy" row (rowIndex 0, markerGlobalScaleRowLabels[0], MarkersTab_Globals_UI.h) — so the id
    // `ImGui::OpenPopup("addLayerTypePopup")` computes here is bit-for-bit the id DrawMarkersTab's own
    // `BeginPopup("addLayerTypePopup")` computes for Alloy's row, regardless of neither ever having
    // clicked "+ Layer" itself this frame.
    HeadlessMouseState openMouse; openMouse.position = openMousePosition;
    RunHeadlessFrame(openMouse, windowSize, [&] {
        ImGui::PushID("markersTab");
        ImGui::PushID("Alloy");
        ImGui::OpenPopup("addLayerTypePopup");
        ImGui::PopID();
        ImGui::PopID();
        DrawMarkersTab(recipe, state, nullptr);
    });

    // Hover -> press -> release over "Procedural"'s measured center — the same click-frame technique
    // ClickAddRuleLayerButton (MarkersTab_RuleLayers_UI_Test.cpp) already uses, and the same "a
    // MenuItem fires on release, like a Button" contract FilePathPicker_UI_Test.cpp's own popup click
    // already established.
    HeadlessMouseState hover;   hover.position = geometry.proceduralItemCenter;
    HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
    HeadlessMouseState release = hover; release.bLeftButtonDown = false;
    RunHeadlessFrame(hover,   windowSize, [&] { DrawMarkersTab(recipe, state, nullptr); });
    RunHeadlessFrame(press,   windowSize, [&] { DrawMarkersTab(recipe, state, nullptr); });
    RunHeadlessFrame(release, windowSize, [&] { DrawMarkersTab(recipe, state, nullptr); });

    Check(!recipe.markerRuleLayers.empty(),
          "clicking \"Procedural\" in the Type-section's own \"+ Layer\" popup pushed a rule layer");
    Check(!recipe.markerRuleLayers.empty() && recipe.markerRuleLayers.back().rules.size() == 1,
          "the layer the Type-section's own \"+ Layer\" button pushes is seeded with exactly one "
          "default rule (STEP208)");
}

} // namespace

int main() {
    RunRuleMirrorChecks();
    RunPlanLimitChecks();
    RunEnumMirrorChecks();
    RunSelectionFenceChecks();
    RunRealtimeDefaultChecks();
    RunResolveAddInstanceLayerIndexChecks();
    RunCanonicalMarkerTypeSectionNameChecks();
    RunMarkerRuleLayerAcceptanceChecks();
    RunGlobalMarkerScaleRowFieldsAcceptanceChecks();
    RunAddProceduralLayerHeaderButtonClickThroughChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
