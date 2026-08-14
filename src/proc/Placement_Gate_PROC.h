// Placement_Gate_PROC.h — the per-position scatter gate, written ONCE.
// Layer: PROC. Pure inline float math: the position hash (no rand(), no global state — the
// scatter is a pure function of (seed, rule, position) as DETERMINISM_SPEC demands), the
// focus-gradient weighting, and the rule gate itself. The GLSL preview twin
// (Placement_PROC.glsl) mirrors these three functions expression for expression, so the
// Gpu density gate can never disagree with the authoritative Cpu bake.
#pragma once
#include <cstdint>
#include "Placement_Kernel_PROC.h"

namespace SanmapGen {
namespace Proc {

// murmur3 fmix32 avalanche — integer-only, so it is bit-identical on every machine.
inline uint32_t AvalancheHash(uint32_t value) {
    value ^= value >> 16; value *= 0x85ebca6bu;
    value ^= value >> 13; value *= 0xc2b2ae35u;
    value ^= value >> 16; return value;
}

// Hash of (seed, rule, cell) — the pattern the old stochastic scorer hinted at, with the
// multipliers lifted out of the code into PlacementConstants.
inline uint32_t HashPosition(const PlacementConstants& constants, int ruleSeed, int cellX, int cellY) {
    uint32_t mixed = (static_cast<uint32_t>(ruleSeed) + constants.positionHashSeedOffset)
                   ^ (static_cast<uint32_t>(cellX) * constants.positionHashPrimeX)
                   ^ (static_cast<uint32_t>(cellY) * constants.positionHashPrimeY);
    return AvalancheHash(mixed);
}

// The decorrelated draws taken from one position hash. Named so the same draw is never
// accidentally reused for two decisions (which would correlate jitter with rotation).
namespace ScatterHashStream {
    constexpr int JitterX  = 0;
    constexpr int JitterY  = 1;
    constexpr int Density  = 2;
    constexpr int Rotation = 3;
    constexpr int Scale    = 4;
    constexpr int Priority = 5;
}

// A second, decorrelated draw from one position hash (jitter X, jitter Y, density, rotation...).
inline uint32_t HashStream(const PlacementConstants& constants, uint32_t positionHash, int streamIndex) {
    return AvalancheHash(positionHash ^ (static_cast<uint32_t>(streamIndex + 1)
                                         * constants.positionHashPrimeJitter));
}

// Uniform [0,1) from the top 24 bits — exact in binary32, no division.
inline float HashToUnitFloat(uint32_t hash) {
    return static_cast<float>(hash >> 8) * (1.0f / 16777216.0f);
}

inline float ClampUnit(float value) { return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value); }

// Spatial weighting. `contrast` is a rational gamma (never std::pow — that is not portable
// across machines): contrast 1 is the identity, >1 darkens, <1 lifts.
inline float FocusGradientWeight(const ScatterRuleConfiguration& configuration, float focusDistance) {
    if (configuration.focusGradientMode == 0 || configuration.focusGradientStrength <= 0.0f) return 1.0f;
    float normalizedDistance = focusDistance * configuration.focusGradientRadiusReciprocal;
    float base;
    if (configuration.focusGradientMode == 1)      base = 1.0f - ClampUnit(normalizedDistance);
    else if (configuration.focusGradientMode == 2) base = ClampUnit(normalizedDistance);
    else {
        float offset = normalizedDistance - 1.0f;
        base = 1.0f - ClampUnit(offset < 0.0f ? -offset : offset);
    }
    float denominator = base + (1.0f - base) * configuration.focusGradientContrast;
    float shaped = denominator > 1e-6f ? base / denominator : base;
    return 1.0f - configuration.focusGradientStrength
         + configuration.focusGradientStrength * shaped;
}

// The gate: 0 rejects the position, >0 is its acceptance weight. Slope arrives as the SQUARED
// height gradient so no square root and no arc-tangent is needed on either backend.
inline float ScatterGateWeight(const ScatterRuleConfiguration& configuration,
                               float heightNormalized, float slopeGradientSquared,
                               float maskWeight, float obstacleDistance, float focusDistance) {
    if (heightNormalized < configuration.heightMinimum) return 0.0f;
    if (heightNormalized > configuration.heightMaximum) return 0.0f;
    if (slopeGradientSquared < configuration.slopeGradientMinimumSquared) return 0.0f;
    if (slopeGradientSquared > configuration.slopeGradientMaximumSquared) return 0.0f;
    if (configuration.maskStratumIndex >= 0 && maskWeight < configuration.maskWeightMinimum) return 0.0f;
    if (configuration.obstacleDistanceMinimum > 0.0f
        && obstacleDistance < configuration.obstacleDistanceMinimum) return 0.0f;
    if ((configuration.selectionFlags & ScatterSelectionFlag::NearCliffs) != 0
        && obstacleDistance > configuration.nearCliffDistanceMaximum) return 0.0f;
    if ((configuration.selectionFlags & ScatterSelectionFlag::AvoidWater) != 0
        && heightNormalized <= configuration.waterSurfaceNormalized) return 0.0f;
    return FocusGradientWeight(configuration, focusDistance);
}

} // namespace Proc
} // namespace SanmapGen
