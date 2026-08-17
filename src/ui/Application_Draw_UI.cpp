// Application_Draw_UI.cpp — the shell's own imgui chrome: the two-pane settings window that puts
// the v1 left column beside the active panel, and the map-preview window. Layer: UI.
// Everything a PANEL draws belongs to the tab that owns that recipe slice; this file only decides
// WHERE they are drawn and routes the active panel to its group's body
// (Application_PanelTerrain_UI.cpp / Application_PanelEnvironment_UI.cpp /
// Application_PanelSystem_UI.cpp). It edits no rule, derives no tier and touches no DATA field.
#include "Application_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

void Application::DrawSettingsWindow() {
    ImGui::SetNextWindowSize(ImVec2(settings.settingsWindowWidth, settings.settingsWindowHeight),
                             ImGuiCond_FirstUseEver);
    ImGui::Begin("Generator Settings");
    ImGui::BeginChild("leftPane", ImVec2(settings.leftPaneWidth, 0.0f), true);
    DrawPanelSwitcher();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("panelPane", ImVec2(0.0f, 0.0f), true);
    DrawActivePanel();
    ImGui::EndChild();
    ImGui::End();
}

// The catalogue names the group; the group's own translation unit names the tab. A panel outside
// the enum draws nothing rather than falling through to a neighbour (Constitution §6).
void Application::DrawActivePanel() {
    const ApplicationPanelEntry* const entry = ApplicationPanelEntryOf(tabState.activePanel);
    if (entry == nullptr) return;
    ImGui::PushID(static_cast<int>(tabState.activePanel));
    ImGui::TextUnformatted(entry->label);
    ImGui::Separator();
    switch (entry->group) {
        case ApplicationPanelGroup::TerrainAndLayers: DrawTerrainGroupPanel();     break;
        case ApplicationPanelGroup::Environment:      DrawEnvironmentGroupPanel(); break;
        case ApplicationPanelGroup::System:           DrawSystemGroupPanel();      break;
        default: break;
    }
    ImGui::PopID();
}

void Application::DrawCanvasWindow() {
    ImGui::Begin("Map Preview");
    if (ImGui::Button("Regenerate")) canvas.RequestRegeneration();
    ImGui::SameLine();
    if (canvas.HasSelection()) ImGui::Text("Selected entity: %u", canvas.SelectedEntityIdentifier());
    else                       ImGui::TextUnformatted("Selected entity: none");
    canvas.Draw("mapCanvas", settings.canvasRegionSidePixels);
    ImGui::End();
}

} // namespace Ui
} // namespace SanmapGen
