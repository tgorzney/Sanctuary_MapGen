// StratumMask_PARAMS.h — how ONE stratum's material mask is produced (adjustable settings).
// Layer: PARAMS (the recipe half MASKING_SPEC calls `StratumSettings`). Carries the slope
// gate (window, feather, smoothstep/hard-clamp, invert, strength), the stored/imported mask
// art plus its `ImportedMaskMode` merge mode, and the per-stratum output remap. Settings
// only — no behavior, no computed data, no GPU handles (ARCH §3.2).
//
// SLOPE UNIT (pinned, MASKING_SPEC "Slope unit ambiguity"): the designer-facing settings in
// this file are in DEGREES (matching the visualization stops 0/29/30). The generated slope
// FIELD is gradient magnitude (rise/run = tan of the angle) — Mask_PROC converts these
// degrees to gradient exactly once, in one place, so producer and consumer can never drift.
#pragma once
#include <vector>
#include "GenerationEnums_PARAMS.h"

namespace SanmapGen {
namespace Params {

struct StratumMask {
    // Slope gate — the stratum shows only where the terrain slope is inside this window.
    bool  bSlopeGateEnabled       = false;
    float minimumSlopeDegrees     = 0.0f;    // window low edge, degrees
    float maximumSlopeDegrees     = 90.0f;   // window high edge, degrees
    float slopeFeatherDegreesLow  = 0.0f;    // smoothstep ramp width below the low edge
    float slopeFeatherDegreesHigh = 0.0f;    // smoothstep ramp width above the high edge
    bool  bUseSmoothstep          = false;   // false = hard clamp (binary in/out window)
    bool  bInvertSlopeGate        = false;   // keep the OUTSIDE of the window instead
    float slopeGateStrength       = 1.0f;    // 0 = gate does nothing, 1 = full gate

    // Stored (imported) stratum mask — the .sanmap stratum TGA art for this slot.
    ImportedMaskMode   importedMaskMode = ImportedMaskMode::Disabled;
    std::vector<float> importedMaskData;     // row-major, importedMaskWidth * importedMaskHeight
    int                importedMaskWidth  = 0;
    int                importedMaskHeight = 0;

    // Final per-stratum remap of the merged mask (MASKING_SPEC maskRemapMin/maskRemapMax).
    // Identity by default; folded into the producer so preview == bake.
    float maskRemapMinimum = 0.0f;
    float maskRemapMaximum = 1.0f;

    bool HasStoredMask() const {
        return importedMaskMode != ImportedMaskMode::Disabled && importedMaskWidth > 0
            && importedMaskHeight > 0
            && importedMaskData.size() >= static_cast<std::size_t>(importedMaskWidth) * importedMaskHeight;
    }
};

} // namespace Params
} // namespace SanmapGen
