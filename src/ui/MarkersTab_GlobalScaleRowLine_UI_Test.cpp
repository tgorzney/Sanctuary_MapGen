// MarkersTab_GlobalScaleRowLine_UI_Test.cpp — acceptance for DrawTypeSectionMarkerSettingsRow's
// single-line composition (STEP136, was DrawGlobalScaleRow/STEP134; STEP240/ARCH §19.32 converts
// the row's ONE scale slider into a base+selected pair of compact dials): icon button / icon-color
// swatch (unlabeled), no RT / "Selected" label + select-color swatch, no RT / base icon-size dial,
// no RT / selected icon-size dial, no RT. Mirrors MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp's
// own headless-frame harness (HeadlessImguiSession/RunHeadlessFrame — no window, no GL).
//
// DrawTypeSectionMarkerSettingsRow draws all six items inside ONE opaque call, so there is no way
// to read an INTERMEDIATE control's own item rect from outside it. This test instead replays its
// own constituent calls (icon button / DrawColorSwatch / TextUnformatted / DrawColorSwatch /
// DrawDialCompact / DrawDialCompact — the exact sequence MarkersTab_Globals_UI.cpp's
// DrawTypeSectionMarkerSettingsRow body runs), bracketing each with GetCursorScreenPos() (its true
// left edge) and GetItemRectMax() immediately after (its true right edge). A final check proves the
// replay is faithful: the real DrawTypeSectionMarkerSettingsRow, called once as production code
// does, lands its own last item at the exact pixel the replay measured.
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

const ImVec2 kWindowSize = ImVec2(1400.0f, 200.0f);

struct RowRects {
    ImVec2 iconMin, iconMax;
    ImVec2 swatch1Min, swatch1Max;
    ImVec2 selectedLabelMin, selectedLabelMax;
    ImVec2 swatch2Min, swatch2Max;
    ImVec2 baseDialMin, baseDialMax;
    ImVec2 selectedDialMin, selectedDialMax;
};

// Replays DrawTypeSectionMarkerSettingsRow's own composition verbatim (MarkersTab_Globals_UI.cpp),
// probing each control's item rect in between — see file header for why this is the only way to see
// each control individually rather than only the row's own last item.
RowRects DrawRowWithProbes(MarkerGlobalScaleRow& row, Params::GlobalMarkerSettings& settings,
                           MarkersTabGlobals& globals) {
    RowRects rects;
    ImGui::PushID("probeRow");

    rects.iconMin = ImGui::GetCursorScreenPos();
    DrawGlobalScaleRowIconButton(row, settings.iconNameAlloy, globals, nullptr, nullptr);
    rects.iconMax = ImGui::GetItemRectMax();
    ImGui::SameLine();

    ColorSwatchOptions compactSwatchOptions = globals.previewColorOptions;
    compactSwatchOptions.bLabelHidden = true;
    compactSwatchOptions.swatchWidth  = kMarkerGlobalScaleRowSwatchWidthPixels;
    compactSwatchOptions.bRealtimeToggleHidden = true;

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
    ImGui::SameLine();

    rects.baseDialMin = ImGui::GetCursorScreenPos();
    DrawDialCompact("Icon Scale (Global)", settings.scaleAlloy, globals.scaleDialRange,
                    row.iconScaleToggle, kMarkerGlobalScaleRowFieldWidthPixels, WidgetStyle(), "%.2f",
                    /*bShowRealtimeToggle=*/false);
    rects.baseDialMax = ImGui::GetItemRectMax();
    ImGui::SameLine();

    rects.selectedDialMin = ImGui::GetCursorScreenPos();
    DrawDialCompact("Icon Scale (Selected)", settings.scaleSelectedAlloy, globals.scaleDialRange,
                    row.selectedScaleToggle, kMarkerGlobalScaleRowFieldWidthPixels, WidgetStyle(), "%.2f",
                    /*bShowRealtimeToggle=*/false);
    rects.selectedDialMax = ImGui::GetItemRectMax();

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
    Check(rects.iconMin.y == rects.swatch1Min.y && rects.swatch1Min.y == rects.selectedLabelMin.y
          && rects.selectedLabelMin.y == rects.swatch2Min.y && rects.swatch2Min.y == rects.baseDialMin.y
          && rects.baseDialMin.y == rects.selectedDialMin.y,
          "all 6 controls share the same row Y");

    // Left-to-right order, no overlap -- human's own instruction: icon, then icon color (unlabeled),
    // then "Selected" + its swatch, then the base icon-size dial, then the selected icon-size dial.
    Check(rects.iconMax.x <= rects.swatch1Min.x, "icon button ends before the icon-color swatch starts");
    Check(rects.swatch1Max.x <= rects.selectedLabelMin.x,
          "the icon-color swatch ends before the \"Selected\" label starts");
    Check(rects.selectedLabelMax.x <= rects.swatch2Min.x,
          "the \"Selected\" label ends before the select-color swatch starts");
    Check(rects.swatch2Max.x <= rects.baseDialMin.x,
          "the select-color swatch ends before the base icon-size dial starts");
    Check(rects.baseDialMax.x <= rects.selectedDialMin.x,
          "the base icon-size dial ends before the selected icon-size dial starts");

    // Human's own instruction: color/scale edits are ALWAYS realtime -- no RT button anywhere in
    // this row, not on either dial and not on either color swatch. Every measured width is
    // therefore just its own fixed content, no realtimeButtonWidth term anywhere.
    const float itemSpacing        = ImGui::GetStyle().ItemSpacing.x;
    const float dialDiameter       = ImGui::GetFrameHeight();   // DrawDialCompact's own
                                                                 // style.dialRadius==0 fallback
    const float expectedDialWidth  = dialDiameter + itemSpacing + kMarkerGlobalScaleRowFieldWidthPixels;
    Check(IsNear(rects.baseDialMax.x - rects.baseDialMin.x, expectedDialWidth),
          "the base icon-size dial's measured width matches knob+field only, no RT button");
    Check(IsNear(rects.selectedDialMax.x - rects.selectedDialMin.x, expectedDialWidth),
          "the selected icon-size dial's measured width matches the same footprint");

    const float expectedSwatchWidth = kMarkerGlobalScaleRowSwatchWidthPixels;
    Check(IsNear(rects.swatch1Max.x - rects.swatch1Min.x, expectedSwatchWidth),
          "the icon-color swatch's measured width matches its fixed swatch width only, no RT button");
    Check(IsNear(rects.swatch2Max.x - rects.swatch2Min.x, expectedSwatchWidth),
          "and the select-color swatch matches the same fixed footprint");

    // Faithfulness: the real DrawTypeSectionMarkerSettingsRow, called once exactly as production
    // code does, lands its own last item at the SAME pixel the probe replay measured.
    ImVec2 realFinalMax;
    RunHeadlessFrame(HeadlessMouseState(), kWindowSize, [&] {
        DrawTypeSectionMarkerSettingsRow(globals, 0, settings, nullptr, nullptr);
        realFinalMax = ImGui::GetItemRectMax();
    });
    Check(realFinalMax.x == rects.selectedDialMax.x && realFinalMax.y == rects.selectedDialMax.y,
          "the real DrawTypeSectionMarkerSettingsRow's own final item matches the probe replay's final control");
}

// ARCH §19.32/STEP240's own Verify item: both dial controls honor the [0.25, 2.0] clamp and write
// to the CORRECT field — proven by seeding each field with an out-of-range value on OPPOSITE sides
// of the clamp (scaleAlloy above the max, scaleSelectedAlloy below the min): if the two controls
// were ever bound to swapped fields, each would clamp to the WRONG bound and this check would catch
// it, rather than merely proving "some clamp happened."
void RunGlobalScaleRowDialClampAndFieldBindingChecks() {
    HeadlessImguiSession session;
    Params::GlobalMarkerSettings settings;
    settings.scaleAlloy         = 999.0f;   // above the [0.25, 2.0] max
    settings.scaleSelectedAlloy = -999.0f;  // below the [0.25, 2.0] min
    MarkersTabGlobals globals;

    RunHeadlessFrame(HeadlessMouseState(), kWindowSize, [&] {
        DrawTypeSectionMarkerSettingsRow(globals, 0, settings, nullptr, nullptr);
    });

    Check(settings.scaleAlloy == 2.0f,
          "the base icon-size dial clamps scaleAlloy to the [0.25, 2.0] range's own maximum");
    Check(settings.scaleSelectedAlloy == 0.25f,
          "the selected icon-size dial clamps scaleSelectedAlloy to the [0.25, 2.0] range's own "
          "minimum — a swapped binding would instead see this field clamp to 2.0f");

    // A second row (Plasma) proves the per-row field resolution the drawn dials bind through
    // (ResolveGlobalMarkerScaleRowFields) is not accidentally aliased to row 0's fields.
    Params::GlobalMarkerSettings plasmaSettings;
    plasmaSettings.scalePlasma         = 999.0f;
    plasmaSettings.scaleSelectedPlasma = -999.0f;
    MarkersTabGlobals plasmaGlobals;
    RunHeadlessFrame(HeadlessMouseState(), kWindowSize, [&] {
        DrawTypeSectionMarkerSettingsRow(plasmaGlobals, 1, plasmaSettings, nullptr, nullptr);
    });
    Check(plasmaSettings.scalePlasma == 2.0f, "row 1's base dial clamps scalePlasma correctly");
    Check(plasmaSettings.scaleSelectedPlasma == 0.25f,
          "row 1's selected dial clamps scaleSelectedPlasma correctly");
}

} // namespace

int main() {
    RunGlobalScaleRowSingleLineChecks();
    RunGlobalScaleRowDialClampAndFieldBindingChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
