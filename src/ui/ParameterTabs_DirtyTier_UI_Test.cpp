// ParameterTabs_DirtyTier_UI_Test.cpp — M5-6 acceptance: a visual-only tab control trips ONLY
// bNeedsPreviewRender, a sim control trips bNeedsMapUpdate, and neither tier is declared by the
// tab. Every edit below goes through the identical call the tabs make —
// PreviewDriver::NotifyParametersChanged() — against a real GenerationAssembler + PreviewComposite
// (the M4-5 scene, reused rather than rebuilt); the tier comes out of the stages' own parameter
// hashes. Cpu twin throughout: no window, no GL.
#include "PreviewIntegration_TestScene_UI.h"
#include "ParameterTabs_TestSupport_UI.h"
#include "MarkersTab_UI.h"
#include "TerrainTab_UI.h"
#include "WaterTab_UI.h"

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

// Setup, not a checked edit: the M4-5 scene composites a height ramp only, so give it a water
// layer for the depth window to shade. Enabling water and raising the level are themselves sim
// edits (Placement hashes both), so they are settled here — before the snapshot below.
void PrepareWaterLayer(PreviewIntegrationScene& scene) {
    scene.recipe.water.bEnabled          = true;
    scene.recipe.water.waterLevelMaximum = 120.0f;      // of a 128-unit height extent
    PreviewCompositeSettings& settings = scene.composite.Settings();
    settings.gradientRamps.push_back(MakeBlackToWhiteRamp());
    settings.fieldLayers.push_back(MakeLayer(PreviewLayerKind::Water, PreviewBlendMode::AlphaBlend,
                                             static_cast<int>(settings.gradientRamps.size()) - 1,
                                             0.0f, 1.0f));
    scene.driver.NotifyParametersChanged();
    scene.driver.Refresh();
}

// The deep-water depth window is read by the preview composite and by NO stage — so the
// derivation, with no per-widget list anywhere, must answer "recolor only".
void RunVisualOnlyControlChecks(PreviewIntegrationScene& scene) {
    PrepareWaterLayer(scene);
    const int settledPipelineRuns = scene.driver.PipelineRunCount();
    const int settledGridBuilds   = scene.assembler.SpatialGridBuildCount();
    const int settledComposites   = scene.driver.PreviewCompositeCount();
    const unsigned long long settledImage = CompositeChecksum(scene.composite.CompositeTexels());

    WaterTabState state;
    LoadWaterTabValues(scene.recipe.water, state);
    StepRangeSliderInteraction(state.deepWaterDepthToggle, state.deepWaterDepthValues,
                               state.deepWaterDepthBounds,
                               GrabRangeHandle(RangeSliderHandle::Maximum, 40.0f));
    StoreWaterTabValues(state, scene.recipe.water);
    Check(StepRangeSliderInteraction(state.deepWaterDepthToggle, state.deepWaterDepthValues,
                                     state.deepWaterDepthBounds, ReleaseRangeHandle()).bCommitted,
          "the depth window commits on release");

    Check(scene.driver.NotifyParametersChanged() == Pipeline::RefreshTier::PreviewRender,
          "a visual-only control trips only bNeedsPreviewRender");
    Check(!scene.driver.NeedsMapUpdate(), "and never bNeedsMapUpdate");
    Check(scene.driver.OwningStageName().empty(), "no stage claims the deep-water window");
    Check(scene.driver.Refresh() == Pipeline::RefreshTier::PreviewRender, "the refresh recolors");
    Check(scene.driver.PipelineRunCount() == settledPipelineRuns, "no pipeline run");
    Check(scene.driver.StagesThatRan().empty(), "no stage ran");
    Check(scene.assembler.SpatialGridBuildCount() == settledGridBuilds,
          "the marker spatial grid was not rebuilt");
    Check(scene.driver.PreviewCompositeCount() == settledComposites + 1, "exactly one composite");
    Check(CompositeChecksum(scene.composite.CompositeTexels()) != settledImage,
          "and the recolor did reach the preview image");
}

// The water LEVEL, by contrast, is consumed by Placement's parameter hash — same tab, same widget
// library, same call, opposite tier.
void RunWaterLevelTierChecks(PreviewIntegrationScene& scene) {
    const int settledGridBuilds = scene.assembler.SpatialGridBuildCount();
    WaterTabState state;
    StepDialInteraction(state.waterLevelToggle, scene.recipe.water.waterLevelMaximum,
                        state.waterLevelRange, DialDrag(-4.0f));
    Check(StepDialInteraction(state.waterLevelToggle, scene.recipe.water.waterLevelMaximum,
                              state.waterLevelRange, DialRelease()).bCommitted,
          "the water level dial commits on release");
    Check(scene.driver.NotifyParametersChanged() == Pipeline::RefreshTier::MapUpdate,
          "a sim control trips bNeedsMapUpdate");
    Check(scene.driver.OwningStageName() == "Placement", "the placement stage claims the level");
    Check(!scene.driver.NeedsPreviewRender(), "a map update does not also raise the preview flag");
    scene.driver.Refresh();
    Check(scene.driver.StagesThatRan().size() == 2, "placement and bake re-ran, nothing upstream");
    Check(scene.assembler.SpatialGridBuildCount() == settledGridBuilds + 1,
          "Placement re-ran, so PIPELINE rebuilt the marker grid");
}

// One rule field from the Markers tab, and the seed from the Terrain tab: different tabs, the
// same derivation, and the seed reaches all the way back to the first stage.
void RunRuleAndSeedTierChecks(PreviewIntegrationScene& scene) {
    MarkersTabState markersState;
    Params::MarkerRule* const rule = SelectedMarkerRule(scene.recipe.markerRuleLayers, markersState);
    Check(rule != nullptr, "the scene has a marker rule to edit");
    if (rule == nullptr) return;
    LoadMarkerRuleValues(*rule, markersState);
    StepDialInteraction(markersState.countToggle, markersState.countValue, markersState.countRange,
                        DialDrag(-50.0f));
    Check(StoreMarkerRuleValues(markersState, *rule), "the marker count moved");
    Check(scene.driver.NotifyParametersChanged() == Pipeline::RefreshTier::MapUpdate,
          "a marker rule is stage-owned, so it trips a map update");
    Check(scene.driver.OwningStageName() == "Placement", "the placement stage claims the rule");
    scene.driver.Refresh();

    TerrainTabState terrainState;
    LoadTerrainTabValues(scene.recipe.geometry, terrainState);
    StepDialInteraction(terrainState.seedToggle, terrainState.seedValue, terrainState.seedRange,
                        DialDrag(-100.0f));
    Check(StoreTerrainTabValues(terrainState, scene.recipe.geometry), "the seed moved");
    Check(scene.driver.NotifyParametersChanged() == Pipeline::RefreshTier::MapUpdate,
          "the seed is stage-owned too");
    Check(scene.driver.OwningStageName() == "NoiseBlend", "the noise stage claims the seed");
    scene.driver.Refresh();
    Check(scene.driver.StagesThatRan().size() == 7, "a seed change re-runs the whole pipeline");
}

} // namespace

void RunTabDirtyTierChecks() {
    PreviewIntegrationScene scene;
    Check(scene.driver.Refresh() == Pipeline::RefreshTier::MapUpdate,
          "the first refresh generates the map");
    Check(scene.driver.Refresh() == Pipeline::RefreshTier::Nothing, "an idle refresh does nothing");
    RunVisualOnlyControlChecks(scene);
    RunWaterLevelTierChecks(scene);
    RunRuleAndSeedTierChecks(scene);
}
