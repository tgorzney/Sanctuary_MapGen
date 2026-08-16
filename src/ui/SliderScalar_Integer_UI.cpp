// SliderScalar_Integer_UI.cpp — the INTEGER scalar slider's draw path. Layer: UI.
// The whole-number twin of SliderScalar_UI.cpp: the same reserved track, the same painting, the
// same RT commit — only the numeric field differs (DragInt, so a typed seed or octave count is
// entered as a whole number). The drag itself runs through the shared float math in
// SliderScalar_UI.h, so there is one interaction implementation, not two.
#include "SliderScalar_Track_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The integer field + the RT button, on the row under the track. A field edit is applied
// immediately and reported, so one interaction step folds it in with the drag.
ScalarSliderPointerInput DrawIntegerFieldRow(int& value, const ScalarSliderRange& range,
                                             RealtimeToggle& realtimeToggle, const WidgetStyle& style,
                                             const char* valueFormat) {
    ScalarSliderPointerInput input;
    ImGui::SetNextItemWidth(ScalarSliderNumericFieldWidth(style));
    if (ImGui::DragInt("##value", &value, range.increment > 1.0f ? range.increment : 1.0f,
                       static_cast<int>(range.minimumValue), static_cast<int>(range.maximumValue),
                       valueFormat)) {
        value = ClampScalarSliderInteger(value, range);
        input.bNumericFieldEdited = true;
    }
    input.bNumericFieldActive = ImGui::IsItemActive();
    ImGui::SameLine();
    DrawRealtimeToggleButton("realtime", realtimeToggle, style);
    return input;
}

} // namespace

WidgetChange DrawSliderScalarInteger(const char* label, int& value, const ScalarSliderRange& rawRange,
                                     RealtimeToggle& realtimeToggle, const WidgetStyle& style,
                                     const char* valueFormat) {
    const ScalarSliderRange range = ResolvedScalarSliderRange(rawRange);
    value = ClampScalarSliderInteger(value, range);

    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    const ScalarSliderTrackGeometry geometry = ReserveScalarSliderTrack(style);
    const bool bHandleGrabbed = ImGui::IsItemActive();

    ScalarSliderPointerInput input = DrawIntegerFieldRow(value, range, realtimeToggle, style, valueFormat);
    input.bHandleGrabbed = bHandleGrabbed;
    input.pointerValue   = ScalarSliderPointerValueAt(geometry, range, ImGui::GetIO().MousePos.x);

    const WidgetChange change = StepScalarSliderIntegerInteraction(realtimeToggle, value, range, input);
    PaintScalarSliderTrack(geometry, static_cast<float>(value), range, style, bHandleGrabbed);
    ImGui::PopID();
    return change;
}

} // namespace Ui
} // namespace SanmapGen
