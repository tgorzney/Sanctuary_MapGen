// MarkerRule_PARAMS.h — one procedural marker rule (spawns / resources / areas).
// Layer: PARAMS. Settings only (PLACEMENT_SCATTER_SPEC). Placement math lives in PROC.
#pragma once
#include "ScatterTransform_PARAMS.h"

namespace SanmapGen {
namespace Params {

enum class MarkerPriority { LargestArea, SmallestArea, LeastVariance };
enum class FocusGradient  { None, CenterFocus, EdgeFocus, Torus };

// What the AI analysis expects to find at this marker (AI_HOSTCLIENT_SPEC §A): start
// positions, alloy (mex) spots, expansion sites — or a plain decorative/area marker.
enum class MarkerCategory { Generic, Spawn, Alloys, Expansion };

struct MarkerRule {
    bool bEnabled = true;
    bool bHidden  = false;     // still generated (clearance/fairness) even when not shown

    MarkerCategory category = MarkerCategory::Generic;

    // Filtering gates
    float minSlope  = 0.0f;
    float maxSlope  = 89.9f;
    float minHeight = 0.0f;
    float maxHeight = 1.0f;

    // Biome / mask gate — absent in v1 (filtering was height+slope+clearance only).
    int   maskStratumIndex  = -1;    // -1 = no gate; else an index into surfaceStratumWeights
    float maskWeightMinimum = 0.0f;  // required VISIBLE weight of that stratum at the position

    // Exclusion from obstacles/boundaries via the Jump-Flood distance field.
    float obstacleDistanceMinimum = 0.0f;

    // Spacing / area
    float clearanceSpacing   = 0.0f;
    int   mapEdgePadding     = 0;
    float areaRadiusMinimum  = 0.0f;
    float areaRadiusMaximum  = 0.0f;
    bool  bCheckMaximumRadius = false;
    float areaHeightRange    = 0.0f;

    // Quantity / selection
    bool           bUseDensity      = false;
    float          density          = 0.02f;
    int            count            = 4;
    bool           bUseAllPositions = false;
    bool           bRandomSelection = false;
    MarkerPriority priority         = MarkerPriority::LargestArea;

    // Spatial weighting
    FocusGradient focusGradient         = FocusGradient::None;
    float         focusGradientRadius   = 0.0f;
    float         focusGradientStrength = 0.0f;
    float         focusGradientContrast = 1.0f;

    // Symmetry
    bool bSymmetryUseGlobal = true;
    int  symmetryMask       = 0;
    // Companion count for the `SymmetryAxis::Radial` bit (ARCH §13) — a flat sibling of
    // `symmetryMask`. Consumed by `AppendRadialTurns`/`BuildSymmetryOrbit` via
    // `ResolveRadialSymmetryRepeatCount` (STEP23), the same `bSymmetryUseGlobal` switch
    // `ResolveSymmetryMask` already uses for `symmetryMask`.
    int  radialSymmetryRepeatCount = 3;

    // Per-layer resource/spawn tuning (SANMAP_FORMAT_SPEC Correction 7): moved from v1's global
    // scalars to per-`MarkerRule` fields, so different marker layers (e.g. an "outer expansions"
    // layer vs. a "start Alloys" layer) can tune these independently (Constitution §8).
    float hydroMultiplier = 1.0f;   // hydro-adjacent resource density multiplier
    float reclaimDensity  = 0.0f;   // wreckage/reclaim density this layer seeds
    float mexDensity      = 0.0f;   // Alloy (mex) spot density this layer seeds
    int   spawnPointCount = 0;      // spawn positions this layer places, when category == Spawn

    // Instance transform (scale/rotation/align ranges + tpId).
    ScatterTransform transform;
};

} // namespace Params
} // namespace SanmapGen
