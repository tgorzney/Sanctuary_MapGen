// RigidTransformPivot_MATH.h — pure rigid-transform math over bare scalars: zero Params:: types,
// reusable by any domain's rotate-around-pivot need (ARCH_19_08 — Bundle's move/rotate and
// Assembly's own future rotate share this one function, not two copies). Layer: MATH.
#pragma once
#include "Trigonometry_MATH.h"

namespace SanmapGen {
namespace Math {

// Rotates (x, z) by angleRadians (counter-clockwise — same convention as AppendRadialTurns,
// Placement_SymmetryOrbit_PROC.h:120-126) around (pivotX, pivotZ). outX/outZ may alias x/z.
inline void RotatePointAroundPivot(float x, float z, float pivotX, float pivotZ, float angleRadians,
                                   float& outX, float& outZ) {
    const float offsetX = x - pivotX;
    const float offsetZ = z - pivotZ;
    const float cosine  = Cosine(angleRadians);
    const float sine    = Sine(angleRadians);
    outX = pivotX + (offsetX * cosine - offsetZ * sine);
    outZ = pivotZ + (offsetX * sine   + offsetZ * cosine);
}

// Hamilton product: applies `second` first, then `first` — same order convention as
// Placement_Transform_PROC.h's QuaternionMultiply, replicated with plain scalar out-params (zero
// Params::/Proc:: types) so a UI-layer rigid-body rotate can call it without a PROC dependency.
inline void MultiplyQuaternions(float firstX, float firstY, float firstZ, float firstW,
                                float secondX, float secondY, float secondZ, float secondW,
                                float& outX, float& outY, float& outZ, float& outW) {
    outX = firstW*secondX + firstX*secondW + firstY*secondZ - firstZ*secondY;
    outY = firstW*secondY - firstX*secondZ + firstY*secondW + firstZ*secondX;
    outZ = firstW*secondZ + firstX*secondY - firstY*secondX + firstZ*secondW;
    outW = firstW*secondW - firstX*secondX - firstY*secondY - firstZ*secondZ;
}

// A yaw-only quaternion (rotation about world Y by angleRadians) — same construction as
// Placement_Transform_PROC.h:74-77 (yawX=0, yawZ=0).
inline void YawQuaternion(float angleRadians, float& outX, float& outY, float& outZ, float& outW) {
    const float halfAngle = angleRadians * 0.5f;
    outX = 0.0f; outY = Sine(halfAngle); outZ = 0.0f; outW = Cosine(halfAngle);
}

} // namespace Math
} // namespace SanmapGen
