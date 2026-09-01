// PropDragGesture_UI_Test.cpp — headless acceptance coverage proving InstanceDragGesture_UI.h's
// generic templates genuinely work for a SECOND Traits instantiation, not just Markers (ARCH §21.3).
// Mirrors MarkerDragGesture_UI_Test.cpp's own live-mirror-follow/growth-materialize/lock-refusal
// tests, plus this domain's own two divergences: PropTransform has no `name` field (materialized
// siblings must never crash on the no-op Seed/MakeUnique hooks) and Props has no reserved,
// cardinality-frozen group (IsCardinalityFrozenGroup is unconditionally false, even for a
// Spawn-shaped name).
#include "PropDragGesture_UI.h"
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

Params::PropTransform MakeTransform(float x, float z, int symmetryGroupIdentifier, int layerIndex = 0) {
    Params::PropTransform transform;
    transform.transform.positionX = x;
    transform.transform.positionZ = z;
    transform.symmetryGroupIdentifier = symmetryGroupIdentifier;
    transform.layerIndex = layerIndex;
    return transform;
}

// ARCH §21.9 — Props has no Link concept: every widened call below passes this permanently-empty
// NoInstanceLink roster, never populated/read (PropDragTraits's own inert pass-through bodies).
const std::vector<NoInstanceLink> kNoLinks;

// Live mirror-follow, growth, and release-time materialize — proves the generic core, not just its
// Marker instantiation, actually drives PropTransform writes correctly.
void RunLiveMirrorAndGrowthChecks() {
    std::vector<Params::PropInstanceGroup> props(1);
    props[0].transforms.push_back(MakeTransform(5.0f, 3.0f, 11, /*layerIndex=*/2));   // ON the axis
    props[0].transforms[0].instanceIdentifier = 5;
    const Params::Geometry geometry = MakeTestGeometry();

    InstanceDragGestureState state;
    Check(BeginInstanceDragGesture<PropDragTraits>(state, props, {}, kNoLinks, geometry,
                                                   Params::SymmetryAxis::MirrorAcrossX, 3, 0, 0),
          "a Prop gesture begins on a grouped member, same as Markers");
    Check(state.gestureStartOrbitCount == 1, "on-axis, no sibling: gesture-start orbit is one point");
    Check(!state.bCardinalityFrozen, "Props has no reserved cardinality-frozen group");

    UpdateInstanceDragGesture<PropDragTraits>(state, props, {}, kNoLinks, geometry, 7.0f, 3.0f);   // off the axis
    Check(state.bCardinalityGrew, "moving off the axis is flagged as growth, identically to Markers");
    Check(static_cast<int>(props[0].transforms.size()) == 1, "zero PARAMS write for the ghost point");
    Check(static_cast<int>(state.currentGhostPoints.size()) == 1, "exactly one ghost point this frame");

    EndInstanceDragGesture<PropDragTraits>(state, props, geometry);
    Check(static_cast<int>(props[0].transforms.size()) == 2, "release materializes exactly one new sibling");
    const Params::PropTransform& materialized = props[0].transforms[1];
    Check(materialized.symmetryGroupIdentifier == 11, "the new sibling shares the group's symmetryGroupIdentifier");
    Check(materialized.layerIndex == 2, "the new sibling inherits the dragged member's layerIndex");
    Check(materialized.instanceIdentifier == 6,
          "the materialized sibling mints a real instanceIdentifier via NextPropInstanceIdentifier");
    // PropTransform has no `name` field at all — the no-op SeedInstanceName/MakeInstanceNamesUnique
    // hooks must never crash and must never be expected to have written anything (there is nothing
    // to write); reaching this line without a compile error or a crash IS the assertion.
}

// A group whose name would trigger Markers' Spawn-refusal never refuses for Props — proves
// IsCardinalityFrozenGroup is unconditionally false here, not accidentally still Spawn-aware.
void RunNoCardinalityFreezeChecks() {
    std::vector<Params::PropInstanceGroup> props(1);
    props[0].transforms.push_back(MakeTransform(2.0f, 3.0f, 99));
    props[0].transforms.push_back(MakeTransform(8.0f, 3.0f, 99));
    const Params::Geometry geometry = MakeTestGeometry();

    InstanceDragGestureState state;
    BeginInstanceDragGesture<PropDragTraits>(state, props, {}, kNoLinks, geometry, Params::SymmetryAxis::MirrorAcrossX, 3, 0, 0);
    Check(!state.bCardinalityFrozen, "PropDragTraits::IsCardinalityFrozenGroup is unconditionally false");

    UpdateInstanceDragGesture<PropDragTraits>(state, props, {}, kNoLinks, geometry, 5.0f, 3.0f);   // collapses the orbit
    Check(!state.bSpawnCardinalityRefused,
          "a cardinality-changing Prop drag is never refused — no reserved group concept exists here");
}

// STEP106-equivalent: a locked Prop layer refuses BeginInstanceDragGesture/RepositionSymmetryGroupMember.
void RunLockRefusalChecks() {
    std::vector<Params::PropInstanceGroup> props(1);
    props[0].transforms.push_back(MakeTransform(2.0f, 3.0f, 0, /*layerIndex=*/0));
    std::vector<Params::PropInstanceLayer> lockedLayers(1);
    lockedLayers[0].bLocked = true;
    const Params::Geometry geometry = MakeTestGeometry();

    InstanceDragGestureState state;
    Check(!BeginInstanceDragGesture<PropDragTraits>(state, props, lockedLayers, kNoLinks, geometry,
                                                    Params::SymmetryAxis::MirrorAcrossX, 3, 0, 0),
          "a locked Prop layer refuses to begin a drag");
    Check(!state.bActive, "the gesture state is left inactive");

    std::vector<Params::PropInstanceGroup> pair(1);
    pair[0].transforms.push_back(MakeTransform(2.0f, 3.0f, 3, /*layerIndex=*/0));
    pair[0].transforms.push_back(MakeTransform(8.0f, 3.0f, 3, /*layerIndex=*/0));
    Check(!RepositionSymmetryGroupMember<PropDragTraits>(pair, lockedLayers, kNoLinks, geometry,
                                                         Params::SymmetryAxis::MirrorAcrossX, 3, 0, 0, 1.0f, 3.0f),
          "a locked Prop layer refuses RepositionSymmetryGroupMember outright");
    Check(NearlyEqual(pair[0].transforms[0].transform.positionX, 2.0f),
          "the moved member's own position is left unchanged on a lock refusal");
}

// ResolveEffectivePropSymmetry's own gate — mirrors the Marker equivalent's ARCH §19.24 behavior.
void RunResolveEffectivePropSymmetryGateChecks() {
    std::vector<Params::PropInstanceLayer> layers(1);
    layers[0].bSymmetryEnabled = true;
    layers[0].symmetry.bSymmetryUseGlobal = false;
    layers[0].symmetry.symmetryMask = Params::SymmetryAxis::MirrorAcrossX;
    layers[0].symmetry.radialSymmetryRepeatCount = 4;

    int mask = -1, radialRepeatCount = -1;
    ResolveEffectivePropSymmetry(layers, 0, Params::SymmetryAxis::MirrorAcrossZ, 2, mask, radialRepeatCount);
    Check(mask == Params::SymmetryAxis::MirrorAcrossX && radialRepeatCount == 4,
          "bSymmetryEnabled == true resolves the layer's own configured mask/count");

    layers[0].bSymmetryEnabled = false;
    ResolveEffectivePropSymmetry(layers, 0, Params::SymmetryAxis::MirrorAcrossZ, 2, mask, radialRepeatCount);
    Check(mask == Params::SymmetryAxis::None && radialRepeatCount == 0,
          "bSymmetryEnabled == false forces the effective mask to None regardless of configured symmetry");
}

} // namespace

int main() {
    RunLiveMirrorAndGrowthChecks();
    RunNoCardinalityFreezeChecks();
    RunLockRefusalChecks();
    RunResolveEffectivePropSymmetryGateChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
