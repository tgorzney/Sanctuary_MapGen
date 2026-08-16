// Stratum_PARAMS.h — THE one per-stratum settings type (ARCH §7.1).
// Layer: PARAMS. Everything a stratum is configured with is reached through `Params::Stratum`:
// the mask slope gate, the stored-art merge mode, the single surface-weight remap, and the
// bake/appearance settings. There is deliberately **no rival per-stratum settings type** — no
// `StratumMask_PARAMS`, no `Proc::StratumBakeSource`; two rival arrays are exactly how the
// double-remap defect was created (ARCH §7.2.5).
//
// Settings only — no behavior, no computed data, no loaded pixels, no GPU handles (ARCH §3.2).
// The stored mask art and the albedo texels are LOADED INPUT and live in `Data::StratumArt`
// (ARCH §7.1: modes/thresholds -> PARAMS, loaded pixels -> DATA).
//
// SLOPE UNIT (pinned, MASKING_SPEC 1.8): the designer-facing settings here are in DEGREES
// (matching the 0/29/30 visualization stops). The computed slope FIELD is gradient magnitude
// (rise/run = tan of the angle). Mask_PROC converts degrees to gradient exactly once, in its
// configuration flattening, so producer and consumer can never drift.
#pragma once
#include "GenerationEnums_PARAMS.h"
#include "StratumAppearance_PARAMS.h"
#include "StratumSoilPhysics_PARAMS.h"

namespace SanmapGen {
namespace Params {

struct Stratum {
    // --- Slope gate: the stratum shows only where the terrain slope is inside this window.
    bool  bSlopeGateEnabled       = false;
    float minimumSlopeDegrees     = 0.0f;    // window low edge, degrees
    float maximumSlopeDegrees     = 90.0f;   // window high edge, degrees
    float slopeFeatherDegreesLow  = 0.0f;    // smoothstep ramp width below the low edge
    float slopeFeatherDegreesHigh = 0.0f;    // smoothstep ramp width above the high edge
    bool  bUseSmoothstep          = false;   // false = hard clamp (binary in/out of the window)
    bool  bInvertSlopeGate        = false;   // keep the OUTSIDE of the window instead
    float slopeGateStrength       = 1.0f;    // 0 = gate does nothing, 1 = full gate

    // --- How the stored .sanmap stratum art merges with the gated procedural weight.
    // Disabled = procedural only; ProceduralStart = additive; StaticOverride = the art wins
    // and is NOT slope-gated. The pixels themselves are Data::StratumArt::importedMask.
    ImportedMaskMode importedMaskMode = ImportedMaskMode::Disabled;

    // --- The ONE surface-weight remap (ARCH §7.2.5). Applied exactly once, in the Mask stage;
    // Bake consumes the remapped weight verbatim. Identity by default. The names match the
    // .sanmap `maskRemapMin`/`maskRemapMax` keys so the round-trip stays literal.
    float maskRemapMinimum = 0.0f;
    float maskRemapMaximum = 1.0f;

    // --- Appearance: how the bake composites this stratum (its albedo lives in Data::StratumArt).
    bool  bEnabled   = true;    // false = the stratum contributes nothing to the composite
    float tintRed    = 1.0f;    // previewColor / diffuseRemap, multiplied onto the albedo
    float tintGreen  = 1.0f;
    float tintBlue   = 1.0f;
    float tileCount  = 1.0f;    // texture repeats across the map (tileSize)

    // --- The member sub-structs (ARCH §7.1 "composition is allowed"). Split into their own
    // headers only because one flat stratum type cannot stay inside the §1.5 ceiling; neither is a
    // settings type a stage may reach on its own.
    // Material identity, texture paths and the shader appearance the `.sanmap` round-trips.
    StratumAppearance  appearance;
    // The soil the sims run on. THE settings home for the five soil numbers (ARCH §7.1);
    // `Proc::MaterialPhysics` is the runtime record they are pushed onto, not a rival store.
    StratumSoilPhysics soilPhysics;
};

} // namespace Params
} // namespace SanmapGen
