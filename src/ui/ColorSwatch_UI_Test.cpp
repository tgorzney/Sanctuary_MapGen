// ColorSwatch_UI_Test.cpp — acceptance test for the picker-only color swatch (A2).
// Covers the channel repair, the packed-color round trip and the open/edit/close commit
// sequence, driven by a synthetic picker sequence. No imgui frame, no window, no GL: the
// interaction is pure by construction (ColorSwatch_UI.h). The popup's pixels are a by-eye check
// against a live frame — nothing here asserts on them. The "no RGBA inputs" rule itself is a
// draw-path flag (ImGuiColorEditFlags_NoInputs in ColorSwatch_UI.cpp), not testable headless.
#include "ColorSwatch_UI.h"
#include <cmath>
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

static void TestClampingRepairsEveryChannel() {
    Ui::ColorSwatchOptions opaqueOptions;                      // alpha editing off
    float outOfRange[4] = {-2.0f, 4.0f, 0.25f, 0.5f};
    Ui::ClampSwatchColor(outOfRange, opaqueOptions);
    Check(IsNear(outOfRange[0], 0.0f) && IsNear(outOfRange[1], 1.0f) && IsNear(outOfRange[2], 0.25f),
          "channels clamp into 0..1");
    Check(IsNear(outOfRange[3], 1.0f), "alpha is forced opaque while alpha editing is off");

    Ui::ColorSwatchOptions alphaOptions;
    alphaOptions.bAlphaEnabled = true;
    float translucent[4] = {0.5f, 0.5f, 0.5f, 0.25f};
    Ui::ClampSwatchColor(translucent, alphaOptions);
    Check(IsNear(translucent[3], 0.25f), "an alpha-enabled swatch keeps its alpha");

    const float notANumber = std::nanf("");
    float corrupt[4] = {notANumber, notANumber, 0.5f, notANumber};
    Ui::ClampSwatchColor(corrupt, alphaOptions);
    Check(IsNear(corrupt[0], 0.0f) && IsNear(corrupt[3], 0.0f), "NaN channels repair to 0, never propagate");
}

static void TestPackedColorRoundTrips() {
    const float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    Check(Ui::PackedColorFromSwatchColor(white) == 0xFFFFFFFFu, "white packs to every byte set");
    const float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    Check(Ui::PackedColorFromSwatchColor(black) == 0u, "transparent black packs to zero");

    // 0xAABBGGRR byte order: pure red must land in the LOW byte, matching IM_COL32.
    const float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    Check(Ui::PackedColorFromSwatchColor(red) == 0xFF0000FFu, "red occupies the low byte (0xAABBGGRR)");

    float restored[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    const float original[4] = {0.0f, 0.2f, 0.6f, 1.0f};
    Ui::SwatchColorFromPackedColor(Ui::PackedColorFromSwatchColor(original), restored);
    Check(IsNear(restored[0], original[0], 0.004f) && IsNear(restored[1], original[1], 0.004f) &&
          IsNear(restored[2], original[2], 0.004f) && IsNear(restored[3], original[3], 0.004f),
          "a pack/unpack round trip lands within one 8-bit step");
    Check(Ui::SwatchColorsMatch(white, white) && !Ui::SwatchColorsMatch(white, red),
          "colors compare channel by channel");
}

// One synthetic picker session: open, edit on the listed frames, then close.
struct PickerTally { int changeCount = 0; int commitCount = 0; bool bCommittedWhileOpen = false; };

static PickerTally RunPickerSession(Ui::RealtimeToggle& realtimeToggle, float color[4],
                                    const Ui::ColorSwatchOptions& options,
                                    const bool* editedPerFrame, int frameCount) {
    PickerTally tally;
    Ui::ColorSwatchInput input;
    input.bPickerOpen = true;
    for (int frame = 0; frame < frameCount; ++frame) {
        input.bColorEdited = editedPerFrame[frame];
        const Ui::WidgetChange change = Ui::StepColorSwatchInteraction(realtimeToggle, color, options, input);
        if (change.bValueChanged) ++tally.changeCount;
        if (change.bCommitted) { ++tally.commitCount; tally.bCommittedWhileOpen = true; }
    }
    Ui::ColorSwatchInput closedInput;                                   // the frame the popup closes
    const Ui::WidgetChange closed = Ui::StepColorSwatchInteraction(realtimeToggle, color, options, closedInput);
    if (closed.bValueChanged) ++tally.changeCount;
    if (closed.bCommitted) ++tally.commitCount;
    return tally;
}

static void TestPickingDefersItsCommitUntilThePopupCloses() {
    const bool editedPerFrame[4] = {true, false, true, false};
    Ui::ColorSwatchOptions options;
    float color[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    Ui::RealtimeToggle realtimeToggle;                                  // RT off

    const PickerTally tally = RunPickerSession(realtimeToggle, color, options, editedPerFrame, 4);
    Check(tally.changeCount == 2, "one live change per frame the picker actually wrote");
    Check(!tally.bCommittedWhileOpen, "no recomposite is paid for while the picker is open");
    Check(tally.commitCount == 1, "exactly one commit, on the frame the popup closes");

    Ui::RealtimeToggle alwaysOnToggle(true);
    float realtimeColor[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    const PickerTally realtimeTally =
        RunPickerSession(alwaysOnToggle, realtimeColor, options, editedPerFrame, 4);
    Check(realtimeTally.changeCount == 2 && realtimeTally.commitCount == 2, "realtime commits live");

    // A session that opened and closed without touching the color costs nothing.
    const bool neverEdited[2] = {false, false};
    Ui::RealtimeToggle idleToggle;
    float untouched[4] = {0.25f, 0.25f, 0.25f, 1.0f};
    const PickerTally idleTally = RunPickerSession(idleToggle, untouched, options, neverEdited, 2);
    Check(idleTally.changeCount == 0 && idleTally.commitCount == 0, "an untouched picker never commits");

    // The step repairs whatever the picker wrote, so an out-of-range write cannot escape.
    Ui::ColorSwatchInput input;
    input.bPickerOpen = true; input.bColorEdited = true;
    float escaping[4] = {2.0f, -1.0f, 0.5f, 0.3f};
    Ui::StepColorSwatchInteraction(idleToggle, escaping, options, input);
    Check(IsNear(escaping[0], 1.0f) && IsNear(escaping[1], 0.0f) && IsNear(escaping[3], 1.0f),
          "the interaction step re-clamps what the picker wrote");
}

int main() {
    TestClampingRepairsEveryChannel();
    TestPackedColorRoundTrips();
    TestPickingDefersItsCommitUntilThePopupCloses();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
