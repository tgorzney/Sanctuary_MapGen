// NoiseBlend_PROC.cpp — stage lifecycle: the two-level dirty hash, the flattened layer
// configurations both backends consume, and the dispatch hand-off. The per-backend work
// lives in NoiseBlend_Noise_PROC.cpp (CPU noise), NoiseBlend_Blend_PROC.cpp (CPU blend +
// proportions) and NoiseBlend_Gpu_PROC.cpp (the GPU speed path).
#include "NoiseBlend_PROC.h"
#include "../math/HeightOcclusion_MATH.h"
#include <cstring>

namespace SanmapGen {
namespace Proc {
namespace {

constexpr std::size_t hashBasis = 1469598103934665603ull;   // FNV offset basis

inline std::size_t HashMix(std::size_t seed, std::size_t value) {
    return seed ^ (value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2));
}
inline std::size_t HashInteger(std::size_t seed, int value) {
    return HashMix(seed, static_cast<std::size_t>(static_cast<unsigned int>(value)));
}
inline std::size_t HashFloat(std::size_t seed, float value) {
    unsigned int bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return HashMix(seed, static_cast<std::size_t>(bits));
}

// Structural noise identity ONLY — levels, density, opacity and blend are deliberately
// excluded so cheap reshaping reuses the cached raw noise (NOISE_BLEND_SPEC "Cache").
std::size_t HashLayerStructure(std::size_t seed, const Params::Layer& layer, int layerSeed, int vertexSize) {
    seed = HashInteger(seed, static_cast<int>(layer.noiseType));
    seed = HashInteger(seed, static_cast<int>(layer.fractalType));
    seed = HashInteger(seed, layer.octaves);
    seed = HashInteger(seed, layerSeed);
    seed = HashInteger(seed, vertexSize);
    seed = HashFloat(seed, layer.frequency);
    seed = HashFloat(seed, layer.gain);
    seed = HashFloat(seed, layer.lacunarity);
    seed = HashFloat(seed, layer.weightedStrength);
    seed = HashFloat(seed, layer.pingPongStrength);
    return HashFloat(seed, layer.cellularJitter);
}

// Everything that only affects the blended result, not the raw noise.
std::size_t HashLayerShaping(std::size_t seed, const Params::Layer& layer) {
    seed = HashInteger(seed, static_cast<int>(layer.blendMode));
    seed = HashInteger(seed, layer.stratumIndex);
    seed = HashFloat(seed, layer.landDensity);
    seed = HashFloat(seed, layer.mountainDensity);
    seed = HashFloat(seed, layer.plateauDensity);
    seed = HashFloat(seed, layer.rampDensity);
    seed = HashFloat(seed, layer.levelsShadows);
    seed = HashFloat(seed, layer.levelsMidtones);
    seed = HashFloat(seed, layer.levelsHighlights);
    seed = HashFloat(seed, layer.levelsOutputBlack);
    seed = HashFloat(seed, layer.levelsOutputWhite);
    seed = HashFloat(seed, layer.opacity);
    seed = HashFloat(seed, layer.heightBlendContrast);
    seed = HashFloat(seed, layer.heightBlendMinimum);
    return HashFloat(seed, layer.heightBlendMaximum);
}

} // namespace

NoiseBlendStage::NoiseBlendStage(const Params::Geometry& geometrySettings,
                                 const Params::LayerStack& layerStackSettings,
                                 Data::MapFields& outputFields)
    : geometry(geometrySettings), layerStack(layerStackSettings), mapFields(outputFields) {}

std::size_t NoiseBlendStage::ComputeStructuralNoiseHash(std::size_t layerIndex) const {
    const std::vector<const Params::Layer*> flatLayers = layerStack.GetFlatLayers();
    if (layerIndex >= flatLayers.size()) return hashBasis;
    const int layerSeed = static_cast<int>(geometry.seed)
                        + static_cast<int>(layerIndex) * constants.layerSeedStride;
    return HashLayerStructure(hashBasis, *flatLayers[layerIndex], layerSeed, geometry.VertexSize());
}

std::size_t NoiseBlendStage::ComputeBlendHash() const { return ComputeParameterHash(); }

std::size_t NoiseBlendStage::ComputeParameterHash() const {
    const std::vector<const Params::Layer*> flatLayers = layerStack.GetFlatLayers();
    std::size_t hash = HashInteger(hashBasis, geometry.mapSize);
    hash = HashInteger(hash, static_cast<int>(geometry.seed));
    hash = HashInteger(hash, static_cast<int>(flatLayers.size()));
    for (std::size_t index = 0; index < flatLayers.size(); ++index) {
        const int layerSeed = static_cast<int>(geometry.seed)
                            + static_cast<int>(index) * constants.layerSeedStride;
        hash = HashLayerStructure(hash, *flatLayers[index], layerSeed, geometry.VertexSize());
        hash = HashLayerShaping(hash, *flatLayers[index]);
    }
    return hash;
}

Sys::ComputeBackend NoiseBlendStage::Run() {
    // Dispatch returns the backend it ROUTED to; RunOnCpu/RunOnGpu record the one that
    // actually ran (the Gpu path falls back to the Cpu when no GL program is available),
    // so lastBackend — not the routing decision — is what callers observe.
    Sys::Dispatch(*this, dispatchPolicy, generationContext, globalBackend,
                  Sys::DataResidency::Either);
    return lastBackend;
}

void NoiseBlendStage::ClearMaterialProportions() {
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        mapFields.materialProportions[stratum].Fill(0.0f);
}

} // namespace Proc
} // namespace SanmapGen
