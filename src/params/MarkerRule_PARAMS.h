// MarkerRule_PARAMS.h — one procedural marker rule (spawns / resources / areas).
// Layer: PARAMS. Settings only (PLACEMENT_SCATTER_SPEC). Placement math lives in PROC.
#pragma once

namespace SanmapGen {
namespace Params {

enum class MarkerPriority { LargestArea, SmallestArea, LeastVariance };
enum class FocusGradient  { None, CenterFocus, EdgeFocus, Torus };

struct MarkerRule {
    // Filtering gates
    float minSlope  = 0.0f;
    float maxSlope  = 89.9f;
    float minHeight = 0.0f;
    float maxHeight = 1.0f;

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
};

} // namespace Params
} // namespace SanmapGen
