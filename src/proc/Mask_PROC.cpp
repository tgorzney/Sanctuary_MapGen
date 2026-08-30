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
    // Which SOURCE record (this stratum's own fields, or the recipe's shared `SlopeDefaults`)
    // `ConfigureSlopeGate` reads from is itself part of this stage's input (Generator Expert
    // ruling, STEP10_SlopeDefaults_Mechanism) — omitting it would let flipping this flag alone
    // silently reuse a stale cached result.
    seed = HashInteger(seed, stratum.bSlopeUseGlobal ? 1 : 0);
    // `maskRemapMinimum`/`maskRemapMaximum` are per-stratum material/appearance pass-through
    // data, NOT a Mask-stage input (Generator Expert ruling) — hashing an unconsumed input
    // would itself be an ARCH §3.4 purity violation the other direction, so they stay out.
    return seed;
}

// `recipe.slopeDefaults` is a Mask-stage input the instant ANY stratum has `bSlopeUseGlobal ==
// true` — and since the flattening step (PrepareRun) re-resolves every stratum slot
// unconditionally whenever the stage runs at all, there is no partial per-stratum re-run this
// would need to protect by hashing it conditionally (Generator Expert ruling, ticket STEP10):
// hashed here UNCONDITIONALLY, same posture as HashConstants below.
std::size_t HashSlopeDefaults(std::size_t seed, const Params::SlopeDefaults& slopeDefaults) {
    seed = HashInteger(seed, slopeDefaults.bSlopeGateEnabled ? 1 : 0);
    seed = HashInteger(seed, slopeDefaults.bUseSmoothstep ? 1 : 0);
    seed = HashInteger(seed, slopeDefaults.bInvertSlopeGate ? 1 : 0);
    seed = HashFloat(seed, slopeDefaults.minimumSlopeDegrees);
    seed = HashFloat(seed, slopeDefaults.maximumSlopeDegrees);
    seed = HashFloat(seed, slopeDefaults.slopeFeatherDegreesLow);
    seed = HashFloat(seed, slopeDefaults.slopeFeatherDegreesHigh);
    return HashFloat(seed, slopeDefaults.slopeGateStrength);
}

// STEP220 — the stored art is a loaded input (Data::StratumArt), so ITS ARRIVAL/CHANGE must move
// the hash — otherwise re-importing different art under the same settings would silently reuse
// the cached weights. Hashing `importedMaskVersion` (bumped by the ONE production writer,
// MapImporter_Fields_IO.cpp's LoadStratumMaskTga, on every successful load) achieves that
// WITHOUT walking the field's own content: this used to loop every texel
// (`art.importedMask.CellCount()` HashFloat calls), which for a real imported mask at its
// source file's native resolution meant hundreds of thousands to millions of hash calls per
// call to ComputeParameterHash() — paid synchronously on the UI thread every time
// NotifyParametersChanged() ran, which is now every frame during any live-drag gesture
// (Map Areas, STEP219) that has nothing to do with the Mask stage at all. Mirrors
// Bake_PROC.cpp's own HashStratumArt, which already does exactly this for the sibling
// albedoWidth/albedoHeight/albedoVersion fields on this same struct.
std::size_t HashStoredArt(std::size_t seed, const Data::StratumArt& art) {
    if (!art.HasImportedMask()) return HashInteger(seed, 0);
    seed = HashInteger(seed, art.importedMask.Width());
    seed = HashInteger(seed, art.importedMask.Height());
    return HashInteger(seed, art.importedMaskVersion);
}

} // namespace

MaskStage::MaskStage(const Params::Geometry& geometrySettings,
                     const std::vector<Params::Stratum>& stratumSettings,
                     const std::vector<Data::StratumArt>& stratumArtInput,
                     Data::MapFields& outputFields,
                     const Params::SlopeDefaults& slopeDefaultSettings)
    : geometry(geometrySettings), strata(stratumSettings), stratumArt(stratumArtInput),
      mapFields(outputFields), slopeDefaults(slopeDefaultSettings) {}

std::size_t MaskStage::ComputeParameterHash() const {
    std::size_t hash = HashInteger(hashBasis, geometry.mapSize);
    hash = HashFloat(hash, geometry.terrainMaxHeight);
    hash = HashFloat(hash, geometry.worldUnitsPerCell);   // the gradient's run (M5-0d)
    hash = HashConstants(hash, constants);
    hash = HashSlopeDefaults(hash, slopeDefaults);
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
