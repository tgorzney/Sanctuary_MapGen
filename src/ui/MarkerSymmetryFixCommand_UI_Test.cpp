// MarkerSymmetryFixCommand_UI_Test.cpp — acceptance coverage for Ui::FixMarkerLayerSymmetry (STEP107
// §3b): skip-mode basic detection and its "never touches an already-grouped marker" guarantee
// (acceptance tests 1/2), fresh id allocation with no collisions (test 4), overwrite mode widening
// the pool (test 5's pure half — the checkbox auto-reset itself is imgui-button-gated and verified by
// code inspection of DrawFixSymmetryCommand, the same "no headless seam" posture STEP106's own Add
// Instance test note establishes), cross-layer isolation (test 6) and its row-scoped-write corollary
// (test 11), and determinism under group-vector reordering (test 7). Pure, imgui-free — no window.
#include "MarkerSymmetryFixCommand_UI.h"
#include "../params/Symmetry_PARAMS.h"
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

// worldUnitsPerCell = 1 -> extent = mapSize, mirror(x) = mapSize - x, hand-verifiable (same
// convention MarkerDragGesture_UI_Test.cpp's MakeTestGeometry uses).
Params::Geometry MakeTestGeometry() {
    Params::Geometry geometry;
    geometry.mapSize = 10;
    geometry.worldUnitsPerCell = 1.0f;
    return geometry;
}

Params::MarkerTransform MakeTransform(const char* name, float x, float z, int symmetryGroupIdentifier,
                                      int layerIndex) {
    Params::MarkerTransform transform;
    transform.name = name;
    transform.transform.positionX = x;
    transform.transform.positionZ = z;
    transform.symmetryGroupIdentifier = symmetryGroupIdentifier;
    transform.layerIndex = layerIndex;
    return transform;
}

// Test 1 (acceptance test 1) — two ungrouped exact-mirror markers on the target layer: skip mode
// backfills one fresh, positive, shared id; the result reports one confirmed group, zero unmatched.
void RunBasicSkipModeChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("A", 2.0f, 3.0f, 0, 0));
    markers[0].transforms.push_back(MakeTransform("B", 8.0f, 3.0f, 0, 0));
    const Params::Geometry geometry = MakeTestGeometry();

    const MarkerSymmetryFixResult result = FixMarkerLayerSymmetry(
        markers, geometry, 0, Params::SymmetryAxis::MirrorAcrossX, 3, 0.5f, false);

    Check(result.confirmedGroupCount == 1, "one fresh group is confirmed");
    Check(result.unmatchedSlotCount == 0, "with no unmatched slots");
    const int idA = markers[0].transforms[0].symmetryGroupIdentifier;
    const int idB = markers[0].transforms[1].symmetryGroupIdentifier;
    Check(idA > 0 && idA == idB, "both markers now share one fresh, positive id");
}

// Test 2 (acceptance test 2) — skip mode never reads or rewrites an already-grouped pair, even when
// its positions no longer mirror each other (a manual nudge after grouping).
void RunSkipModeIgnoresAlreadyGroupedChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("A", 2.0f, 3.0f, 0, 0));
    markers[0].transforms.push_back(MakeTransform("B", 8.0f, 3.0f, 0, 0));
    // Already grouped (id 7), deliberately NOT a mirror pair anymore.
    markers[0].transforms.push_back(MakeTransform("C", 1.0f, 1.0f, 7, 0));
    markers[0].transforms.push_back(MakeTransform("D", 9.0f, 9.0f, 7, 0));
    const Params::Geometry geometry = MakeTestGeometry();

    FixMarkerLayerSymmetry(markers, geometry, 0, Params::SymmetryAxis::MirrorAcrossX, 3, 0.5f, false);

    Check(markers[0].transforms[2].symmetryGroupIdentifier == 7
              && markers[0].transforms[2].transform.positionX == 1.0f
              && markers[0].transforms[2].transform.positionZ == 1.0f,
          "the already-grouped member's id and position are byte-identical after skip mode");
    Check(markers[0].transforms[3].symmetryGroupIdentifier == 7
              && markers[0].transforms[3].transform.positionX == 9.0f
              && markers[0].transforms[3].transform.positionZ == 9.0f,
          "and so is its sibling's");
}

// Test 3 (acceptance test 4) — a layer already holding ids 1 and 3 (a gap is legal) plus a fresh
// ungrouped mirrored pair: the new pair gets id 4 (max existing + 1), never 2 or a collision.
void RunFreshIdAllocationChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("Existing1", 0.5f, 0.5f, 1, 0));
    markers[0].transforms.push_back(MakeTransform("Existing3", 0.6f, 0.6f, 3, 0));
    markers[0].transforms.push_back(MakeTransform("A", 2.0f, 3.0f, 0, 0));
    markers[0].transforms.push_back(MakeTransform("B", 8.0f, 3.0f, 0, 0));
    const Params::Geometry geometry = MakeTestGeometry();

    const MarkerSymmetryFixResult result = FixMarkerLayerSymmetry(
        markers, geometry, 0, Params::SymmetryAxis::MirrorAcrossX, 3, 0.5f, false);

    Check(result.confirmedGroupCount == 1, "exactly one new group is confirmed");
    Check(markers[0].transforms[2].symmetryGroupIdentifier == 4
              && markers[0].transforms[3].symmetryGroupIdentifier == 4,
          "the new pair receives id 4 (max existing id 3, plus one), not 2 or a collision");
    Check(markers[0].transforms[0].symmetryGroupIdentifier == 1
              && markers[0].transforms[1].symmetryGroupIdentifier == 3,
          "the pre-existing ids are untouched");
}

// Test 4 (acceptance test 5) — overwrite mode widens the pool to every in-layer marker (including
// an already, still-correctly, grouped pair) and re-derives ids from scratch: both pairs end up
// freshly grouped under two DIFFERENT positive ids, not necessarily reusing the original id 1.
void RunOverwriteModeWidensPoolChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("A", 2.0f, 3.0f, 1, 0));   // already grouped, still a mirror
    markers[0].transforms.push_back(MakeTransform("B", 8.0f, 3.0f, 1, 0));
    markers[0].transforms.push_back(MakeTransform("C", 4.0f, 5.0f, 0, 0));   // ungrouped mirror pair
    markers[0].transforms.push_back(MakeTransform("D", 6.0f, 5.0f, 0, 0));
    const Params::Geometry geometry = MakeTestGeometry();

    const MarkerSymmetryFixResult result = FixMarkerLayerSymmetry(
        markers, geometry, 0, Params::SymmetryAxis::MirrorAcrossX, 3, 0.5f, true);

    Check(result.confirmedGroupCount == 2, "both pairs are re-confirmed under overwrite mode");
    const int idAB = markers[0].transforms[0].symmetryGroupIdentifier;
    const int idCD = markers[0].transforms[2].symmetryGroupIdentifier;
    Check(markers[0].transforms[0].symmetryGroupIdentifier == markers[0].transforms[1].symmetryGroupIdentifier,
          "pair A/B still shares one id");
    Check(markers[0].transforms[2].symmetryGroupIdentifier == markers[0].transforms[3].symmetryGroupIdentifier,
          "pair C/D shares its own id");
    Check(idAB > 0 && idCD > 0 && idAB != idCD,
          "the two pairs land on two distinct, freshly-allocated positive ids");
}

// Test 5 (acceptance test 6) — two markers on DIFFERENT layers whose positions happen to be exact
// mirrors of each other are never cross-matched: running Fix Symmetry on layer 0 leaves the layer-0
// marker ungrouped (its only "partner" belongs to layer 1, outside the candidate pool).
void RunCrossLayerIsolationChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("OnLayer0", 2.0f, 3.0f, 0, 0));
    markers[0].transforms.push_back(MakeTransform("OnLayer1", 8.0f, 3.0f, 0, 1));
    const Params::Geometry geometry = MakeTestGeometry();

    const MarkerSymmetryFixResult result = FixMarkerLayerSymmetry(
        markers, geometry, 0, Params::SymmetryAxis::MirrorAcrossX, 3, 0.5f, false);

    Check(result.confirmedGroupCount == 0, "a different-layer mirror position is never a match candidate");
    Check(markers[0].transforms[0].symmetryGroupIdentifier == 0, "the layer-0 marker stays ungrouped");
    Check(markers[0].transforms[1].symmetryGroupIdentifier == 0, "the layer-1 marker is untouched too");
}

// Test 6 (acceptance test 11's command-level corollary) — with TWO layers each holding their own,
// independently-mirrored, ungrouped pair, running Fix Symmetry targeting layer 0 only groups layer
// 0's pair; layer 1's pair stays untouched even though it would also be a legitimate match on its
// own layer.
void RunRowScopedWriteChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("Layer0A", 2.0f, 3.0f, 0, 0));
    markers[0].transforms.push_back(MakeTransform("Layer0B", 8.0f, 3.0f, 0, 0));
    markers[0].transforms.push_back(MakeTransform("Layer1A", 4.0f, 5.0f, 0, 1));
    markers[0].transforms.push_back(MakeTransform("Layer1B", 6.0f, 5.0f, 0, 1));
    const Params::Geometry geometry = MakeTestGeometry();

    FixMarkerLayerSymmetry(markers, geometry, 0, Params::SymmetryAxis::MirrorAcrossX, 3, 0.5f, false);

    Check(markers[0].transforms[0].symmetryGroupIdentifier > 0
              && markers[0].transforms[0].symmetryGroupIdentifier
                     == markers[0].transforms[1].symmetryGroupIdentifier,
          "layer 0's pair is grouped by the layer-0 press");
    Check(markers[0].transforms[2].symmetryGroupIdentifier == 0
              && markers[0].transforms[3].symmetryGroupIdentifier == 0,
          "layer 1's pair is left untouched — the write path stays row-correct");
}

// Test 7 (acceptance test 7 — determinism) — the SAME positions/mask/tolerance, fed via two
// different (but equivalent) MarkerInstanceGroup orderings, produce the same confirmed/unmatched
// counts and group the same physical positions together.
void RunDeterminismChecks() {
    const Params::Geometry geometry = MakeTestGeometry();

    std::vector<Params::MarkerInstanceGroup> forwardMarkers(2);
    forwardMarkers[0].transforms.push_back(MakeTransform("A", 2.0f, 3.0f, 0, 0));
    forwardMarkers[1].transforms.push_back(MakeTransform("B", 8.0f, 3.0f, 0, 0));
    std::vector<Params::MarkerInstanceGroup> reversedMarkers(2);
    reversedMarkers[0].transforms.push_back(MakeTransform("B", 8.0f, 3.0f, 0, 0));
    reversedMarkers[1].transforms.push_back(MakeTransform("A", 2.0f, 3.0f, 0, 0));

    const MarkerSymmetryFixResult forwardResult = FixMarkerLayerSymmetry(
        forwardMarkers, geometry, 0, Params::SymmetryAxis::MirrorAcrossX, 3, 0.5f, false);
    const MarkerSymmetryFixResult reversedResult = FixMarkerLayerSymmetry(
        reversedMarkers, geometry, 0, Params::SymmetryAxis::MirrorAcrossX, 3, 0.5f, false);

    Check(forwardResult.confirmedGroupCount == reversedResult.confirmedGroupCount,
          "reordering the group vector does not change the confirmed group count");
    Check(forwardResult.unmatchedSlotCount == reversedResult.unmatchedSlotCount,
          "nor the unmatched slot count");
    Check(forwardMarkers[0].transforms[0].symmetryGroupIdentifier
              == forwardMarkers[1].transforms[0].symmetryGroupIdentifier,
          "forward order: the (2,3)/(8,3) pair shares one id");
    Check(reversedMarkers[0].transforms[0].symmetryGroupIdentifier
              == reversedMarkers[1].transforms[0].symmetryGroupIdentifier,
          "reversed order: the same physical pair shares one id");
}

} // namespace

int main() {
    RunBasicSkipModeChecks();
    RunSkipModeIgnoresAlreadyGroupedChecks();
    RunFreshIdAllocationChecks();
    RunOverwriteModeWidensPoolChecks();
    RunCrossLayerIsolationChecks();
    RunRowScopedWriteChecks();
    RunDeterminismChecks();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
