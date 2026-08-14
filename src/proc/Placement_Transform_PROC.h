// Placement_Transform_PROC.h — the per-instance transform, sampled from the position hash.
// Layer: PROC. Scale range, rotation range and align-to-normal are capabilities v1 simply
// did not have (PLACEMENT_SCATTER_SPEC "Missing capability"); they are sampled here from
// dedicated hash streams, so the transform is reproducible from (seed, rule, position) and
// never touches rand(). Rotation is a quaternion (x, y, z, w), matching the marker transform
// model the format expects. Trigonometry_MATH supplies the portable sine/cosine.
#pragma once
#include <cmath>
#include "Placement_Gate_PROC.h"
#include "../math/Trigonometry_MATH.h"

namespace SanmapGen {
namespace Proc {

struct SampledInstanceTransform {
    float rotationX = 0.0f, rotationY = 0.0f, rotationZ = 0.0f, rotationW = 1.0f;
    float scaleX = 1.0f, scaleY = 1.0f, scaleZ = 1.0f;
};

// Upward terrain normal from the height gradient (game units per cell), normalized.
inline void TerrainNormalFromGradient(float gradientX, float gradientZ,
                                      float& normalX, float& normalY, float& normalZ) {
    const float lengthSquared = gradientX * gradientX + gradientZ * gradientZ + 1.0f;
    const float reciprocalLength = 1.0f / std::sqrt(lengthSquared);
    normalX = -gradientX * reciprocalLength;
    normalY = reciprocalLength;
    normalZ = -gradientZ * reciprocalLength;
}

// Hamilton product: applies `second` first, then `first`.
inline void QuaternionMultiply(float firstX, float firstY, float firstZ, float firstW,
                               float secondX, float secondY, float secondZ, float secondW,
                               SampledInstanceTransform& outTransform) {
    outTransform.rotationX = firstW * secondX + firstX * secondW + firstY * secondZ - firstZ * secondY;
    outTransform.rotationY = firstW * secondY - firstX * secondZ + firstY * secondW + firstZ * secondX;
    outTransform.rotationZ = firstW * secondZ + firstX * secondY - firstY * secondX + firstZ * secondW;
    outTransform.rotationW = firstW * secondW - firstX * secondX - firstY * secondY - firstZ * secondZ;
}

// Shortest-arc rotation from world up (0,1,0) onto `normal` — the half-way quaternion, so no
// arc-cosine is needed (and none exists in the portable math set).
inline void AlignmentQuaternion(float normalX, float normalY, float normalZ,
                                float& outX, float& outY, float& outZ, float& outW) {
    const float weight = 1.0f + normalY;
    if (weight < 1.0e-6f) { outX = 1.0f; outY = 0.0f; outZ = 0.0f; outW = 0.0f; return; }
    float axisX = normalZ, axisY = 0.0f, axisZ = -normalX;
    const float lengthSquared = axisX * axisX + axisY * axisY + axisZ * axisZ + weight * weight;
    const float reciprocalLength = 1.0f / std::sqrt(lengthSquared);
    outX = axisX * reciprocalLength;
    outY = axisY * reciprocalLength;
    outZ = axisZ * reciprocalLength;
    outW = weight * reciprocalLength;
}

// yaw = yawScale * sampledYaw + yawOffset — the symmetry orbit supplies the two terms so a
// mirrored clone faces the mirrored way.
inline SampledInstanceTransform SampleInstanceTransform(const PlacementConstants& constants,
                                                        const ScatterRuleConfiguration& configuration,
                                                        uint32_t positionHash,
                                                        float yawScale, float yawOffsetRadians,
                                                        float normalX, float normalY, float normalZ) {
    const float scaleUnit = HashToUnitFloat(HashStream(constants, positionHash, ScatterHashStream::Scale));
    const float scale = configuration.scaleMinimum
                      + (configuration.scaleMaximum - configuration.scaleMinimum) * scaleUnit;
    const float yawUnit = HashToUnitFloat(HashStream(constants, positionHash, ScatterHashStream::Rotation));
    const float sampledYaw = configuration.rotationMinimumRadians
                           + (configuration.rotationMaximumRadians - configuration.rotationMinimumRadians)
                           * yawUnit;
    const float yaw = yawScale * sampledYaw + yawOffsetRadians;
    const float halfYaw = yaw * 0.5f;

    SampledInstanceTransform transform;
    transform.scaleX = transform.scaleY = transform.scaleZ = scale;
    const float yawX = 0.0f, yawY = Math::Sine(halfYaw), yawZ = 0.0f, yawW = Math::Cosine(halfYaw);
    if ((configuration.selectionFlags & ScatterSelectionFlag::AlignToNormal) == 0) {
        transform.rotationX = yawX; transform.rotationY = yawY;
        transform.rotationZ = yawZ; transform.rotationW = yawW;
        return transform;
    }
    float alignX, alignY, alignZ, alignW;
    AlignmentQuaternion(normalX, normalY, normalZ, alignX, alignY, alignZ, alignW);
    QuaternionMultiply(alignX, alignY, alignZ, alignW, yawX, yawY, yawZ, yawW, transform);
    return transform;
}

} // namespace Proc
} // namespace SanmapGen
