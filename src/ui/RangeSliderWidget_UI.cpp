// RangeSliderWidget_UI.cpp — the ImDrawList draw path of the dual-handle slider. Layer: UI.
// One of the three translation units that include imgui.h; all clamping and drag logic is pure
// and lives in the header (see WidgetHelpers_UI.h "THE SPLIT"), so this file is only geometry,
// hit-testing and rectangles. Rendering is verified by eye against a live frame, never by test.
#include "RangeSliderWidget_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

struct TrackGeometry {
    ImVec2 origin      = ImVec2(0.0f, 0.0f);
    float  width       = 0.0f;
    float  height      = 0.0f;
    float  handleWidth = 1.0f;
    float  usableWidth = 0.0f;   // the span a handle's LEFT edge may travel over
};

// The track value under a screen-space cursor x, measured against handle CENTERS so grabbing a
// handle does not shift it by half its width.
float PointerValueAt(const TrackGeometry& geometry, const RangeSliderBounds& bounds, float cursorX) {
    if (!(geometry.usableWidth > 0.0f)) return bounds.lowerLimit;
    const float relativeX = cursorX - (geometry.origin.x + geometry.handleWidth * 0.5f);
    return ValueAtNormalizedPosition(relativeX / geometry.usableWidth, bounds.lowerLimit, bounds.upperLimit);
}

// One handle's InvisibleButton, over [leftX, rightX) of the track row. Never narrower than one
// pixel, so a handle pinned against its limit stays grabbable.
bool HandleIsActive(const char* identifier, const TrackGeometry& geometry, float leftX, float rightX) {
    const float hitWidth = rightX - leftX;
    ImGui::SetCursorScreenPos(ImVec2(leftX, geometry.origin.y));
    ImGui::InvisibleButton(identifier, ImVec2(hitWidth > 1.0f ? hitWidth : 1.0f, geometry.height));
    return ImGui::IsItemActive();
}

void DrawTrack(const TrackGeometry& geometry, const RangeSliderValues& values, const RangeSliderBounds& bounds,
               const WidgetStyle& style, bool bMinimumActive, bool bMaximumActive) {
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const float rounding = ResolveWidgetRounding(style);
    const float minimumOffset = RangeSliderHandleOffset(values.minimumValue, bounds, geometry.width, geometry.handleWidth);
    const float maximumOffset = RangeSliderHandleOffset(values.maximumValue, bounds, geometry.width, geometry.handleWidth);
    const float halfHandle = geometry.handleWidth * 0.5f;
    const ImVec2 origin = geometry.origin;

    drawList->AddRectFilled(origin, ImVec2(origin.x + geometry.width, origin.y + geometry.height),
                            ResolveWidgetColor(style.trackColor, ImGuiCol_FrameBg), rounding);
    drawList->AddRectFilled(ImVec2(origin.x + minimumOffset + halfHandle, origin.y),
                            ImVec2(origin.x + maximumOffset + halfHandle, origin.y + geometry.height),
                            ResolveWidgetColor(style.fillColor, ImGuiCol_SliderGrab), rounding);

    const ImU32 idleColor   = ResolveWidgetColor(style.handleColor, ImGuiCol_Button);
    const ImU32 activeColor = ResolveWidgetColor(style.handleActiveColor, ImGuiCol_ButtonActive);
    drawList->AddRectFilled(ImVec2(origin.x + minimumOffset, origin.y),
                            ImVec2(origin.x + minimumOffset + geometry.handleWidth, origin.y + geometry.height),
                            bMinimumActive ? activeColor : idleColor, rounding);
    drawList->AddRectFilled(ImVec2(origin.x + maximumOffset, origin.y),
                            ImVec2(origin.x + maximumOffset + geometry.handleWidth, origin.y + geometry.height),
                            bMaximumActive ? activeColor : idleColor, rounding);
}

// The two numeric fields and the RT button. A field edit is applied immediately and reported, so
// the caller's single interaction step folds it into the same commit decision as a drag.
RangeSliderPointerInput DrawNumericFields(RangeSliderValues& values, const RangeSliderBounds& bounds,
                                          RealtimeToggle& realtimeToggle, const WidgetStyle& style,
                                          const char* valueFormat) {
    RangeSliderPointerInput fieldInput;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float fieldWidth = (ImGui::GetContentRegionAvail().x - spacing * 2.0f - style.realtimeButtonWidth) * 0.5f;
    const float dragSpeed = bounds.minimumSeparation > 0.0f ? bounds.minimumSeparation
                                                            : (bounds.upperLimit - bounds.lowerLimit) * 0.001f;

    ImGui::SetNextItemWidth(fieldWidth);
    if (ImGui::DragFloat("##minimumField", &values.minimumValue, dragSpeed,
                         bounds.lowerLimit, bounds.upperLimit, valueFormat)) {
        values = MoveRangeSliderHandle(values, bounds, RangeSliderHandle::Minimum, values.minimumValue);
        fieldInput.bNumericFieldEdited = true;
    }
    fieldInput.bNumericFieldActive = ImGui::IsItemActive();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(fieldWidth);
    if (ImGui::DragFloat("##maximumField", &values.maximumValue, dragSpeed,
                         bounds.lowerLimit, bounds.upperLimit, valueFormat)) {
        values = MoveRangeSliderHandle(values, bounds, RangeSliderHandle::Maximum, values.maximumValue);
        fieldInput.bNumericFieldEdited = true;
    }
    fieldInput.bNumericFieldActive = fieldInput.bNumericFieldActive || ImGui::IsItemActive();
    ImGui::SameLine();
    DrawRealtimeToggleButton("realtime", realtimeToggle, style);
    return fieldInput;
}

} // namespace

WidgetChange DrawRangeSlider(const char* label, RangeSliderValues& values,
                             const RangeSliderBounds& rawBounds, RealtimeToggle& realtimeToggle,
                             const WidgetStyle& style, const char* valueFormat) {
    const RangeSliderBounds bounds = ResolvedRangeSliderBounds(rawBounds);
    values = ClampRangeSliderValues(values, bounds);

    ImGui::PushID(label);
    ImGui::TextUnformatted(label);

    TrackGeometry geometry;
    geometry.origin      = ImGui::GetCursorScreenPos();
    geometry.width       = ImGui::GetContentRegionAvail().x;
    geometry.height      = ResolveWidgetTrackHeight(style);
    geometry.handleWidth = style.handleWidth > 1.0f ? style.handleWidth : 1.0f;
    geometry.usableWidth = geometry.width - geometry.handleWidth;
    ImGui::Dummy(ImVec2(geometry.width, geometry.height));       // reserve the track row
    const ImVec2 belowTrack = ImGui::GetCursorScreenPos();

    // Two hit-tests, one per handle. Where the handles overlap in pixels their shared span is
    // split at the midpoint, so neither handle can ever become unreachable.
    const float minimumLeftX = geometry.origin.x +
        RangeSliderHandleOffset(values.minimumValue, bounds, geometry.width, geometry.handleWidth);
    const float maximumLeftX = geometry.origin.x +
        RangeSliderHandleOffset(values.maximumValue, bounds, geometry.width, geometry.handleWidth);
    float splitX = minimumLeftX + geometry.handleWidth;
    if (splitX > maximumLeftX) splitX = (minimumLeftX + maximumLeftX + geometry.handleWidth) * 0.5f;
    const bool bMinimumActive = HandleIsActive("##minimumHandle", geometry, minimumLeftX, splitX);
    const bool bMaximumActive = HandleIsActive("##maximumHandle", geometry,
        splitX > maximumLeftX ? splitX : maximumLeftX, maximumLeftX + geometry.handleWidth);

    ImGui::SetCursorScreenPos(belowTrack);
    RangeSliderPointerInput input = DrawNumericFields(values, bounds, realtimeToggle, style, valueFormat);
    input.grabbedHandle = bMinimumActive ? RangeSliderHandle::Minimum
                        : (bMaximumActive ? RangeSliderHandle::Maximum : RangeSliderHandle::None);
    input.pointerValue  = PointerValueAt(geometry, bounds, ImGui::GetIO().MousePos.x);

    const WidgetChange change = StepRangeSliderInteraction(realtimeToggle, values, bounds, input);
    DrawTrack(geometry, values, bounds, style, bMinimumActive, bMaximumActive);   // zero-lag handles
    ImGui::PopID();
    return change;
}

} // namespace Ui
} // namespace SanmapGen
