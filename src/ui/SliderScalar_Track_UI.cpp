// SliderScalar_Track_UI.cpp — the scalar slider's geometry and ImDrawList painting. Layer: UI.
// One of the translation units that include imgui.h; all clamping, snapping and drag logic is pure
// and lives in SliderScalar_UI.h (see WidgetHelpers_UI.h "THE SPLIT"), so this file is only
// rectangles and one hit-test. Rendering is verified by eye against a live frame, never by test.
#include "SliderScalar_Track_UI.h"

namespace SanmapGen {
namespace Ui {

ScalarSliderTrackGeometry ReserveScalarSliderTrack(const WidgetStyle& style, float requestedWidthPixels) {
    ScalarSliderTrackGeometry geometry;
    geometry.origin      = ImGui::GetCursorScreenPos();
    geometry.width       = requestedWidthPixels > 0.0f ? requestedWidthPixels : ImGui::GetContentRegionAvail().x;
    geometry.height      = ResolveWidgetTrackHeight(style);
    geometry.handleWidth = style.handleWidth > 1.0f ? style.handleWidth : 1.0f;
    geometry.usableWidth = geometry.width - geometry.handleWidth;
    ImGui::InvisibleButton("##track", ImVec2(geometry.width > 1.0f ? geometry.width : 1.0f, geometry.height));
    return geometry;
}

float ScalarSliderPointerValueAt(const ScalarSliderTrackGeometry& geometry,
                                 const ScalarSliderRange& range, float cursorX) {
    if (!(geometry.usableWidth > 0.0f)) return range.minimumValue;
    const float relativeX = cursorX - (geometry.origin.x + geometry.handleWidth * 0.5f);
    return ValueAtNormalizedPosition(relativeX / geometry.usableWidth, range.minimumValue, range.maximumValue);
}

void PaintScalarSliderTrack(const ScalarSliderTrackGeometry& geometry, float value,
                            const ScalarSliderRange& range, const WidgetStyle& style,
                            bool bHandleGrabbed) {
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const float rounding = ResolveWidgetRounding(style);
    const float handleOffset = ScalarSliderHandleOffset(value, range, geometry.width, geometry.handleWidth);
    const ImVec2 origin = geometry.origin;

    drawList->AddRectFilled(origin, ImVec2(origin.x + geometry.width, origin.y + geometry.height),
                            ResolveWidgetColor(style.trackColor, ImGuiCol_FrameBg), rounding);
    drawList->AddRectFilled(origin, ImVec2(origin.x + handleOffset + geometry.handleWidth * 0.5f,
                                           origin.y + geometry.height),
                            ResolveWidgetColor(style.fillColor, ImGuiCol_SliderGrab), rounding);
    drawList->AddRectFilled(ImVec2(origin.x + handleOffset, origin.y),
                            ImVec2(origin.x + handleOffset + geometry.handleWidth, origin.y + geometry.height),
                            ResolveWidgetColor(bHandleGrabbed ? style.handleActiveColor : style.handleColor,
                                               bHandleGrabbed ? ImGuiCol_ButtonActive : ImGuiCol_Button),
                            rounding);
}

float ScalarSliderNumericFieldWidth(const WidgetStyle& style) {
    const float fieldWidth = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x
                           - style.realtimeButtonWidth;
    return fieldWidth > 1.0f ? fieldWidth : 1.0f;
}

} // namespace Ui
} // namespace SanmapGen
