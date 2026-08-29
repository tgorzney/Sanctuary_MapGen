// Application_PreviewSetup_UI.cpp — the composition the shell shows on a fresh launch: which baked
// fields are colorized, in what Z order, through which ramps. Layer: UI.
// These are PRESENTATION settings (PreviewComposite_Settings_UI.h) — they are not recipe content
// and they feed no stage, which is exactly why moving one of them recolors without a regeneration:
// no stage's parameter hash can see them, so PreviewDriver derives bNeedsPreviewRender.
// Every layer here SAMPLES a field the pipeline baked; none re-derives one (ARCH §3.2).
//
// SIX LAYERS, NOT THREE. The Slope, Flow and Accumulation tabs each edit ONE composite layer of
// their own kind (TerrainOverlayTab_UI.h), and the left column's `[O]` row switches that same
// layer — so the composition has to CARRY those layers for either to have anything to point at.
// They ship disabled: v1 drew them as exclusive preview modes, while v2 composites them over the
// terrain, so v1's "on" default would paint the height ramp out on the first frame.
#include "Application_PreviewRamps_UI.h"
#include "Application_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The ramp rows, in the order they are pushed below. Named so a layer cites a row rather than a
// bare integer (Constitution §8).
enum PreviewRampRow : int {
    heightRampRow = 0, waterDepthRampRow, slopeRampRow, flowRampRow, accumulationRampRow
};

PreviewFieldLayer MakeFieldLayer(PreviewLayerKind kind, PreviewBlendMode blendMode, int rampIndex,
                                 float domainMinimum, float domainMaximum, float opacity) {
    PreviewFieldLayer layer;
    layer.kind              = kind;
    layer.blendMode         = blendMode;
    layer.gradientRampIndex = rampIndex;
    layer.domainMinimum     = domainMinimum;
    layer.domainMaximum     = domainMaximum;
    layer.opacity           = opacity;
    layer.bEnabled          = false;   // the left column's catalogue decides; see ApplyPanelVisibility
    return layer;
}

// Flow and accumulation carry no natural range — one map's peak accumulation is another's noise —
// so they take their domain from the baked field itself rather than from a guessed pair.
PreviewFieldLayer MakeAutoDomainLayer(PreviewLayerKind kind, int rampIndex, float opacity) {
    PreviewFieldLayer layer = MakeFieldLayer(kind, PreviewBlendMode::AlphaBlend, rampIndex,
                                             0.0f, 1.0f, opacity);
    layer.bAutoDomainFromField = true;
    return layer;
}

void PushDefaultRamps(PreviewCompositeSettings& previewSettings) {
    previewSettings.gradientRamps.push_back(MakeTerrainHeightRamp());
    previewSettings.gradientRamps.push_back(MakeWaterDepthRamp());
    previewSettings.gradientRamps.push_back(MakeSlopeRamp());
    previewSettings.gradientRamps.push_back(MakeFlowRamp());
    previewSettings.gradientRamps.push_back(MakeAccumulationRamp());
}

} // namespace

// Terrain first (it is the Replace layer that clears the frame), then the strata splat and the
// water, then the three analytical overlays on top of everything they describe.
void ConfigureDefaultPreview(PreviewCompositeSettings& previewSettings, int previewResolution,
                             float worldUnitsPerCell) {
    previewSettings.previewResolution = previewResolution;
    previewSettings.worldUnitsPerCell = worldUnitsPerCell;
    PushDefaultRamps(previewSettings);
    previewSettings.fieldLayers.push_back(MakeFieldLayer(
        PreviewLayerKind::HeightRamp, PreviewBlendMode::Replace, heightRampRow, 0.0f, 1.0f, 1.0f));
    previewSettings.fieldLayers.push_back(MakeFieldLayer(
        PreviewLayerKind::StratumSplat, PreviewBlendMode::AlphaBlend, -1, 0.0f, 1.0f, 0.65f));
    previewSettings.fieldLayers.push_back(MakeFieldLayer(
        PreviewLayerKind::Water, PreviewBlendMode::AlphaBlend, waterDepthRampRow, 0.0f, 1.0f, 1.0f));
    // The slope domain is gradient magnitude (rise/run), the pinned unit: 0..1 is 0..45 degrees,
    // which is the pair SlopeTabState shows on its first frame (SlopeTab_UI.h).
    previewSettings.fieldLayers.push_back(MakeFieldLayer(
        PreviewLayerKind::Slope, PreviewBlendMode::AlphaBlend, slopeRampRow, 0.0f, 1.0f, 1.0f));
    previewSettings.fieldLayers.push_back(
        MakeAutoDomainLayer(PreviewLayerKind::Flow, flowRampRow, 1.0f));
    previewSettings.fieldLayers.push_back(
        MakeAutoDomainLayer(PreviewLayerKind::Accumulation, accumulationRampRow, 1.0f));
    // ARCH §14.17 item 10 — topmost, no ramp (gradientRampIndex = -1, the same posture StratumSplat
    // uses: the color comes from the per-area tint, never a ramp lookup), Overlay blend, full opacity.
    previewSettings.fieldLayers.push_back(MakeFieldLayer(
        PreviewLayerKind::MapAreas, PreviewBlendMode::Overlay, -1, 0.0f, 1.0f, 1.0f));
    previewSettings.entityMarkRadiusPixels = 3.0f;
}

} // namespace Ui
} // namespace SanmapGen
