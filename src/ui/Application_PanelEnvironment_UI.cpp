// Application_PanelEnvironment_UI.cpp — the bodies of the ENVIRONMENT group. Layer: UI.
// Behind Application_UI.h (ARCH §1.5). One call per tab; this file draws no control of its own.
//
// The three placement tabs are handed the RESOLVED instances the Placement stage produced
// (`Data::PlacementResults`) read-only, so their placed-item lists show what the pipeline actually
// accepted. They re-test no rule and they write no DATA field — every one of those arrays has
// exactly one writing stage (Constitution §1, ARCH §3.4), and it is not a tab.
#include "Application_UI.h"
#include "AreasTab_UI.h"
#include "ArmiesTab_UI.h"
#include "AtmosphereTab_UI.h"
#include "DecalsTab_UI.h"
#include "MarkersTab_UI.h"
#include "PropsTab_UI.h"
#include "ScenariosTab_UI.h"
#include "WaterTab_UI.h"

namespace SanmapGen {
namespace Ui {

void Application::DrawEnvironmentGroupPanel() {
    switch (tabState.activePanel) {
        case ApplicationPanel::Water: {
            PreviewCompositeSettings& previewSettings = composite.Settings();
            const PreviewFieldLayer* const waterLayer =
                PreviewFieldLayerOfKind(previewSettings, PreviewLayerKind::Water);
            Params::GradientRamp* const waterRamp =
                waterLayer == nullptr ? nullptr
                                      : PreviewRampOfFieldLayer(previewSettings, *waterLayer);
            DrawWaterTab(recipe, tabState.water, &previewDriver, waterRamp);
            break;
        }
        case ApplicationPanel::Atmosphere:
            DrawAtmosphereTab(tabState.atmosphere, &previewDriver);
            break;
        case ApplicationPanel::Markers:
            DrawMarkersTab(recipe, tabState.markers, &previewDriver, ActiveIconManifest(), &IconPairingLookup(),
                          &assembler.Placements().markers, selectManualMarkerInstanceCallback,
                          selectProceduralMarkerInstanceCallback);
            break;
        case ApplicationPanel::Armies:
            // STEP96_FootprintBakeAndStalenessCheck_IO.md §2 — the live, session-scoped ingestion
            // report ("Resolve Footprint")'s data source; empty/default until ticket 91's "Ingest
            // game templates" button has run once this session.
            DrawArmiesTab(recipe, tabState.armies, &previewDriver, ActiveIconManifest(),
                         &assetBridge.templateIngestReport);
            break;
        case ApplicationPanel::Props:
            DrawPropsTab(recipe, tabState.props, &previewDriver, ActiveIconManifest(),
                         &assembler.Placements().props, &assetBridge.templateIngestReport);
            break;
        case ApplicationPanel::Decals:
            DrawDecalsTab(recipe, tabState.decals, &previewDriver, ActiveIconManifest(),
                         &assembler.Placements().decals);
            break;
        case ApplicationPanel::Areas:
            DrawAreasTab(recipe, tabState.areas, &previewDriver, composite.Settings().areaColors);
            break;
        case ApplicationPanel::Scenarios:
            // Passed for interface parity only, exactly like every other tab's call site here —
            // DrawScenariosTab itself never calls it (see ScenariosTab_UI.h's own note).
            DrawScenariosTab(recipe, tabState.scenarios, &previewDriver);
            break;
        default: break;
    }
}

} // namespace Ui
} // namespace SanmapGen
