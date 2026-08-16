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
// decision as a drag.
ScalarSliderPointerInput DrawFloatFieldRow(float& value, const ScalarSliderRange& range,
                                           RealtimeToggle& realtimeToggle, const WidgetStyle& style,
                                           const char* valueFormat) {
    ScalarSliderPointerInput input;
    const float dragSpeed = range.increment > 0.0f ? range.increment
                                                   : (range.maximumValue - range.minimumValue) * 0.001f;
    ImGui::SetNextItemWidth(ScalarSliderNumericFieldWidth(style));
    if (ImGui::DragFloat("##value", &value, dragSpeed, range.minimumValue, range.maximumValue, valueFormat)) {
        value = ClampScalarSliderValue(value, range);
        input.bNumericFieldEdited = true;
    }
    input.bNumericFieldActive = ImGui::IsItemActive();
    ImGui::SameLine();
    DrawRealtimeToggleButton("realtime", realtimeToggle, style);
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

} // namespace Ui
} // namespace SanmapGen
