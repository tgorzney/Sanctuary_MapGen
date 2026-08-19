// ScatterRule_PARAMS.h — prop, decal and unit scatter rules.
// Layer: PARAMS. Settings only (PLACEMENT_SCATTER_SPEC). Scatter math lives in PROC.
// The v1 rules carried density + height/slope only; the spacing (Poisson radius), biome/
// mask gate, edge padding, symmetry and transform ranges below are the "missing capability"
// list from the spec, exposed as first-class tweakables (Constitution §8).
#pragma once
#include "ScatterTransform_PARAMS.h"

namespace SanmapGen {
namespace Params {

struct PropRule {
    bool  bEnabled  = true;
    float density   = 0.5f;
    float minSlope  = 0.0f;
    float maxSlope  = 89.9f;
    float minHeight = 0.0f;
    float maxHeight = 1.0f;
    bool  bAvoidWater = false;
    bool  bNearCliffs = false;

    float spacingMinimum          = 0.0f;   // Poisson-disk radius, in cells
    int   mapEdgePadding          = 0;
    int   maskStratumIndex        = -1;     // -1 = no biome gate; else a surfaceStratumWeights index
    float maskWeightMinimum       = 0.0f;
    float obstacleDistanceMinimum = 0.0f;   // Jump-Flood exclusion
    float nearCliffDistanceMaximum = 0.0f;  // bNearCliffs: max distance to a steep cell

    bool  bSymmetryUseGlobal = true;
    int   symmetryMask       = 0;
    // Companion count for the `SymmetryAxis::Radial` bit (ARCH §13) — a flat sibling of
    // `symmetryMask`. Zero PROC consumer yet (STEP16 ruling #1/#3).
    int   radialSymmetryRepeatCount = 3;

    ScatterTransform transform;
};

struct DecalRule {
    bool  bEnabled  = true;
    float density   = 0.5f;
    float minSlope  = 0.0f;
    float maxSlope  = 89.9f;
    float minHeight = 0.0f;
    float maxHeight = 1.0f;

    float spacingMinimum   = 0.0f;
    int   mapEdgePadding   = 0;
    int   maskStratumIndex = -1;
    float maskWeightMinimum = 0.0f;

    bool  bSymmetryUseGlobal = true;
    int   symmetryMask       = 0;
    // Companion count for the `SymmetryAxis::Radial` bit (ARCH §13) — a flat sibling of
    // `symmetryMask`. Zero PROC consumer yet (STEP16 ruling #1/#3).
    int   radialSymmetryRepeatCount = 3;

    ScatterTransform transform;
};

// Pre-placed army units. Replaces the GUI widget's jittered rows x cols grid + rand()
// (PLACEMENT_SCATTER_SPEC "Unit spawning") with the same seeded scatter every other rule uses.
struct UnitRule {
    bool  bEnabled  = true;
    int   armyIndex = 0;        // Chosen = 0, Guard = 1, EDA = 2
    int   count     = 0;
    float minSlope  = 0.0f;
    float maxSlope  = 89.9f;
    float minHeight = 0.0f;
    float maxHeight = 1.0f;

    float spacingMinimum   = 1.0f;
    int   mapEdgePadding   = 0;
    int   maskStratumIndex = -1;
    float maskWeightMinimum = 0.0f;

    bool  bSymmetryUseGlobal = true;
    int   symmetryMask       = 0;
    // Companion count for the `SymmetryAxis::Radial` bit (ARCH §13) — a flat sibling of
    // `symmetryMask`. Zero PROC consumer yet (STEP16 ruling #1/#3).
    int   radialSymmetryRepeatCount = 3;

    ScatterTransform transform;
};

} // namespace Params
} // namespace SanmapGen
