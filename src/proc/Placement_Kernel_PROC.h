// Placement_Kernel_PROC.h — the one scatter kernel contract shared by both backends.
// Layer: PROC. Declares (a) every tweakable stage constant — including the hash multipliers
// and the old hardcoded stochastic seed 12345, which PLACEMENT_SCATTER_SPEC calls out as a
// violation (Constitution §8) — and (b) the flattened per-rule record whose field order and
// types are the std430 layout the GLSL twin (Placement_PROC.glsl) mirrors EXACTLY, so the
// CPU gate and the preview gate can never drift (DISPATCH_INTERFACE_SPEC §4).
#pragma once

namespace SanmapGen {
namespace Proc {

// Stage constants — defaults only; every one is settable per project (§8).
struct PlacementConstants {
    unsigned int positionHashPrimeX      = 73856093u;    // Teschner spatial-hash primes
    unsigned int positionHashPrimeY      = 83492791u;
    unsigned int positionHashPrimeRule   = 19349663u;
    unsigned int positionHashPrimeJitter = 2654435761u;  // per-axis decorrelation
    unsigned int positionHashSeedOffset  = 0u;           // was the hardcoded 12345

    float candidateJitterStrength   = 1.0f;   // 0 = grid centers, 1 = full cell jitter
    float candidateCellSizeFactor   = 0.70710678f;   // 1/sqrt(2): Poisson grid cell per spacing
    float candidateCellSizeMinimum  = 1.0f;   // never finer than one heightfield cell
    float spacingEpsilon            = 1e-4f;  // spacing below this means "no spacing rule"
    float symmetryDuplicateEpsilon  = 0.5f;   // a clone this close to a sibling is the same point

    // worldUnitsPerCell is NOT here: it is map geometry, not a placement constant, and lives
    // on Params::Geometry (M5-0a). Every reader takes it from the recipe.
    float clearanceHeightTolerance  = 0.02f;  // fallback when a rule sets no areaHeightRange
    int   clearanceSearchRadiusMaximum = 64;  // cap for the radial-clearance gallop

    float obstacleGradientTolerance = 0.25f;  // Jump-Flood seed: normalized height gradient
    float obstacleDistanceMaximum   = 64.0f;  // clamp of the distance field
    float playableHeightMinimum     = 0.0f;   // Jump-Flood seed: out-of-band cells
    float playableHeightMaximum     = 1.0f;

    float focusGradientEpsilon      = 1e-4f;  // guards a zero focus radius
    float varianceSampleRadius      = 2.0f;   // LeastVariance priority window, in cells

    float slopeVerticalDegrees      = 89.99f; // at/above this a slope gate is "no limit"
    float slopeGradientSentinel     = 1.0e6f; // the gradient that stands in for vertical
    float degreesToRadians          = 0.017453292f;

    int   scatterTileWidth          = 8;      // GPU preview gate workgroup (WorkgroupSize)
    int   scatterTileHeight         = 8;
    int   candidateCountMaximum     = 1 << 20;  // safety cap on one rule's candidate grid
};

// One flattened scatter rule, ready for either backend. 32 scalars = 128 bytes; the trailing
// padding keeps the std430 array stride a 16-byte multiple. Order is load-bearing.
struct ScatterRuleConfiguration {
    int   ruleIndex          = 0;
    int   collectionIndex    = 0;   // 0 markers, 1 props, 2 units, 3 decals
    int   category           = 0;   // Params::MarkerCategory as int
    int   priorityMode       = 0;   // Params::MarkerPriority as int
    int   focusGradientMode  = 0;   // Params::FocusGradient as int
    int   symmetryMask       = 0;
    int   maskStratumIndex   = -1;
    int   targetCount        = 0;
    int   mapEdgePadding     = 0;
    int   selectionFlags     = 0;   // see ScatterSelectionFlag below
    int   armyIndex          = -1;
    int   ruleSeed           = 0;   // geometry.seed mixed with the rule identity
    float density                    = 0.0f;
    float heightMinimum              = 0.0f;   // normalized 0..1
    float heightMaximum              = 1.0f;
    float slopeGradientMinimumSquared = 0.0f;  // (tan of the slope gate) squared
    float slopeGradientMaximumSquared = 0.0f;
    float spacingMinimum             = 0.0f;
    float clearanceRadiusMinimum     = 0.0f;
    float clearanceRadiusMaximum     = 0.0f;
    float clearanceHeightTolerance   = 0.0f;
    float maskWeightMinimum          = 0.0f;
    float obstacleDistanceMinimum    = 0.0f;
    float nearCliffDistanceMaximum   = 0.0f;
    float focusGradientRadiusReciprocal = 0.0f;
    float focusGradientStrength      = 0.0f;
    float focusGradientContrast      = 1.0f;
    float scaleMinimum               = 1.0f;
    float scaleMaximum               = 1.0f;
    float rotationMinimumRadians     = 0.0f;
    float rotationMaximumRadians     = 0.0f;
    float waterSurfaceNormalized     = 0.0f;   // water level as a normalized height
};

// Bits of ScatterRuleConfiguration::selectionFlags (mirrored in the GLSL twin).
namespace ScatterSelectionFlag {
    constexpr int UseDensity        = 1 << 0;
    constexpr int UseAllPositions   = 1 << 1;
    constexpr int RandomSelection   = 1 << 2;
    constexpr int AvoidWater        = 1 << 3;
    constexpr int NearCliffs        = 1 << 4;
    constexpr int AlignToNormal     = 1 << 5;
    constexpr int Collidable        = 1 << 6;
    constexpr int Hidden            = 1 << 7;
    constexpr int CheckMaximumRadius = 1 << 8;
}

} // namespace Proc
} // namespace SanmapGen
