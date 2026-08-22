// Symmetry_PARAMS.h — the placement symmetry axis bit-flags.
// Layer: PARAMS. A rule's `symmetryMask` is an OR of these; the scatter (PROC) turns one
// accepted position into its full symmetry orbit, all clones sharing one symmetry id.
// The legacy names map on: Symmetry_X -> MirrorAcrossX, Symmetry_Z -> MirrorAcrossZ,
// Symmetry_XZ -> MirrorAcrossX|MirrorAcrossZ, Symmetry_Point -> RotateHalfTurn.
// (Y is the vertical axis of a heightmap, so only the two ground axes mirror.)
#pragma once

namespace SanmapGen {
namespace Params {

namespace SymmetryAxis {
    constexpr int None           = 0;
    constexpr int MirrorAcrossX  = 1 << 0;   // x -> (extent - x)
    constexpr int MirrorAcrossZ  = 1 << 1;   // z -> (extent - z)
    constexpr int RotateHalfTurn = 1 << 2;   // 180 degrees about the map center (point symmetry)
    constexpr int QuarterTurns   = 1 << 3;   // 90/180/270 degrees about the map center
    // N-way rotation (ARCH_13_RadialSymmetry.md §13, STEP16 ruling #1/#3, STEP23): count
    // set by the sibling `radialSymmetryRepeatCount` field.
    // `BuildSymmetryOrbit`/`AppendRadialTurns` (Placement_Symmetry_PROC.h) generate the orbit
    // directly from each turn's angle (STEP23 ruling #1) — bit-check order tracks ascending
    // bit value, so `Radial` runs last, after `QuarterTurns`.
    constexpr int Radial         = 1 << 4;
}

// The `[minimum, maximum]` range `radialSymmetryRepeatCount` is clamped to at every IO read site
// (STEP23 ruling #2/#6) — single source of truth for IO, the PROC-level defensive clamp, and the
// future UI slider. `N < 2` is not a rotation (identity only); `12` is a policy ceiling, generous
// for RTS-scale radial team symmetry (no existing player/army-count constant to derive it from).
constexpr int radialSymmetryRepeatCountMinimum = 2;
constexpr int radialSymmetryRepeatCountMaximum = 12;

// Highest clone count any single mask can produce: mirror X * mirror Z * quarter turns * Radial's
// N (STEP23 ruling #6). Must stay >= 16 * radialSymmetryRepeatCountMaximum so a future ceiling
// change doesn't silently reopen the buffer-overflow risk ARCH §13 Defect 2 flagged (at N_max=12:
// 16 * 12 = 192; 256 leaves one page / power-of-two headroom against future symmetry families).
constexpr int symmetryOrbitMaximum = 256;

// The two settings the Symmetry tab promotes out of v1 (WO C1). They are about RECOGNISING
// symmetry in a heightfield that already exists — an imported map, or an authored stack that is
// nearly but not exactly symmetric — which is why they are a separate record from the axis mask
// above: the mask says what symmetry to PRODUCE, these say how much error still counts as
// symmetric and whether to correct it. `Params::MapRecipe::globalSymmetryMask` stays the ONE home
// of the mask; nothing here duplicates it (ARCH §7.1 — no rival settings type).
struct SymmetryDetection {
    // Largest per-sample difference (normalized height, 0..1) two mirrored samples may show and
    // still be called symmetric. v1's default.
    float detectionTolerance = 0.01f;

    // Rewrite a nearly-symmetric field into an exactly symmetric one rather than only reporting
    // it. Off by default: it is a destructive correction.
    bool bSnapImperfectSymmetry = false;
};

// The six exotic-blend scalars SANMAP_FORMAT_SPEC Correction 4's `Symmetry` section carries
// (`SymSuperpositionBlend`/`SymmetryBlurRadius`/`CrossFadeWidth`/`CylinderZScale`/
// `TorusMajorRadius`/`TorusMinorRadius`). Zero PROC consumer today — reserved from the moment
// they are settable (Constitution §8), same posture as `SymmetryDetection`/`StratumAppearance`.
// `Params::SymAlgorithm`, the enum choosing WHICH of these blends is active, is explicitly OUT OF
// SCOPE for this ticket (STEP16 ruling #1) — defining a brand-new enum type is a bigger design
// act than these plain scalars; that field/JSON key is deferred to a future work-order, not
// omitted by oversight.
struct SymmetryBlend {
    float superpositionBlend = 0.5f;   // Superposition mix, 0 = source only, 1 = fully blended
    float blurRadius         = 4.0f;   // Blur-mode symmetry, in cells
    float crossFadeWidth     = 8.0f;   // CrossFade-mode seam width, in cells
    float cylinderZScale     = 1.0f;   // Cylinder3D wrap scale along Z
    float torusMajorRadius   = 64.0f;  // Torus3D major radius, in cells
    float torusMinorRadius   = 16.0f;  // Torus3D minor radius, in cells
};

// A single manual-layer/rule's own mirror-mask setting (ARCH_16_01_NewParamsShapes.md §16.1,
// SANMAP_FORMAT_SPEC Correction 16) — what a "place with symmetry" tool resolves against.
// `bSymmetryUseGlobal` mirrors the `bSlopeUseGlobal` switch `SlopeDefaults` already established
// (MASKING_SPEC §1.7): true resolves against `MapRecipe::globalSymmetryMask`/
// `radialSymmetryRepeatCount`, false uses this record's own `symmetryMask`/
// `radialSymmetryRepeatCount` instead. First landed by STEP60/STEP66 (whichever lands first
// defines it; the other checks for its existence before re-adding it, same "first ticket to land
// wins" rule already used elsewhere in this backlog).
struct SymmetrySetting {
    bool bSymmetryUseGlobal = true;
    int  symmetryMask       = 0;
    int  radialSymmetryRepeatCount = 3;
};

} // namespace Params
} // namespace SanmapGen
