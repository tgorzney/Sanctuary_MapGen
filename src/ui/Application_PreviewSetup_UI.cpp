// Application_PreviewSetup_UI.cpp — the composition the shell shows on a fresh launch: which baked
// fields are colorized, in what Z order, through which ramps. Layer: UI.
// These are PRESENTATION settings (PreviewComposite_Settings_UI.h) — they are not recipe content
// and they feed no stage, which is exactly why moving one of them recolors without a regeneration:
// no stage's parameter hash can see them, so PreviewDriver derives bNeedsPreviewRender.
// Every layer here SAMPLES a field the pipeline baked; none re-derives one (ARCH §3.2).
#include "Application_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

Params::GradientRamp MakeTerrainHeightRamp() {
    Params::GradientRamp ramp;
    ramp.name = "Height";
    Params::GradientStop low;
    low.location = 0.0f;  low.color[0] = 0.11f; low.color[1] = 0.17f; low.color[2] = 0.13f;
    Params::GradientStop middle;
    middle.location = 0.45f; middle.color[0] = 0.42f; middle.color[1] = 0.44f; middle.color[2] = 0.30f;
    Params::GradientStop high;
    high.location = 1.0f; high.color[0] = 0.92f; high.color[1] = 0.92f; high.color[2] = 0.90f;
    ramp.stops.push_back(low);
    ramp.stops.push_back(middle);
    ramp.stops.push_back(high);
    return ramp;
}

Params::GradientRamp MakeWaterDepthRamp() {
    Params::GradientRamp ramp;
    ramp.name = "Water Depth";
    Params::GradientStop shallow;
    shallow.location = 0.0f; shallow.color[0] = 0.28f; shallow.color[1] = 0.55f;
    shallow.color[2] = 0.68f; shallow.color[3] = 0.55f;
    Params::GradientStop deep;
    deep.location = 1.0f; deep.color[0] = 0.04f; deep.color[1] = 0.13f;
    deep.color[2] = 0.34f; deep.color[3] = 0.95f;
    ramp.stops.push_back(shallow);
    ramp.stops.push_back(deep);
    return ramp;
}

PreviewFieldLayer MakeFieldLayer(PreviewLayerKind kind, PreviewBlendMode blendMode, int rampIndex,
                                 float domainMinimum, float domainMaximum, float opacity) {
    PreviewFieldLayer layer;
    layer.kind              = kind;
    layer.blendMode         = blendMode;
    layer.gradientRampIndex = rampIndex;
    layer.domainMinimum     = domainMinimum;
    layer.domainMaximum     = domainMaximum;
    layer.opacity           = opacity;
    return layer;
}

} // namespace

// Height ramp -> stratum splat -> water: the terrain reads as terrain on the first frame, and the
// splat proves the Mask stage's surfaceStratumWeights reached the image.
void ConfigureDefaultPreview(PreviewCompositeSettings& previewSettings, int previewResolution,
                             float worldUnitsPerCell) {
    previewSettings.previewResolution = previewResolution;
    previewSettings.worldUnitsPerCell = worldUnitsPerCell;
    previewSettings.gradientRamps.push_back(MakeTerrainHeightRamp());
    previewSettings.gradientRamps.push_back(MakeWaterDepthRamp());
    previewSettings.fieldLayers.push_back(
        MakeFieldLayer(PreviewLayerKind::HeightRamp, PreviewBlendMode::Replace, 0, 0.0f, 1.0f, 1.0f));
    previewSettings.fieldLayers.push_back(
        MakeFieldLayer(PreviewLayerKind::StratumSplat, PreviewBlendMode::AlphaBlend, -1, 0.0f, 1.0f, 0.65f));
    previewSettings.fieldLayers.push_back(
        MakeFieldLayer(PreviewLayerKind::Water, PreviewBlendMode::AlphaBlend, 1, 0.0f, 1.0f, 1.0f));
    previewSettings.entityMarkRadiusPixels = 3.0f;
}

} // namespace Ui
} // namespace SanmapGen
