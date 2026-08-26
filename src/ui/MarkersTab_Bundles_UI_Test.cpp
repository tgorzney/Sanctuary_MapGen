// MarkersTab_Bundles_UI_Test.cpp — STEP120 acceptance for MarkersTab_Bundles_UI.h's own pure
// helpers: BuildMarkerLayerBundleLeafIndex, NextMarkerLayerBundleId, and the Move/Rotate Apply
// functions' own call-boundary behavior. Pure logic only — no imgui frame needed, mirroring
// STEP106's own "defer the imgui-coupled path, test the definitely-pure pieces" posture.
#include "MarkersTab_Bundles_UI.h"
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
    MarkerLayerBundlesState state;

    const std::vector<Params::MarkerLayerBundle> alloyFiltered = BuildFilteredMarkerLayerBundlesByType(bundles, "Alloy");
    Check(alloyFiltered.size() == 1u && alloyFiltered[0].identifier == 20,
         "the \"Alloy\" filtered copy contains only bundle 20");

    TreeListSignal<MarkerGroupLeafKey_UI> signal;
    signal.kind                 = TreeListSignalKind::Reparent;
    signal.sourceKind            = TreeNodeSourceKind::Node;
    signal.sourceNodeIdentifier  = 20;
    signal.targetNodeIdentifier  = -1;
    signal.dropZone              = TreeDropZone::OnAsChild;

    ApplyMarkerLayerBundleTreeSignal(signal, bundles, ruleLayers, instanceLayers, state);

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

} // namespace

int main() {
    TestBuildMarkerLayerBundleLeafIndex();
    TestNextMarkerLayerBundleId();
    TestApplyMarkerLayerBundleMove();
    TestApplyMarkerLayerBundleRotation();
    TestProceduralOnlyBundleResolvesToEmptyMembership();
    TestBuildFilteredMarkerLayerBundlesByType();
    TestApplyMarkerLayerBundleTreeSignalFilteredCopyWriteSafety();
    TestCrossTypeSectionNestedBundleCutoff();
    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
