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

struct RangeSliderHitTest {
    TrackGeometry geometry;
    bool bMinimumActive = false;
    bool bMaximumActive = false;
    bool bTrackHovered  = false;   // captured right after the reserving Dummy — DrawRangeSliderCompact's
                                   // own tooltip trigger; the two handle InvisibleButtons drawn after
                                   // would otherwise become "the last item" imgui's own IsItemHovered
                                   // reads from.
};

// Reserves the track row and hit-tests both handles — the split-at-midpoint logic every caller
// needs — then leaves the imgui cursor exactly where DrawRangeSlider's own numeric-field row always
// expected it: directly below the track, at the row's own left edge, regardless of where the two
// handle InvisibleButtons last repositioned it internally. DrawRangeSliderCompact overrides that
// cursor placement itself (SameLine composition instead of a row below), reading `geometry` back
// out rather than relying on imgui's own last-item tracking.
RangeSliderHitTest ReserveAndHitTestTrack(const RangeSliderValues& values, const RangeSliderBounds& bounds,
                                          const WidgetStyle& style, float requestedWidthPixels) {
    RangeSliderHitTest result;
    TrackGeometry& geometry = result.geometry;
    geometry.origin      = ImGui::GetCursorScreenPos();
    geometry.width       = requestedWidthPixels > 0.0f ? requestedWidthPixels : ImGui::GetContentRegionAvail().x;
    geometry.height      = ResolveWidgetTrackHeight(style);
    geometry.handleWidth = style.handleWidth > 1.0f ? style.handleWidth : 1.0f;
    geometry.usableWidth = geometry.width - geometry.handleWidth;
    ImGui::Dummy(ImVec2(geometry.width, geometry.height));       // reserve the track row
    result.bTrackHovered = ImGui::IsItemHovered();
    const ImVec2 belowTrack = ImGui::GetCursorScreenPos();

    // Two hit-tests, one per handle. Where the handles overlap in pixels their shared span is
    // split at the midpoint, so neither handle can ever become unreachable.
    const float minimumLeftX = geometry.origin.x +
        RangeSliderHandleOffset(values.minimumValue, bounds, geometry.width, geometry.handleWidth);
    const float maximumLeftX = geometry.origin.x +
        RangeSliderHandleOffset(values.maximumValue, bounds, geometry.width, geometry.handleWidth);
    float splitX = minimumLeftX + geometry.handleWidth;
    if (splitX > maximumLeftX) splitX = (minimumLeftX + maximumLeftX + geometry.handleWidth) * 0.5f;
    result.bMinimumActive = HandleIsActive("##minimumHandle", geometry, minimumLeftX, splitX);
    result.bMaximumActive = HandleIsActive("##maximumHandle", geometry,
        splitX > maximumLeftX ? splitX : maximumLeftX, maximumLeftX + geometry.handleWidth);

    ImGui::SetCursorScreenPos(belowTrack);
    return result;
}

RangeSliderPointerInput HandleFrameInput(const RangeSliderHitTest& hit, const RangeSliderBounds& bounds,
                                         RangeSliderPointerInput fieldInput) {
    fieldInput.grabbedHandle = hit.bMinimumActive ? RangeSliderHandle::Minimum
                             : (hit.bMaximumActive ? RangeSliderHandle::Maximum : RangeSliderHandle::None);
    fieldInput.pointerValue  = PointerValueAt(hit.geometry, bounds, ImGui::GetIO().MousePos.x);
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

    const RangeSliderHitTest hit = ReserveAndHitTestTrack(values, bounds, style, 0.0f);
    RangeSliderPointerInput input =
        HandleFrameInput(hit, bounds, DrawNumericFields(values, bounds, realtimeToggle, style, valueFormat));

    const WidgetChange change = StepRangeSliderInteraction(realtimeToggle, values, bounds, input);
    DrawTrack(hit.geometry, values, bounds, style, hit.bMinimumActive, hit.bMaximumActive);   // zero-lag handles
    ImGui::PopID();
    return change;
}

// STEP154 — the single-line variant (DrawRangeSliderCompact, see header comment). Composition:
// minimum field -> SameLine -> fixed-width track -> (cursor placed explicitly, not via SameLine,
// since the track's own last imgui item by this point is a handle InvisibleButton, not the Dummy
// SameLine would otherwise chain from) -> maximum field -> SameLine -> RT button (optional).
WidgetChange DrawRangeSliderCompact(const char* label, RangeSliderValues& values,
                                    const RangeSliderBounds& rawBounds, RealtimeToggle& realtimeToggle,
                                    float trackWidthPixels, float fieldWidthPixels, const WidgetStyle& style,
                                    const char* valueFormat, bool bShowRealtimeToggle) {
    const RangeSliderBounds bounds = ResolvedRangeSliderBounds(rawBounds);
    values = ClampRangeSliderValues(values, bounds);
    const float dragSpeed = bounds.minimumSeparation > 0.0f ? bounds.minimumSeparation
                                                            : (bounds.upperLimit - bounds.lowerLimit) * 0.001f;

    ImGui::PushID(label);

    RangeSliderPointerInput fieldInput;
    ImGui::SetNextItemWidth(fieldWidthPixels);
    if (ImGui::DragFloat("##minimumField", &values.minimumValue, dragSpeed,
                         bounds.lowerLimit, bounds.upperLimit, valueFormat)) {
        values = MoveRangeSliderHandle(values, bounds, RangeSliderHandle::Minimum, values.minimumValue);
        fieldInput.bNumericFieldEdited = true;
    }
    fieldInput.bNumericFieldActive = ImGui::IsItemActive();
    ImGui::SameLine();

    const RangeSliderHitTest hit = ReserveAndHitTestTrack(values, bounds, style, trackWidthPixels);
    if (hit.bTrackHovered) ImGui::SetTooltip("%s", label);
    ImGui::SetCursorScreenPos(ImVec2(hit.geometry.origin.x + hit.geometry.width + ImGui::GetStyle().ItemSpacing.x,
                                     hit.geometry.origin.y));

    ImGui::SetNextItemWidth(fieldWidthPixels);
    if (ImGui::DragFloat("##maximumField", &values.maximumValue, dragSpeed,
                         bounds.lowerLimit, bounds.upperLimit, valueFormat)) {
        values = MoveRangeSliderHandle(values, bounds, RangeSliderHandle::Maximum, values.maximumValue);
        fieldInput.bNumericFieldEdited = true;
    }
    fieldInput.bNumericFieldActive = fieldInput.bNumericFieldActive || ImGui::IsItemActive();
    if (bShowRealtimeToggle) {
        ImGui::SameLine();
        DrawRealtimeToggleButton("realtime", realtimeToggle, style);
    }

    const RangeSliderPointerInput input = HandleFrameInput(hit, bounds, fieldInput);
    const WidgetChange change = StepRangeSliderInteraction(realtimeToggle, values, bounds, input);
    DrawTrack(hit.geometry, values, bounds, style, hit.bMinimumActive, hit.bMaximumActive);   // zero-lag handles
    ImGui::PopID();
    return change;
}

} // namespace Ui
} // namespace SanmapGen
