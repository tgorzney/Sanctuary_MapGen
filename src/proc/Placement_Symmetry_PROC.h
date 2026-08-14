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

} // namespace SymmetryDetail

// Fills `outPoints` with the source point followed by its clones; returns the orbit size.
// `extent` is the last vertex index, so a mirror is extent - position.
inline int BuildSymmetryOrbit(int symmetryMask, float extent, float positionX, float positionY,
                              float duplicateEpsilon, SymmetryOrbitPoint* outPoints, int maximumPoints) {
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
    return count;
}

} // namespace Proc
} // namespace SanmapGen
