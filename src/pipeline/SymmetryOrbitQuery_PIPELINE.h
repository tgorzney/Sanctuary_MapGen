// SymmetryOrbitQuery_PIPELINE.h — a stateless, on-demand mirror-orbit query for UI callers.
// Layer: PIPELINE. Deliberately not marker-named: the parameters carry no marker concept at all
// (geometry, a mask, a world position, output points), so a future Props/Decals/Units "place with
// mirroring" feature calls this SAME function instead of an independent copy (ARCH_16_03's ruling,
// STEP68). This is the legal `UI -> PIPELINE -> PROC` call path ARCH_03_ModuleBoundaries.md §3.1's
// dependency table requires — `ARCH_16_03_ModuleBoundaryChain.md` §16.3 rules a stateless query
// function may live here alongside Generation_PIPELINE/PreviewDriver_PIPELINE's stage-conductor
// responsibilities, with no DAG participation of its own (no dirty-hash, no DispatchPolicy, nothing
// owned across frames).
//
// Wraps Proc::BuildSymmetryOrbit (Placement_Symmetry_PROC.h) UNCHANGED, handling the two things a
// UI caller cannot do itself:
//  - World <-> cell coordinate conversion. BuildSymmetryOrbit operates in heightfield cell space;
//    MarkerTransform.transform (and every other domain's transform) stores absolute world units.
//    Divide by geometry.worldUnitsPerCell going in, multiply going back out — the exact conversion
//    Placement_Emit_PROC.cpp already applies for procedural placement, not a second derivation.
//  - `extent` without a baked heightfield: geometry.VertexSize() - 1, PARAMS-only, since a manual
//    marker may be edited before any generation has run.
#pragma once
#include "../params/Geometry_PARAMS.h"

namespace SanmapGen {
namespace Pipeline {

struct WorldSymmetryOrbitPoint {
    float worldPositionX = 0.0f;
    float worldPositionZ = 0.0f;
};

// Synchronous, on-demand — explicitly NOT a Generation_PIPELINE/PreviewDriver_PIPELINE DAG stage.
// Returns the orbit size (>=1, always includes the source point), written into `outPoints`
// (capacity `maximumPoints`, the same buffer sizing discipline Proc::BuildSymmetryOrbit's other
// callers already use — see Params::symmetryOrbitMaximum, Symmetry_PARAMS.h).
int BuildWorldSymmetryOrbit(const Params::Geometry& geometry, int symmetryMask,
                            int radialSymmetryRepeatCount, float worldPositionX,
                            float worldPositionZ, WorldSymmetryOrbitPoint* outPoints,
                            int maximumPoints);

// STEP75: composes `sourceRotation` with a hardcoded 180-degree yaw about the vertical (Y) axis
// (x=0, y=1, z=0, w=0 — sin(90deg)=1, cos(90deg)=0, hardcoded rather than computed via trig to
// avoid float error). A pure sibling of BuildWorldSymmetryOrbit's position half, so a UI caller
// mirroring an entity 180 degrees about map center (`RotateHalfTurn`) can also mirror its
// orientation without reaching into PROC directly (ARCH_03_ModuleBoundaries.md SS3.1: `UI ->
// PIPELINE -> PROC` is legal, `UI -> PROC` is not). ONLY valid for RotateHalfTurn (a pure
// rotation) -- do NOT reuse this for a mirror axis (MirrorAcrossX/MirrorAcrossZ), which reflects
// rather than rotates and needs a separate, verified design.
void ApplyHalfTurnYaw(float sourceRotationX, float sourceRotationY, float sourceRotationZ,
                      float sourceRotationW, float& outRotationX, float& outRotationY,
                      float& outRotationZ, float& outRotationW);

} // namespace Pipeline
} // namespace SanmapGen
