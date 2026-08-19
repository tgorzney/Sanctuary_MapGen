// Placement_SymmetryOrbit_PROC.h — the orbit-building primitives behind BuildSymmetryOrbit.
// Layer: PROC. Split out of Placement_Symmetry_PROC.h (STEP33) purely to land both files under
// the ARCH §1.5 150-line ceiling; no behavior change. AppendQuarterTurns and AppendRadialTurns
// intentionally do NOT share one generic implementation: collapsing QuarterTurns into
// Radial(turnCount=4) would replace exact swap-based 90-degree rotation with trig calls, a real
// determinism risk (Math::Cosine/Sine at pi/2 boundaries may not be bit-exact 0/+-1 the way
// integer swaps are) and a needless perf regression for the cheapest transform family.
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
} // namespace Proc
} // namespace SanmapGen
