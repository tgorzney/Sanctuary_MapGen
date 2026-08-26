// MarkersTab_ManualLayers_UI_Test.cpp — STEP106 acceptance coverage for the two pure gate/
// quantize functions this ticket adds to MarkersTab_ManualLayers_UI.h: `IsMarkerInstanceLayerLocked`
// (§3) and `QuantizeMarkerPositionToLayerGrid` (§6). Pure logic only — no imgui frame, no window,
// no GL context. Mirrors MarkerLayerIndexRepair_UI_Test.cpp's assertion shape.
#include "MarkersTab_ManualLayers_UI.h"
#include "ListWidget_TestFrame_UI.h"
#include "MarkersTab_ManualLayerHelpers_UI.h"
#include <cmath>
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

bool NearlyEqual(float a, float b) { return std::fabs(a - b) <= 0.001f; }

// Empty vector, out-of-range indices against a non-empty vector, and the ordinary in-range cases.
void RunIsMarkerInstanceLayerLockedChecks() {
    std::vector<Params::MarkerInstanceLayer> emptyLayers;
    Check(!IsMarkerInstanceLayerLocked(emptyLayers, 0), "an empty vector resolves to false at any index");
    Check(!IsMarkerInstanceLayerLocked(emptyLayers, -1), "and at a negative index too");

    std::vector<Params::MarkerInstanceLayer> markerLayers(2);
    markerLayers[1].bLocked = true;
    Check(!IsMarkerInstanceLayerLocked(markerLayers, 0), "index 0 (unlocked) resolves to false");
    Check(IsMarkerInstanceLayerLocked(markerLayers, 1), "index 1 (locked) resolves to true");
    Check(!IsMarkerInstanceLayerLocked(markerLayers, -1),
          "a negative index against a non-empty vector still resolves to false");
    Check(!IsMarkerInstanceLayerLocked(markerLayers, 2),
          "an index at size() resolves to false, never trusted as 'locked'");
}

// bGridSnapEnabled == false leaves the position untouched regardless of value; a non-positive
// gridSnapSizeWorldUnits is a defensive no-op, not a divide-by-zero; an out-of-range layerIndex
// leaves the position unchanged too.
void RunQuantizeMarkerPositionToLayerGridChecks() {
    std::vector<Params::MarkerInstanceLayer> markerLayers(3);
    markerLayers[0].bGridSnapEnabled = false;
    markerLayers[0].gridSnapSizeWorldUnits = 4.0f;
    markerLayers[1].bGridSnapEnabled = true;
    markerLayers[1].gridSnapSizeWorldUnits = 4.0f;
    markerLayers[2].bGridSnapEnabled = true;
    markerLayers[2].gridSnapSizeWorldUnits = 0.0f;

    float worldX = 6.1f, worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, 0, worldX, worldZ);
    Check(NearlyEqual(worldX, 6.1f) && NearlyEqual(worldZ, -3.9f),
          "grid snap off on the layer leaves the position unchanged regardless of value");

    worldX = 6.1f; worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, 1, worldX, worldZ);
    Check(NearlyEqual(worldX, 8.0f) && NearlyEqual(worldZ, -4.0f),
          "(6.1, -3.9) snaps to the nearest 4.0-unit cell: (8.0, -4.0)");

    // Tie case: std::round is ties-away-from-zero, so 2.0 / 4.0 == 0.5 rounds to 1.0, landing on 4.0.
    worldX = 2.0f; worldZ = 0.0f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, 1, worldX, worldZ);
    Check(NearlyEqual(worldX, 4.0f), "an exact tie (2.0 against a 4.0 cell) rounds away from zero to 4.0");

    worldX = 6.1f; worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, 2, worldX, worldZ);
    Check(NearlyEqual(worldX, 6.1f) && NearlyEqual(worldZ, -3.9f),
          "a non-positive gridSnapSizeWorldUnits is a defensive no-op, not a divide-by-zero");

    worldX = 6.1f; worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, -1, worldX, worldZ);
    Check(NearlyEqual(worldX, 6.1f) && NearlyEqual(worldZ, -3.9f),
          "an out-of-range layerIndex leaves the position unchanged");
    worldX = 6.1f; worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, 3, worldX, worldZ);
    Check(NearlyEqual(worldX, 6.1f) && NearlyEqual(worldZ, -3.9f),
          "and so does an index at size()");
}

// STEP126: BuildManualInstanceLayerIndex — three groups, transforms spread across layerIndex
// 0/1/2, including one group with TWO transforms on the same layerIndex. instancesByLayerIndex[0]
// holds exactly the expected (groupIndex, transformIndex) pairs, in encounter order; a layerIndex
// with zero transforms is simply absent from the map (not present with an empty vector) — the
// find() == end() case DrawLayerRowBody's own "(none)" branch depends on.
void RunBuildManualInstanceLayerIndexChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(3);
    markers[0].transforms.push_back(Params::MarkerTransform{});                       // group 0, transform 0, layerIndex 0
    markers[0].transforms.back().layerIndex = 0;
    markers[0].transforms.push_back(Params::MarkerTransform{});                       // group 0, transform 1, layerIndex 1
    markers[0].transforms.back().layerIndex = 1;
    markers[1].transforms.push_back(Params::MarkerTransform{});                       // group 1, transform 0, layerIndex 0
    markers[1].transforms.back().layerIndex = 0;
    markers[2].transforms.push_back(Params::MarkerTransform{});                       // group 2, transform 0, layerIndex 2
    markers[2].transforms.back().layerIndex = 2;

    const ManualInstanceLayerIndex_UI index = BuildManualInstanceLayerIndex(markers);

    const auto layerZeroIt = index.instancesByLayerIndex.find(0);
    Check(layerZeroIt != index.instancesByLayerIndex.end(), "layerIndex 0 is present in the index");
    if (layerZeroIt != index.instancesByLayerIndex.end()) {
        Check(static_cast<int>(layerZeroIt->second.size()) == 2, "layerIndex 0 holds exactly two pairs");
        Check(layerZeroIt->second.size() >= 2
              && layerZeroIt->second[0] == std::pair<int, int>(0, 0)
              && layerZeroIt->second[1] == std::pair<int, int>(1, 0),
              "layerIndex 0's pairs are (0,0) then (1,0), in encounter order");
    }

    const auto layerOneIt = index.instancesByLayerIndex.find(1);
    Check(layerOneIt != index.instancesByLayerIndex.end() && layerOneIt->second.size() == 1
          && layerOneIt->second[0] == std::pair<int, int>(0, 1), "layerIndex 1 holds exactly (0,1)");

    Check(index.instancesByLayerIndex.find(3) == index.instancesByLayerIndex.end(),
          "a layerIndex with zero transforms is absent from the map, not present with an empty vector");
}

// STEP125, ARCH §19.15(c): the `||` composition — type-mismatch-only, bundle-membership-only, both,
// and neither — mirrors IsMarkerRuleLayerRowSuppressed's own four-case shape one tier over
// (MarkersTab_RuleLayers_UI_Test.cpp), on Params::MarkerInstanceLayer.
void RunIsMarkerInstanceLayerRowSuppressedChecks() {
    Params::MarkerInstanceLayer typeMismatchOnly;
    typeMismatchOnly.parentBundleIdentifier = -1;
    typeMismatchOnly.markerTypeName         = "Alloy";
    Check(!IsMarkerInstanceLayerRowSuppressed(typeMismatchOnly, "Alloy"),
          "an ungrouped layer whose own type matches the filter is NOT suppressed");
    Check(IsMarkerInstanceLayerRowSuppressed(typeMismatchOnly, "Plasma"),
          "an ungrouped layer whose own type does NOT match the filter IS suppressed (type-mismatch only)");

    Params::MarkerInstanceLayer bundleMembershipOnly;
    bundleMembershipOnly.parentBundleIdentifier = 5;
    bundleMembershipOnly.markerTypeName         = "Alloy";
    Check(IsMarkerInstanceLayerRowSuppressed(bundleMembershipOnly, "Alloy"),
          "a bundled layer IS suppressed even when its own type matches the filter (bundle-membership only)");

    Params::MarkerInstanceLayer both;
    both.parentBundleIdentifier = 5;
    both.markerTypeName         = "Alloy";
    Check(IsMarkerInstanceLayerRowSuppressed(both, "Plasma"),
          "a bundled layer with a mismatched type IS suppressed (the compound case, not an XOR)");
}

// STEP125: DrawLayerListButtons's new markerTypeNameForNewLayer parameter — mirrors
// DrawAddMarkerRuleLayerButton's own procedural-side check (MarkersTab_RuleLayers_UI_Test.cpp), a
// live headless imgui frame since the button must actually be clicked to push a new row.
struct AddManualLayerButtonFrameResult {
    ImVec2 origin;
    ImVec2 size;
    bool   bReturned = false;
};

AddManualLayerButtonFrameResult RunAddManualLayerButtonFrame(HeadlessMouseState mouse,
        std::vector<Params::MarkerInstanceLayer>& layers, ManualMarkerLayersState& state,
        const std::string& markerTypeNameForNewLayer) {
    AddManualLayerButtonFrameResult result;
    RunHeadlessFrame(mouse, ImVec2(300.0f, 100.0f), [&] {
        result.origin    = ImGui::GetCursorScreenPos();
        result.bReturned = DrawLayerListButtons(layers, state, -1, markerTypeNameForNewLayer);
        result.size      = ImGui::GetItemRectSize();
    });
    return result;
}

void ClickAddManualLayerButton(std::vector<Params::MarkerInstanceLayer>& layers, ManualMarkerLayersState& state,
                               const std::string& markerTypeNameForNewLayer,
                               AddManualLayerButtonFrameResult& outClickedResult) {
    const AddManualLayerButtonFrameResult settle =
        RunAddManualLayerButtonFrame(HeadlessMouseState(), layers, state, markerTypeNameForNewLayer);
    const ImVec2 center(settle.origin.x + settle.size.x * 0.5f, settle.origin.y + settle.size.y * 0.5f);
    HeadlessMouseState hover;   hover.position = center;
    HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
    HeadlessMouseState release = hover; release.bLeftButtonDown = false;
    RunAddManualLayerButtonFrame(hover, layers, state, markerTypeNameForNewLayer);
    RunAddManualLayerButtonFrame(press, layers, state, markerTypeNameForNewLayer);
    outClickedResult = RunAddManualLayerButtonFrame(release, layers, state, markerTypeNameForNewLayer);
}

void RunDrawLayerListButtonsTypeSeedChecks() {
    HeadlessImguiSession session;

    std::vector<Params::MarkerInstanceLayer> seededLayers;
    ManualMarkerLayersState seededState;
    AddManualLayerButtonFrameResult seededClick;
    ClickAddManualLayerButton(seededLayers, seededState, "Spawn", seededClick);
    Check(seededClick.bReturned, "clicking Add Marker Layer reports a layer was added");
    Check(!seededLayers.empty() && seededLayers.back().markerTypeName == "Spawn",
          "markerTypeNameForNewLayer = \"Spawn\" lands on the newly pushed layer's own markerTypeName");

    std::vector<Params::MarkerInstanceLayer> defaultLayers;
    ManualMarkerLayersState defaultState;
    AddManualLayerButtonFrameResult defaultClick;
    ClickAddManualLayerButton(defaultLayers, defaultState, "", defaultClick);
    Check(!defaultLayers.empty() && defaultLayers.back().markerTypeName.empty(),
          "the parameter omitted (default, empty) leaves markerTypeName empty — unchanged existing behavior");
}

// STEP118: RT enabled by default for ManualMarkerLayersState's toggles — RealtimeToggle's
// own class default stays off (RtToggleWidget_UI_Test.cpp). STEP127: layerIconScaleToggle deleted
// (dead field, item 2) — five toggles remain, not six.
void RunRealtimeDefaultChecks() {
    ManualMarkerLayersState state;
    Check(state.groupColorToggle.IsRealtimeEnabled()
          && state.selectedLayerColorToggle.IsRealtimeEnabled()
          && state.selectedLayerIconScaleToggle.IsRealtimeEnabled()
          && state.selectedLayerGridSnapToggle.IsRealtimeEnabled()
          && state.fixSymmetryToleranceToggle.IsRealtimeEnabled(),
          "ManualMarkerLayersState's five toggles default to realtime ON (STEP118)");
}

} // namespace

int main() {
    RunIsMarkerInstanceLayerLockedChecks();
    RunQuantizeMarkerPositionToLayerGridChecks();
    RunBuildManualInstanceLayerIndexChecks();
    RunIsMarkerInstanceLayerRowSuppressedChecks();
    RunDrawLayerListButtonsTypeSeedChecks();
    RunRealtimeDefaultChecks();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
