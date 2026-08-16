// RtToggleWidget_UI.cpp — the imgui side of the RT wrapper, plus the style resolvers the whole
// widget library shares. Layer: UI. This is one of the three translation units that include
// imgui.h; the RT state machine itself (Ui::RealtimeToggle) is pure and lives in the header, so
// its acceptance test needs no imgui frame (see RtToggleWidget_UI.h "THE SPLIT").
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

bool DrawRealtimeToggleButton(const char* identifier, RealtimeToggle& realtimeToggle,
                              const WidgetStyle& style) {
    const bool bRealtimeEnabled = realtimeToggle.IsRealtimeEnabled();
    const ImU32 activeColor = ResolveWidgetColor(style.realtimeActiveColor, ImGuiCol_ButtonActive);

    ImGui::PushID(identifier);
    ImGui::PushStyleColor(ImGuiCol_Button, bRealtimeEnabled ? activeColor
                                                            : ImGui::GetColorU32(ImGuiCol_Button));
    const bool bClicked = ImGui::Button("RT", ImVec2(style.realtimeButtonWidth, 0.0f));
    ImGui::PopStyleColor();
    if (bClicked) realtimeToggle.SetRealtimeEnabled(!bRealtimeEnabled);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(bRealtimeEnabled
            ? "Realtime: ON - recompute on every change"
            : "Realtime: OFF - recompute once, on mouse release");
    ImGui::PopID();
    return bClicked;
}

} // namespace Ui
} // namespace SanmapGen
