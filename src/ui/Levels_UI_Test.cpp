// Levels_UI_Test.cpp — acceptance test for the Levels control (A2).
// Covers the transfer function, the clamping contract, the histogram bucketing and the
// drag/release commit. No imgui frame, no window, no GL: the logic is pure by construction
// (Levels_UI.h / LevelsHistogram_UI.h). The strip's pixels are a by-eye check against a live
// frame — nothing here asserts on them.
#include "Levels_UI.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static bool IsNear(float value, float expected, float tolerance = 1.0e-5f) {
    const float difference = value - expected;
    return difference < tolerance && difference > -tolerance;
}

static void TestDefaultsAreTheIdentityCurve() {
    const Ui::LevelsSettings identity;
    Check(IsNear(Ui::ApplyLevels(0.0f, identity), 0.0f) && IsNear(Ui::ApplyLevels(1.0f, identity), 1.0f) &&
          IsNear(Ui::ApplyLevels(0.37f, identity), 0.37f), "the default settings pass values through");
}

static void TestInputRangeAndMidtonesShapeTheCurve() {
    Ui::LevelsSettings settings;
    settings.inputShadows = 0.25f; settings.inputHighlights = 0.75f;
    Check(IsNear(Ui::ApplyLevels(0.25f, settings), 0.0f), "the shadow point maps to black");
    Check(IsNear(Ui::ApplyLevels(0.75f, settings), 1.0f), "the highlight point maps to white");
    Check(IsNear(Ui::ApplyLevels(0.50f, settings), 0.5f), "and the span is linear between them");
    Check(IsNear(Ui::ApplyLevels(0.10f, settings), 0.0f) && IsNear(Ui::ApplyLevels(0.90f, settings), 1.0f),
          "values outside the input span clip rather than escape 0..1");

    Ui::LevelsSettings brightened = settings;
    brightened.inputMidtones = 2.0f;
    Check(Ui::ApplyLevels(0.5f, brightened) > 0.5f, "a midtone gamma above 1 brightens");
    Ui::LevelsSettings darkened = settings;
    darkened.inputMidtones = 0.5f;
    Check(Ui::ApplyLevels(0.5f, darkened) < 0.5f, "and below 1 darkens");
    Check(IsNear(Ui::ApplyLevels(0.25f, brightened), 0.0f) && IsNear(Ui::ApplyLevels(0.75f, brightened), 1.0f),
          "the gamma never moves the two end points");

    Ui::LevelsSettings compressed;
    compressed.outputBlack = 0.2f; compressed.outputWhite = 0.8f;
    Check(IsNear(Ui::ApplyLevels(0.0f, compressed), 0.2f) && IsNear(Ui::ApplyLevels(1.0f, compressed), 0.8f),
          "the output range compresses the result");
    Ui::LevelsSettings inverted;
    inverted.outputBlack = 1.0f; inverted.outputWhite = 0.0f;
    Check(IsNear(Ui::ApplyLevels(0.0f, inverted), 1.0f) && IsNear(Ui::ApplyLevels(1.0f, inverted), 0.0f),
          "an inverted output range is honored, not repaired");
}

static void TestClampingRepairsNonsenseSettings() {
    const Ui::LevelsBounds bounds;
    Ui::LevelsSettings outOfRange;
    outOfRange.inputShadows = -4.0f; outOfRange.inputHighlights = 9.0f;
    outOfRange.inputMidtones = 50.0f; outOfRange.outputWhite = 3.0f;
    const Ui::LevelsSettings clamped = Ui::ClampLevelsSettings(outOfRange, bounds);
    Check(IsNear(clamped.inputShadows, 0.0f) && IsNear(clamped.inputHighlights, 1.0f),
          "input points clamp into 0..1");
    Check(IsNear(clamped.inputMidtones, bounds.midtonesMaximum), "midtones clamp to their limits");
    Check(IsNear(clamped.outputWhite, 1.0f), "output points clamp into 0..1");

    Ui::LevelsSettings invertedInput;
    invertedInput.inputShadows = 0.8f; invertedInput.inputHighlights = 0.2f;
    const Ui::LevelsSettings ordered = Ui::ClampLevelsSettings(invertedInput, bounds);
    Check(IsNear(ordered.inputShadows, 0.2f) && IsNear(ordered.inputHighlights, 0.8f),
          "an inverted input pair is put back in order without losing either value");

    Ui::LevelsSettings collapsed;
    collapsed.inputShadows = 0.5f; collapsed.inputHighlights = 0.5f;
    const Ui::LevelsSettings separated = Ui::ClampLevelsSettings(collapsed, bounds);
    Check(IsNear(separated.inputHighlights - separated.inputShadows, bounds.minimumInputSeparation),
          "a collapsed input span is opened to the minimum separation");
    Ui::LevelsSettings atTheTop;
    atTheTop.inputShadows = 1.0f; atTheTop.inputHighlights = 1.0f;
    const Ui::LevelsSettings pushedDown = Ui::ClampLevelsSettings(atTheTop, bounds);
    Check(IsNear(pushedDown.inputHighlights, 1.0f) && pushedDown.inputShadows < 1.0f,
          "separation at the top pushes the shadow point down, never past the limit");
    Check(IsNear(Ui::ApplyLevels(0.5f, collapsed), 0.0f) && IsNear(Ui::ApplyLevels(0.9f, collapsed), 1.0f),
          "a degenerate span answers a defined near-step instead of dividing by zero");
}

static void TestHistogramBucketing() {
    float buckets[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    const float samples[8] = {0.0f, 0.1f, 0.3f, 0.4f, 0.6f, 0.99f, -5.0f, 7.0f};
    Ui::AccumulateLevelsHistogram(samples, 8, 1, buckets, 4);
    Check(IsNear(buckets[0], 3.0f), "the low bucket also takes everything below the range");
    Check(IsNear(buckets[1], 2.0f) && IsNear(buckets[2], 1.0f), "samples land in the bucket they belong to");
    Check(IsNear(buckets[3], 2.0f), "the high bucket also takes everything above the range");
    Check(IsNear(buckets[0] + buckets[1] + buckets[2] + buckets[3], 8.0f), "no sample is dropped");

    float strided[2] = {0.0f, 0.0f};
    Ui::AccumulateLevelsHistogram(samples, 8, 4, strided, 2);
    Check(IsNear(strided[0] + strided[1], 2.0f), "a stride summarizes without reading every sample");

    Check(IsNear(Ui::LargestHistogramBucket(buckets, 4), 3.0f), "the tallest bucket is found");
    Ui::NormalizeLevelsHistogram(buckets, 4);
    Check(IsNear(Ui::LargestHistogramBucket(buckets, 4), 1.0f), "normalizing scales the tallest to 1");
    float emptyBuckets[3] = {0.0f, 0.0f, 0.0f};   Ui::NormalizeLevelsHistogram(emptyBuckets, 3);
    Check(IsNear(emptyBuckets[0], 0.0f), "an empty histogram normalizes without dividing by zero");

    Ui::LevelsHistogramView view;
    Check(!Ui::LevelsHistogramViewIsDrawable(view), "an unset view is not drawn");
    view.bucketWeights = buckets; view.bucketCount = 4;
    Check(Ui::LevelsHistogramViewIsDrawable(view), "a filled view is");
    Ui::AccumulateLevelsHistogram(nullptr, 8, 1, buckets, 4);          // must not crash or write
    Check(IsNear(Ui::LargestHistogramBucket(buckets, 4), 1.0f), "a null sample array is ignored");
}

static void TestFieldDragDefersItsCommitUntilRelease() {
    const Ui::LevelsBounds bounds;
    Ui::LevelsSettings settings;
    Ui::RealtimeToggle realtimeToggle;                                  // RT off
    Ui::LevelsFieldInput input;
    input.bFieldActive = true;
    int commitCount = 0, changeCount = 0;
    for (int frame = 0; frame < 3; ++frame) {
        input.bFieldEdited = frame != 1;                                // frame 1 does not move
        settings.inputMidtones += 0.1f;
        const Ui::WidgetChange change = Ui::StepLevelsInteraction(realtimeToggle, settings, bounds, input);
        if (change.bValueChanged) ++changeCount;
        if (change.bCommitted) ++commitCount;
    }
    Check(changeCount == 2 && commitCount == 0, "no recompute is paid for during the drag");
    input.bFieldActive = false; input.bFieldEdited = false;
    const Ui::WidgetChange release = Ui::StepLevelsInteraction(realtimeToggle, settings, bounds, input);
    Check(release.bCommitted, "exactly one commit, on release");

    Ui::LevelsSettings escaping;   escaping.inputMidtones = 500.0f;
    Ui::RealtimeToggle idleToggle;
    const Ui::LevelsFieldInput idleInput;
    Ui::StepLevelsInteraction(idleToggle, escaping, bounds, idleInput);
    Check(IsNear(escaping.inputMidtones, bounds.midtonesMaximum), "the step re-clamps what a field wrote");
}

int main() {
    TestDefaultsAreTheIdentityCurve();
    TestInputRangeAndMidtonesShapeTheCurve();
    TestClampingRepairsNonsenseSettings();
    TestHistogramBucketing();
    TestFieldDragDefersItsCommitUntilRelease();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
