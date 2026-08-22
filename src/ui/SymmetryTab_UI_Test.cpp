// SymmetryTab_UI_Test.cpp — WO C1 acceptance for the Symmetry tab, rewritten for STEP8: the tab
// now edits `recipe.globalSymmetryMask` directly through the shared `DrawIndependentSymmetryAxes`
// (`PlacementRuleSections_UI.h`), so these checks drive that pure bit-mask arithmetic instead of
// the removed exclusive-option scheme. No imgui frame, window or GL context is needed.
#include "SymmetryTab_UI.h"
#include "PlacementRuleSections_UI.h"
#include "../params/MapRecipe_PARAMS.h"
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

// Each of the 4 real axis bits toggles independently and round-trips through the same mask word
// the tab now edits directly — no presentation word, no exclusivity.
void RunAxisOptionChecks() {
    int symmetryMask = Params::SymmetryAxis::None;
    for (int axisIndex = 0; axisIndex < kPlacementSymmetryAxisCount; ++axisIndex)
        Check(!IsPlacementSymmetryAxisSet(symmetryMask, axisIndex), "a fresh mask sets no axis");

    // Toggle every axis on, one at a time, and confirm the others are left alone.
    for (int axisIndex = 0; axisIndex < kPlacementSymmetryAxisCount; ++axisIndex) {
        symmetryMask = PlacementSymmetryMaskAfterToggle(symmetryMask, axisIndex);
        Check(IsPlacementSymmetryAxisSet(symmetryMask, axisIndex),
              "toggling an axis on sets exactly that axis");
    }
    for (int axisIndex = 0; axisIndex < kPlacementSymmetryAxisCount; ++axisIndex)
        Check(IsPlacementSymmetryAxisSet(symmetryMask, axisIndex),
              "all four axes coexist - the row the old exclusive scheme could never draw");

    // A combination the old 5-option row could never show: Mirror X + Quarter Turns.
    int combinationMask = Params::SymmetryAxis::MirrorAcrossX | Params::SymmetryAxis::QuarterTurns;
    Check(IsPlacementSymmetryAxisSet(combinationMask, 0) && IsPlacementSymmetryAxisSet(combinationMask, 3)
          && !IsPlacementSymmetryAxisSet(combinationMask, 1) && !IsPlacementSymmetryAxisSet(combinationMask, 2),
          "MirrorAcrossX | QuarterTurns shows exactly its two bits and nothing else");

    // The actual regression case: Mirror X + Half Turn together, arrived at from OUTSIDE the tab
    // (e.g. a per-rule override copied to global, or a hand-edited .sanmap) - showable as-is.
    const int regressionMask = Params::SymmetryAxis::MirrorAcrossX | Params::SymmetryAxis::RotateHalfTurn;
    Check(IsPlacementSymmetryAxisSet(regressionMask, 0) && IsPlacementSymmetryAxisSet(regressionMask, 2),
          "MirrorAcrossX | RotateHalfTurn is showable - the old exclusive row could not represent it");
    Check(ResolvedPlacementSymmetryMask(regressionMask) == regressionMask,
          "and it survives the Constitution Section 6 repair unchanged, since both bits are legal");

    // Settable: ticking two boxes (simulated as two sequential toggles from empty) produces the OR
    // of their bits, exactly as DrawIndependentSymmetryAxes's loop would leave it.
    int settledMask = Params::SymmetryAxis::None;
    settledMask = PlacementSymmetryMaskAfterToggle(settledMask, 0); // Mirror X
    settledMask = PlacementSymmetryMaskAfterToggle(settledMask, 2); // Half Turn
    Check(settledMask == regressionMask,
          "ticking Mirror X then Half Turn from the tab produces their OR, matching the recipe mask");

    // An out-of-range index changes nothing rather than shifting by an illegal amount.
    const int beforeMask = settledMask;
    Check(PlacementSymmetryMaskAfterToggle(settledMask, -1) == beforeMask
          && PlacementSymmetryMaskAfterToggle(settledMask, kPlacementSymmetryAxisCount) == beforeMask,
          "an out-of-range axis index is refused, not obeyed");
}

// The tab edits `recipe.globalSymmetryMask` directly - no mirror word, no separate load/store step.
void RunMirrorChecks() {
    Params::MapRecipe recipe;
    // STEP16_SymmetryGlobalSettings_IO (ARCH-ratified): the default changed from None to
    // RotateHalfTurn.
    Check(recipe.globalSymmetryMask == Params::SymmetryAxis::RotateHalfTurn,
          "a new recipe carries the RotateHalfTurn default");

    recipe.globalSymmetryMask = Params::SymmetryAxis::MirrorAcrossZ;
    Check(IsPlacementSymmetryAxisSet(recipe.globalSymmetryMask, 1), "the recipe's mask is read directly");

    // Toggling Mirror X on the recipe's own mask ORs it in rather than replacing Mirror Z - the
    // exact behaviour the old exclusive row could not offer.
    recipe.globalSymmetryMask = PlacementSymmetryMaskAfterToggle(recipe.globalSymmetryMask, 0);
    Check(recipe.globalSymmetryMask
          == (Params::SymmetryAxis::MirrorAcrossX | Params::SymmetryAxis::MirrorAcrossZ),
          "ticking a second axis combines with the first rather than replacing it");

    // Clicking an active axis off again clears just that bit; "no symmetry" remains reachable.
    recipe.globalSymmetryMask = PlacementSymmetryMaskAfterToggle(recipe.globalSymmetryMask, 0);
    recipe.globalSymmetryMask = PlacementSymmetryMaskAfterToggle(recipe.globalSymmetryMask, 1);
    Check(recipe.globalSymmetryMask == Params::SymmetryAxis::None,
          "toggling both active axes back off returns the recipe to no symmetry");

    // A hand-edited recipe carrying a stray bit no v2 axis owns is repaired, never obeyed
    // (Constitution Section 6) - the same repair `DrawIndependentSymmetryAxes` performs on draw.
    const int strayBit = 1 << 20;
    recipe.globalSymmetryMask = Params::SymmetryAxis::MirrorAcrossX | strayBit;
    Check(ResolvedPlacementSymmetryMask(recipe.globalSymmetryMask) == Params::SymmetryAxis::MirrorAcrossX,
          "a stray bit outside the five real axes is dropped by the repair");
}

// The two settings this work-order promoted into Symmetry_PARAMS.h.
void RunDetectionSettingChecks() {
    Params::SymmetryDetection detection;
    Check(detection.detectionTolerance == 0.01f, "detection tolerance carries v1's default");
    Check(!detection.bSnapImperfectSymmetry, "snapping is off by default - it is destructive");

    SymmetryTabState state;
    Check(state.detectionToleranceRange.minimumValue == 0.0f
          && state.detectionToleranceRange.maximumValue > detection.detectionTolerance,
          "the default tolerance sits inside its own slider");
    Check(ClampScalarSliderValue(5.0f, state.detectionToleranceRange)
          == state.detectionToleranceRange.maximumValue,
          "a tolerance driven past the top of the range is held there");

    WidgetChange change = StepCheckboxInteraction(detection.bSnapImperfectSymmetry, true);
    Check(change.bValueChanged && detection.bSnapImperfectSymmetry, "the snap tick flips the setting");
    change = StepCheckboxInteraction(detection.bSnapImperfectSymmetry, false);
    Check(!change.bValueChanged && detection.bSnapImperfectSymmetry, "an untouched frame costs nothing");
}

} // namespace

int main() {
    RunAxisOptionChecks();
    RunMirrorChecks();
    RunDetectionSettingChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
