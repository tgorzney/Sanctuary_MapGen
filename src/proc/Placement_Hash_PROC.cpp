// Placement_Hash_PROC.cpp — the stage's parameter hash (the dirty-hash key PIPELINE reads).
// Hashed field by field, never as raw struct bytes: struct padding is indeterminate, and a
// hash that depends on padding is not a function of the settings (DETERMINISM_SPEC).
#include "Placement_PROC.h"
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
inline std::size_t HashTransform(std::size_t seed, const Params::ScatterTransform& transform) {
    seed = HashFloat(seed, transform.scaleMinimum);
    seed = HashFloat(seed, transform.scaleMaximum);
    seed = HashFloat(seed, transform.rotationMinimumDegrees);
    seed = HashFloat(seed, transform.rotationMaximumDegrees);
    seed = HashInteger(seed, (transform.bAlignToTerrainNormal ? 1 : 0) | (transform.bCollidable ? 2 : 0));
    for (int index = 0; index < 8; ++index)
        seed = HashInteger(seed, static_cast<int>(transform.templateIdentifier[index]));
    return seed;
}
// The four rule families share this gate core; only their selection fields differ.
template <typename RuleType>
inline std::size_t HashGateCore(std::size_t seed, const RuleType& rule) {
    seed = HashFloat(seed, rule.minSlope);
    seed = HashFloat(seed, rule.maxSlope);
    seed = HashFloat(seed, rule.minHeight);
    seed = HashFloat(seed, rule.maxHeight);
    seed = HashInteger(seed, rule.mapEdgePadding);
    seed = HashInteger(seed, rule.maskStratumIndex);
    seed = HashFloat(seed, rule.maskWeightMinimum);
    return HashTransform(seed, rule.transform);
}

std::size_t HashMarkerRule(std::size_t seed, const Params::MarkerRule& rule) {
    seed = HashGateCore(seed, rule);
    seed = HashInteger(seed, (rule.bEnabled ? 1 : 0) | (rule.bHidden ? 2 : 0)
                           | (rule.bUseDensity ? 4 : 0) | (rule.bUseAllPositions ? 8 : 0)
                           | (rule.bRandomSelection ? 16 : 0) | (rule.bCheckMaximumRadius ? 32 : 0)
                           | (rule.bSymmetryUseGlobal ? 64 : 0));
    seed = HashInteger(seed, static_cast<int>(rule.category));
    seed = HashInteger(seed, static_cast<int>(rule.priority));
    seed = HashInteger(seed, static_cast<int>(rule.focusGradient));
    seed = HashInteger(seed, rule.count);
    seed = HashInteger(seed, rule.symmetryMask);
    seed = HashFloat(seed, rule.density);
    seed = HashFloat(seed, rule.clearanceSpacing);
    seed = HashFloat(seed, rule.areaRadiusMinimum);
    seed = HashFloat(seed, rule.areaRadiusMaximum);
    seed = HashFloat(seed, rule.areaHeightRange);
    seed = HashFloat(seed, rule.obstacleDistanceMinimum);
    seed = HashFloat(seed, rule.focusGradientRadius);
    seed = HashFloat(seed, rule.focusGradientStrength);
    return HashFloat(seed, rule.focusGradientContrast);
}

std::size_t HashPropRule(std::size_t seed, const Params::PropRule& rule) {
    seed = HashGateCore(seed, rule);
    seed = HashInteger(seed, (rule.bEnabled ? 1 : 0) | (rule.bAvoidWater ? 2 : 0)
                           | (rule.bNearCliffs ? 4 : 0) | (rule.bSymmetryUseGlobal ? 8 : 0));
    seed = HashInteger(seed, rule.symmetryMask);
    seed = HashFloat(seed, rule.density);
    seed = HashFloat(seed, rule.spacingMinimum);
    seed = HashFloat(seed, rule.obstacleDistanceMinimum);
    return HashFloat(seed, rule.nearCliffDistanceMaximum);
}

std::size_t HashUnitRule(std::size_t seed, const Params::UnitRule& rule) {
    seed = HashGateCore(seed, rule);
    seed = HashInteger(seed, (rule.bEnabled ? 1 : 0) | (rule.bSymmetryUseGlobal ? 2 : 0));
    seed = HashInteger(seed, rule.armyIndex);
    seed = HashInteger(seed, rule.count);
    seed = HashInteger(seed, rule.symmetryMask);
    return HashFloat(seed, rule.spacingMinimum);
}

std::size_t HashDecalRule(std::size_t seed, const Params::DecalRule& rule) {
    seed = HashGateCore(seed, rule);
    seed = HashInteger(seed, rule.bEnabled ? 1 : 0);
    seed = HashFloat(seed, rule.density);
    return HashFloat(seed, rule.spacingMinimum);
}

} // namespace

std::size_t PlacementStage::ComputeParameterHash() const {
    std::size_t hash = HashInteger(hashBasis, recipe.geometry.mapSize);
    hash = HashInteger(hash, static_cast<int>(recipe.geometry.seed));
    hash = HashFloat(hash, recipe.geometry.terrainMaxHeight);
    hash = HashInteger(hash, recipe.globalSymmetryMask);
    hash = HashInteger(hash, recipe.water.bEnabled ? 1 : 0);
    hash = HashFloat(hash, recipe.water.waterLevelMaximum);

    // Stage constants are settable too (§8) and every one changes the output. The struct is
    // a homogeneous run of 4-byte scalars, so it has no padding to poison the hash.
    const unsigned int* constantWords = reinterpret_cast<const unsigned int*>(&constants);
    for (std::size_t word = 0; word < sizeof(PlacementConstants) / sizeof(unsigned int); ++word)
        hash = HashMix(hash, static_cast<std::size_t>(constantWords[word]));

    for (const Params::MarkerRule& rule : recipe.markerRules) hash = HashMarkerRule(hash, rule);
    for (const Params::PropRule& rule : recipe.propRules)     hash = HashPropRule(hash, rule);
    for (const Params::UnitRule& rule : recipe.unitRules)     hash = HashUnitRule(hash, rule);
    for (const Params::DecalRule& rule : recipe.decalRules)   hash = HashDecalRule(hash, rule);
    return hash;
}

} // namespace Proc
} // namespace SanmapGen
