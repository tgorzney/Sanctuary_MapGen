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

} // namespace

int main() {
    TestBuildMarkerLayerBundleLeafIndex();
    TestNextMarkerLayerBundleId();
    TestApplyMarkerLayerBundleMove();
    TestApplyMarkerLayerBundleRotation();
    TestProceduralOnlyBundleResolvesToEmptyMembership();
    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
