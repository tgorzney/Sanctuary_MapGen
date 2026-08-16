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
}

// Highest clone count any single mask can produce (mirror X * mirror Z * quarter turns).
constexpr int symmetryOrbitMaximum = 16;

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

} // namespace Params
} // namespace SanmapGen
