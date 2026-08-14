// Placement_RuleBuild_PROC.h — the shared rule-flattening helpers.
// Layer: PROC. Marker / prop / unit / decal rules differ only in their selection fields;
// their gate core is identical, so it is written once here as a template and the four
// families add only what is theirs (Placement_Rules_PROC.cpp).
// The slope gate is converted to a SQUARED height gradient with the portable minimax
// tangent (Sine/Cosine from Trigonometry_MATH — never std::tan), so the gate itself needs
// no transcendental and stays bit-identical across machines (DETERMINISM_SPEC).
#pragma once
#include "Placement_Gate_PROC.h"
#include "../math/Trigonometry_MATH.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/ScatterTransform_PARAMS.h"
#include "../params/Water_PARAMS.h"

namespace SanmapGen {
namespace Proc {

inline float TangentFromDegrees(const PlacementConstants& constants, float degrees) {
    if (degrees <= 0.0f) return 0.0f;
    if (degrees >= constants.slopeVerticalDegrees) return constants.slopeGradientSentinel;
    float radians = degrees * constants.degreesToRadians;
    float cosine = Math::Cosine(radians);
    if (cosine < 1.0e-6f) return constants.slopeGradientSentinel;
    return Math::Sine(radians) / cosine;
}

inline float SlopeGradientSquaredFromDegrees(const PlacementConstants& constants, float degrees) {
    float tangent = TangentFromDegrees(constants, degrees);
    return tangent * tangent;
}

// A rule's own seed: decorrelated per (map seed, collection, rule index) so two rules with
// identical settings never scatter onto the same candidate positions.
inline int MakeRuleSeed(const PlacementConstants& constants, unsigned int mapSeed,
                        int collectionIndex, int ruleIndex) {
    uint32_t mixed = mapSeed
                   ^ (static_cast<uint32_t>(collectionIndex + 1) * constants.positionHashPrimeRule)
                   ^ (static_cast<uint32_t>(ruleIndex + 1) * constants.positionHashPrimeJitter);
    return static_cast<int>(AvalancheHash(mixed));
}

inline void ApplyTransformToConfiguration(const PlacementConstants& constants,
                                          const Params::ScatterTransform& transform,
                                          ScatterRuleConfiguration& configuration) {
    configuration.scaleMinimum = transform.scaleMinimum;
    configuration.scaleMaximum = transform.scaleMaximum < transform.scaleMinimum
                               ? transform.scaleMinimum : transform.scaleMaximum;
    configuration.rotationMinimumRadians = transform.rotationMinimumDegrees * constants.degreesToRadians;
    configuration.rotationMaximumRadians = transform.rotationMaximumDegrees * constants.degreesToRadians;
    if (transform.bAlignToTerrainNormal) configuration.selectionFlags |= ScatterSelectionFlag::AlignToNormal;
    if (transform.bCollidable)           configuration.selectionFlags |= ScatterSelectionFlag::Collidable;
}

inline int ResolveSymmetryMask(bool bUseGlobal, int ruleMask, int globalMask) {
    return bUseGlobal ? globalMask : ruleMask;
}

// Everything the four rule families share: the gates, the edge padding, the mask gate, the
// water surface and the instance transform.
template <typename RuleType>
inline ScatterRuleConfiguration MakeCommonConfiguration(const PlacementConstants& constants,
                                                        const Params::Geometry& geometry,
                                                        const Params::Water& water,
                                                        const RuleType& rule,
                                                        int ruleIndex, int collectionIndex) {
    ScatterRuleConfiguration configuration;
    configuration.ruleIndex        = ruleIndex;
    configuration.collectionIndex  = collectionIndex;
    configuration.ruleSeed         = MakeRuleSeed(constants, geometry.seed, collectionIndex, ruleIndex);
    configuration.heightMinimum    = rule.minHeight;
    configuration.heightMaximum    = rule.maxHeight;
    configuration.slopeGradientMinimumSquared = SlopeGradientSquaredFromDegrees(constants, rule.minSlope);
    configuration.slopeGradientMaximumSquared = SlopeGradientSquaredFromDegrees(constants, rule.maxSlope);
    configuration.mapEdgePadding      = rule.mapEdgePadding;
    configuration.maskStratumIndex    = rule.maskStratumIndex;
    configuration.maskWeightMinimum   = rule.maskWeightMinimum;
    configuration.clearanceHeightTolerance = constants.clearanceHeightTolerance;
    configuration.waterSurfaceNormalized =
        (water.bEnabled && geometry.terrainMaxHeight > 0.0f)
            ? water.waterLevelMaximum / geometry.terrainMaxHeight : 0.0f;
    ApplyTransformToConfiguration(constants, rule.transform, configuration);
    return configuration;
}

} // namespace Proc
} // namespace SanmapGen
