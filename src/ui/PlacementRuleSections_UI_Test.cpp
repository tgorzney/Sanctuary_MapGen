// PlacementRuleSections_UI_Test.cpp — tab-rebuild WO C4 acceptance, part 2: the blocks the four
// placement tabs SHARE. This is the test that earns the shared file: the symmetry mask arithmetic
// and the transform mirrors are asserted ONCE, so Markers / Armies / Props / Areas cannot drift
// apart the way the v1 per-tab duplicates did. Pure logic only — no imgui frame, window or GL.
// Registered with CTest as its own binary by gate CD-int (CMakeLists.txt "Tab-rebuild batch C4"):
// it is nobody's tab, so it is nobody's translation unit either.
#include "PlacementRuleSections_UI.h"
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

// v1 offered five EXCLUSIVE choices; v2's mask is OR-able, so the tab draws INDEPENDENT bits.
// The combination v1 could not express (X and Z together) is the point of that change.
void RunSymmetryMaskChecks() {
    int symmetryMask = Params::SymmetryAxis::None;
    for (int axisIndex = 0; axisIndex < kPlacementSymmetryAxisCount; ++axisIndex)
        Check(!IsPlacementSymmetryAxisSet(symmetryMask, axisIndex), "an empty mask sets no axis");

    symmetryMask = PlacementSymmetryMaskAfterToggle(symmetryMask, 0);
    Check(IsPlacementSymmetryAxisSet(symmetryMask, 0), "toggling an axis on sets it");
    symmetryMask = PlacementSymmetryMaskAfterToggle(symmetryMask, 1);
    Check(IsPlacementSymmetryAxisSet(symmetryMask, 0) && IsPlacementSymmetryAxisSet(symmetryMask, 1),
          "two axes coexist - the combination v1's exclusive row could not express");
    symmetryMask = PlacementSymmetryMaskAfterToggle(symmetryMask, 0);
    Check(!IsPlacementSymmetryAxisSet(symmetryMask, 0) && IsPlacementSymmetryAxisSet(symmetryMask, 1),
          "and toggling one back off leaves the other alone");

    // An index no axis owns must change nothing rather than shift by an illegal amount.
    const int beforeMask = symmetryMask;
    Check(PlacementSymmetryMaskAfterToggle(symmetryMask, -1) == beforeMask
          && PlacementSymmetryMaskAfterToggle(symmetryMask, kPlacementSymmetryAxisCount) == beforeMask,
          "an out-of-range axis index is refused, not obeyed");
    Check(!IsPlacementSymmetryAxisSet(symmetryMask, 99), "and never reports itself set");
}

// Constitution §6: a mask from a hand-edited recipe is REPAIRED, never obeyed.
void RunSymmetryRepairChecks() {
    int legalBits = Params::SymmetryAxis::None;
    for (int axisIndex = 0; axisIndex < kPlacementSymmetryAxisCount; ++axisIndex)
        legalBits |= PlacementSymmetryAxisBit(axisIndex);
    Check(ResolvedPlacementSymmetryMask(legalBits) == legalBits,
          "every bit the tab can draw survives the repair");

    const int strayBit = 1 << 20;
    Check((strayBit & legalBits) == 0, "the probe bit really is one no axis owns");
    Check(ResolvedPlacementSymmetryMask(legalBits | strayBit) == legalBits,
          "and a bit no axis owns is dropped");
    Check(ResolvedPlacementSymmetryMask(strayBit) == Params::SymmetryAxis::None,
          "a mask of nothing but stray bits repairs to no symmetry at all");
}

// The transform mirrors. The PARAMS default is a scale band of exactly 1..1 ("no random scale"),
// which a range slider with a forced minimum separation would silently widen.
void RunTransformMirrorChecks() {
    Params::ScatterTransform transform;
    PlacementTransformState state;
    LoadPlacementTransformValues(transform, state);
    Check(state.scaleValues.minimumValue == transform.scaleMinimum
          && state.scaleValues.maximumValue == transform.scaleMaximum
          && state.rotationValues.minimumValue == transform.rotationMinimumDegrees
          && state.rotationValues.maximumValue == transform.rotationMaximumDegrees,
          "both bands reach their widget mirrors");
    Check(state.scaleBounds.minimumSeparation == 0.0f
          && state.rotationBounds.minimumSeparation == 0.0f,
          "the bands allow a zero-width range, so a 1..1 scale survives being drawn");
    Check(!StorePlacementTransformValues(state, transform),
          "storing back what was loaded reports no move");

    state.rotationValues.maximumValue = 180.0f;
    Check(StorePlacementTransformValues(state, transform)
          && transform.rotationMaximumDegrees == 180.0f,
          "and a real edit reports the move and lands on the transform");
}

// The gate block is shared by all four tabs, so its "-1 means no gate" convention is asserted here
// rather than four times over.
void RunGateStateChecks() {
    PlacementGateState state;
    Check(state.maskStratumIndexRange.minimumValue == -1.0f,
          "the stratum gate slider starts one step below zero: -1 is 'no gate'");
    Check(state.maskStratumIndexRange.increment >= 1.0f, "and snaps to whole stratum indices");
    Check(state.maskWeightRange.minimumValue == 0.0f && state.maskWeightRange.maximumValue == 1.0f,
          "the mask weight gate is a 0-1 weight");
    Check(state.edgePaddingRange.increment >= 1.0f, "edge padding snaps to whole cells");
}

} // namespace

int main() {
    RunSymmetryMaskChecks();
    RunSymmetryRepairChecks();
    RunTransformMirrorChecks();
    RunGateStateChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
