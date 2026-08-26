// MarkersTab_GlobalScaleRowLine_UI_Test.cpp — STEP134 acceptance: DrawGlobalScaleRow's new
// genuine single-line composition (icon button / label / compact scale slider / normal-color
// swatch / select-color swatch). Mirrors MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp's
// own headless-frame harness (HeadlessImguiSession/RunHeadlessFrame — no window, no GL).
//
// DrawGlobalScaleRow draws all five controls inside ONE opaque call, so there is no way to read an
// INTERMEDIATE control's own item rect from outside it. This test instead replays
// DrawGlobalScaleRow's own five constituent calls (DrawGlobalScaleRowIconButton / TextUnformatted /
// DrawSliderScalarCompact / DrawColorSwatch x2 — the exact sequence MarkersTab_Globals_UI.cpp's
// DrawGlobalScaleRow body runs), bracketing each with GetCursorScreenPos() (its true left edge —
// none of these widgets draw any leading padding before their first item) and GetItemRectMax()
// immediately after (its true right edge — each is either one atomic item or a composite whose own
// LAST sub-item, an RT button, is its rightmost element). A final check proves the replay is
// faithful: the real DrawGlobalScaleRow, called once as production code does, lands its own last
// item (the select-color swatch's RT button) at the exact pixel the replay measured for the same
// control.
#include "ListWidget_TestFrame_UI.h"
#include "MarkersTab_Globals_UI.h"
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

bool IsNear(float value, float expected, float tolerance = 0.5f) {
    const float difference = value - expected;
    return difference < tolerance && difference > -tolerance;
}

const ImVec2 kWindowSize = ImVec2(500.0f, 100.0f);

struct RowRects {
    ImVec2 iconMin, iconMax;
    ImVec2 labelMin, labelMax;
    ImVec2 sliderMin, sliderMax;
    ImVec2 swatch1Min, swatch1Max;
    ImVec2 swatch2Min, swatch2Max;
};

// Replays DrawGlobalScaleRow's own five-call composition verbatim (MarkersTab_Globals_UI.cpp),
// probing each control's item rect in between — see file header for why this is the only way to
// see the five controls individually rather than only the row's own last item.
RowRects DrawRowWithProbes(MarkerGlobalScaleRow& row, Params::GlobalMarkerSettings& settings,
                           MarkersTabGlobals& globals) {
    RowRects rects;
    ImGui::PushID("probeRow");

    rects.iconMin = ImGui::GetCursorScreenPos();
    DrawGlobalScaleRowIconButton(row, settings.iconNameAlloy, globals, nullptr, nullptr);
    rects.iconMax = ImGui::GetItemRectMax();
    ImGui::SameLine();

    rects.labelMin = ImGui::GetCursorScreenPos();
    ImGui::TextUnformatted(markerGlobalScaleRowLabels[0]);
    rects.labelMax = ImGui::GetItemRectMax();
    ImGui::SameLine();

    rects.sliderMin = ImGui::GetCursorScreenPos();
    DrawSliderScalarCompact(markerGlobalScaleRowLabels[0], settings.scaleAlloy, globals.iconScaleRange,
                            row.iconScaleToggle, kMarkerGlobalScaleRowTrackWidthPixels,
                            kMarkerGlobalScaleRowFieldWidthPixels, WidgetStyle(), "%.2f");
    rects.sliderMax = ImGui::GetItemRectMax();
    ImGui::SameLine();

    ColorSwatchOptions compactSwatchOptions = globals.previewColorOptions;
    compactSwatchOptions.bLabelHidden = true;
    compactSwatchOptions.swatchWidth  = kMarkerGlobalScaleRowSwatchWidthPixels;

    rects.swatch1Min = ImGui::GetCursorScreenPos();
    DrawColorSwatch("PreviewColor", settings.colorAlloy, compactSwatchOptions, row.previewColorToggle);
    rects.swatch1Max = ImGui::GetItemRectMax();
    ImGui::SameLine();

    rects.swatch2Min = ImGui::GetCursorScreenPos();
    DrawColorSwatch("SelectColor", settings.selectColorAlloy, compactSwatchOptions, row.selectColorToggle);
    rects.swatch2Max = ImGui::GetItemRectMax();

    ImGui::PopID();
    return rects;
}

void RunGlobalScaleRowSingleLineChecks() {
    HeadlessImguiSession session;
    Params::GlobalMarkerSettings settings;
    MarkersTabGlobals globals;
    MarkerGlobalScaleRow& row = globals.scaleRows[0];

    RowRects rects;
    RunHeadlessFrame(HeadlessMouseState(), kWindowSize, [&] {
        rects = DrawRowWithProbes(row, settings, globals);
    });

    // Genuinely one line: every one of the 5 controls starts at the SAME row Y, not the old
    // 3-line-tall (label / track / value+RT) shape DrawSliderScalar would have produced.
    Check(rects.iconMin.y == rects.labelMin.y && rects.labelMin.y == rects.sliderMin.y
          && rects.sliderMin.y == rects.swatch1Min.y && rects.swatch1Min.y == rects.swatch2Min.y,
          "all 5 controls share the same row Y");

    // Left-to-right order, no overlap: each control's left edge sits at/after the previous
    // control's right edge (the gap between is SameLine's own ItemSpacing, never negative).
    Check(rects.iconMax.x <= rects.labelMin.x, "icon button ends before the label starts");
    Check(rects.labelMax.x <= rects.sliderMin.x, "label ends before the compact slider starts");
    Check(rects.sliderMax.x <= rects.swatch1Min.x,
          "the compact slider ends before the normal-color swatch starts");
    Check(rects.swatch1Max.x <= rects.swatch2Min.x,
          "the normal-color swatch ends before the select-color swatch starts");

    // The compact slider and both swatches honor their fixed, caller-supplied widths — not the
    // "claim the rest of the line" behavior an unbounded DrawSliderScalar/DrawColorSwatch would use.
    const float itemSpacing         = ImGui::GetStyle().ItemSpacing.x;
    const float realtimeButtonWidth = WidgetStyle().realtimeButtonWidth;
    const float expectedSliderWidth = kMarkerGlobalScaleRowTrackWidthPixels + itemSpacing
                                     + kMarkerGlobalScaleRowFieldWidthPixels + itemSpacing
                                     + realtimeButtonWidth;
    Check(IsNear(rects.sliderMax.x - rects.sliderMin.x, expectedSliderWidth),
          "the compact slider's measured width matches track+field+RT at their fixed pixel widths");

    const float expectedSwatchWidth = kMarkerGlobalScaleRowSwatchWidthPixels + itemSpacing + realtimeButtonWidth;
    Check(IsNear(rects.swatch1Max.x - rects.swatch1Min.x, expectedSwatchWidth),
          "the normal-color swatch's measured width matches its fixed swatch width + its own RT button");
    Check(IsNear(rects.swatch2Max.x - rects.swatch2Min.x, expectedSwatchWidth),
          "and the select-color swatch matches the same fixed footprint");

    // Faithfulness: the real DrawGlobalScaleRow, called once exactly as production code does,
    // lands its own last item (the select-color swatch's RT button) at the SAME pixel the probe
    // replay above measured for control 5 — proving the replay is not drifted from the real one.
    ImVec2 realFinalMax;
    RunHeadlessFrame(HeadlessMouseState(), kWindowSize, [&] {
        DrawGlobalScaleRow(globals, 0, settings, nullptr, nullptr);
        realFinalMax = ImGui::GetItemRectMax();
    });
    Check(realFinalMax.x == rects.swatch2Max.x && realFinalMax.y == rects.swatch2Max.y,
          "the real DrawGlobalScaleRow's own final item matches the probe replay's control 5");

    // STEP134 width-budget check (design doc's own flagged risk): the WHOLE row -- icon through the
    // select-color swatch's own RT button -- must fit comfortably inside a modest docked-panel
    // width. Measured (not estimated): icon 32 + label ~35 + compact slider 148 + swatch1 58 +
    // swatch2 58 + spacing lands at ~363px, inside the design doc's own 350-380px ballpark.
    Check(rects.swatch2Max.x - rects.iconMin.x < 400.0f,
          "the whole 5-control row fits under a 400px width budget");
}

} // namespace

int main() {
    RunGlobalScaleRowSingleLineChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
