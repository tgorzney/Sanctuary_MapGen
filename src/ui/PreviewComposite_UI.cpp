// PreviewComposite_UI.cpp — construction, backend selection, and the stage-wide + per-stratum
// record flattening. Layer: UI. No GL here (PreviewComposite_Gpu_UI.cpp) and no field math
// (PreviewComposite_Cpu_UI.cpp and its .glsl twin); the layer/entity flattening that needs the
// baked fields is in PreviewComposite_Prepare_UI.cpp.
#include "PreviewComposite_UI.h"
#include <cstddef>

namespace SanmapGen {
namespace Ui {
namespace {

int ClampPreviewResolution(int requested) {
    if (requested < kMinimumPreviewResolution) return kMinimumPreviewResolution;
    if (requested > kMaximumPreviewResolution) return kMaximumPreviewResolution;
    return requested;
}

} // namespace

PreviewComposite::PreviewComposite(const Params::Geometry& geometrySettings,
                                   const Params::Water& waterSettings,
                                   const std::vector<Params::Stratum>& stratumSettings,
                                   const Data::MapFields& inputFields,
                                   const Data::PlacementInstances& placedInstances,
                                   Data::EntityIdBuffer& entityIdentifierOutput)
    : geometry(geometrySettings), water(waterSettings), strata(stratumSettings),
      mapFields(inputFields), instances(placedInstances),
      entityIdentifierBuffer(entityIdentifierOutput) {}

// Gpu is the composite's backend (ARCH §4.2 "preview color = Gpu / Visual"); the Cpu twin runs
// when no resource manager was handed in, so a headless caller still gets a correct image
// instead of nothing.
void PreviewComposite::Compose(bool bNeedsTexelReadback) {
    if (gpuResourceManager != nullptr) { ComposeOnGpu(bNeedsTexelReadback); return; }
    ComposeOnCpu();
}

// Everything the kernels need that is not per-layer: sizes, the water window, the marks, the
// clear color. Every one of them comes from settings or params — none is written in the shader.
void PreviewComposite::BuildConfigurationRecord() {
    configuration.previewResolution = ClampPreviewResolution(settings.previewResolution);
    configuration.vertexSize = mapFields.VertexSize();
    configuration.splatWeightEpsilon = settings.splatWeightEpsilon;
    configuration.bNormalizeSplatWeights = settings.bNormalizeSplatWeights ? 1 : 0;
    configuration.bWaterEnabled = water.bEnabled ? 1 : 0;
    configuration.waterLevelMaximum = water.waterLevelMaximum;
    configuration.terrainMaxHeight = geometry.terrainMaxHeight;
    configuration.deepWaterDepthMinimum = water.deepWaterDepthMinimum;
    configuration.deepWaterDepthRangeReciprocal =
        ReciprocalOrZero(water.deepWaterDepthMaximum - water.deepWaterDepthMinimum);
    configuration.entityMarkRadiusPixels = settings.entityMarkRadiusPixels;
    configuration.clearColorRed = settings.clearColor[0];
    configuration.clearColorGreen = settings.clearColor[1];
    configuration.clearColorBlue = settings.clearColor[2];
    configuration.clearColorAlpha = settings.clearColor[3];
    configuration.entityMarkColorRed = settings.entityMarkColor[0];
    configuration.entityMarkColorGreen = settings.entityMarkColor[1];
    configuration.entityMarkColorBlue = settings.entityMarkColor[2];
    configuration.entityMarkColorAlpha = settings.entityMarkColor[3];
}

// The stratum splat tint comes from `Params::Stratum` — the composite keeps no rival
// per-stratum settings array (ARCH §7.1). A stratum the recipe does not configure is disabled,
// never silently defaulted into the image.
void PreviewComposite::BuildStratumConfigurations() {
    stratumConfigurations.assign(Data::MapFields::stratumCount, PreviewStratumConfiguration());
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        PreviewStratumConfiguration& record = stratumConfigurations[stratum];
        if (static_cast<std::size_t>(stratum) >= strata.size()) { record.bEnabled = 0; continue; }
        const Params::Stratum& stratumSettings = strata[stratum];
        record.previewColorRed = stratumSettings.tintRed;      // the legacy previewColor
        record.previewColorGreen = stratumSettings.tintGreen;
        record.previewColorBlue = stratumSettings.tintBlue;
        record.bEnabled = stratumSettings.bEnabled ? 1 : 0;
    }
}

void PreviewComposite::PrepareRun() {
    BuildConfigurationRecord();
    BuildStratumConfigurations();
    BuildLayerConfigurations();
    BuildEntityPoints();
    const std::size_t texelCount = static_cast<std::size_t>(configuration.previewResolution)
                                 * configuration.previewResolution;
    compositeTexels.assign(texelCount, 0u);
    if (entityIdentifierBuffer.Width() != configuration.previewResolution
        || entityIdentifierBuffer.Height() != configuration.previewResolution)
        entityIdentifierBuffer.Resize(configuration.previewResolution,
                                      configuration.previewResolution);
    executedPassCount = 0;
}

} // namespace Ui
} // namespace SanmapGen
