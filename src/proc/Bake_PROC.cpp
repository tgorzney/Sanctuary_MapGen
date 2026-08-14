// Bake_PROC.cpp — stage lifecycle: the ARCH §4.2 Gpu/Visual policy, the parameter hash the
// pipeline dirty-tracks on, the flattened stratum configurations both backends consume, and
// the dispatch hand-off. The per-backend work lives in Bake_Composite_PROC.cpp (the CPU
// parity/fallback path) and Bake_Gpu_PROC.cpp (the speed path).
#include "Bake_PROC.h"
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

std::size_t HashStratumSource(std::size_t seed, const StratumBakeSource& stratum) {
    seed = HashInteger(seed, stratum.bEnabled ? 1 : 0);
    seed = HashInteger(seed, stratum.albedoPixels != nullptr ? 1 : 0);
    seed = HashInteger(seed, stratum.albedoWidth);
    seed = HashInteger(seed, stratum.albedoHeight);
    seed = HashInteger(seed, stratum.textureVersion);
    seed = HashFloat(seed, stratum.tintRed);
    seed = HashFloat(seed, stratum.tintGreen);
    seed = HashFloat(seed, stratum.tintBlue);
    seed = HashFloat(seed, stratum.tileCount);
    seed = HashFloat(seed, stratum.maskRemapMinimum);
    return HashFloat(seed, stratum.maskRemapMaximum);
}

} // namespace

BakeStage::BakeStage(const Params::Geometry& geometrySettings, const Data::MapFields& inputFields,
                     BakedTextureSet& outputTextures)
    : geometry(geometrySettings), mapFields(inputFields), bakedTextures(outputTextures) {
    // ARCH §4.2: the bake is Gpu/Visual in BOTH contexts — decorative, determinism-exempt.
    dispatchPolicy.previewBackend  = Sys::ComputeBackend::Gpu;
    dispatchPolicy.outputBackend   = Sys::ComputeBackend::Gpu;
    dispatchPolicy.previewAccuracy = Sys::AccuracyClass::Visual;
    dispatchPolicy.outputAccuracy  = Sys::AccuracyClass::Visual;
}

int BakeStage::OutputResolution() const {
    int resolution = geometry.mapSize * constants.outputResolutionMultiplier;
    if (resolution < constants.minimumOutputResolution) resolution = constants.minimumOutputResolution;
    if (resolution > constants.maximumOutputResolution) resolution = constants.maximumOutputResolution;
    return resolution;
}

std::size_t BakeStage::ComputeParameterHash() const {
    std::size_t hash = HashInteger(hashBasis, geometry.mapSize);
    hash = HashInteger(hash, constants.outputResolutionMultiplier);
    hash = HashInteger(hash, constants.minimumOutputResolution);
    hash = HashInteger(hash, constants.maximumOutputResolution);
    hash = HashInteger(hash, constants.baseStratumIndex);
    hash = HashFloat(hash, constants.weightEpsilon);
    hash = HashInteger(hash, constants.bNormalizeWeights ? 1 : 0);
    hash = HashFloat(hash, constants.compositeAlphaValue);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        hash = HashStratumSource(hash, stratumSources[stratum]);
    return hash;
}

void BakeStage::PrepareRun() {
    const int resolution = OutputResolution();
    if (bakedTextures.resolution != resolution) bakedTextures.Resize(resolution);

    stratumConfigurations.assign(Data::MapFields::stratumCount, StratumKernelConfiguration());
    int texelOffset = 0;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        const StratumBakeSource& source = stratumSources[stratum];
        StratumKernelConfiguration& configuration = stratumConfigurations[stratum];
        const bool bHasTexture = source.albedoPixels != nullptr
                              && source.albedoWidth > 0 && source.albedoHeight > 0;
        configuration.albedoPixelOffset = texelOffset;
        configuration.albedoWidth  = bHasTexture ? source.albedoWidth : 0;
        configuration.albedoHeight = bHasTexture ? source.albedoHeight : 0;
        configuration.bEnabled     = source.bEnabled ? 1 : 0;
        configuration.tintRed      = source.tintRed;
        configuration.tintGreen    = source.tintGreen;
        configuration.tintBlue     = source.tintBlue;
        configuration.tileCount    = source.tileCount > 0.0f ? source.tileCount : 1.0f;
        configuration.maskRemapMinimum = source.maskRemapMinimum;
        configuration.maskRemapRangeReciprocal = source.maskRemapMaximum > source.maskRemapMinimum
            ? 1.0f / (source.maskRemapMaximum - source.maskRemapMinimum) : 0.0f;
        configuration.weightEpsilon     = constants.weightEpsilon;
        configuration.bNormalizeWeights = constants.bNormalizeWeights ? 1 : 0;
        if (bHasTexture) texelOffset += source.albedoWidth * source.albedoHeight;
    }
}

Sys::ComputeBackend BakeStage::Run() {
    bLastRunUsedGpu = false;
    Sys::Dispatch(*this, dispatchPolicy, generationContext, globalBackend, Sys::DataResidency::Either);
    lastBackend = bLastRunUsedGpu ? Sys::ComputeBackend::Gpu : Sys::ComputeBackend::Cpu;
    return lastBackend;
}

} // namespace Proc
} // namespace SanmapGen
