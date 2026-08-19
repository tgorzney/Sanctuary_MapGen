// Placement_Symmetry_PROC.h — one accepted position -> its full symmetry orbit.
// Layer: PROC. Symmetry is owned HERE, in the scatter, not in a GUI widget: the v1
// CalculateMarkerSymmetryGroups was declared, never defined and never called, so symmetry
// alignment simply did not work (PLACEMENT_SCATTER_SPEC). The orbit is a pure function of
// the mask and the position, so clones land on exactly mirrored coordinates and every
// member of one orbit shares a single symmetry id.
// Each clone also carries how the source yaw transforms: finalYaw = yawScale*yaw + yawOffset
// (a mirror reverses handedness, a rotation only adds).
// The orbit-building primitives (SymmetryDetail::*) live in Placement_SymmetryOrbit_PROC.h,
// split out under STEP33 to keep both files under the ARCH §1.5 150-line ceiling.
#pragma once
#include "Placement_SymmetryOrbit_PROC.h"

namespace SanmapGen {
namespace Proc {

// Fills `outPoints` with the source point followed by its clones; returns the orbit size.
// `extent` is the last vertex index, so a mirror is extent - position. `radialSymmetryRepeatCount`
// is the designer's N for the `Radial` bit (ruling 3): a flat sibling of `symmetryMask`, not folded
// into it, so a local-override rule can carry its own count independently of the global one.
inline int BuildSymmetryOrbit(int symmetryMask, int radialSymmetryRepeatCount, float extent,
                              float positionX, float positionY, float duplicateEpsilon,
                              SymmetryOrbitPoint* outPoints, int maximumPoints) {
    using namespace SymmetryDetail;
    SymmetryOrbitPoint source;
    source.positionX = positionX;
    source.positionY = positionY;
    int count = AppendPoint(outPoints, 0, maximumPoints, source, duplicateEpsilon);
    if ((symmetryMask & Params::SymmetryAxis::MirrorAcrossX) != 0)
        count = AppendTransformedSet(outPoints, count, maximumPoints,
                                     OrbitTransform::MirrorAcrossX, extent, duplicateEpsilon);
    if ((symmetryMask & Params::SymmetryAxis::MirrorAcrossZ) != 0)
        count = AppendTransformedSet(outPoints, count, maximumPoints,
                                     OrbitTransform::MirrorAcrossZ, extent, duplicateEpsilon);
    if ((symmetryMask & Params::SymmetryAxis::RotateHalfTurn) != 0)
        count = AppendTransformedSet(outPoints, count, maximumPoints,
                                     OrbitTransform::RotateHalfTurn, extent, duplicateEpsilon);
    if ((symmetryMask & Params::SymmetryAxis::QuarterTurns) != 0)
        count = AppendQuarterTurns(outPoints, count, maximumPoints, extent, duplicateEpsilon);
    if ((symmetryMask & Params::SymmetryAxis::Radial) != 0)
        count = AppendRadialTurns(outPoints, count, maximumPoints, radialSymmetryRepeatCount, extent,
                                  duplicateEpsilon);
    return count;
}

} // namespace Proc
} // namespace SanmapGen
