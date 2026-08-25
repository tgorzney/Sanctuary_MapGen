// MarkerDragGesture_UI_Test.cpp — STEP94's headless acceptance coverage: live per-frame mirror
// writes (test 1), cardinality collapse + restore-on-backoff + commit-only-at-release (test 2),
// cardinality growth ghost + release-time materialize (test 3), Spawn-group freeze/refusal with an
// unrestricted same-count reposition still working (test 4), and an ungrouped marker's free drag
// (test 5, plus the "no other marker moves" side-effect check its own zero-orbit-call guard implies)
// — mirroring MarkerLayerIndexRepair_UI_Test.cpp's pure, no-imgui-frame posture. `RepositionSymmetry
// GroupMember`'s ordinary and refusal cases round the coverage out (R2 §4's coordination mechanism).
#include "MarkerDragGesture_UI.h"
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

// extent = mapSize = 10 (VertexSize() - 1), worldUnitsPerCell = 1 -> world units == cell units, so
// the mirror math below (`extent - position`) is exact and easy to hand-verify.
Params::Geometry MakeTestGeometry() {
    Params::Geometry geometry;
    geometry.mapSize = 10;
    geometry.worldUnitsPerCell = 1.0f;
    return geometry;
}

Params::MarkerTransform MakeTransform(const char* name, float x, float z, int symmetryGroupIdentifier,
                                      int layerIndex = 0) {
    Params::MarkerTransform transform;
    transform.name = name;
    transform.alias = std::string(name) + "Alias";
    transform.transform.positionX = x;
    transform.transform.positionZ = z;
    transform.symmetryGroupIdentifier = symmetryGroupIdentifier;
    transform.layerIndex = layerIndex;
    return transform;
}

// Test 1 — two markers sharing a symmetryGroupIdentifier under MirrorAcrossX: dragging one updates
// the other's position every frame (not only at release) to the exact mirrored point; alias/name
// are untouched.
void RunLiveMirrorFollowChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].name = "Resources";
    markers[0].transforms.push_back(MakeTransform("Mex0", 2.0f, 3.0f, 42));
    markers[0].transforms.push_back(MakeTransform("Mex1", 8.0f, 3.0f, 42));
    const Params::Geometry geometry = MakeTestGeometry();

    MarkerDragGestureState state;
    Check(BeginMarkerDragGesture(state, markers, {}, geometry, Params::SymmetryAxis::MirrorAcrossX, 3, 0, 0),
          "gesture begins on a grouped member");
    Check(static_cast<int>(state.correspondence.size()) == 1, "exactly one sibling matched at gesture-start");

    UpdateMarkerDragGesture(state, markers, {}, geometry, 4.0f, 3.0f);
    Check(NearlyEqual(markers[0].transforms[0].transform.positionX, 4.0f), "dragged member follows the cursor");
    Check(NearlyEqual(markers[0].transforms[1].transform.positionX, 6.0f),
          "sibling mirrors LIVE, this frame, not deferred to release");
    Check(markers[0].transforms[1].name == "Mex1" && markers[0].transforms[1].alias == "Mex1Alias",
          "the sibling's name/alias are never touched by a live write");

    UpdateMarkerDragGesture(state, markers, {}, geometry, 1.0f, 3.0f);
    Check(NearlyEqual(markers[0].transforms[1].transform.positionX, 9.0f), "and again the very next frame");

    EndMarkerDragGesture(state, markers, geometry);
    Check(static_cast<int>(markers[0].transforms.size()) == 2, "no structural change for an ordinary drag");
}

// Test 2 — dragging onto the mirror axis collapses 2 -> 1: the sibling soft-hides (not erased)
// during the drag, restores live when dragged back off-axis, and the collapse only COMMITS
// (sibling actually removed) at release, exactly on-axis.
void RunCollapseRestoreCommitChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("A", 2.0f, 3.0f, 7));
    markers[0].transforms.push_back(MakeTransform("B", 8.0f, 3.0f, 7));
    const Params::Geometry geometry = MakeTestGeometry();

    MarkerDragGestureState state;
    BeginMarkerDragGesture(state, markers, {}, geometry, Params::SymmetryAxis::MirrorAcrossX, 3, 0, 0);
    Check(state.gestureStartOrbitCount == 2, "gesture-start orbit is the full 2-point mirror pair");

    UpdateMarkerDragGesture(state, markers, {}, geometry, 5.0f, 3.0f);   // exactly on the mirror axis
    Check(state.bCardinalityShrank, "landing exactly on the axis is flagged as a shrink this frame");
    Check(IsMarkerSoftHiddenThisFrame(state, 0, 1), "the sibling soft-hides while on-axis");
    Check(NearlyEqual(markers[0].transforms[1].transform.positionX, 8.0f),
          "the soft-hidden sibling's REAL data is untouched mid-drag");

    UpdateMarkerDragGesture(state, markers, {}, geometry, 4.0f, 3.0f);   // back off the axis
    Check(!state.bCardinalityShrank && !IsMarkerSoftHiddenThisFrame(state, 0, 1),
          "dragging back off-axis restores the sibling live");
    Check(NearlyEqual(markers[0].transforms[1].transform.positionX, 6.0f), "restored to the correct mirror point");

    UpdateMarkerDragGesture(state, markers, {}, geometry, 5.0f, 3.0f);   // release exactly on-axis
    EndMarkerDragGesture(state, markers, geometry);
    Check(static_cast<int>(markers[0].transforms.size()) == 1,
          "the collapse only commits (sibling actually removed) at release, on-axis");
    Check(markers[0].transforms[0].name == "A", "the dragged member itself survives the collapse");
}

// Test 3 — a drag that GROWS the orbit (here: off an axis with no sibling yet, under MirrorAcrossX)
// renders/tracks the new point as a zero-write ghost until release, then materializes it as a real
// MarkerTransform sharing the group's symmetryGroupIdentifier and the dragged member's layerIndex.
void RunGrowthGhostMaterializeChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("Seed", 5.0f, 3.0f, 11, /*layerIndex=*/2));   // ON the axis
    const Params::Geometry geometry = MakeTestGeometry();

    MarkerDragGestureState state;
    BeginMarkerDragGesture(state, markers, {}, geometry, Params::SymmetryAxis::MirrorAcrossX, 3, 0, 0);
    Check(state.gestureStartOrbitCount == 1, "on-axis, no sibling: the gesture-start orbit is one point");

    UpdateMarkerDragGesture(state, markers, {}, geometry, 7.0f, 3.0f);   // off the axis
    Check(state.bCardinalityGrew, "moving off the axis is flagged as growth this frame");
    Check(static_cast<int>(markers[0].transforms.size()) == 1, "zero PARAMS write for the ghost point");
    Check(static_cast<int>(state.currentGhostPoints.size()) == 1, "exactly one ghost point this frame");
    Check(NearlyEqual(state.currentGhostPoints[0].worldPositionX, 3.0f), "the ghost sits at the true mirror point");

    EndMarkerDragGesture(state, markers, geometry);
    Check(static_cast<int>(markers[0].transforms.size()) == 2, "release materializes exactly one new sibling");
    const Params::MarkerTransform& materialized = markers[0].transforms[1];
    Check(materialized.symmetryGroupIdentifier == 11, "the new sibling shares the group's symmetryGroupIdentifier");
    Check(materialized.layerIndex == 2, "the new sibling inherits the dragged member's layerIndex");
    Check(NearlyEqual(materialized.transform.positionX, 3.0f), "at the correct materialized position");
}

// Regression check — a release that materializes MULTIPLE new siblings in one call
// (`EndMarkerDragGesture`'s materialize loop calls `group->transforms.push_back` once per unclaimed
// slot). A stale pointer to the dragged member's own transform, cached before that loop and read
// again inside it, would go dangling the moment an earlier push_back reallocates the vector's
// backing storage — this drag (center of both mirror axes, orbitCount 1 -> 4) forces exactly that:
// three materializations in one release, so a corrupted read would show up as a wrong positionY/
// layerIndex on the second or third new sibling, not just the first.
void RunMultiPointGrowthMaterializeChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("Seed", 5.0f, 5.0f, 21, /*layerIndex=*/6));
    markers[0].transforms[0].transform.positionY = 12.5f;   // distinctive, must survive every materialize
    const Params::Geometry geometry = MakeTestGeometry();
    const int mask = Params::SymmetryAxis::MirrorAcrossX | Params::SymmetryAxis::MirrorAcrossZ;

    MarkerDragGestureState state;
    BeginMarkerDragGesture(state, markers, {}, geometry, mask, 3, 0, 0);
    Check(state.gestureStartOrbitCount == 1, "dead center of both axes: gesture-start orbit is one point");

    UpdateMarkerDragGesture(state, markers, {}, geometry, 7.0f, 8.0f);   // off BOTH axes -> full 4-point orbit
    Check(state.bCardinalityGrew, "moving fully off-axis is flagged as growth");
    Check(static_cast<int>(state.currentGhostPoints.size()) == 3, "three ghost points this frame");

    EndMarkerDragGesture(state, markers, geometry);
    Check(static_cast<int>(markers[0].transforms.size()) == 4, "release materializes all three new siblings");
    for (std::size_t index = 1; index < markers[0].transforms.size(); ++index) {
        const Params::MarkerTransform& materialized = markers[0].transforms[index];
        Check(materialized.symmetryGroupIdentifier == 21, "each new sibling shares the group's symmetryGroupIdentifier");
        Check(materialized.layerIndex == 6, "each new sibling inherits the dragged member's layerIndex intact");
        Check(NearlyEqual(materialized.transform.positionY, 12.5f),
              "each new sibling inherits the dragged member's positionY intact, even after prior push_backs");
    }
}

// Test 4 — a "Spawn"-named group refuses any cardinality-changing drag outright: the whole group
// freezes, `bSpawnCardinalityRefused` is set, and the count is unchanged after release. A same-count
// reposition of a Spawn group remains exactly as unrestricted as any other group's.
void RunSpawnRefusalChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].name = kArmyKeyedMarkerGroupName;
    markers[0].transforms.push_back(MakeTransform("Player1", 2.0f, 3.0f, 99));
    markers[0].transforms.push_back(MakeTransform("Player2", 8.0f, 3.0f, 99));
    const Params::Geometry geometry = MakeTestGeometry();

    MarkerDragGestureState state;
    BeginMarkerDragGesture(state, markers, {}, geometry, Params::SymmetryAxis::MirrorAcrossX, 3, 0, 0);
    Check(state.bSpawnGroup, "the group is recognized as the reserved Spawn roster");

    UpdateMarkerDragGesture(state, markers, {}, geometry, 4.0f, 3.0f);   // same-count reposition
    Check(!state.bSpawnCardinalityRefused, "an ordinary, same-count Spawn drag is unrestricted");
    Check(NearlyEqual(markers[0].transforms[1].transform.positionX, 6.0f), "and mirrors live like any group");

    UpdateMarkerDragGesture(state, markers, {}, geometry, 5.0f, 3.0f);   // would collapse the orbit
    Check(state.bSpawnCardinalityRefused, "a cardinality-changing Spawn drag is refused this frame");
    Check(NearlyEqual(markers[0].transforms[0].transform.positionX, 4.0f),
          "the dragged member ITSELF freezes at the last valid position (Spawn only)");
    Check(NearlyEqual(markers[0].transforms[1].transform.positionX, 6.0f), "and so does the sibling");

    EndMarkerDragGesture(state, markers, geometry);
    Check(static_cast<int>(markers[0].transforms.size()) == 2,
          "after release the Spawn group still holds exactly its pre-attempt count");
}

// Test 5 — an ungrouped marker (symmetryGroupIdentifier == 0) drags freely with an empty
// correspondence table; a structurally-similar sibling group elsewhere is never touched (the
// functional half of the "zero orbit call" claim — the code path itself never reaches
// Pipeline::BuildWorldSymmetryOrbit for symmetryGroupIdentifier == 0, see Begin/Update/End's own
// early-return guards).
void RunUngroupedFreeDragChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(2);
    markers[0].transforms.push_back(MakeTransform("Loose", 2.0f, 3.0f, 0));
    markers[1].transforms.push_back(MakeTransform("OtherA", 5.0f, 3.0f, 55));
    markers[1].transforms.push_back(MakeTransform("OtherB", 5.0f, 7.0f, 55));
    const Params::Geometry geometry = MakeTestGeometry();

    MarkerDragGestureState state;
    Check(BeginMarkerDragGesture(state, markers, {}, geometry, Params::SymmetryAxis::MirrorAcrossX, 3, 0, 0),
          "an ungrouped hit still begins a (free-drag) gesture");
    Check(state.correspondence.empty(), "with an empty correspondence table");

    UpdateMarkerDragGesture(state, markers, {}, geometry, 9.0f, 1.0f);
    Check(NearlyEqual(markers[0].transforms[0].transform.positionX, 9.0f)
          && NearlyEqual(markers[0].transforms[0].transform.positionZ, 1.0f),
          "the ungrouped member follows the cursor exactly");
    Check(NearlyEqual(markers[1].transforms[0].transform.positionX, 5.0f)
          && NearlyEqual(markers[1].transforms[1].transform.positionX, 5.0f),
          "an unrelated group is never touched by an ungrouped drag");

    EndMarkerDragGesture(state, markers, geometry);
    Check(static_cast<int>(markers[0].transforms.size()) == 1 && static_cast<int>(markers[1].transforms.size()) == 2,
          "no structural change anywhere for an ungrouped drag");
}

// RepositionSymmetryGroupMember (R2 §4's roster-slider counterpart): the ordinary case mirrors a
// sibling in one call, the cardinality-changing case refuses the sibling write but still applies
// the moved member's own target position.
void RunRepositionSymmetryGroupMemberChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("A", 2.0f, 3.0f, 3));
    markers[0].transforms.push_back(MakeTransform("B", 8.0f, 3.0f, 3));
    const Params::Geometry geometry = MakeTestGeometry();
    std::vector<Params::MarkerInstanceLayer> noLayers;

    Check(RepositionSymmetryGroupMember(markers, noLayers, geometry, Params::SymmetryAxis::MirrorAcrossX, 3,
                                        0, 0, 1.0f, 3.0f),
          "an ordinary, cardinality-preserving reposition succeeds");
    Check(NearlyEqual(markers[0].transforms[0].transform.positionX, 1.0f), "the moved member lands at the target");
    Check(NearlyEqual(markers[0].transforms[1].transform.positionX, 9.0f), "and its sibling mirrors in the same call");

    Check(!RepositionSymmetryGroupMember(markers, noLayers, geometry, Params::SymmetryAxis::MirrorAcrossX, 3,
                                         0, 0, 5.0f, 3.0f),
          "a cardinality-changing target is refused (no gesture to defer a create/delete to)");
    Check(NearlyEqual(markers[0].transforms[0].transform.positionX, 5.0f),
          "the moved member's own position still applies even when refused");
    Check(NearlyEqual(markers[0].transforms[1].transform.positionX, 9.0f),
          "the sibling is left exactly where it was - never silently misplaced");
}

// STEP106 §4 — a locked marker layer refuses `BeginMarkerDragGesture` outright: the gesture never
// activates for a dragged member whose own layer is locked.
void RunLockRefusesBeginMarkerDragGestureChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("A", 2.0f, 3.0f, 0, /*layerIndex=*/0));
    std::vector<Params::MarkerInstanceLayer> lockedLayers(1);
    lockedLayers[0].bLocked = true;
    const Params::Geometry geometry = MakeTestGeometry();

    MarkerDragGestureState state;
    Check(!BeginMarkerDragGesture(state, markers, lockedLayers, geometry, Params::SymmetryAxis::MirrorAcrossX,
                                  3, 0, 0),
          "a locked layer's marker refuses to begin a drag");
    Check(!state.bActive, "the gesture state is left inactive");
}

// STEP106 §4 — a locked marker layer refuses `RepositionSymmetryGroupMember` outright: the moved
// member's position is left untouched (unlike the ordinary cardinality-changing refusal, which
// still applies the moved member's own target).
void RunLockRefusesRepositionSymmetryGroupMemberChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform("A", 2.0f, 3.0f, 3, /*layerIndex=*/0));
    markers[0].transforms.push_back(MakeTransform("B", 8.0f, 3.0f, 3, /*layerIndex=*/0));
    std::vector<Params::MarkerInstanceLayer> lockedLayers(1);
    lockedLayers[0].bLocked = true;
    const Params::Geometry geometry = MakeTestGeometry();

    Check(!RepositionSymmetryGroupMember(markers, lockedLayers, geometry, Params::SymmetryAxis::MirrorAcrossX, 3,
                                         0, 0, 1.0f, 3.0f),
          "a locked layer's marker refuses to reposition");
    Check(NearlyEqual(markers[0].transforms[0].transform.positionX, 2.0f),
          "the moved member's own position is left unchanged - a lock refusal, unlike a cardinality "
          "refusal, applies no partial write");
    Check(NearlyEqual(markers[0].transforms[1].transform.positionX, 8.0f), "and so is its sibling's");
}

} // namespace

int main() {
    RunLiveMirrorFollowChecks();
    RunCollapseRestoreCommitChecks();
    RunGrowthGhostMaterializeChecks();
    RunMultiPointGrowthMaterializeChecks();
    RunSpawnRefusalChecks();
    RunUngroupedFreeDragChecks();
    RunRepositionSymmetryGroupMemberChecks();
    RunLockRefusesBeginMarkerDragGestureChecks();
    RunLockRefusesRepositionSymmetryGroupMemberChecks();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
