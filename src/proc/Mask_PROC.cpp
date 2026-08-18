// Mask_PROC.cpp — stage lifecycle: the dirty hash and the dispatch hand-off. The flattening
// of settings into kernel records lives in Mask_Prepare_PROC.cpp, the CPU accuracy path in
// Mask_Apply_PROC.cpp, and the GPU speed path in Mask_Gpu_PROC.cpp — one small header, several
// aspect translation units (ARCH §1.5).
#include "Mask_PROC.h"
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

// Every tweakable of the stage itself (Constitution §8 values are inputs like any other).
std::size_t HashConstants(std::size_t seed, const MaskConstants& constants) {
    seed = HashFloat(seed, constants.degreesToRadians);
    seed = HashFloat(seed, constants.maximumSlopeDegreesLimit);
    seed = HashFloat(seed, constants.smoothstepShoulder);
    seed = HashFloat(seed, constants.smoothstepScale);
    seed = HashFloat(seed, constants.maskMinimum);
    seed = HashFloat(seed, constants.maskMaximum);
    return HashFloat(seed, constants.centralDifferenceSpan);
}

std::size_t HashStratumSettings(std::size_t seed, const Params::Stratum& stratum) {
    seed = HashInteger(seed, stratum.bSlopeGateEnabled ? 1 : 0);
    seed = HashInteger(seed, stratum.bUseSmoothstep ? 1 : 0);
    seed = HashInteger(seed, stratum.bInvertSlopeGate ? 1 : 0);
    seed = HashFloat(seed, stratum.minimumSlopeDegrees);
    seed = HashFloat(seed, stratum.maximumSlopeDegrees);
    seed = HashFloat(seed, stratum.slopeFeatherDegreesLow);
    seed = HashFloat(seed, stratum.slopeFeatherDegreesHigh);
    seed = HashFloat(seed, stratum.slopeGateStrength);
    seed = HashInteger(seed, static_cast<int>(stratum.importedMaskMode));
    // `maskRemapMinimum`/`maskRemapMaximum` are per-stratum material/appearance pass-through
    // data, NOT a Mask-stage input (Generator Expert ruling) — hashing an unconsumed input
    // would itself be an ARCH §3.4 purity violation the other direction, so they stay out.
    return seed;
}

// The stored art is a loaded input (Data::StratumArt), so its CONTENT is hashed — otherwise
// re-importing different art under the same settings would silently reuse the cached weights.
std::size_t HashStoredArt(std::size_t seed, const Data::StratumArt& art) {
    if (!art.HasImportedMask()) return HashInteger(seed, 0);
    seed = HashInteger(seed, art.importedMask.Width());
    seed = HashInteger(seed, art.importedMask.Height());
    const float* values = art.importedMask.Data();
    for (std::size_t index = 0; index < art.importedMask.CellCount(); ++index)
        seed = HashFloat(seed, values[index]);
    return seed;
}

} // namespace

MaskStage::MaskStage(const Params::Geometry& geometrySettings,
                     const std::vector<Params::Stratum>& stratumSettings,
                     const std::vector<Data::StratumArt>& stratumArtInput,
                     Data::MapFields& outputFields)
    : geometry(geometrySettings), strata(stratumSettings), stratumArt(stratumArtInput),
      mapFields(outputFields) {}

std::size_t MaskStage::ComputeParameterHash() const {
    std::size_t hash = HashInteger(hashBasis, geometry.mapSize);
    hash = HashFloat(hash, geometry.terrainMaxHeight);
    hash = HashFloat(hash, geometry.worldUnitsPerCell);   // the gradient's run (M5-0d)
    hash = HashConstants(hash, constants);
    hash = HashInteger(hash, static_cast<int>(strata.size()));
    for (const Params::Stratum& stratum : strata) hash = HashStratumSettings(hash, stratum);
    hash = HashInteger(hash, static_cast<int>(stratumArt.size()));
    for (const Data::StratumArt& art : stratumArt) hash = HashStoredArt(hash, art);
    return hash;
}

Sys::ComputeBackend MaskStage::Run() {
    lastBackend = Sys::Dispatch(*this, dispatchPolicy, generationContext, globalBackend,
                                Sys::DataResidency::Either);
    return lastBackend;
}

} // namespace Proc
} // namespace SanmapGen
