// MarkersTab_ManualLayers_UI_Test.cpp — STEP106 acceptance coverage for the two pure gate/
// quantize functions this ticket adds to MarkersTab_ManualLayers_UI.h: `IsMarkerInstanceLayerLocked`
// (§3) and `QuantizeMarkerPositionToLayerGrid` (§6). Pure logic only — no imgui frame, no window,
// no GL context. Mirrors MarkerLayerIndexRepair_UI_Test.cpp's assertion shape.
#include "MarkersTab_ManualLayers_UI.h"
#include "ListWidget_TestFrame_UI.h"
#include "MarkersTab_ManualLayerHelpers_UI.h"
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "../params/MarkerLink_PARAMS.h"
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

// Constructs a MarkerTransform pinned to `layerIndex` (and optionally an instance-tier `linkIdentifier`)
// — everything else default. Shared by every QuantizeMarkerPositionToLayerGrid/IsMarkerInstanceLocked
// check below.
Params::MarkerTransform MakeTransformAt(int layerIndex, int linkIdentifier = -1) {
    Params::MarkerTransform transform;
    transform.layerIndex = layerIndex;
    transform.linkIdentifier = linkIdentifier;
    return transform;
}

// bGridSnapEnabled == false leaves the position untouched regardless of value; a non-positive
// gridSnapSizeWorldUnits is a defensive no-op, not a divide-by-zero; an out-of-range layerIndex
// leaves the position unchanged too. No Links in play here — STEP246's own Link-tier resolution is
// covered separately by RunQuantizeMarkerPositionToLayerGridLinkTierChecks below.
void RunQuantizeMarkerPositionToLayerGridChecks() {
    std::vector<Params::MarkerInstanceLayer> markerLayers(3);
    markerLayers[0].bGridSnapEnabled = false;
    markerLayers[0].gridSnapSizeWorldUnits = 4.0f;
    markerLayers[1].bGridSnapEnabled = true;
    markerLayers[1].gridSnapSizeWorldUnits = 4.0f;
    markerLayers[2].bGridSnapEnabled = true;
    markerLayers[2].gridSnapSizeWorldUnits = 0.0f;
    const std::vector<Params::MarkerLink> noLinks;

    float worldX = 6.1f, worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, MakeTransformAt(0), noLinks, worldX, worldZ);
    Check(NearlyEqual(worldX, 6.1f) && NearlyEqual(worldZ, -3.9f),
          "grid snap off on the layer leaves the position unchanged regardless of value");

    worldX = 6.1f; worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, MakeTransformAt(1), noLinks, worldX, worldZ);
    Check(NearlyEqual(worldX, 8.0f) && NearlyEqual(worldZ, -4.0f),
          "(6.1, -3.9) snaps to the nearest 4.0-unit cell: (8.0, -4.0)");

    // Tie case: std::round is ties-away-from-zero, so 2.0 / 4.0 == 0.5 rounds to 1.0, landing on 4.0.
    worldX = 2.0f; worldZ = 0.0f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, MakeTransformAt(1), noLinks, worldX, worldZ);
    Check(NearlyEqual(worldX, 4.0f), "an exact tie (2.0 against a 4.0 cell) rounds away from zero to 4.0");

    worldX = 6.1f; worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, MakeTransformAt(2), noLinks, worldX, worldZ);
    Check(NearlyEqual(worldX, 6.1f) && NearlyEqual(worldZ, -3.9f),
          "a non-positive gridSnapSizeWorldUnits is a defensive no-op, not a divide-by-zero");

    worldX = 6.1f; worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, MakeTransformAt(-1), noLinks, worldX, worldZ);
    Check(NearlyEqual(worldX, 6.1f) && NearlyEqual(worldZ, -3.9f),
          "an out-of-range layerIndex leaves the position unchanged");
    worldX = 6.1f; worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, MakeTransformAt(3), noLinks, worldX, worldZ);
    Check(NearlyEqual(worldX, 6.1f) && NearlyEqual(worldZ, -3.9f),
          "and so does an index at size()");
}

// STEP246, ARCH §19.33/§21.9 — instance-tier-first, THEN Layer-tier (itself Link-aware), THEN the
// Layer's own stored field. Grid-snap chosen as the representative case for
// QuantizeMarkerPositionToLayerGrid; IsMarkerInstanceLocked covers the sixth governed field below.
void RunQuantizeMarkerPositionToLayerGridLinkTierChecks() {
    std::vector<Params::MarkerInstanceLayer> markerLayers(1);
    markerLayers[0].bGridSnapEnabled = false;           // the Layer's OWN stored field: snap OFF
    markerLayers[0].gridSnapSizeWorldUnits = 4.0f;
    markerLayers[0].linkIdentifier = 100;                // Layer-tier bound to Link 100

    std::vector<Params::MarkerLink> links(2);
    links[0].identifier = 100; links[0].bGridSnapEnabled = true; links[0].gridSnapSizeWorldUnits = 5.0f;
    links[1].identifier = 200; links[1].bGridSnapEnabled = true; links[1].gridSnapSizeWorldUnits = 2.0f;

    // An instance tagged directly to Link 200 resolves THAT Link's own pair, even though its owning
    // Layer is bound to a DIFFERENT Link (100) — the exact new capability this correction is for.
    float worldX = 3.1f, worldZ = 0.0f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, MakeTransformAt(0, /*linkIdentifier=*/200), links, worldX, worldZ);
    Check(NearlyEqual(worldX, 4.0f),
          "an instance tagged to Link 200 resolves THAT Link's grid (2.0), not its Layer's Link 100");

    // An untagged instance on this same Link-bound Layer still resolves the LAYER's own Link (100) —
    // existing §19.31 Layer-tier behavior, unchanged by this correction.
    worldX = 3.1f; worldZ = 0.0f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, MakeTransformAt(0), links, worldX, worldZ);
    Check(NearlyEqual(worldX, 5.0f), "an untagged instance still resolves its Layer's own bound Link (100)");

    // A dangling instance-tier linkIdentifier (no matching Params::MarkerLink) soft-degrades to the
    // Layer-tier result — never a crash, never a refusal (Constitution §6).
    worldX = 3.1f; worldZ = 0.0f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, MakeTransformAt(0, /*linkIdentifier=*/999), links, worldX, worldZ);
    Check(NearlyEqual(worldX, 5.0f), "a dangling instance linkIdentifier soft-degrades to the Layer-tier result");
}

// The sixth governed field (bLocked) — same three-tier resolution order, exercised through the
// out-of-range-safe IsMarkerInstanceLocked wrapper.
void RunIsMarkerInstanceLockedChecks() {
    std::vector<Params::MarkerInstanceLayer> markerLayers(1);
    markerLayers[0].bLocked = false;
    markerLayers[0].linkIdentifier = 100;

    std::vector<Params::MarkerLink> links(2);
    links[0].identifier = 100; links[0].bLocked = false;
    links[1].identifier = 200; links[1].bLocked = true;

    Check(IsMarkerInstanceLocked(MakeTransformAt(0, /*linkIdentifier=*/200), markerLayers, links),
          "an instance tagged to Link 200 (locked) resolves locked, even though its Layer's own "
          "bound Link (100) is unlocked");
    Check(!IsMarkerInstanceLocked(MakeTransformAt(0), markerLayers, links),
          "an untagged instance still resolves its Layer's own bound Link (100, unlocked)");
    Check(!IsMarkerInstanceLocked(MakeTransformAt(0, /*linkIdentifier=*/999), markerLayers, links),
          "a dangling instance linkIdentifier soft-degrades to the Layer-tier result, never a crash");
    Check(!IsMarkerInstanceLocked(MakeTransformAt(-1), markerLayers, links),
          "an out-of-range layerIndex resolves to false, mirroring IsMarkerInstanceLayerLocked");
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

// STEP145 (human's own bug report — "the buttons on the Layer Header are all overlapping each
// other slightly") — DrawRightAlignedSymmetryColorOverrideCluster's own right-align push used to
// size itself off `ImGui::GetContentRegionAvail()`, which reaches the row's TRUE right edge, PAST
// the built-in [o]/[L]/[X] affordance strip's own reserved kAffordanceStripWidthPixels
// (DraggableListWidget_RowAffordances_UI.h) — overshooting by exactly that strip width and landing
// the cluster on top of it. Reproduces both reservations at the SAME row width production uses
// (`kMarkerLayerHeaderExtraCombinedWidthPixels`, `RenderCollapsibleRow`'s own SameLine contract,
// DraggableListWidget_RowLayout_UI.h) and asserts the cluster's own rightmost edge lands AT OR
// BEFORE the strip's leftmost edge — never past it.
// Runs the exact same geometry check `bPushExaggeratedItemSpacing` optionally wraps in a non-default
// ImGuiStyleVar_ItemSpacing before the frame -- proves the fix (STEP206) reads the LIVE style value
// rather than a hardcoded gap constant: the invariant must hold at BOTH the default AND an exaggerated
// spacing, not just whichever spacing the test constants happened to assume.
void RunUngroupedClusterDoesNotOverlapAffordanceStripCheck(bool bPushExaggeratedItemSpacing = false) {
    HeadlessImguiSession session;
    Params::MarkerInstanceLayer layer;
    ManualMarkerLayersState state;
    bool bAnyCommitted = false;
    DraggableListRow row;   // bLocked/bVisible default (irrelevant to this geometry check)
    DraggableListSignal signal;

    ImVec2 clusterMax, stripMin;
    RunHeadlessFrame(HeadlessMouseState(), ImVec2(400.0f, 100.0f), [&] {
        if (bPushExaggeratedItemSpacing)
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20.0f, 4.0f));
        ImGui::PushID("row");
        ImGui::CollapsingHeader("Some Fairly Long Manual Layer Name",
            ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow);
        const float rowAvailWidthPixels = ImGui::GetContentRegionAvail().x;
        // Mirrors RenderCollapsibleRow's own two calls, in the SAME order (DraggableListWidget_RowLayout_UI.h):
        // the header-extra zone first, then the affordance strip.
        ImGui::SameLine(rowAvailWidthPixels - static_cast<float>(kAffordanceStripWidthPixels)
            - kMarkerLayerHeaderExtraCombinedWidthPixels);
        DrawRightAlignedSymmetryColorOverrideCluster(layer, state, bAnyCommitted);
        clusterMax = ImGui::GetItemRectMax();
        RowLayoutDetail::DrawRowAffordances(row, 0, signal, 0.0f, rowAvailWidthPixels, false);
        stripMin = ImGui::GetItemRectMin();   // the strip's FIRST item, [o]/[-] visibility
        ImGui::PopID();
        if (bPushExaggeratedItemSpacing)
            ImGui::PopStyleVar();
    });

    Check(clusterMax.x <= stripMin.x + 0.5f,
         "the [SYM][COL][swatch] cluster's own rightmost edge lands at or before the built-in "
         "[o]/[L]/[X] strip's leftmost edge -- never past it (STEP145/STEP206)");
}

} // namespace

int main() {
    RunIsMarkerInstanceLayerLockedChecks();
    RunQuantizeMarkerPositionToLayerGridChecks();
    RunQuantizeMarkerPositionToLayerGridLinkTierChecks();
    RunIsMarkerInstanceLockedChecks();
    RunBuildManualInstanceLayerIndexChecks();
    RunIsMarkerInstanceLayerRowSuppressedChecks();
    RunDrawLayerListButtonsTypeSeedChecks();
    RunRealtimeDefaultChecks();
    RunUngroupedClusterDoesNotOverlapAffordanceStripCheck();
    RunUngroupedClusterDoesNotOverlapAffordanceStripCheck(/*bPushExaggeratedItemSpacing=*/true);

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
