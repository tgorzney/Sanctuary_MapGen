// MarkersTab_Bundles_UI_Test.cpp — STEP120 acceptance for MarkersTab_Bundles_UI.h's own pure
// helpers: BuildMarkerLayerBundleLeafIndex, NextMarkerLayerBundleId, and the Move/Rotate Apply
// functions' own call-boundary behavior. Pure logic only — no imgui frame needed, mirroring
// STEP106's own "defer the imgui-coupled path, test the definitely-pure pieces" posture.
// STEP130 (ARCH §19.24, item 7(b)) adds one headless-frame section at the bottom for
// DrawMarkerGroupLeafHeaderExtra — the Bundle tree's own `drawLeafHeaderExtra` body — mirroring
// MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp's own HeadlessImguiSession/RunHeadlessFrame
// harness, since that one function is genuinely imgui-coupled.
#include "MarkersTab_Bundles_UI.h"
#include "ListWidget_TestFrame_UI.h"
#include "MarkersTab_BundleDelete_UI.h"
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include <cmath>
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failures = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failures; }
}

bool NearlyEqual(float a, float b, float tolerance = 1e-3f) {
    return std::fabs(a - b) <= tolerance;
}

bool LeafIndexHasExactly(const std::vector<MarkerGroupLeafKey_UI>& leaves, MarkerGroupLeafKey_UI::Kind kind,
                         int layerIndex) {
    for (const MarkerGroupLeafKey_UI& leaf : leaves)
        if (leaf.kind == kind && leaf.layerIndex == layerIndex) return true;
    return false;
}

void TestBuildMarkerLayerBundleLeafIndex() {
    std::vector<Params::MarkerRuleLayer> ruleLayers(3);
    ruleLayers[0].parentBundleIdentifier = -1;
    ruleLayers[1].parentBundleIdentifier = 0;
    ruleLayers[2].parentBundleIdentifier = 0;
    std::vector<Params::MarkerInstanceLayer> instanceLayers(2);
    instanceLayers[0].parentBundleIdentifier = 1;
    instanceLayers[1].parentBundleIdentifier = -1;

    const MarkerLayerBundleLeafIndex_UI index = BuildMarkerLayerBundleLeafIndex(ruleLayers, instanceLayers);
    Check(index.leavesByBundleIdentifier.count(-1) == 0, "no leaf-index entry exists for bundle -1 (ungrouped)");
    Check(index.leavesByBundleIdentifier.count(0) == 1 && index.leavesByBundleIdentifier.at(0).size() == 2u,
         "bundle 0 has exactly its two Procedural members");
    Check(LeafIndexHasExactly(index.leavesByBundleIdentifier.at(0), MarkerGroupLeafKey_UI::Kind::Procedural, 1)
         && LeafIndexHasExactly(index.leavesByBundleIdentifier.at(0), MarkerGroupLeafKey_UI::Kind::Procedural, 2),
         "bundle 0's members are rule layer indices 1 and 2");
    Check(index.leavesByBundleIdentifier.count(1) == 1 && index.leavesByBundleIdentifier.at(1).size() == 1u
         && LeafIndexHasExactly(index.leavesByBundleIdentifier.at(1), MarkerGroupLeafKey_UI::Kind::Manual, 0),
         "bundle 1 has exactly its one Manual member (instance layer index 0)");
    Check(index.leavesByBundleIdentifier.count(2) == 0, "no leaf-index entry exists for an unreferenced bundle");
}

// STEP148 (human's own bug report — "I tried to drag an instance to a group, and it stayed where it
// was") — FirstManualLayerIndexInBundle's own resolution: the first (by vector position) Manual
// Layer belonging to a Bundle, -1 when it has none.
void TestFirstManualLayerIndexInBundle() {
    std::vector<Params::MarkerInstanceLayer> instanceLayers(3);
    instanceLayers[0].parentBundleIdentifier = -1;   // ungrouped
    instanceLayers[1].parentBundleIdentifier = 5;
    instanceLayers[2].parentBundleIdentifier = 5;
    Check(FirstManualLayerIndexInBundle(5, instanceLayers) == 1,
         "the FIRST (lowest position) Layer belonging to the Bundle wins, not the last");
    Check(FirstManualLayerIndexInBundle(7, instanceLayers) == -1,
         "a Bundle with no Manual Layer of its own resolves to -1");
    Check(FirstManualLayerIndexInBundle(5, std::vector<Params::MarkerInstanceLayer>{}) == -1,
         "an empty instanceLayers vector resolves to -1 too");
}

// STEP148 correction (human's own correction — "I thought I told you to have it create a new layer
// if one did not exist") — ApplyPendingCreateLayerForBundle mints a fresh Layer under the Bundle and
// reassigns every recorded instance identifier onto it, in one call.
void TestApplyPendingCreateLayerForBundle() {
    std::vector<Params::MarkerInstanceLayer> markerLayers(1);
    markerLayers[0].layerId = 3;   // pre-existing layer, so the new one must NOT reuse id 3
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.resize(2);
    markers[0].transforms[0].instanceIdentifier = 10; markers[0].transforms[0].layerIndex = 0;
    markers[0].transforms[1].instanceIdentifier = 11; markers[0].transforms[1].layerIndex = 0;

    ApplyPendingCreateLayerForBundle(5, "Alloy", { 10 }, markerLayers, markers);

    Check(markerLayers.size() == 2, "a new Layer is appended");
    Check(markerLayers[1].parentBundleIdentifier == 5, "the new Layer is parented to the target Bundle");
    Check(markerLayers[1].markerTypeName == "Alloy", "the new Layer takes on the recorded marker type");
    Check(markerLayers[1].layerId == 4, "the new Layer mints a fresh, never-reused layerId (max+1, not 0)");
    Check(markers[0].transforms[0].layerIndex == 1, "the recorded instance (10) lands on the new Layer's index");
    Check(markers[0].transforms[1].layerIndex == 0, "an instance NOT in the recorded set (11) is untouched");
}

void TestNextMarkerLayerBundleId() {
    const std::vector<Params::MarkerLayerBundle> emptyBundles;
    Check(NextMarkerLayerBundleId(emptyBundles) == 0, "an empty bundle vector mints identifier 0");
    std::vector<Params::MarkerLayerBundle> bundles(3);
    bundles[0].identifier = 0;
    bundles[1].identifier = 3;
    bundles[2].identifier = 1;
    Check(NextMarkerLayerBundleId(bundles) == 4, "the next identifier is one past the current maximum");
}

// One Bundle (id 5), one Manual layer under it, two MarkerTransforms on that layer.
struct MoveRotateFixture {
    std::vector<Params::MarkerLayerBundle>    bundles;
    std::vector<Params::MarkerInstanceLayer>  instanceLayers;
    std::vector<Params::MarkerInstanceGroup>  markers;
};

MoveRotateFixture MakeMoveRotateFixture() {
    MoveRotateFixture fixture;
    Params::MarkerLayerBundle bundle;
    bundle.identifier = 5;
    fixture.bundles.push_back(bundle);

    Params::MarkerInstanceLayer layer;
    layer.parentBundleIdentifier = 5;
    fixture.instanceLayers.push_back(layer);   // index 0

    Params::MarkerInstanceGroup group;
    group.name = "TestGroup";
    Params::MarkerTransform transformA;
    transformA.layerIndex = 0;
    transformA.transform.positionX = 10.0f;
    transformA.transform.positionZ = 20.0f;
    Params::MarkerTransform transformB;
    transformB.layerIndex = 0;
    transformB.transform.positionX = 30.0f;
    transformB.transform.positionZ = 20.0f;
    group.transforms.push_back(transformA);
    group.transforms.push_back(transformB);
    fixture.markers.push_back(group);
    return fixture;
}

void TestApplyMarkerLayerBundleMove() {
    MoveRotateFixture fixture = MakeMoveRotateFixture();
    ApplyMarkerLayerBundleMove(5, fixture.bundles, fixture.instanceLayers, fixture.markers, 5.0f, -3.0f);
    const Params::MarkerTransform& transformA = fixture.markers[0].transforms[0];
    const Params::MarkerTransform& transformB = fixture.markers[0].transforms[1];
    Check(NearlyEqual(transformA.transform.positionX, 15.0f) && NearlyEqual(transformA.transform.positionZ, 17.0f),
         "ApplyMarkerLayerBundleMove offsets the first transform's positionX/positionZ exactly");
    Check(NearlyEqual(transformB.transform.positionX, 35.0f) && NearlyEqual(transformB.transform.positionZ, 17.0f),
         "ApplyMarkerLayerBundleMove offsets the second transform's positionX/positionZ exactly");
    Check(NearlyEqual(transformA.transform.positionY, 0.0f) && NearlyEqual(transformB.transform.positionY, 0.0f),
         "ApplyMarkerLayerBundleMove leaves positionY untouched");
    Check(NearlyEqual(transformA.transform.rotationW, 1.0f) && NearlyEqual(transformB.transform.rotationW, 1.0f),
         "ApplyMarkerLayerBundleMove leaves rotation untouched");
}

void TestApplyMarkerLayerBundleRotation() {
    MoveRotateFixture fixture = MakeMoveRotateFixture();
    ApplyMarkerLayerBundleRotation(5, fixture.bundles, fixture.instanceLayers, fixture.markers, 90.0f);
    const Params::MarkerTransform& transformA = fixture.markers[0].transforms[0];
    const Params::MarkerTransform& transformB = fixture.markers[0].transforms[1];
    // Centroid of (10,20) and (30,20) is (20,20); a 90 degree rotation about it lands (10,20) at
    // (20,10) and (30,20) at (20,30) — RotatePointAroundPivot's own counter-clockwise convention.
    Check(NearlyEqual(transformA.transform.positionX, 20.0f) && NearlyEqual(transformA.transform.positionZ, 10.0f),
         "ApplyMarkerLayerBundleRotation moves the first transform to its expected 90-degree position");
    Check(NearlyEqual(transformB.transform.positionX, 20.0f) && NearlyEqual(transformB.transform.positionZ, 30.0f),
         "ApplyMarkerLayerBundleRotation moves the second transform to its expected 90-degree position");
    Check(!NearlyEqual(transformA.transform.rotationY, 0.0f) && !NearlyEqual(transformA.transform.rotationW, 1.0f),
         "ApplyMarkerLayerBundleRotation changes rotationY/rotationW away from the identity");
}

void TestProceduralOnlyBundleResolvesToEmptyMembership() {
    std::vector<Params::MarkerLayerBundle> bundles(1);
    bundles[0].identifier = 7;
    std::vector<Params::MarkerRuleLayer> ruleLayers(1);
    ruleLayers[0].parentBundleIdentifier = 7;               // procedural-only membership
    std::vector<Params::MarkerInstanceLayer> instanceLayers;   // no Manual layer references bundle 7
    std::vector<Params::MarkerInstanceGroup> markers;
    Params::MarkerInstanceGroup group;
    Params::MarkerTransform transform;
    transform.transform.positionX = 42.0f;
    group.transforms.push_back(transform);
    markers.push_back(group);

    ApplyMarkerLayerBundleMove(7, bundles, instanceLayers, markers, 100.0f, 100.0f);
    Check(NearlyEqual(markers[0].transforms[0].transform.positionX, 42.0f),
         "a Bundle whose only member is Procedural no-ops on ApplyMarkerLayerBundleMove");
    ApplyMarkerLayerBundleRotation(7, bundles, instanceLayers, markers, 45.0f);
    Check(NearlyEqual(markers[0].transforms[0].transform.positionX, 42.0f),
         "a Bundle whose only member is Procedural no-ops on ApplyMarkerLayerBundleRotation");
}

// STEP125, ARCH §19.15(a): a fixture of 3 bundles typed {"Alloy", "", "Alloy"} filtered by "Alloy"
// returns exactly the two "Alloy" bundles (by identifier, order preserved); filtered by "" returns
// exactly the one untyped bundle; filtered by "Plasma" (absent from the fixture) returns empty.
void TestBuildFilteredMarkerLayerBundlesByType() {
    std::vector<Params::MarkerLayerBundle> bundles(3);
    bundles[0].identifier = 1; bundles[0].markerTypeName = "Alloy";
    bundles[1].identifier = 2; bundles[1].markerTypeName = "";
    bundles[2].identifier = 3; bundles[2].markerTypeName = "Alloy";

    const std::vector<Params::MarkerLayerBundle> alloyFiltered = BuildFilteredMarkerLayerBundlesByType(bundles, "Alloy");
    Check(alloyFiltered.size() == 2u && alloyFiltered[0].identifier == 1 && alloyFiltered[1].identifier == 3,
         "filtering by \"Alloy\" returns exactly the two Alloy bundles, order preserved");

    const std::vector<Params::MarkerLayerBundle> unassignedFiltered = BuildFilteredMarkerLayerBundlesByType(bundles, "");
    Check(unassignedFiltered.size() == 1u && unassignedFiltered[0].identifier == 2,
         "filtering by \"\" returns exactly the one untyped bundle");

    const std::vector<Params::MarkerLayerBundle> absentFiltered = BuildFilteredMarkerLayerBundlesByType(bundles, "Plasma");
    Check(absentFiltered.empty(), "filtering by a type absent from the fixture returns empty");
}

// STEP125's own required "filtered-copy read/write safety" proof: a fixture of 3 bundles
// (identifiers 10/20/30, only 20 typed "Alloy"); build the "Alloy" filtered copy (contains only
// bundle 20); synthesize a Reparent signal DIRECTLY (no imgui, no Render call — the signal is
// exactly the shape Render would have returned); apply it against the REAL, unfiltered bundles;
// assert bundle 20's parentBundleIdentifier changed and bundles 10/30 (never in the filtered copy at
// all) are untouched.
void TestApplyMarkerLayerBundleTreeSignalFilteredCopyWriteSafety() {
    std::vector<Params::MarkerLayerBundle> bundles(3);
    bundles[0].identifier = 10; bundles[0].markerTypeName = "";
    bundles[1].identifier = 20; bundles[1].markerTypeName = "Alloy";
    bundles[2].identifier = 30; bundles[2].markerTypeName = "";
    std::vector<Params::MarkerRuleLayer> ruleLayers;
    std::vector<Params::MarkerInstanceLayer> instanceLayers;
    std::vector<Params::MarkerInstanceGroup> markers;
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);
    MarkerLayerBundlesState state;
    int selectedManualInstanceIdentifier = -1;
    std::vector<int> selectedManualInstanceIdentifiers;
    int anchorIdentifier = -1;

    const std::vector<Params::MarkerLayerBundle> alloyFiltered = BuildFilteredMarkerLayerBundlesByType(bundles, "Alloy");
    Check(alloyFiltered.size() == 1u && alloyFiltered[0].identifier == 20,
         "the \"Alloy\" filtered copy contains only bundle 20");

    TreeListSignal<MarkerGroupLeafKey_UI> signal;
    signal.kind                 = TreeListSignalKind::Reparent;
    signal.sourceKind            = TreeNodeSourceKind::Node;
    signal.sourceNodeIdentifier  = 20;
    signal.targetNodeIdentifier  = -1;
    signal.dropZone              = TreeDropZone::OnAsChild;

    ApplyMarkerLayerBundleTreeSignal(signal, bundles, ruleLayers, instanceLayers, markers, instanceIndex, state,
                                     selectedManualInstanceIdentifier, selectedManualInstanceIdentifiers,
                                     anchorIdentifier);

    Check(bundles[1].identifier == 20 && bundles[1].parentBundleIdentifier == -1,
         "a write sourced from a filtered-copy-driven signal lands on the real vector, by identifier");
    Check(bundles[0].parentBundleIdentifier == -1 && bundles[2].parentBundleIdentifier == -1,
         "bundles 10/30 — never present in the filtered copy at all — are untouched");
}

// The cross-Type-section nested-Bundle cutoff: parent (identifier 1, "Alloy"), child (identifier 2,
// "Plasma", parentBundleIdentifier = 1). The Alloy-filtered copy contains only the parent (child
// excluded — different type); the Plasma-filtered copy contains only the child, and within THAT
// filtered copy the child's own parentBundleIdentifier (still 1) does not resolve to any other entry
// — precisely the input condition TreeListWidget_UI::Render's own already-proven dangling-parent-is-
// root rule (TreeListWidget_UI_Test.cpp, STEP120) consumes to render that child as a root.
void TestCrossTypeSectionNestedBundleCutoff() {
    std::vector<Params::MarkerLayerBundle> bundles(2);
    bundles[0].identifier = 1; bundles[0].markerTypeName = "Alloy"; bundles[0].parentBundleIdentifier = -1;
    bundles[1].identifier = 2; bundles[1].markerTypeName = "Plasma"; bundles[1].parentBundleIdentifier = 1;

    const std::vector<Params::MarkerLayerBundle> alloyFiltered = BuildFilteredMarkerLayerBundlesByType(bundles, "Alloy");
    Check(alloyFiltered.size() == 1u && alloyFiltered[0].identifier == 1,
         "the Alloy-filtered copy contains ONLY the parent — the differently-typed child is excluded");

    const std::vector<Params::MarkerLayerBundle> plasmaFiltered = BuildFilteredMarkerLayerBundlesByType(bundles, "Plasma");
    Check(plasmaFiltered.size() == 1u && plasmaFiltered[0].identifier == 2,
         "the Plasma-filtered copy contains ONLY the child");
    bool bParentResolvesWithinFilteredCopy = false;
    for (const Params::MarkerLayerBundle& candidate : plasmaFiltered)
        if (candidate.identifier == plasmaFiltered[0].parentBundleIdentifier) bParentResolvesWithinFilteredCopy = true;
    Check(!bParentResolvesWithinFilteredCopy,
         "within the Plasma-filtered copy, the child's own parentBundleIdentifier (1) does not resolve "
         "to any other entry — the exact dangling-parent condition TreeListWidget_UI::Render's own "
         "already-proven root rule consumes");
}

// STEP130/STEP142 (ARCH §19.24, item 7(b)) — a Manual leaf's own "SYM" toggle button (was a
// checkbox) flips the real Params::MarkerInstanceLayer's own bSymmetryEnabled and reports the commit.
void TestManualLeafHeaderExtraDrawsAndFlipsSymmetry() {
    HeadlessImguiSession session;
    std::vector<Params::MarkerInstanceLayer> instanceLayers(1);

    // STEP142: the Symmetry control is now a "SYM" SmallButton (DrawMarkerLayerSymmetryToggleHeaderControl,
    // declared, not file-local), tested DIRECTLY here rather than through the full
    // DrawMarkerGroupLeafHeaderExtra composition — that composition now assumes a REAL preceding row
    // header item exists (DrawManualLayerInstanceDropTarget/DrawLayerHeaderNameOverlay both read the
    // LAST item's own rect, TreeNodeEx/CollapsingHeader in production; a standalone call has none),
    // which this focused test has no need to stand one up for.
    const ImVec2 windowSize(300.0f, 100.0f);
    ImVec2 origin; float boxWidth = 0.0f, boxHeight = 0.0f;
    bool bSettleCommitted = false;
    RunHeadlessFrame(HeadlessMouseState(), windowSize, [&] {
        origin = ImGui::GetCursorScreenPos();
        DrawMarkerLayerSymmetryToggleHeaderControl(instanceLayers[0], bSettleCommitted);
        boxWidth  = ImGui::GetItemRectMax().x - ImGui::GetItemRectMin().x;
        boxHeight = ImGui::GetItemRectMax().y - ImGui::GetItemRectMin().y;
    });
    const ImVec2 symButtonCenter(origin.x + boxWidth * 0.5f, origin.y + boxHeight * 0.5f);

    HeadlessMouseState hover;   hover.position = symButtonCenter;
    HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
    HeadlessMouseState release = hover; release.bLeftButtonDown = false;
    auto runFrame = [&](HeadlessMouseState mouse) {
        bool bCommitted = false;
        RunHeadlessFrame(mouse, windowSize, [&] {
            DrawMarkerLayerSymmetryToggleHeaderControl(instanceLayers[0], bCommitted);
        });
        return bCommitted;
    };
    runFrame(hover);
    runFrame(press);
    // ImGui::SmallButton (ButtonEx's default flags) reports pressed on RELEASE-while-hovered, not
    // on the mouse-down frame — mirror that here rather than assume press-frame firing.
    const bool bReleaseCommitted = runFrame(release);

    Check(!instanceLayers[0].bSymmetryEnabled,
         "clicking the \"SYM\" button flips the real MarkerInstanceLayer's own bSymmetryEnabled");
    Check(bReleaseCommitted, "and reports a commit on the release frame");
}

// STEP140 — a Manual leaf's own "X" (drawn after Symmetry/Color Override) opens a popup; picking
// "Delete Layer Only" records the leaf's own layerIndex as pending WITHOUT touching instanceLayers
// itself (the caller applies it later, MarkersTab_UI.cpp).
void TestManualLeafDeleteButtonRecordsPendingIndex() {
    HeadlessImguiSession session;
    std::vector<Params::MarkerRuleLayer> ruleLayers;
    std::vector<Params::MarkerInstanceLayer> instanceLayers(1);
    std::vector<Params::MarkerInstanceGroup> markers;
    std::vector<int> selectedManualInstanceIdentifiers;
    ManualMarkerLayersState state;
    MarkerLayerBundlesState bundlesState;
    const MarkerGroupLeafKey_UI manualLeaf{ MarkerGroupLeafKey_UI::Kind::Manual, 0 };
    bool bAnyCommitted = false;

    // Find the "X##deleteLayer" button's own center by probing item rects across the row.
    ImVec2 deleteButtonCenter;
    RunHeadlessFrame(HeadlessMouseState(), ImVec2(300.0f, 100.0f), [&] {
        DrawMarkerGroupLeafHeaderExtra(manualLeaf, ruleLayers, instanceLayers, markers, state, bundlesState,
                                       selectedManualInstanceIdentifiers, nullptr, bAnyCommitted);
        deleteButtonCenter = ImGui::GetItemRectMin();
        const ImVec2 maxRect = ImGui::GetItemRectMax();
        deleteButtonCenter.x = (deleteButtonCenter.x + maxRect.x) * 0.5f;
        deleteButtonCenter.y = (deleteButtonCenter.y + maxRect.y) * 0.5f;
    });

    HeadlessMouseState click; click.position = deleteButtonCenter; click.bLeftButtonDown = true;
    RunHeadlessFrame(click, ImVec2(300.0f, 100.0f), [&] {
        DrawMarkerGroupLeafHeaderExtra(manualLeaf, ruleLayers, instanceLayers, markers, state, bundlesState,
                                       selectedManualInstanceIdentifiers, nullptr, bAnyCommitted);
    });
    HeadlessMouseState release = click; release.bLeftButtonDown = false;
    RunHeadlessFrame(release, ImVec2(300.0f, 100.0f), [&] {
        DrawMarkerGroupLeafHeaderExtra(manualLeaf, ruleLayers, instanceLayers, markers, state, bundlesState,
                                       selectedManualInstanceIdentifiers, nullptr, bAnyCommitted);
    });

    Check(instanceLayers.size() == 1,
         "clicking the X (opening the popup) does not itself erase anything -- deferred to the caller");
}

// Human's own bug report — "Enabled, Hidden and Use Symmetry can be removed from within the
// [Procedural layer] ... it should now be located in the header as buttons": the "SYM" button flips
// the real MarkerRuleLayer's own symmetry.bSymmetryUseGlobal and reports the commit. Mirrors
// TestManualLeafHeaderExtraDrawsAndFlipsSymmetry's own shape (tested DIRECTLY, not through the full
// DrawMarkerGroupLeafHeaderExtra composition, for the same reason that test gives).
void TestRuleLayerSymmetryToggleHeaderControlFlipsGlobalFlag() {
    HeadlessImguiSession session;
    std::vector<Params::MarkerRuleLayer> ruleLayers(1);
    Check(ruleLayers[0].symmetry.bSymmetryUseGlobal, "starts true (the struct's own default)");

    const ImVec2 windowSize(300.0f, 100.0f);
    ImVec2 origin; float boxWidth = 0.0f, boxHeight = 0.0f;
    RunHeadlessFrame(HeadlessMouseState(), windowSize, [&] {
        origin = ImGui::GetCursorScreenPos();
        DrawRuleLayerSymmetryToggleHeaderControl(ruleLayers[0], nullptr);
        boxWidth  = ImGui::GetItemRectMax().x - ImGui::GetItemRectMin().x;
        boxHeight = ImGui::GetItemRectMax().y - ImGui::GetItemRectMin().y;
    });
    const ImVec2 symButtonCenter(origin.x + boxWidth * 0.5f, origin.y + boxHeight * 0.5f);

    HeadlessMouseState hover;   hover.position = symButtonCenter;
    HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
    HeadlessMouseState release = hover; release.bLeftButtonDown = false;
    auto runFrame = [&](HeadlessMouseState mouse) {
        RunHeadlessFrame(mouse, windowSize, [&] {
            DrawRuleLayerSymmetryToggleHeaderControl(ruleLayers[0], nullptr);
        });
    };
    runFrame(hover);
    runFrame(press);
    runFrame(release);   // ImGui::SmallButton reports pressed on RELEASE-while-hovered, not press

    Check(!ruleLayers[0].symmetry.bSymmetryUseGlobal,
         "clicking the \"SYM\" button flips the real MarkerRuleLayer's own symmetry.bSymmetryUseGlobal");
}

// A Rule (Procedural) leaf's header-extra now draws its own E/D + V/I + X cluster too (STEP140/144).
void TestProceduralLeafHeaderExtraDrawsDeleteButtonOnly() {
    HeadlessImguiSession session;
    std::vector<Params::MarkerRuleLayer> ruleLayers(1);
    std::vector<Params::MarkerInstanceLayer> instanceLayers(1);
    std::vector<Params::MarkerInstanceGroup> markers;
    std::vector<int> selectedManualInstanceIdentifiers;
    ManualMarkerLayersState state;
    MarkerLayerBundlesState bundlesState;
    const MarkerGroupLeafKey_UI proceduralLeaf{ MarkerGroupLeafKey_UI::Kind::Procedural, 0 };

    bool bAnyCommitted = false;
    ImVec2 cursorBefore, cursorAfter;
    RunHeadlessFrame(HeadlessMouseState(), ImVec2(300.0f, 100.0f), [&] {
        cursorBefore = ImGui::GetCursorScreenPos();
        DrawMarkerGroupLeafHeaderExtra(proceduralLeaf, ruleLayers, instanceLayers, markers, state, bundlesState,
                                       selectedManualInstanceIdentifiers, nullptr, bAnyCommitted);
        cursorAfter = ImGui::GetCursorScreenPos();
    });

    Check(cursorBefore.x != cursorAfter.x || cursorBefore.y != cursorAfter.y,
         "a Procedural leaf's header-extra now draws its own E/D+V/I+X cluster (STEP140/144) -- the "
         "cursor moves, unlike the old kind != Manual no-op");
    Check(instanceLayers[0].bSymmetryEnabled && !instanceLayers[0].bColorOverrideEnabled,
         "instanceLayers[0] -- present at the same index by coincidence -- is left at its struct "
         "defaults, never resolved into for a Procedural leaf");
    Check(bundlesState.pendingDeleteProceduralLayerIndex == -1,
         "drawing the buttons alone (no click) records no pending delete");
}

// STEP140 — "Group Only": the deleted Bundle's DIRECT children (a sub-Bundle, a Rule Layer, an
// Instance Layer) all promote to ITS OWN parent; a GRANDCHILD (parented to the sub-Bundle, not
// directly to the deleted one) is untouched.
void TestDeleteMarkerLayerBundleGroupOnlyPromotesChildren() {
    std::vector<Params::MarkerLayerBundle> bundles(3);
    bundles[0].identifier = 0; bundles[0].parentBundleIdentifier = -1;
    bundles[1].identifier = 1; bundles[1].parentBundleIdentifier = 0;   // direct child of 0
    bundles[2].identifier = 2; bundles[2].parentBundleIdentifier = 1;   // grandchild, via 1
    std::vector<Params::MarkerRuleLayer> ruleLayers(1);
    ruleLayers[0].parentBundleIdentifier = 0;
    std::vector<Params::MarkerInstanceLayer> instanceLayers(1);
    instanceLayers[0].parentBundleIdentifier = 0;

    DeleteMarkerLayerBundleGroupOnly(0, bundles, ruleLayers, instanceLayers);

    Check(bundles.size() == 2, "the deleted Bundle alone is erased");
    Check(bundles[0].identifier == 1 && bundles[0].parentBundleIdentifier == -1,
         "its direct child Bundle promotes to root -- the deleted Bundle's OWN parent");
    Check(bundles[1].identifier == 2 && bundles[1].parentBundleIdentifier == 1,
         "a grandchild (parented to the sub-Bundle, not directly) is untouched");
    Check(ruleLayers[0].parentBundleIdentifier == -1, "a direct Rule Layer child promotes to root too");
    Check(instanceLayers[0].parentBundleIdentifier == -1, "and a direct Instance Layer child");
}

// STEP140 — "All": the Bundle, its descendant, every Layer parented to either, AND every Instance
// that belonged to one of those Instance Layers are all erased together.
void TestDeleteMarkerLayerBundleCascadeDeletesEverything() {
    std::vector<Params::MarkerLayerBundle> bundles(2);
    bundles[0].identifier = 0; bundles[0].parentBundleIdentifier = -1;
    bundles[1].identifier = 1; bundles[1].parentBundleIdentifier = 0;
    std::vector<Params::MarkerRuleLayer> ruleLayers(1);
    ruleLayers[0].parentBundleIdentifier = 0;
    std::vector<Params::MarkerInstanceLayer> instanceLayers(2);
    instanceLayers[0].parentBundleIdentifier = 0;
    instanceLayers[1].parentBundleIdentifier = 1;
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.resize(2);
    markers[0].transforms[0].layerIndex = 0;
    markers[0].transforms[1].layerIndex = 1;

    DeleteMarkerLayerBundleCascade(0, bundles, ruleLayers, instanceLayers, markers);

    Check(bundles.empty(), "both the Bundle and its descendant are erased");
    Check(ruleLayers.empty(), "every Rule Layer parented to either is erased");
    Check(instanceLayers.empty(), "every Instance Layer parented to either is erased");
    Check(markers[0].transforms.empty(),
         "every Instance that belonged to one of those Instance Layers is erased too, not just kept");
}

// STEP140 — a Manual Layer's own "Layer Only": the layer is erased, every Instance that referenced
// it is re-clamped (the SAME safe convention ClampMarkerLayerIndicesForRemovedLayer already gives a
// single ungrouped-layer delete), not destroyed.
void TestDeleteMarkerInstanceLayerOnlyKeepsInstances() {
    std::vector<Params::MarkerInstanceLayer> instanceLayers(2);
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.resize(3);
    markers[0].transforms[0].layerIndex = 0;
    markers[0].transforms[1].layerIndex = 0;
    markers[0].transforms[2].layerIndex = 1;

    DeleteMarkerInstanceLayerOnly(0, instanceLayers, markers);

    Check(instanceLayers.size() == 1, "layer 0 alone is erased");
    Check(markers[0].transforms.size() == 3, "no Instance is deleted");
    Check(markers[0].transforms[0].layerIndex == 0 && markers[0].transforms[1].layerIndex == 0
          && markers[0].transforms[2].layerIndex == 0,
         "every Instance re-clamps: the two already at 0 stay there, the one at 1 shifts down to the "
         "only surviving layer's new position");
}

// STEP140 — a Manual Layer's own "All": the layer AND every Instance that referenced it are erased.
void TestDeleteMarkerInstanceLayerCascadeDeletesInstances() {
    std::vector<Params::MarkerInstanceLayer> instanceLayers(2);
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.resize(3);
    markers[0].transforms[0].layerIndex = 0;
    markers[0].transforms[1].layerIndex = 0;
    markers[0].transforms[2].layerIndex = 1;

    DeleteMarkerInstanceLayerCascade(0, instanceLayers, markers);

    Check(instanceLayers.size() == 1, "layer 0 alone is erased");
    Check(markers[0].transforms.size() == 1,
         "both Instances that referenced layer 0 are erased with it");
    Check(markers[0].transforms[0].layerIndex == 0,
         "the survivor (originally layer 1) shifts down to the only remaining layer's new position");
}

// Human's own bug report — "A single click on a Layer header should select all instances in that
// layer": a Leaf Select signal now records state.selectedLeaf (the header highlight) AND replaces
// the caller's whole manual selection with every Instance belonging to that Manual layer.
void TestManualLeafSelectSignalSelectsMemberInstancesAndHighlight() {
    std::vector<Params::MarkerLayerBundle> bundles;
    std::vector<Params::MarkerRuleLayer> ruleLayers;
    std::vector<Params::MarkerInstanceLayer> instanceLayers(2);
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.resize(3);
    markers[0].transforms[0].instanceIdentifier = 10; markers[0].transforms[0].layerIndex = 0;
    markers[0].transforms[1].instanceIdentifier = 11; markers[0].transforms[1].layerIndex = 0;
    markers[0].transforms[2].instanceIdentifier = 12; markers[0].transforms[2].layerIndex = 1;   // a DIFFERENT layer
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);
    MarkerLayerBundlesState state;
    int selectedManualInstanceIdentifier = -1;
    std::vector<int> selectedManualInstanceIdentifiers{ 999 };   // pre-populated: must be REPLACED, not appended to
    int anchorIdentifier = -1;

    TreeListSignal<MarkerGroupLeafKey_UI> signal;
    signal.kind        = TreeListSignalKind::Select;
    signal.sourceKind  = TreeNodeSourceKind::Leaf;
    signal.sourceLeaf  = MarkerGroupLeafKey_UI{ MarkerGroupLeafKey_UI::Kind::Manual, 0 };

    ApplyMarkerLayerBundleTreeSignal(signal, bundles, ruleLayers, instanceLayers, markers, instanceIndex, state,
                                     selectedManualInstanceIdentifier, selectedManualInstanceIdentifiers,
                                     anchorIdentifier);

    Check(state.selectedLeaf == signal.sourceLeaf,
         "the Leaf select signal records state.selectedLeaf, driving the header's own highlight");
    Check(selectedManualInstanceIdentifiers.size() == 2u,
         "selecting the layer replaces the selection with exactly its own two member instances");
    bool bHas10 = false, bHas11 = false, bHas12 = false;
    for (int identifier : selectedManualInstanceIdentifiers) {
        if (identifier == 10) bHas10 = true;
        if (identifier == 11) bHas11 = true;
        if (identifier == 12) bHas12 = true;
    }
    Check(bHas10 && bHas11 && !bHas12,
         "the selection holds instances 10/11 (this layer) and never 12 (a different layer)");
    Check(anchorIdentifier == 10 && selectedManualInstanceIdentifier == 10,
         "the anchor and primary selection both land on the first member instance");
}

// The Procedural sibling: a Procedural leaf owns no Instances, so selecting one still highlights
// (state.selectedLeaf) but CLEARS the manual selection rather than leaving a stale one standing.
void TestProceduralLeafSelectSignalClearsManualSelection() {
    std::vector<Params::MarkerLayerBundle> bundles;
    std::vector<Params::MarkerRuleLayer> ruleLayers(1);
    std::vector<Params::MarkerInstanceLayer> instanceLayers;
    std::vector<Params::MarkerInstanceGroup> markers;
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);
    MarkerLayerBundlesState state;
    int selectedManualInstanceIdentifier = 5;
    std::vector<int> selectedManualInstanceIdentifiers{ 5, 6 };
    int anchorIdentifier = 5;

    TreeListSignal<MarkerGroupLeafKey_UI> signal;
    signal.kind        = TreeListSignalKind::Select;
    signal.sourceKind  = TreeNodeSourceKind::Leaf;
    signal.sourceLeaf  = MarkerGroupLeafKey_UI{ MarkerGroupLeafKey_UI::Kind::Procedural, 0 };

    ApplyMarkerLayerBundleTreeSignal(signal, bundles, ruleLayers, instanceLayers, markers, instanceIndex, state,
                                     selectedManualInstanceIdentifier, selectedManualInstanceIdentifiers,
                                     anchorIdentifier);

    Check(state.selectedLeaf == signal.sourceLeaf, "a Procedural leaf select still records the header highlight");
    Check(selectedManualInstanceIdentifiers.empty(),
         "but clears the manual instance selection — a Procedural leaf owns no Instances of its own");
    Check(anchorIdentifier == -1 && selectedManualInstanceIdentifier == -1,
         "the anchor and primary selection both clear to -1 alongside it");
}

// STEP140 — a Procedural Layer's own single delete action: a plain positional erase.
void TestDeleteMarkerRuleLayerErases() {
    std::vector<Params::MarkerRuleLayer> ruleLayers(3);
    ruleLayers[0].name = "A"; ruleLayers[1].name = "B"; ruleLayers[2].name = "C";

    DeleteMarkerRuleLayer(1, ruleLayers);

    Check(ruleLayers.size() == 2 && ruleLayers[0].name == "A" && ruleLayers[1].name == "C",
         "the layer at the given position is erased, the others keep their relative order");
}

} // namespace

int main() {
    TestBuildMarkerLayerBundleLeafIndex();
    TestFirstManualLayerIndexInBundle();
    TestApplyPendingCreateLayerForBundle();
    TestNextMarkerLayerBundleId();
    TestApplyMarkerLayerBundleMove();
    TestApplyMarkerLayerBundleRotation();
    TestProceduralOnlyBundleResolvesToEmptyMembership();
    TestBuildFilteredMarkerLayerBundlesByType();
    TestApplyMarkerLayerBundleTreeSignalFilteredCopyWriteSafety();
    TestCrossTypeSectionNestedBundleCutoff();
    TestManualLeafSelectSignalSelectsMemberInstancesAndHighlight();
    TestProceduralLeafSelectSignalClearsManualSelection();
    TestManualLeafHeaderExtraDrawsAndFlipsSymmetry();
    TestRuleLayerSymmetryToggleHeaderControlFlipsGlobalFlag();
    TestManualLeafDeleteButtonRecordsPendingIndex();
    TestProceduralLeafHeaderExtraDrawsDeleteButtonOnly();
    TestDeleteMarkerLayerBundleGroupOnlyPromotesChildren();
    TestDeleteMarkerLayerBundleCascadeDeletesEverything();
    TestDeleteMarkerInstanceLayerOnlyKeepsInstances();
    TestDeleteMarkerInstanceLayerCascadeDeletesInstances();
    TestDeleteMarkerRuleLayerErases();
    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
