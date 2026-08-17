// Application_PanelTerrain_UI.cpp — the bodies of the TERRAIN & LAYERS group. Layer: UI.
// Behind Application_UI.h (ARCH §1.5). Every case below is ONE call into the tab that owns that
// slice of the recipe: this file decides WHERE a tab is drawn and hands it the object it edits, and
// it draws no control of its own — the single exception being the composite's height ramp, which is
// presentation the shell owns because no tab does (the same standing the old Preview panel had).
#include "Application_UI.h"
#include "AccumulationTab_UI.h"
#include "DetailNormalTab_UI.h"
#include "FlowTab_UI.h"
#include "HeightmapTab_UI.h"
#include "HolesTab_UI.h"
#include "SlopeTab_UI.h"
#include "SmoothnessTab_UI.h"
#include "StratumsTab_UI.h"
#include "SymmetryTab_UI.h"
#include "TintTab_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

// The height ramp has no tab: it colorizes the heightfield the Heightmap tab shapes, it is
// composite PRESENTATION (PreviewComposite_Settings_UI.h) rather than recipe content, and no stage
// hashes it — so editing it recolors with no regeneration, which is the driver's own derivation.
void Application::DrawHeightRampSection() {
    PreviewCompositeSettings& previewSettings = composite.Settings();
    PreviewFieldLayer* const heightLayer =
        PreviewFieldLayerOfKind(previewSettings, PreviewLayerKind::HeightRamp);
    Params::GradientRamp* const heightRamp =
        heightLayer == nullptr ? nullptr : PreviewRampOfFieldLayer(previewSettings, *heightLayer);
    if (heightRamp == nullptr) return;
    tabState.gradientEditors.resize(previewSettings.gradientRamps.size());
    const std::size_t editorIndex = static_cast<std::size_t>(heightLayer->gradientRampIndex);
    ImGui::Separator();
    if (DrawGradientEditor("Preview Height Ramp", *heightRamp, tabState.gradientEditors[editorIndex]))
        previewDriver.NotifyParametersChanged();
}

void Application::DrawTerrainGroupPanel() {
    switch (tabState.activePanel) {
        case ApplicationPanel::Symmetry:
            DrawSymmetryTab(recipe, hostedSettings.symmetryDetection, tabState.symmetry,
                            &previewDriver);
            break;
        case ApplicationPanel::Heightmap:
            DrawHeightmapTab(recipe, tabState.heightmap, &assembler, &previewDriver);
            DrawHeightRampSection();
            break;
        case ApplicationPanel::Slope:
            DrawSlopeTab(composite.Settings(), tabState.slope, &previewDriver);
            break;
        case ApplicationPanel::Flow:
            DrawFlowTab(composite.Settings(), tabState.flow, &assembler, &previewDriver);
            break;
        case ApplicationPanel::Accumulation:
            DrawAccumulationTab(composite.Settings(), tabState.accumulation, &assembler,
                                &previewDriver);
            break;
        case ApplicationPanel::Stratums:
            DrawStratumsTab(recipe.strata, tabState.stratums, &assembler, &previewDriver);
            break;
        case ApplicationPanel::DetailNormal:
            DrawDetailNormalTab(hostedSettings.detailNormalLayers, tabState.detailNormal,
                                &assembler, &previewDriver);
            break;
        case ApplicationPanel::Tint:
            DrawTintTab(hostedSettings.tintLayers, tabState.tint, &assembler, &previewDriver);
            break;
        case ApplicationPanel::Holes:
            DrawHolesTab(hostedSettings.holeLayers, tabState.holes, &assembler, &previewDriver);
            break;
        case ApplicationPanel::Smoothness:
            DrawSmoothnessTab(hostedSettings.smoothnessLayers, tabState.smoothness, &assembler,
                              &previewDriver);
            break;
        default: break;
    }
}

} // namespace Ui
} // namespace SanmapGen
