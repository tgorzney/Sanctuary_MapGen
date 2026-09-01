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

// The numeric field + optional RT button. `fieldWidthPixels` mirrors SliderScalar_UI.cpp's
// DrawFloatFieldRow convention (STEP236): the caller (label row or compact row alike) has already
// resolved the exact pixel width, so this helper only draws — it never touches the label or the
// group. A field edit is applied immediately and reported, so one interaction step folds it in
// with the knob drag.
DialPointerInput DrawFieldAndToggle(float& value, const DialRange& range, RealtimeToggle& realtimeToggle,
                                    const WidgetStyle& style, const char* valueFormat,
                                    float fieldWidthPixels, bool bShowRealtimeToggle) {
    DialPointerInput input;
    const float dragSpeed = range.increment > 0.0f ? range.increment
                                                   : (range.maximumValue - range.minimumValue) * 0.001f;
    ImGui::SetNextItemWidth(fieldWidthPixels > 1.0f ? fieldWidthPixels : 1.0f);
    if (ImGui::DragFloat("##value", &value, dragSpeed, range.minimumValue, range.maximumValue, valueFormat)) {
        value = ClampDialValue(value, range);
        input.bFieldEdited = true;
    }
    input.bFieldActive = ImGui::IsItemActive();
    if (bShowRealtimeToggle) {
        ImGui::SameLine();
        DrawRealtimeToggleButton("realtime", realtimeToggle, style);
    }
    return input;
}

// The knob's InvisibleButton + drag bookkeeping, shared by the labelled and compact layouts. The
// grab frame contributes NO delta: on the frame the knob is seized the mouse may have arrived from
// anywhere on screen, and MouseDelta still carries that jump. Charging it to the dial would snap
// the value on a plain click.
DialPointerInput DrawKnobButton(float radius, ImVec2& outKnobOrigin) {
    DialPointerInput input;
    outKnobOrigin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##knob", ImVec2(radius * 2.0f, radius * 2.0f));
    input.bDragInProgress = ImGui::IsItemActive();
    const bool bGrabbedThisFrame = ImGui::IsItemActivated();
    input.dragDeltaY = (input.bDragInProgress && !bGrabbedThisFrame) ? ImGui::GetIO().MouseDelta.y : 0.0f;
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
    ImVec2 knobOrigin;
    DialPointerInput input = DrawKnobButton(radius, knobOrigin);
    const bool bDragInProgress = input.bDragInProgress;
    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::TextUnformatted(label);
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float fieldWidth = ImGui::GetContentRegionAvail().x - spacing - style.realtimeButtonWidth;
    const DialPointerInput fieldInput =
        DrawFieldAndToggle(value, range, realtimeToggle, style, valueFormat, fieldWidth, true);
    ImGui::EndGroup();
    input.bFieldEdited = fieldInput.bFieldEdited;
    input.bFieldActive = fieldInput.bFieldActive;

    const WidgetChange change = StepDialInteraction(realtimeToggle, value, range, input);
    DrawKnob(ImVec2(knobOrigin.x + radius, knobOrigin.y + radius), radius, value, range,
             style, bDragInProgress);                                   // drawn last: zero-lag pointer
    ImGui::PopID();
    return change;
}

// STEP236: the single-line variant. Same interaction/paint pipeline as DrawLabelledDial above, only
// the layout differs — a knob sized to one row SameLine'd with a fixed-width field, a hover tooltip
// standing in for the dropped label line.
WidgetChange DrawDialCompact(const char* label, float& value, const DialRange& rawRange,
                             RealtimeToggle& realtimeToggle, float fieldWidthPixels,
                             const WidgetStyle& style, const char* valueFormat, bool bShowRealtimeToggle) {
    const DialRange range = ResolvedDialRange(rawRange);
    value = ClampDialValue(value, range);

    ImGui::PushID(label);
    const float radius = style.dialRadius > 0.0f ? style.dialRadius : ImGui::GetFrameHeight() * 0.5f;
    ImVec2 knobOrigin;
    DialPointerInput input = DrawKnobButton(radius, knobOrigin);
    const bool bDragInProgress = input.bDragInProgress;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", label);
    ImGui::SameLine();

    const DialPointerInput fieldInput = DrawFieldAndToggle(value, range, realtimeToggle, style, valueFormat,
                                                           fieldWidthPixels, bShowRealtimeToggle);
    input.bFieldEdited = fieldInput.bFieldEdited;
    input.bFieldActive = fieldInput.bFieldActive;

    const WidgetChange change = StepDialInteraction(realtimeToggle, value, range, input);
    DrawKnob(ImVec2(knobOrigin.x + radius, knobOrigin.y + radius), radius, value, range,
             style, bDragInProgress);                                   // drawn last: zero-lag pointer
    ImGui::PopID();
    return change;
}

} // namespace Ui
} // namespace SanmapGen
