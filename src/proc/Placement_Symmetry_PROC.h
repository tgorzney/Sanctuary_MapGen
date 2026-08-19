// Placement_Symmetry_PROC.h — one accepted position -> its full symmetry orbit.
// Layer: PROC. Symmetry is owned HERE, in the scatter, not in a GUI widget: the v1
// CalculateMarkerSymmetryGroups was declared, never defined and never called, so symmetry
// alignment simply did not work (PLACEMENT_SCATTER_SPEC). The orbit is a pure function of
// the mask and the position, so clones land on exactly mirrored coordinates and every
// member of one orbit shares a single symmetry id.
// Each clone also carries how the source yaw transforms: finalYaw = yawScale*yaw + yawOffset
// (a mirror reverses handedness, a rotation only adds).
#pragma once
#include "../params/Symmetry_PARAMS.h"
#include "../math/Trigonometry_MATH.h"

namespace SanmapGen {
namespace Proc {

constexpr float symmetryPi = 3.14159265358979323846f;

struct SymmetryOrbitPoint {
    float positionX = 0.0f;
    float positionY = 0.0f;
    float yawScale  = 1.0f;
    float yawOffsetRadians = 0.0f;
};

namespace SymmetryDetail {

enum class OrbitTransform { MirrorAcrossX, MirrorAcrossZ, RotateHalfTurn };

inline SymmetryOrbitPoint ApplyOrbitTransform(const SymmetryOrbitPoint& source,
                                              OrbitTransform transform, float extent) {
    SymmetryOrbitPoint clone = source;
    if (transform == OrbitTransform::MirrorAcrossX) {
        clone.positionX = extent - source.positionX;
        clone.yawScale  = -source.yawScale;
        clone.yawOffsetRadians = symmetryPi - source.yawOffsetRadians;
    } else if (transform == OrbitTransform::MirrorAcrossZ) {
        clone.positionY = extent - source.positionY;
        clone.yawScale  = -source.yawScale;
        clone.yawOffsetRadians = -source.yawOffsetRadians;
    } else {
        clone.positionX = extent - source.positionX;
        clone.positionY = extent - source.positionY;
        clone.yawOffsetRadians = source.yawOffsetRadians + symmetryPi;
    }
    return clone;
}

inline bool IsDuplicatePoint(const SymmetryOrbitPoint* points, int count,
                             float positionX, float positionY, float epsilon) {
    for (int index = 0; index < count; ++index) {
        float deltaX = points[index].positionX - positionX;
        float deltaY = points[index].positionY - positionY;
        if (deltaX < 0.0f) deltaX = -deltaX;
        if (deltaY < 0.0f) deltaY = -deltaY;
        if (deltaX <= epsilon && deltaY <= epsilon) return true;
    }
    return false;
}

inline int AppendPoint(SymmetryOrbitPoint* points, int count, int maximumPoints,
                       const SymmetryOrbitPoint& candidate, float epsilon) {
    if (count >= maximumPoints) return count;
    if (IsDuplicatePoint(points, count, candidate.positionX, candidate.positionY, epsilon)) return count;
    points[count] = candidate;
    return count + 1;
}

// Applies one transform to every point already in the orbit (the group closes as masks combine).
inline int AppendTransformedSet(SymmetryOrbitPoint* points, int count, int maximumPoints,
                                OrbitTransform transform, float extent, float epsilon) {
    const int sourceCount = count;
    for (int index = 0; index < sourceCount; ++index)
        count = AppendPoint(points, count, maximumPoints,
                            ApplyOrbitTransform(points[index], transform, extent), epsilon);
    return count;
}

inline int AppendQuarterTurns(SymmetryOrbitPoint* points, int count, int maximumPoints,
                              float extent, float epsilon) {
    const int sourceCount = count;
    const float center = extent * 0.5f;
    for (int index = 0; index < sourceCount; ++index) {
        const SymmetryOrbitPoint source = points[index];
        float offsetX = source.positionX - center;
        float offsetY = source.positionY - center;
        for (int turn = 0; turn < 3; ++turn) {
            const float rotatedX = -offsetY;        // 90 degrees about the map center
            offsetY = offsetX;
            offsetX = rotatedX;
            SymmetryOrbitPoint clone = source;
            clone.positionX = center + offsetX;
            clone.positionY = center + offsetY;
            clone.yawOffsetRadians = source.yawOffsetRadians
                                   + symmetryPi * 0.5f * static_cast<float>(turn + 1);
            count = AppendPoint(points, count, maximumPoints, clone, epsilon);
        }
    }
    return count;
}

// Same outer shape as AppendQuarterTurns/AppendTransformedSet: capture sourceCount BEFORE
// appending, then rotate every point already in the orbit (not just the original candidate) —
// this is what makes Radial compose correctly with prior bits (MirrorAcrossX/MirrorAcrossZ/
// QuarterTurns) when it runs last, and what ruling 6's 16N worst-case sizing assumes.
inline int AppendRadialTurns(SymmetryOrbitPoint* points, int count, int maximumPoints,
                             int radialSymmetryRepeatCount, float extent, float epsilon) {
    int turnCount = radialSymmetryRepeatCount > 1 ? radialSymmetryRepeatCount : 1; // ruling 2 floor
    // Defensive clamp against maximumPoints, mirroring MakeCandidateGridLayout's gridSide clamp
    // (Placement_Scatter_PROC.cpp): a silent clamp at the point of consumption, engaging BEFORE the
    // append loop runs, so an absurd caller-supplied count (e.g. 100000) cannot loop needlessly —
    // AppendPoint's own per-point cap already prevents memory corruption, but without this the loop
    // would still iterate to completion doing nothing useful. NOT a substitute for the IO-level
    // [2, 12] clamp (Symmetry_PARAMS.h), which protects designers from pathological saved data.
    if (turnCount > maximumPoints) turnCount = maximumPoints;
    const int sourceCount = count;
    const float center = extent * 0.5f;
    for (int index = 0; index < sourceCount; ++index) {
        const SymmetryOrbitPoint source = points[index];
        const float offsetX = source.positionX - center;
        const float offsetY = source.positionY - center;
        for (int turn = 1; turn < turnCount; ++turn) {
            const float angle = static_cast<float>(turn) * (2.0f * symmetryPi / static_cast<float>(turnCount));
            const float cosine = Math::Cosine(angle);
            const float sine   = Math::Sine(angle);
            SymmetryOrbitPoint clone = source;
            clone.positionX = center + (offsetX * cosine - offsetY * sine);
            clone.positionY = center + (offsetX * sine   + offsetY * cosine);
            clone.yawOffsetRadians = source.yawOffsetRadians + angle;   // yawScale unchanged: rotation, not mirror
            count = AppendPoint(points, count, maximumPoints, clone, epsilon);
        }
    }
    return count;
}

} // namespace SymmetryDetail

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
