// GradientEditorWidget_UI_Test.cpp — acceptance test for the M5-3 gradient editor.
// The edit semantics are imgui-free by construction, so every case runs headless and each edit is
// proved END TO END: mutate the Params::GradientRamp, then bake it with the REAL M4-2
// Ui::BakeGradientLut and assert the LUT reflects the edit.
#include "GradientEditorWidget_UI.h"
#include "GradientLut_UI.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static bool IsNear(float value, float expected) {
    const float difference = value - expected;
    return difference < 1.0e-6f && difference > -1.0e-6f;
}

static const float kBlack[4] = {0.0f, 0.0f, 0.0f, 1.0f};
static const float kWhite[4] = {1.0f, 1.0f, 1.0f, 1.0f};
static const float kRed[4]   = {1.0f, 0.0f, 0.0f, 1.0f};
static const float kGreen[4] = {0.0f, 1.0f, 0.0f, 1.0f};

static Params::GradientRamp MakeBlackToWhiteRamp() {
    Params::GradientRamp ramp;
    ramp.bSmoothInterpolation = false;
    Ui::AddGradientStop(ramp, 1.0f, kWhite);      // deliberately added out of order
    Ui::AddGradientStop(ramp, 0.0f, kBlack);
    return ramp;
}

static void TestAddStopSortsAndReachesTheBake() {
    Params::GradientRamp ramp = MakeBlackToWhiteRamp();
    Check(ramp.stops.size() == 2u, "add builds two stops");
    Check(ramp.stops[0].location == 0.0f && ramp.stops[1].location == 1.0f, "add keeps sorted");
    const std::vector<float> beforeTable = Ui::BakeGradientLut(ramp, 5);
    Check(IsNear(beforeTable[2 * 4], 0.5f), "baseline LUT midpoint is grey");

    const int newStopIndex = Ui::AddGradientStop(ramp, 0.5f, kRed);
    Check(newStopIndex == 1, "new stop lands between its neighbours");
    Check(ramp.stops.size() == 3u && ramp.stops[1].color[0] == 1.0f, "added stop carries its color");

    const std::vector<float> afterTable = Ui::BakeGradientLut(ramp, 5);
    Check(IsNear(afterTable[2 * 4], 1.0f) && IsNear(afterTable[2 * 4 + 1], 0.0f),
          "LUT midpoint is now the added stop");
    Check(IsNear(afterTable[1 * 4], 0.5f) && IsNear(afterTable[1 * 4 + 1], 0.0f),
          "LUT quarter point blends toward the added stop");
}

static void TestMoveStopClampsAndKeepsIdentity() {
    Params::GradientRamp ramp = MakeBlackToWhiteRamp();
    const int redIndex = Ui::AddGradientStop(ramp, 0.5f, kRed);
    Check(Ui::MoveGradientStop(ramp, redIndex, 0.25f), "move reports a change");
    Check(!Ui::MoveGradientStop(ramp, redIndex, 0.25f), "a no-op move reports no change");
    Check(!Ui::MoveGradientStop(ramp, 7, 0.5f), "out-of-range move is rejected");
    Check(ramp.stops[1].location == 0.25f, "moved stop holds its new location");
    const std::vector<float> movedTable = Ui::BakeGradientLut(ramp, 5);
    Check(IsNear(movedTable[1 * 4], 1.0f) && IsNear(movedTable[1 * 4 + 1], 0.0f),
          "LUT quarter point is now the moved stop");

    // Dragged PAST a neighbour: index identity survives, the vector goes unsorted, and the bake
    // still matches the equivalent pre-sorted ramp (BakeGradientLut sorts into its own copy).
    Ui::MoveGradientStop(ramp, 0, 0.75f);
    Check(ramp.stops[0].color[0] == 0.0f && ramp.stops[0].location == 0.75f,
          "index 0 is still the black stop after the drag");
    Params::GradientRamp sortedRamp;
    sortedRamp.bSmoothInterpolation = false;
    Ui::AddGradientStop(sortedRamp, 0.25f, kRed);
    Ui::AddGradientStop(sortedRamp, 0.75f, kBlack);
    Ui::AddGradientStop(sortedRamp, 1.0f, kWhite);
    Check(Ui::BakeGradientLut(ramp, 33) == Ui::BakeGradientLut(sortedRamp, 33),
          "an unsorted drag bakes identically to the sorted ramp");

    Ui::MoveGradientStop(ramp, 0, 5.0f);
    Check(ramp.stops[0].location == 1.0f, "move clamps above 1");
    Ui::MoveGradientStop(ramp, 0, -5.0f);
    Check(ramp.stops[0].location == 0.0f, "move clamps below 0");
}

static void TestDeleteStopReachesTheBake() {
    Params::GradientRamp ramp = MakeBlackToWhiteRamp();
    Ui::AddGradientStop(ramp, 0.5f, kRed);
    const std::vector<float> withRedTable = Ui::BakeGradientLut(ramp, 5);
    Check(!Ui::DeleteGradientStop(ramp, 9), "out-of-range delete is rejected");
    Check(Ui::DeleteGradientStop(ramp, 1), "delete reports a change");
    Check(ramp.stops.size() == 2u && ramp.stops[1].location == 1.0f, "the right stop was removed");

    const std::vector<float> withoutRedTable = Ui::BakeGradientLut(ramp, 5);
    Check(withoutRedTable != withRedTable, "delete changes the LUT");
    Check(withoutRedTable == Ui::BakeGradientLut(MakeBlackToWhiteRamp(), 5),
          "the LUT is back to the two-stop bake");

    Ui::DeleteGradientStop(ramp, 1);
    Ui::DeleteGradientStop(ramp, 0);
    Check(ramp.stops.empty(), "deleting every stop is allowed");
    Check(Ui::BakeGradientLut(ramp, 4).size() == 4u * 4u, "an emptied ramp still bakes safely");
}

static void TestRecolorAndSmoothToggleReachTheBake() {
    Params::GradientRamp ramp = MakeBlackToWhiteRamp();
    Check(Ui::RecolorGradientStop(ramp, 1, kGreen), "recolor reports a change");
    Check(!Ui::RecolorGradientStop(ramp, 1, kGreen), "a no-op recolor reports no change");
    Check(!Ui::RecolorGradientStop(ramp, 4, kGreen), "out-of-range recolor is rejected");

    const std::vector<float> linearTable = Ui::BakeGradientLut(ramp, 5);
    Check(IsNear(linearTable[4 * 4 + 1], 1.0f) && IsNear(linearTable[4 * 4], 0.0f),
          "recolored endpoint shows in the LUT");
    Check(IsNear(linearTable[1 * 4 + 1], 0.25f), "linear quarter point");

    Check(Ui::SetGradientSmoothInterpolation(ramp, true), "smooth toggle reports a change");
    Check(!Ui::SetGradientSmoothInterpolation(ramp, true), "a no-op toggle reports no change");
    const std::vector<float> smoothTable = Ui::BakeGradientLut(ramp, 5);
    Check(IsNear(smoothTable[1 * 4 + 1], 0.15625f), "smooth quarter point (the M4-2 curve)");
}

static void TestStopFenceAndNearestQuery() {
    Params::GradientRamp fullRamp;
    for (int stop = 0; stop < Ui::kMaximumGradientStopCount; ++stop)
        Check(Ui::AddGradientStop(fullRamp, 0.5f, kWhite) >= 0, "fence fills");
    Check(Ui::AddGradientStop(fullRamp, 0.5f, kWhite) == -1, "the stop-count fence rejects");
    Check(static_cast<int>(fullRamp.stops.size()) == Ui::kMaximumGradientStopCount,
          "a rejected add does not grow the ramp");

    Params::GradientRamp ramp = MakeBlackToWhiteRamp();
    Ui::AddGradientStop(ramp, 0.5f, kRed);
    Check(Ui::NearestGradientStopIndex(ramp, 0.45f) == 1, "nearest picks the middle stop");
    Check(Ui::NearestGradientStopIndex(ramp, 0.99f) == 2, "nearest picks the end stop");
    Params::GradientRamp emptyRamp;
    Check(Ui::NearestGradientStopIndex(emptyRamp, 0.5f) == -1, "nearest on an empty ramp is -1");
}

int main() {
    TestAddStopSortsAndReachesTheBake();
    TestMoveStopClampsAndKeepsIdentity();
    TestDeleteStopReachesTheBake();
    TestRecolorAndSmoothToggleReachTheBake();
    TestStopFenceAndNearestQuery();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
