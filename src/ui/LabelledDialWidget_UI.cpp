// LabelledDialWidget_UI.cpp — the ImDrawList draw path of the labelled dial. Layer: UI.
// One of the three translation units that include imgui.h; all clamping, snapping and drag logic
// is pure and lives in the header (see WidgetHelpers_UI.h "THE SPLIT"), so this file is only
// geometry, one hit-test and arcs. Rendering is verified by eye against a live frame.
#include "LabelledDialWidget_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Track arc, filled value arc, and the pointer — all from the ONE angle mapping in the header,
// so the pointer can never disagree with the value the field shows.
void DrawKnob(const ImVec2& center, float radius, float value, const DialRange& range,
              const WidgetStyle& style, bool bDragInProgress) {
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const float thickness = style.dialThickness > 1.0f ? style.dialThickness : 1.0f;
    const float arcRadius = radius - thickness * 0.5f;
    if (!(arcRadius > 0.0f)) return;

    const float startAngle = DialAngleRadians(0.0f, style.dialSweepStartDegrees, style.dialSweepDegrees);
    const float endAngle   = DialAngleRadians(1.0f, style.dialSweepStartDegrees, style.dialSweepDegrees);
    const float valueAngle = DialAngleRadians(DialNormalizedPosition(value, range),
                                              style.dialSweepStartDegrees, style.dialSweepDegrees);
    const ImU32 trackColor = ResolveWidgetColor(style.trackColor, ImGuiCol_FrameBg);

    drawList->AddCircleFilled(center, arcRadius - thickness * 0.5f, trackColor);
    drawList->PathArcTo(center, arcRadius, startAngle, endAngle);
    drawList->PathStroke(trackColor, thickness);
    drawList->PathArcTo(center, arcRadius, startAngle, valueAngle);
    drawList->PathStroke(ResolveWidgetColor(style.fillColor, ImGuiCol_SliderGrab), thickness);

    const ImU32 pointerColor = bDragInProgress
        ? ResolveWidgetColor(style.handleActiveColor, ImGuiCol_ButtonActive)
        : ResolveWidgetColor(style.handleColor, ImGuiCol_Button);
    drawList->AddLine(center, ImVec2(center.x + std::cos(valueAngle) * arcRadius,
                                     center.y + std::sin(valueAngle) * arcRadius),
                      pointerColor, thickness);
}

// The label, the numeric field and the RT button, stacked beside the knob. A field edit is
// applied immediately and reported, so one interaction step folds it in with the knob drag.
DialPointerInput DrawLabelAndField(const char* label, float& value, const DialRange& range,
                                   RealtimeToggle& realtimeToggle, const WidgetStyle& style,
                                   const char* valueFormat) {
    DialPointerInput input;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float dragSpeed = range.increment > 0.0f ? range.increment
                                                   : (range.maximumValue - range.minimumValue) * 0.001f;
    ImGui::BeginGroup();
    ImGui::TextUnformatted(label);
    const float fieldWidth = ImGui::GetContentRegionAvail().x - spacing - style.realtimeButtonWidth;
    ImGui::SetNextItemWidth(fieldWidth > 1.0f ? fieldWidth : 1.0f);
    if (ImGui::DragFloat("##value", &value, dragSpeed, range.minimumValue, range.maximumValue, valueFormat)) {
        value = ClampDialValue(value, range);
        input.bFieldEdited = true;
    }
    input.bFieldActive = ImGui::IsItemActive();
    ImGui::SameLine();
    DrawRealtimeToggleButton("realtime", realtimeToggle, style);
    ImGui::EndGroup();
    return input;
}

} // namespace

WidgetChange DrawLabelledDial(const char* label, float& value, const DialRange& rawRange,
                              RealtimeToggle& realtimeToggle, const WidgetStyle& style,
                              const char* valueFormat) {
    const DialRange range = ResolvedDialRange(rawRange);
    value = ClampDialValue(value, range);

    ImGui::PushID(label);
    const float radius = style.dialRadius > 0.0f ? style.dialRadius : ImGui::GetFrameHeight();
    const ImVec2 knobOrigin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##knob", ImVec2(radius * 2.0f, radius * 2.0f));
    const bool bDragInProgress = ImGui::IsItemActive();
    // The grab frame contributes NO delta: on the frame the knob is seized the mouse may have
    // arrived from anywhere on screen, and MouseDelta still carries that jump. Charging it to the
    // dial would snap the value on a plain click.
    const bool bGrabbedThisFrame = ImGui::IsItemActivated();
    ImGui::SameLine();

    DialPointerInput input = DrawLabelAndField(label, value, range, realtimeToggle, style, valueFormat);
    input.bDragInProgress = bDragInProgress;
    input.dragDeltaY      = (bDragInProgress && !bGrabbedThisFrame) ? ImGui::GetIO().MouseDelta.y : 0.0f;

    const WidgetChange change = StepDialInteraction(realtimeToggle, value, range, input);
    DrawKnob(ImVec2(knobOrigin.x + radius, knobOrigin.y + radius), radius, value, range,
             style, bDragInProgress);                                   // drawn last: zero-lag pointer
    ImGui::PopID();
    return change;
}

} // namespace Ui
} // namespace SanmapGen
