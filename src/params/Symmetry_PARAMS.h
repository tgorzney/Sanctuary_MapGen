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

} // namespace Params
} // namespace SanmapGen
