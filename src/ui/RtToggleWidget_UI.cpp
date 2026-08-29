// RtToggleWidget_UI.cpp — the imgui side of the universal toggle button (STEP213) and the RT
// wrapper built on it, plus the style resolvers the whole widget library shares. Layer: UI. This
// is one of the translation units that include imgui.h; the RT state machine itself
// (Ui::RealtimeToggle) is pure and lives in the header, so its acceptance test needs no imgui
// frame (see RtToggleWidget_UI.h "THE SPLIT").
#include "RtToggleWidget_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

unsigned int ResolveWidgetColor(PackedColor requestedColor, int imguiThemeColor) {
    if (requestedColor == kThemeColor)
        return ImGui::GetColorU32(static_cast<ImGuiCol>(imguiThemeColor));
    return static_cast<unsigned int>(requestedColor);
}

float ResolveWidgetTrackHeight(const WidgetStyle& style) {
    return style.trackHeight > 0.0f ? style.trackHeight : ImGui::GetFrameHeight();
}

float ResolveWidgetRounding(const WidgetStyle& style) {
    return style.cornerRounding >= 0.0f ? style.cornerRounding : ImGui::GetStyle().FrameRounding;
}

bool DrawToggleButton(const char* identifier, const char* label, bool& enabled,
                      const WidgetStyle& style, float buttonWidth,
                      const char* tooltipOn, const char* tooltipOff) {
    // Captured BEFORE the click is applied — the color push below must reflect the state the
    // button was actually drawn in, and the tooltip (below) intentionally still describes that
    // same pre-click state on the click's own frame, exactly matching the original
    // DrawRealtimeToggleButton's own (pre-existing) ordering.
    const bool bWasEnabled = enabled;
    const ImU32 activeColor = ResolveWidgetColor(style.realtimeActiveColor, ImGuiCol_ButtonActive);

    ImGui::PushID(identifier);
    ImGui::PushStyleColor(ImGuiCol_Button, bWasEnabled ? activeColor : ImGui::GetColorU32(ImGuiCol_Button));
    const bool bClicked = ImGui::Button(label, ImVec2(buttonWidth, 0.0f));
    ImGui::PopStyleColor();
    if (bClicked) enabled = !bWasEnabled;
    const char* const tooltip = bWasEnabled ? tooltipOn : tooltipOff;
    if (tooltip != nullptr && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
    ImGui::PopID();
    return bClicked;
}

bool DrawRealtimeToggleButton(const char* identifier, RealtimeToggle& realtimeToggle,
                              const WidgetStyle& style) {
    bool bRealtimeEnabled = realtimeToggle.IsRealtimeEnabled();
    const bool bClicked = DrawToggleButton(identifier, "RT", bRealtimeEnabled, style,
                                           style.realtimeButtonWidth,
                                           "Realtime: ON - recompute on every change",
                                           "Realtime: OFF - recompute once, on mouse release");
    if (bClicked) realtimeToggle.SetRealtimeEnabled(bRealtimeEnabled);
    return bClicked;
}

} // namespace Ui
} // namespace SanmapGen
