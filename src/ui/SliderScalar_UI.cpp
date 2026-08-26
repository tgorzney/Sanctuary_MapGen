// SliderScalar_UI.cpp — the FLOAT scalar slider's draw path. Layer: UI.
// One of the translation units that include imgui.h (through SliderScalar_Track_UI.h); all
// clamping, snapping and drag logic is pure and lives in the header (see WidgetHelpers_UI.h "THE
// SPLIT"), so this file is only layout. The integer twin is SliderScalar_Integer_UI.cpp — the same
// track and the same RT commit, a different numeric field.
#include "SliderScalar_Track_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The numeric field + the RT button, on the row under the track. A field edit is applied
// immediately and reported, so the caller's single interaction step folds it into the same commit
// decision as a drag. `fieldWidthPixels` mirrors ReserveScalarSliderTrack's own <=0/>0 convention
// (STEP134): <=0 keeps today's "rest of the line, less the RT button" width; >0 is a caller-fixed
// width, for DrawSliderScalarCompact's narrow field.
ScalarSliderPointerInput DrawFloatFieldRow(float& value, const ScalarSliderRange& range,
                                           RealtimeToggle& realtimeToggle, const WidgetStyle& style,
                                           const char* valueFormat, float fieldWidthPixels = 0.0f,
                                           bool bShowRealtimeToggle = true) {
    ScalarSliderPointerInput input;
    const float dragSpeed = range.increment > 0.0f ? range.increment
                                                   : (range.maximumValue - range.minimumValue) * 0.001f;
    const float fieldWidth = fieldWidthPixels > 0.0f ? fieldWidthPixels : ScalarSliderNumericFieldWidth(style);
    ImGui::SetNextItemWidth(fieldWidth);
    if (ImGui::DragFloat("##value", &value, dragSpeed, range.minimumValue, range.maximumValue, valueFormat)) {
        value = ClampScalarSliderValue(value, range);
        input.bNumericFieldEdited = true;
    }
    input.bNumericFieldActive = ImGui::IsItemActive();
    if (bShowRealtimeToggle) {
        ImGui::SameLine();
        DrawRealtimeToggleButton("realtime", realtimeToggle, style);
    }
    return input;
}

} // namespace

WidgetChange DrawSliderScalar(const char* label, float& value, const ScalarSliderRange& rawRange,
                              RealtimeToggle& realtimeToggle, const WidgetStyle& style,
                              const char* valueFormat) {
    const ScalarSliderRange range = ResolvedScalarSliderRange(rawRange);
    value = ClampScalarSliderValue(value, range);

    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    const ScalarSliderTrackGeometry geometry = ReserveScalarSliderTrack(style);
    const bool bHandleGrabbed = ImGui::IsItemActive();

    ScalarSliderPointerInput input = DrawFloatFieldRow(value, range, realtimeToggle, style, valueFormat);
    input.bHandleGrabbed = bHandleGrabbed;
    input.pointerValue   = ScalarSliderPointerValueAt(geometry, range, ImGui::GetIO().MousePos.x);

    const WidgetChange change = StepScalarSliderInteraction(realtimeToggle, value, range, input);
    PaintScalarSliderTrack(geometry, value, range, style, bHandleGrabbed);   // painted last: zero-lag handle
    ImGui::PopID();
    return change;
}

// STEP134: the single-line variant. Same interaction/paint pipeline as DrawSliderScalar above,
// only the layout differs — a fixed-width track SameLine'd with a fixed-width field, a hover
// tooltip standing in for the dropped label line.
WidgetChange DrawSliderScalarCompact(const char* label, float& value, const ScalarSliderRange& rawRange,
                                     RealtimeToggle& realtimeToggle, float trackWidthPixels,
                                     float fieldWidthPixels, const WidgetStyle& style,
                                     const char* valueFormat, bool bShowRealtimeToggle) {
    const ScalarSliderRange range = ResolvedScalarSliderRange(rawRange);
    value = ClampScalarSliderValue(value, range);

    ImGui::PushID(label);
    const ScalarSliderTrackGeometry geometry = ReserveScalarSliderTrack(style, trackWidthPixels);
    const bool bHandleGrabbed = ImGui::IsItemActive();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", label);
    ImGui::SameLine();

    ScalarSliderPointerInput input =
        DrawFloatFieldRow(value, range, realtimeToggle, style, valueFormat, fieldWidthPixels, bShowRealtimeToggle);
    input.bHandleGrabbed = bHandleGrabbed;
    input.pointerValue   = ScalarSliderPointerValueAt(geometry, range, ImGui::GetIO().MousePos.x);

    const WidgetChange change = StepScalarSliderInteraction(realtimeToggle, value, range, input);
    PaintScalarSliderTrack(geometry, value, range, style, bHandleGrabbed);   // painted last: zero-lag handle
    ImGui::PopID();
    return change;
}

} // namespace Ui
} // namespace SanmapGen
