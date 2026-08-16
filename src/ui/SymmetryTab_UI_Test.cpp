// SymmetryTab_UI_Test.cpp — WO C1 acceptance for the Symmetry tab. Drives the PURE option/mask
// conversion and the two promoted detection settings with synthetic input, so no imgui frame, no
// window and no GL context is needed.
// NOT YET REGISTERED IN CMake — WO C1 does not own CMakeLists.txt; a later batch adds the
// add_sangen_test line. The file is dormant until then (and the lib glob filters *_Test.cpp out).
#include "SymmetryTab_UI.h"
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

// Five presentation options over a real bit mask in which XY is TWO bits.
void RunAxisOptionChecks() {
    Check(SymmetryAxisMaskOfOption(static_cast<int>(SymmetryAxisOption::Point))
          == Params::SymmetryAxis::RotateHalfTurn, "Point is the half-turn bit");
    Check(SymmetryAxisMaskOfOption(static_cast<int>(SymmetryAxisOption::MirrorXZ))
          == (Params::SymmetryAxis::MirrorAcrossX | Params::SymmetryAxis::MirrorAcrossZ),
          "XY is BOTH mirror bits, which is why the row is not a raw bit group");
    Check(SymmetryAxisMaskOfOption(static_cast<int>(SymmetryAxisOption::Radial))
          == Params::SymmetryAxis::QuarterTurns, "Radial is the quarter-turn bit");
    Check(SymmetryAxisMaskOfOption(-1) == Params::SymmetryAxis::None
          && SymmetryAxisMaskOfOption(kSymmetryAxisOptionCount) == Params::SymmetryAxis::None,
          "an option outside the row answers None, never a stray bit");

    for (int optionIndex = 0; optionIndex < kSymmetryAxisOptionCount; ++optionIndex) {
        const int axisMask = SymmetryAxisMaskOfOption(optionIndex);
        Check(SymmetryAxisOptionOfMask(axisMask) == optionIndex,
              "every option round-trips through the recipe's mask");
        Check(SymmetryAxisMaskOfOptionBits(SymmetryOptionBitsOfMask(axisMask)) == axisMask,
              "and through the presentation word the checkbox row edits");
        Check(symmetryAxisOptionLabels[optionIndex] != nullptr, "every option is labelled");
    }

    Check(SymmetryAxisOptionOfMask(Params::SymmetryAxis::None) == -1, "None ticks nothing");
    const int unshowableMask = Params::SymmetryAxis::MirrorAcrossX | Params::SymmetryAxis::QuarterTurns;
    Check(SymmetryAxisOptionOfMask(unshowableMask) == -1,
          "a combination the row cannot express ticks nothing rather than snapping to a neighbour");
    Check(SymmetryOptionBitsOfMask(unshowableMask) == 0u, "and its presentation word is empty");
}

// The mirror must never become a second home for the mask.
void RunMirrorChecks() {
    Params::MapRecipe recipe;
    SymmetryTabState state;
    Check(recipe.globalSymmetryMask == Params::SymmetryAxis::None, "a new recipe carries no symmetry");

    recipe.globalSymmetryMask = Params::SymmetryAxis::MirrorAcrossZ;
    LoadSymmetryTabValues(recipe.globalSymmetryMask, state);
    Check(state.axisOptionBits == (1u << static_cast<int>(SymmetryAxisOption::MirrorZ)),
          "the mirror loaded the recipe's option");
    Check(!StoreSymmetryTabValues(state, recipe.globalSymmetryMask),
          "an untouched round trip reports nothing moved");

    // One frame of the exclusive group: clicking XY replaces whatever was ticked.
    WidgetChange change = StepExclusiveCheckboxInteraction(
        state.axisOptionBits, kSymmetryAxisOptionCount,
        static_cast<int>(SymmetryAxisOption::MirrorXZ), true);
    Check(change.bValueChanged && change.bCommitted,
          "a tick commits on the same frame - a boolean has no drag to defer");
    Check(StoreSymmetryTabValues(state, recipe.globalSymmetryMask), "the store reports the move");
    Check(recipe.globalSymmetryMask
          == (Params::SymmetryAxis::MirrorAcrossX | Params::SymmetryAxis::MirrorAcrossZ),
          "and both mirror bits reached the recipe from one tick");

    // Clicking the active option clears it: "no symmetry" is a legal recipe.
    StepExclusiveCheckboxInteraction(state.axisOptionBits, kSymmetryAxisOptionCount,
                                     static_cast<int>(SymmetryAxisOption::MirrorXZ), true);
    StoreSymmetryTabValues(state, recipe.globalSymmetryMask);
    Check(recipe.globalSymmetryMask == Params::SymmetryAxis::None,
          "clicking the ticked option clears the mask");
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
