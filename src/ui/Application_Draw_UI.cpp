// Application_Draw_UI.cpp — the shell's own imgui chrome: the two-pane settings window with the
// left-pane panel switcher, the panel bodies, and the map-preview window. Layer: UI.
// Everything a PANEL draws belongs to the M5-6 tab that owns that recipe slice; this file only
// decides WHERE they are drawn. It edits no rule, derives no tier, and touches no DATA field —
// the one exception is the Preview panel, which edits the composite's own presentation settings
// because no tab owns them and they are not recipe content.
#include "Application_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

struct PanelEntry { ApplicationPanel panel; const char* label; };

const PanelEntry panelEntries[] = {
    { ApplicationPanel::Terrain, "Terrain" }, { ApplicationPanel::Layers,  "Layers"  },
    { ApplicationPanel::Water,   "Water"   }, { ApplicationPanel::Markers, "Markers" },
    { ApplicationPanel::Props,   "Props"   }, { ApplicationPanel::Preview, "Preview" },
    { ApplicationPanel::System,  "System"  },
};

const char* PreviewLayerLabel(PreviewLayerKind kind) {
    switch (kind) {
        case PreviewLayerKind::HeightRamp:   return "Height";
        case PreviewLayerKind::StratumSplat: return "Strata";
        case PreviewLayerKind::Flow:         return "Flow";
        case PreviewLayerKind::Accumulation: return "Accumulation";
        case PreviewLayerKind::Water:        return "Water";
        case PreviewLayerKind::Slope:        return "Slope";
    }
    return "Layer";
}

} // namespace

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

// Which panel is visible is presentation state: it trips no dirty flag and moves no parameter.
// The counters below are the driver's own instrumentation, shown so the two tiers are visible.
void Application::DrawPanelSwitcher() {
    ImGui::TextUnformatted("PANELS");
    ImGui::Separator();
    for (const PanelEntry& entry : panelEntries)
        if (ImGui::Selectable(entry.label, tabState.activePanel == entry.panel))
            tabState.activePanel = entry.panel;
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Regenerations: %d", previewDriver.PipelineRunCount());
    ImGui::Text("Composites: %d", previewDriver.PreviewCompositeCount());
    ImGui::Text("Stages last run: %d", static_cast<int>(previewDriver.StagesThatRan().size()));
    ImGui::TextWrapped("%s", assetStatusMessage.c_str());
}

void Application::DrawActivePanel() {
    switch (tabState.activePanel) {
        case ApplicationPanel::Terrain:
            DrawTerrainTab(recipe, tabState.terrain, &previewDriver); break;
        case ApplicationPanel::Layers:
            DrawLayersTab(recipe, tabState.layers, &previewDriver); break;
        case ApplicationPanel::Water:
            DrawWaterTab(recipe, tabState.water, &previewDriver); break;
        case ApplicationPanel::Markers:
            DrawMarkersTab(recipe, tabState.markers, &previewDriver, ActiveIconManifest()); break;
        case ApplicationPanel::Props:
            DrawPropsTab(recipe, tabState.props, &previewDriver, ActiveIconManifest()); break;
        case ApplicationPanel::Preview: DrawPreviewPanel(); break;
        case ApplicationPanel::System:  DrawSystemPanel(); break;
    }
}

// The composite's presentation settings. No stage's parameter hash can see any of them, so the
// driver derives bNeedsPreviewRender and the image recolors with no regeneration — that
// derivation is the driver's, and this panel declares no tier of its own (UI_FRAMEWORK_SPEC).
void Application::DrawPreviewPanel() {
    PreviewCompositeSettings& previewSettings = composite.Settings();
    tabState.gradientEditors.resize(previewSettings.gradientRamps.size());
    bool bPresentationMoved = false;
    for (std::size_t layerIndex = 0; layerIndex < previewSettings.fieldLayers.size(); ++layerIndex) {
        PreviewFieldLayer& fieldLayer = previewSettings.fieldLayers[layerIndex];
        ImGui::PushID(static_cast<int>(layerIndex));
        bool bLayerEnabled = fieldLayer.bEnabled;
        if (ImGui::Checkbox(PreviewLayerLabel(fieldLayer.kind), &bLayerEnabled)) {
            fieldLayer.bEnabled = bLayerEnabled;
            bPresentationMoved = true;
        }
        ImGui::PopID();
    }
    ImGui::Separator();
    for (std::size_t rampIndex = 0; rampIndex < previewSettings.gradientRamps.size(); ++rampIndex) {
        Params::GradientRamp& ramp = previewSettings.gradientRamps[rampIndex];
        ImGui::PushID(static_cast<int>(rampIndex));
        if (DrawGradientEditor(ramp.name.c_str(), ramp, tabState.gradientEditors[rampIndex]))
            bPresentationMoved = true;
        ImGui::PopID();
    }
    if (bPresentationMoved) previewDriver.NotifyParametersChanged();
}

void Application::DrawSystemPanel() {
    DrawSystemTab(tabState.system, &dispatchPolicy, &assembler, &previewDriver);
    ImGui::Separator();
    DrawAssetPanel();
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
