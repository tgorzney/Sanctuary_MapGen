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

std::size_t HashStratumAppearance(std::size_t seed, const Params::Stratum& stratum) {
    seed = HashInteger(seed, stratum.bEnabled ? 1 : 0);
    seed = HashFloat(seed, stratum.tintRed);
    seed = HashFloat(seed, stratum.tintGreen);
    seed = HashFloat(seed, stratum.tintBlue);
    return HashFloat(seed, stratum.tileCount);
}

// The texels themselves are megabytes, so the loader's version counter stands in for them.
std::size_t HashStratumArt(std::size_t seed, const Data::StratumArt& art) {
    seed = HashInteger(seed, art.HasAlbedo() ? 1 : 0);
    seed = HashInteger(seed, art.albedoWidth);
    seed = HashInteger(seed, art.albedoHeight);
    return HashInteger(seed, art.albedoVersion);
}

} // namespace

BakeStage::BakeStage(const Params::Geometry& geometrySettings,
                     const std::vector<Params::Stratum>& stratumSettings,
                     const std::vector<Data::StratumArt>& stratumArtInput,
                     const Data::MapFields& inputFields, BakedTextureSet& outputTextures)
    : geometry(geometrySettings), strata(stratumSettings), stratumArt(stratumArtInput),
      mapFields(inputFields), bakedTextures(outputTextures) {
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
    hash = HashInteger(hash, static_cast<int>(strata.size()));
    for (const Params::Stratum& stratum : strata) hash = HashStratumAppearance(hash, stratum);
    hash = HashInteger(hash, static_cast<int>(stratumArt.size()));
    for (const Data::StratumArt& art : stratumArt) hash = HashStratumArt(hash, art);
    return hash;
}

void BakeStage::PrepareRun() {
    const int resolution = OutputResolution();
    if (bakedTextures.resolution != resolution) bakedTextures.Resize(resolution);

    static const Params::Stratum defaultStratum;
    static const Data::StratumArt defaultArt;
    stratumConfigurations.assign(Data::MapFields::stratumCount, StratumKernelConfiguration());
    int texelOffset = 0;
    for (int index = 0; index < Data::MapFields::stratumCount; ++index) {
        const Params::Stratum& stratum = static_cast<std::size_t>(index) < strata.size()
                                       ? strata[index] : defaultStratum;
        const Data::StratumArt& art = static_cast<std::size_t>(index) < stratumArt.size()
                                    ? stratumArt[index] : defaultArt;
        StratumKernelConfiguration& configuration = stratumConfigurations[index];
        const bool bHasTexture = art.HasAlbedo();
        configuration.albedoPixelOffset = texelOffset;
        configuration.albedoWidth  = bHasTexture ? art.albedoWidth : 0;
        configuration.albedoHeight = bHasTexture ? art.albedoHeight : 0;
        configuration.bEnabled     = stratum.bEnabled ? 1 : 0;
        configuration.tintRed      = stratum.tintRed;
        configuration.tintGreen    = stratum.tintGreen;
        configuration.tintBlue     = stratum.tintBlue;
        configuration.tileCount    = stratum.tileCount > 0.0f ? stratum.tileCount : 1.0f;
        configuration.weightEpsilon     = constants.weightEpsilon;
        configuration.bNormalizeWeights = constants.bNormalizeWeights ? 1 : 0;
        if (bHasTexture) texelOffset += art.albedoWidth * art.albedoHeight;
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
