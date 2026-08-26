// MarkersTab_GlobalScaleRowLine_UI_Test.cpp — acceptance for DrawGlobalScaleRow's single-line
// composition (icon button / type label / compact scale slider, no RT / "Icon" label + normal-
// color swatch / "Selected" label + select-color swatch) and for DrawMarkersTabGlobals's own
// outer loop putting all three type rows on ONE shared line with spacing between each row's own
// cluster. Mirrors MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp's own headless-frame
// harness (HeadlessImguiSession/RunHeadlessFrame — no window, no GL).
//
// DrawGlobalScaleRow draws all seven items inside ONE opaque call, so there is no way to read an
// INTERMEDIATE control's own item rect from outside it. This test instead replays DrawGlobalScaleRow's
// own constituent calls (icon button / TextUnformatted / DrawSliderScalarCompact / TextUnformatted /
// DrawColorSwatch x2 with a TextUnformatted before each — the exact sequence
// MarkersTab_Globals_UI.cpp's DrawGlobalScaleRow body runs), bracketing each with
// GetCursorScreenPos() (its true left edge) and GetItemRectMax() immediately after (its true right
// edge). A final check proves the replay is faithful: the real DrawGlobalScaleRow, called once as
// production code does, lands its own last item at the exact pixel the replay measured.
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

const ImVec2 kWindowSize = ImVec2(1400.0f, 100.0f);

struct RowRects {
    ImVec2 iconMin, iconMax;
    ImVec2 typeLabelMin, typeLabelMax;
    ImVec2 sliderMin, sliderMax;
    ImVec2 iconLabelMin, iconLabelMax;
    ImVec2 swatch1Min, swatch1Max;
    ImVec2 selectedLabelMin, selectedLabelMax;
    ImVec2 swatch2Min, swatch2Max;
};

// Replays DrawGlobalScaleRow's own composition verbatim (MarkersTab_Globals_UI.cpp), probing each
// control's item rect in between — see file header for why this is the only way to see each
// control individually rather than only the row's own last item.
RowRects DrawRowWithProbes(MarkerGlobalScaleRow& row, Params::GlobalMarkerSettings& settings,
                           MarkersTabGlobals& globals) {
    RowRects rects;
    ImGui::PushID("probeRow");

    rects.iconMin = ImGui::GetCursorScreenPos();
    DrawGlobalScaleRowIconButton(row, settings.iconNameAlloy, globals, nullptr, nullptr);
    rects.iconMax = ImGui::GetItemRectMax();
    ImGui::SameLine();

    rects.typeLabelMin = ImGui::GetCursorScreenPos();
    ImGui::TextUnformatted(markerGlobalScaleRowLabels[0]);
    rects.typeLabelMax = ImGui::GetItemRectMax();
    ImGui::SameLine();

    rects.sliderMin = ImGui::GetCursorScreenPos();
    DrawSliderScalarCompact("Icon Scale (Global)", settings.scaleAlloy, globals.iconScaleRange,
                            row.iconScaleToggle, kMarkerGlobalScaleRowTrackWidthPixels,
                            kMarkerGlobalScaleRowFieldWidthPixels, WidgetStyle(), "%.2f",
                            /*bShowRealtimeToggle=*/false);
    rects.sliderMax = ImGui::GetItemRectMax();
    ImGui::SameLine();

    ColorSwatchOptions compactSwatchOptions = globals.previewColorOptions;
    compactSwatchOptions.bLabelHidden = true;
    compactSwatchOptions.swatchWidth  = kMarkerGlobalScaleRowSwatchWidthPixels;

    rects.iconLabelMin = ImGui::GetCursorScreenPos();
    ImGui::TextUnformatted("Icon");
    rects.iconLabelMax = ImGui::GetItemRectMax();
    ImGui::SameLine();

    rects.swatch1Min = ImGui::GetCursorScreenPos();
    DrawColorSwatch("PreviewColor", settings.colorAlloy, compactSwatchOptions, row.previewColorToggle);
    rects.swatch1Max = ImGui::GetItemRectMax();
    ImGui::SameLine();

    rects.selectedLabelMin = ImGui::GetCursorScreenPos();
    ImGui::TextUnformatted("Selected");
    rects.selectedLabelMax = ImGui::GetItemRectMax();
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

    // Genuinely one line: every control starts at the SAME row Y.
    Check(rects.iconMin.y == rects.typeLabelMin.y && rects.typeLabelMin.y == rects.sliderMin.y
          && rects.sliderMin.y == rects.iconLabelMin.y && rects.iconLabelMin.y == rects.swatch1Min.y
          && rects.swatch1Min.y == rects.selectedLabelMin.y && rects.selectedLabelMin.y == rects.swatch2Min.y,
          "all 7 controls share the same row Y");

    // Left-to-right order, no overlap.
    Check(rects.iconMax.x <= rects.typeLabelMin.x, "icon button ends before the type label starts");
    Check(rects.typeLabelMax.x <= rects.sliderMin.x, "type label ends before the compact slider starts");
    Check(rects.sliderMax.x <= rects.iconLabelMin.x, "the compact slider ends before the \"Icon\" label starts");
    Check(rects.iconLabelMax.x <= rects.swatch1Min.x, "the \"Icon\" label ends before the normal-color swatch starts");
    Check(rects.swatch1Max.x <= rects.selectedLabelMin.x,
          "the normal-color swatch ends before the \"Selected\" label starts");
    Check(rects.selectedLabelMax.x <= rects.swatch2Min.x,
          "the \"Selected\" label ends before the select-color swatch starts");

    // The compact slider carries NO RT button now (human's own instruction: RT stays only on the
    // two color swatches) — its measured width is track+field only, no realtimeButtonWidth term.
    const float itemSpacing         = ImGui::GetStyle().ItemSpacing.x;
    const float realtimeButtonWidth = WidgetStyle().realtimeButtonWidth;
    const float expectedSliderWidth = kMarkerGlobalScaleRowTrackWidthPixels + itemSpacing
                                     + kMarkerGlobalScaleRowFieldWidthPixels;
    Check(IsNear(rects.sliderMax.x - rects.sliderMin.x, expectedSliderWidth),
          "the compact slider's measured width matches track+field only, no RT button");

    // Both color swatches keep their RT button.
    const float expectedSwatchWidth = kMarkerGlobalScaleRowSwatchWidthPixels + itemSpacing + realtimeButtonWidth;
    Check(IsNear(rects.swatch1Max.x - rects.swatch1Min.x, expectedSwatchWidth),
          "the normal-color swatch's measured width matches its fixed swatch width + its own RT button");
    Check(IsNear(rects.swatch2Max.x - rects.swatch2Min.x, expectedSwatchWidth),
          "and the select-color swatch matches the same fixed footprint");

    // Faithfulness: the real DrawGlobalScaleRow, called once exactly as production code does,
    // lands its own last item at the SAME pixel the probe replay measured.
    ImVec2 realFinalMax;
    RunHeadlessFrame(HeadlessMouseState(), kWindowSize, [&] {
        DrawGlobalScaleRow(globals, 0, settings, nullptr, nullptr);
        realFinalMax = ImGui::GetItemRectMax();
    });
    Check(realFinalMax.x == rects.swatch2Max.x && realFinalMax.y == rects.swatch2Max.y,
          "the real DrawGlobalScaleRow's own final item matches the probe replay's final control");
}

// Human's own instruction: all three type rows (Alloy/Plasma/Spawn) share ONE line, with spacing
// between each row's own control cluster — not one line per type. Replays
// DrawMarkersTabGlobals's own loop body (SameLine with kMarkerGlobalScaleRowGroupSpacingPixels
// between rows) without the Section wrap, to keep this test focused on the loop's own layout.
void RunGlobalRowsShareOneLineChecks() {
    HeadlessImguiSession session;
    Params::GlobalMarkerSettings settings;
    MarkersTabGlobals globals;

    ImVec2 row0Min, row0Max, row1Min, row1Max, row2Min, row2Max;
    RunHeadlessFrame(HeadlessMouseState(), kWindowSize, [&] {
        for (int rowIndex = 0; rowIndex < kMarkerGlobalScaleRowCount; ++rowIndex) {
            if (rowIndex > 0) ImGui::SameLine(0.0f, kMarkerGlobalScaleRowGroupSpacingPixels);
            const ImVec2 rowMin = ImGui::GetCursorScreenPos();
            DrawGlobalScaleRow(globals, rowIndex, settings, nullptr, nullptr);
            const ImVec2 rowMax = ImGui::GetItemRectMax();
            if (rowIndex == 0) { row0Min = rowMin; row0Max = rowMax; }
            else if (rowIndex == 1) { row1Min = rowMin; row1Max = rowMax; }
            else { row2Min = rowMin; row2Max = rowMax; }
        }
    });

    Check(row0Min.y == row1Min.y && row1Min.y == row2Min.y,
          "all three type rows share the same Y -- one shared line, not one line each");
    Check(row0Max.x <= row1Min.x && row1Max.x <= row2Min.x,
          "each row's own cluster starts after the previous row's own cluster ends, left to right");

    // ImGui::SameLine(offset, spacing_w)'s spacing_w REPLACES the default ItemSpacing.x when >= 0
    // (it does not add to it) -- the gap between clusters is exactly the configured constant.
    const float gap01 = row1Min.x - row0Max.x;
    const float gap12 = row2Min.x - row1Max.x;
    Check(IsNear(gap01, kMarkerGlobalScaleRowGroupSpacingPixels, 1.0f),
          "the gap between row 0 and row 1 is the configured group spacing");
    Check(IsNear(gap12, kMarkerGlobalScaleRowGroupSpacingPixels, 1.0f),
          "and the same gap sits between row 1 and row 2");
}

} // namespace

int main() {
    RunGlobalScaleRowSingleLineChecks();
    RunGlobalRowsShareOneLineChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
