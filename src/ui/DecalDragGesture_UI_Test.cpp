// DecalDragGesture_UI_Test.cpp — headless acceptance coverage proving InstanceDragGesture_UI.h's
// generic templates work for a THIRD Traits instantiation (ARCH §21.3). PropDragGesture_UI_Test.cpp
// already proves the generic core in depth for a second (non-Marker) domain; this file is a lighter
// smoke pass for Decals plus its own resolver gate, not a full re-run of every Marker test.
#include "DecalDragGesture_UI.h"
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
bool NearlyEqual(float a, float b) { return std::fabs(a - b) <= 0.01f; }

Params::Geometry MakeTestGeometry() {
    Params::Geometry geometry;
    geometry.mapSize = 10;
    geometry.worldUnitsPerCell = 1.0f;
    return geometry;
}

Params::DecalTransform MakeTransform(float x, float z, int symmetryGroupIdentifier, int layerIndex = 0) {
    Params::DecalTransform transform;
    transform.transform.positionX = x;
    transform.transform.positionZ = z;
    transform.symmetryGroupIdentifier = symmetryGroupIdentifier;
    transform.layerIndex = layerIndex;
    return transform;
}

void RunLiveMirrorFollowChecks() {
    std::vector<Params::DecalInstanceGroup> decals(1);
    decals[0].transforms.push_back(MakeTransform(2.0f, 3.0f, 42));
    decals[0].transforms.push_back(MakeTransform(8.0f, 3.0f, 42));
    const Params::Geometry geometry = MakeTestGeometry();

    InstanceDragGestureState state;
    Check(BeginInstanceDragGesture<DecalDragTraits>(state, decals, {}, geometry,
                                                    Params::SymmetryAxis::MirrorAcrossX, 3, 0, 0),
          "a Decal gesture begins on a grouped member");
    Check(!state.bCardinalityFrozen, "Decals has no reserved cardinality-frozen group");

    UpdateInstanceDragGesture<DecalDragTraits>(state, decals, {}, geometry, 4.0f, 3.0f);
    Check(NearlyEqual(decals[0].transforms[0].transform.positionX, 4.0f), "the dragged member follows the cursor");
    Check(NearlyEqual(decals[0].transforms[1].transform.positionX, 6.0f), "the sibling mirrors live, this frame");

    EndInstanceDragGesture<DecalDragTraits>(state, decals, geometry);
    Check(static_cast<int>(decals[0].transforms.size()) == 2, "no structural change for an ordinary drag");
}

void RunLockRefusalChecks() {
    std::vector<Params::DecalInstanceGroup> decals(1);
    decals[0].transforms.push_back(MakeTransform(2.0f, 3.0f, 0, /*layerIndex=*/0));
    std::vector<Params::DecalInstanceLayer> lockedLayers(1);
    lockedLayers[0].bLocked = true;
    const Params::Geometry geometry = MakeTestGeometry();

    InstanceDragGestureState state;
    Check(!BeginInstanceDragGesture<DecalDragTraits>(state, decals, lockedLayers, geometry,
                                                     Params::SymmetryAxis::MirrorAcrossX, 3, 0, 0),
          "a locked Decal layer refuses to begin a drag");
}

void RunResolveEffectiveDecalSymmetryGateChecks() {
    std::vector<Params::DecalInstanceLayer> layers(1);
    layers[0].bSymmetryEnabled = false;
    int mask = -1, radialRepeatCount = -1;
    ResolveEffectiveDecalSymmetry(layers, 0, Params::SymmetryAxis::MirrorAcrossZ, 2, mask, radialRepeatCount);
    Check(mask == Params::SymmetryAxis::None && radialRepeatCount == 0,
          "bSymmetryEnabled == false forces the effective mask to None, same as every other domain");
}

} // namespace

int main() {
    RunLiveMirrorFollowChecks();
    RunLockRefusalChecks();
    RunResolveEffectiveDecalSymmetryGateChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
