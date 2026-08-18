// SlopeDefaults_PARAMS.h — the shared-default layer the Mask stage's config-flattening step
// consults for any stratum that opts into it (`Params::Stratum::bSlopeUseGlobal`).
// Layer: PARAMS. MASKING_SPEC.md §1.7: "Settings live in ONE per-stratum type — plus a shared-
// default layer." A single global record, NOT a per-stratum type any stage reaches on its own
// (ARCH §7.1 "no rival settings type") — the Mask stage's flattening step (Mask_Prepare_PROC.cpp)
// is the ONLY place this record is read, alongside each stratum's own fields, exactly the way
// `Params::Stratum`'s own slope-gate fields are read today.
//
// Same seven fields, same names, same units as `Params::Stratum`'s own slope-gate block
// (Stratum_PARAMS.h) — this is a pure default-vs-override SOURCE choice at flattening time, not a
// new shape (MASKING_SPEC.md §1.7). The field defaults below are copied VERBATIM from
// `Params::Stratum`'s current hardcoded defaults so a freshly-constructed stratum (which now
// defaults `bSlopeUseGlobal = true`) resolves to the exact same gate it always has.
#pragma once

namespace SanmapGen {
namespace Params {

struct SlopeDefaults {
    bool  bSlopeGateEnabled       = false;
    float minimumSlopeDegrees     = 0.0f;    // window low edge, degrees
    float maximumSlopeDegrees     = 90.0f;   // window high edge, degrees
    float slopeFeatherDegreesLow  = 0.0f;    // smoothstep ramp width below the low edge
    float slopeFeatherDegreesHigh = 0.0f;    // smoothstep ramp width above the high edge
    bool  bUseSmoothstep          = false;   // false = hard clamp (binary in/out of the window)
    bool  bInvertSlopeGate        = false;   // keep the OUTSIDE of the window instead
    float slopeGateStrength       = 1.0f;    // 0 = gate does nothing, 1 = full gate
};

} // namespace Params
} // namespace SanmapGen
