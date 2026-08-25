// Application_Draw_UI.cpp — the shell's own imgui chrome: the two-pane settings window that puts
// the v1 left column beside the active panel, and the map-preview window. Layer: UI.
// Everything a PANEL draws belongs to the tab that owns that recipe slice; this file only decides
// WHERE they are drawn and routes the active panel to its group's body
// (Application_PanelTerrain_UI.cpp / Application_PanelEnvironment_UI.cpp /
// Application_PanelSystem_UI.cpp). It edits no rule, derives no tier and touches no DATA field.
#include "Application_UI.h"
#include <algorithm>
#include <cfloat>
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
    // STEP78 — auto-exit: browsing away from the Scenarios panel closes its detail panel in every
    // practical sense, so Scenario Edit Mode must not keep exclusive canvas ownership behind it.
    if (scenarioEditMode.IsActive() && tabState.activePanel != ApplicationPanel::Scenarios)
        scenarioEditMode.Deactivate();

    ImGui::Begin("Map Preview");
    if (ImGui::Button("View")) ImGui::OpenPopup("ViewLayersPopup");
    // STEP200 — defense-in-depth against the auto-fit-to-content growth feedback loop (an
    // unconstrained item width inside a BeginPopup window feeds back into that same window's next-
    // frame width): every item the popup draws is now itself fixed-width, but a max width here means
    // a future item added without SetNextItemWidth still cannot reintroduce runaway growth.
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(420.0f, FLT_MAX));
    if (ImGui::BeginPopup("ViewLayersPopup")) {
        DrawViewLayersPopup();
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (canvas.HasSelection()) ImGui::Text("Selected entity: %u", canvas.SelectedEntityIdentifier());
    else                       ImGui::TextUnformatted("Selected entity: none");
    // STEP78 — the legend strip + "Preview As" toggle row, drawn above the canvas image itself.
    DrawScenarioEditModeChrome(scenarioEditMode, recipe.armies, recipe.scenarios.maxArmySlotCount);
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float  fittedSide = std::min(available.x, available.y);
    const float  regionSide = fittedSide > 0.0f ? fittedSide : settings.canvasRegionSidePixels;
    canvas.Draw("mapCanvas", regionSide);
    ImGui::End();
}

} // namespace Ui
} // namespace SanmapGen
