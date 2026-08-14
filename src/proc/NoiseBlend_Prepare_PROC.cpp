// NoiseBlend_Prepare_PROC.cpp — flattens the layer stack into the backend-neutral
// LayerKernelConfiguration records and sizes the output fields and the per-layer noise
// cache. Everything a kernel would otherwise recompute per cell (fractal bounding, the
// terrace height, the levels range reciprocal, the swap-guarded occlusion window) is
// precomputed here ONCE — reciprocal multiply in the loop, never division (Constitution §3).
#include "NoiseBlend_PROC.h"
#include "../math/HeightOcclusion_MATH.h"
#include <cmath>

namespace SanmapGen {
namespace Proc {

namespace {
// FastNoiseLite's fractal normalizer, reproduced so the GPU uses the identical value.
float ComputeFractalBounding(int octaves, float gain) {
    const float gainAbsolute = gain < 0.0f ? -gain : gain;
    float amplitude = gainAbsolute;
    float amplitudeFractal = 1.0f;
    for (int octave = 1; octave < octaves; ++octave) {
        amplitudeFractal += amplitude;
        amplitude *= gainAbsolute;
    }
    return 1.0f / amplitudeFractal;
}
} // namespace

void NoiseBlendStage::PrepareRun() {
    const std::vector<const Params::Layer*> flatLayers = layerStack.GetFlatLayers();
    const int vertexSize = geometry.VertexSize();

    layerConfigurations.clear();
    layerConfigurations.reserve(flatLayers.size());
    for (std::size_t index = 0; index < flatLayers.size(); ++index) {
        const Params::Layer& layer = *flatLayers[index];
        LayerKernelConfiguration configuration;
        configuration.noiseType        = static_cast<int>(layer.noiseType);
        configuration.fractalType      = static_cast<int>(layer.fractalType);
        configuration.layerSeed        = static_cast<int>(geometry.seed)
                                       + static_cast<int>(index) * constants.layerSeedStride;
        configuration.octaves          = layer.octaves > 1 ? layer.octaves : 1;
        configuration.frequency        = layer.frequency;
        configuration.gain             = layer.gain;
        configuration.lacunarity       = layer.lacunarity;
        configuration.weightedStrength = layer.weightedStrength;
        configuration.pingPongStrength = layer.pingPongStrength;
        configuration.cellularJitter   = layer.cellularJitter;
        configuration.fractalBounding  = ComputeFractalBounding(configuration.octaves, layer.gain);

        configuration.landDensityMultiplier = layer.landDensity * constants.landDensityScale;
        configuration.mountainDensity  = layer.mountainDensity;
        configuration.plateauDensity   = layer.plateauDensity;
        configuration.rampDensity      = layer.rampDensity;
        const float terraceCount = constants.terraceCountBase
                                 + layer.plateauDensity * constants.terraceCountRange;
        configuration.terraceHeight           = 1.0f / terraceCount;
        configuration.terraceHeightReciprocal = terraceCount;
        configuration.levelsShadows    = layer.levelsShadows;
        configuration.levelsMidtones   = layer.levelsMidtones;
        configuration.levelsHighlights = layer.levelsHighlights;
        configuration.levelsRangeReciprocal = layer.levelsHighlights > layer.levelsShadows
            ? 1.0f / (layer.levelsHighlights - layer.levelsShadows) : 0.0f;
        configuration.levelsOutputBlack = layer.levelsOutputBlack;
        configuration.levelsOutputWhite = layer.levelsOutputWhite;

        configuration.blendMode           = static_cast<int>(layer.blendMode);
        configuration.stratumIndex        = layer.stratumIndex < 0 ? 0
            : (layer.stratumIndex >= Data::MapFields::stratumCount
               ? Data::MapFields::stratumCount - 1 : layer.stratumIndex);
        configuration.opacity             = layer.opacity;
        configuration.heightBlendContrast = layer.heightBlendContrast;
        Math::OrderOcclusionWindow(layer.heightBlendMinimum, layer.heightBlendMaximum,
                                   constants.occlusionWindowEpsilon,
                                   configuration.occlusionWindowLow, configuration.occlusionWindowHigh);
        configuration.heightMinimum = constants.heightMinimum;
        configuration.heightMaximum = constants.heightMaximum;
        layerConfigurations.push_back(configuration);
    }

    if (mapFields.VertexSize() != vertexSize) mapFields.Resize(vertexSize);

    const bool bCacheStale = cachedRawNoiseCpu.size() != layerConfigurations.size()
        || (!cachedRawNoiseCpu.empty() && cachedRawNoiseCpu[0].Width() != vertexSize);
    if (bCacheStale) {
        const std::size_t hashUnset = ~std::size_t(0);
        cachedRawNoiseCpu.assign(layerConfigurations.size(), Data::FloatField(vertexSize, vertexSize, 0.0f));
        cachedStructuralHashesCpu.assign(layerConfigurations.size(), hashUnset);
        cachedStructuralHashesGpu.assign(layerConfigurations.size(), hashUnset);
        bBlendCacheValid = false;
    }
}

} // namespace Proc
} // namespace SanmapGen
