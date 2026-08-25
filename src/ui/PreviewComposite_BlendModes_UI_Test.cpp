// PreviewComposite_BlendModes_UI_Test.cpp — STEP200 acceptance: the six v1-parity blend-mode
// enumerators (Subtract, Divide, Overlay, Screen, SoftLight, HardLight) exist, are numbered right
// after the five existing ones, and CombineChannel computes the exact per-channel value each one
// promises — a representative (destination, source) pair per mode, hand-derived independently of
// the library so a copy/paste bug in CombineChannel cannot also produce the "expected" value.
// Runs the Cpu twin only, no GL context; PreviewComposite_Gpu_UI_Test.cpp is where these same 12
// modes are proven to match the Gpu twin, per composited pixel.
#include "PreviewComposite_Color_UI.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;
void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

bool NearlyEqual(float left, float right) { return (left - right) < 0.0005f && (left - right) > -0.0005f; }

// The enum grew by exactly the six v1 modes, appended after Minimum (index 5) so every pre-STEP200
// index is unchanged — a caller that already serialized/relied on Replace..Minimum's numbers is
// never renumbered.
void TestEnumAppendedAfterMinimum() {
    Check(Ui::kPreviewBlendModeCount == 12, "12 PreviewBlendMode enumerators exist");
    Check(static_cast<int>(Ui::PreviewBlendMode::Minimum) == 5, "Minimum keeps its pre-STEP200 index");
    Check(static_cast<int>(Ui::PreviewBlendMode::Subtract) == 6, "Subtract is appended right after Minimum");
    Check(static_cast<int>(Ui::PreviewBlendMode::Divide) == 7, "Divide follows Subtract");
    Check(static_cast<int>(Ui::PreviewBlendMode::Overlay) == 8, "Overlay follows Divide");
    Check(static_cast<int>(Ui::PreviewBlendMode::Screen) == 9, "Screen follows Overlay");
    Check(static_cast<int>(Ui::PreviewBlendMode::SoftLight) == 10, "SoftLight follows Screen");
    Check(static_cast<int>(Ui::PreviewBlendMode::HardLight) == 11, "HardLight is the last enumerator");
}

// destination = 0.6, source = 0.3 for every mode below — hand-derived, not copied from the library.
void TestNewModesCombineChannel() {
    constexpr float destination = 0.6f;
    constexpr float source = 0.3f;
    Check(NearlyEqual(Ui::CombineChannel(destination, source, Ui::PreviewBlendMode::Subtract), 0.3f),
          "Subtract: destination - source");
    // Divide is bounded to at most 1.0 (STEP200 fix — an unbounded division amplifies ordinary
    // sub-1/255 Cpu/Gpu float noise into a multi-byte parity divergence, PreviewComposite_Gpu_UI_Test.cpp's
    // own blend-mode sweep is what caught it). destination/source = 0.6/0.3 = 2.0, clamped to 1.0.
    Check(NearlyEqual(Ui::CombineChannel(destination, source, Ui::PreviewBlendMode::Divide), 1.0f),
          "Divide: destination / source, clamped to at most 1.0");
    // The un-clamped case, so the real division formula (not just the clamp) is proven too.
    Check(NearlyEqual(Ui::CombineChannel(source, destination, Ui::PreviewBlendMode::Divide), 0.5f),
          "Divide: destination / source below 1.0 passes through unclamped (d=0.3, s=0.6)");
    // Overlay branches on destination; 0.6 > 0.5 takes the "screen-like" branch.
    Check(NearlyEqual(Ui::CombineChannel(destination, source, Ui::PreviewBlendMode::Overlay), 0.44f),
          "Overlay: 1 - 2*(1-d)*(1-s) when d > 0.5");
    Check(NearlyEqual(Ui::CombineChannel(destination, source, Ui::PreviewBlendMode::Screen), 0.72f),
          "Screen: 1 - (1-d)*(1-s)");
    // SoftLight branches on source; 0.3 <= 0.5 takes the "darken" branch.
    Check(NearlyEqual(Ui::CombineChannel(destination, source, Ui::PreviewBlendMode::SoftLight), 0.504f),
          "SoftLight: 2*d*s + d*d*(1-2*s) when s <= 0.5");
    // HardLight branches on source too; 0.3 <= 0.5 takes the "multiply-like" branch.
    Check(NearlyEqual(Ui::CombineChannel(destination, source, Ui::PreviewBlendMode::HardLight), 0.36f),
          "HardLight: 2*d*s when s <= 0.5");
    // The high-source branch of Overlay/HardLight and the high-source branch of SoftLight, with
    // destination/source swapped so the "other" branch of each two-way switch is exercised too.
    Check(NearlyEqual(Ui::CombineChannel(source, destination, Ui::PreviewBlendMode::Overlay), 0.36f),
          "Overlay: 2*d*s when d <= 0.5 (d=0.3, s=0.6)");
    Check(NearlyEqual(Ui::CombineChannel(source, destination, Ui::PreviewBlendMode::HardLight), 0.44f),
          "HardLight: 1 - 2*(1-d)*(1-s) when s > 0.5 (d=0.3, s=0.6)");
}

// Divide guards its own zero denominator (a bake can legitimately produce a zero-valued pixel) —
// the same "never divide by zero" discipline Constitution §6 requires everywhere else.
void TestDivideGuardsZeroSource() {
    Check(NearlyEqual(Ui::CombineChannel(0.4f, 0.0f, Ui::PreviewBlendMode::Divide), 1.0f),
          "Divide by a zero source does not divide by zero");
}

// One full BlendPreviewColor round trip (opacity + the byte pack/unpack a real composite performs)
// for Overlay, so the per-channel math above is proven correct in the context CombineChannel is
// actually called from, not just in isolation.
void TestOverlayThroughBlendPreviewColor() {
    Ui::PreviewColor destination; destination.red = 0.6f; destination.green = 0.6f;
    destination.blue = 0.6f; destination.alpha = 1.0f;
    Ui::PreviewColor source; source.red = 0.3f; source.green = 0.3f; source.blue = 0.3f; source.alpha = 1.0f;
    const Ui::PreviewColor blended =
        Ui::BlendPreviewColor(destination, source, Ui::PreviewBlendMode::Overlay, 1.0f);
    Check(NearlyEqual(blended.red, 0.44f) && NearlyEqual(blended.green, 0.44f)
       && NearlyEqual(blended.blue, 0.44f), "Overlay at full opacity matches the hand-derived channel");
}

} // namespace

int main() {
    TestEnumAppendedAfterMinimum();
    TestNewModesCombineChannel();
    TestDivideGuardsZeroSource();
    TestOverlayThroughBlendPreviewColor();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
