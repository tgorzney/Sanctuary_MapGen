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
    seed = HashFloat(seed, constants.cellSize);
    seed = HashFloat(seed, constants.degreesToRadians);
    seed = HashFloat(seed, constants.maximumSlopeDegreesLimit);
    seed = HashFloat(seed, constants.smoothstepShoulder);
    seed = HashFloat(seed, constants.smoothstepScale);
    seed = HashFloat(seed, constants.maskMinimum);
    seed = HashFloat(seed, constants.maskMaximum);
    return HashFloat(seed, constants.centralDifferenceSpan);
}

std::size_t HashSlopeGate(std::size_t seed, const Params::StratumMask& stratumMask) {
    seed = HashInteger(seed, stratumMask.bSlopeGateEnabled ? 1 : 0);
    seed = HashInteger(seed, stratumMask.bUseSmoothstep ? 1 : 0);
    seed = HashInteger(seed, stratumMask.bInvertSlopeGate ? 1 : 0);
    seed = HashFloat(seed, stratumMask.minimumSlopeDegrees);
    seed = HashFloat(seed, stratumMask.maximumSlopeDegrees);
    seed = HashFloat(seed, stratumMask.slopeFeatherDegreesLow);
    seed = HashFloat(seed, stratumMask.slopeFeatherDegreesHigh);
    return HashFloat(seed, stratumMask.slopeGateStrength);
}

// The stored art is an input, so its CONTENT is hashed — otherwise re-importing different art
// under the same settings would silently reuse the cached masks. Only walked when the art is
// actually in use, so the common (no stored mask) case stays free.
std::size_t HashStoredMask(std::size_t seed, const Params::StratumMask& stratumMask) {
    seed = HashInteger(seed, static_cast<int>(stratumMask.importedMaskMode));
    seed = HashFloat(seed, stratumMask.maskRemapMinimum);
    seed = HashFloat(seed, stratumMask.maskRemapMaximum);
    if (!stratumMask.HasStoredMask()) return seed;
    seed = HashInteger(seed, stratumMask.importedMaskWidth);
    seed = HashInteger(seed, stratumMask.importedMaskHeight);
    for (float value : stratumMask.importedMaskData) seed = HashFloat(seed, value);
    return seed;
}

} // namespace

MaskStage::MaskStage(const Params::Geometry& geometrySettings,
                     const std::vector<Params::StratumMask>& stratumMaskSettings,
                     Data::MapFields& outputFields)
    : geometry(geometrySettings), stratumMasks(stratumMaskSettings), mapFields(outputFields) {}

std::size_t MaskStage::ComputeParameterHash() const {
    std::size_t hash = HashInteger(hashBasis, geometry.mapSize);
    hash = HashFloat(hash, geometry.terrainMaxHeight);
    hash = HashConstants(hash, constants);
    hash = HashInteger(hash, static_cast<int>(stratumMasks.size()));
    for (const Params::StratumMask& stratumMask : stratumMasks) {
        hash = HashSlopeGate(hash, stratumMask);
        hash = HashStoredMask(hash, stratumMask);
    }
    return hash;
}

Sys::ComputeBackend MaskStage::Run() {
    lastBackend = Sys::Dispatch(*this, dispatchPolicy, generationContext, globalBackend,
                                Sys::DataResidency::Either);
    return lastBackend;
}

} // namespace Proc
} // namespace SanmapGen
