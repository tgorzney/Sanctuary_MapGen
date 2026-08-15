// PreviewIntegration_DirtyTier_UI_Test.cpp — the two-tier dirty contract, end to end.
// A parameter a PROC stage owns trips a map update: the pipeline re-runs from that stage, the
// marker grid is rebuilt, one composite follows. A pure-visual control trips only a preview
// render: the composite alone runs — no stage, no sim, and the spatial grid does not move.
// Neither tier is declared per widget; both are derived from which stage's parameter hash moved.
#include "PreviewIntegration_TestScene_UI.h"

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

// The preview resolution is presentation too — same tier, and it resizes the id buffer with it.
void CheckPreviewResolutionTier(PreviewIntegrationScene& scene, int settledGridBuilds) {
    scene.composite.Settings().previewResolution = previewIntegrationResolution / 2;
    check(scene.driver.NotifyParametersChanged() == Pipeline::RefreshTier::PreviewRender,
          "the preview resolution is a pure-visual control");
    scene.driver.Refresh();
    check(scene.composite.Resolution() == previewIntegrationResolution / 2, "the image resized");
    check(scene.entityIdentifiers.Width() == previewIntegrationResolution / 2,
          "the entity-id buffer followed the preview resolution");
    check(scene.assembler.SpatialGridBuildCount() == settledGridBuilds, "still no grid rebuild");

    scene.composite.Settings().previewResolution = previewIntegrationResolution;
    scene.driver.NotifyParametersChanged();
    scene.driver.Refresh();
    const unsigned long long restoredImage = CompositeChecksum(scene.composite.CompositeTexels());
    scene.driver.NotifyParametersChanged();       // nothing moved: still presentation-tier
    scene.driver.Refresh();
    check(CompositeChecksum(scene.composite.CompositeTexels()) == restoredImage,
          "the composite is a pure function of the bake — recompositing reproduces the image");
}

} // namespace

void RunMapUpdateTierChecks(PreviewIntegrationScene& scene) {
    const unsigned long long settledImage = CompositeChecksum(scene.composite.CompositeTexels());

    scene.recipe.layerStack.geoLayers[0].layers[0].frequency = 0.035f;
    check(scene.driver.NotifyParametersChanged() == Pipeline::RefreshTier::MapUpdate,
          "a layer frequency is stage-owned, so it trips a map update");
    check(scene.driver.OwningStageName() == "NoiseBlend", "the noise stage claims the frequency");
    check(scene.driver.NeedsMapUpdate() && !scene.driver.NeedsPreviewRender(),
          "a map update does not also raise the preview-only flag");

    check(scene.driver.Refresh() == Pipeline::RefreshTier::MapUpdate, "the refresh services it");
    check(scene.driver.StagesThatRan().size() == 7, "the pipeline re-ran from the noise stage on");
    check(scene.assembler.SpatialGridBuildCount() == 2, "Placement re-ran, so the grid was rebuilt");
    check(scene.driver.PreviewCompositeCount() == 2, "a map update composites exactly once");
    check(CompositeChecksum(scene.composite.CompositeTexels()) != settledImage,
          "the regenerated terrain reached the preview image");

    // A mid-pipeline parameter: the derivation names Mask, and the conductor runs Mask onward.
    scene.recipe.strata[AssemblerTest::detailStratumIndex].maximumSlopeDegrees = 40.0f;
    check(scene.driver.NotifyParametersChanged() == Pipeline::RefreshTier::MapUpdate,
          "a stratum slope gate is stage-owned too");
    check(scene.driver.OwningStageName() == "Mask", "the mask stage claims the slope gate");
    scene.driver.Refresh();
    check(scene.driver.StagesThatRan().size() == 3 && scene.driver.StagesThatRan()[0] == "Mask",
          "only mask, placement and bake re-ran");
    check(scene.assembler.SpatialGridBuildCount() == 3, "Placement re-ran again, grid rebuilt again");
}

void RunPreviewTierChecks(PreviewIntegrationScene& scene) {
    const unsigned long long settledHeight =
        AssemblerTest::FieldChecksum(scene.assembler.Fields().heightfield);
    const unsigned long long settledImage = CompositeChecksum(scene.composite.CompositeTexels());
    const int settledGridBuilds   = scene.assembler.SpatialGridBuildCount();
    const int settledPipelineRuns = scene.driver.PipelineRunCount();
    const int settledComposites   = scene.driver.PreviewCompositeCount();

    Params::GradientRamp& heightRamp = scene.composite.Settings().gradientRamps[heightRampIndex];
    heightRamp.stops[0].color[0] = 1.0f;          // black -> red at the low end of the ramp
    check(scene.driver.NotifyParametersChanged() == Pipeline::RefreshTier::PreviewRender,
          "a gradient ramp color is owned by no stage, so it trips a preview render only");
    check(!scene.driver.NeedsMapUpdate(), "a ramp color does not request a map update");
    check(scene.driver.OwningStageName().empty(), "no stage claims a ramp color");

    check(scene.driver.Refresh() == Pipeline::RefreshTier::PreviewRender, "the refresh recolors");
    check(scene.driver.PreviewCompositeCount() == settledComposites + 1, "exactly one composite");
    check(scene.driver.PipelineRunCount() == settledPipelineRuns, "no pipeline run");
    check(scene.driver.StagesThatRan().empty(), "no stage ran");
    check(scene.assembler.SpatialGridBuildCount() == settledGridBuilds,
          "the spatial grid was NOT rebuilt by a preview render");
    check(AssemblerTest::FieldChecksum(scene.assembler.Fields().heightfield) == settledHeight,
          "the baked heightfield is untouched by a recolor");
    check(CompositeChecksum(scene.composite.CompositeTexels()) != settledImage,
          "the recolor did reach the image");

    CheckPreviewResolutionTier(scene, settledGridBuilds);
}
