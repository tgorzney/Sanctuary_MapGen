// SymmetryOrbitQuery_PIPELINE.cpp — see SymmetryOrbitQuery_PIPELINE.h for the full rationale.
#include "SymmetryOrbitQuery_PIPELINE.h"
#include "../params/Symmetry_PARAMS.h"
#include "../proc/Placement_Symmetry_PROC.h"
#include "../proc/Placement_Kernel_PROC.h"
#include "../proc/Placement_Transform_PROC.h"

namespace SanmapGen {
namespace Pipeline {

int BuildWorldSymmetryOrbit(const Params::Geometry& geometry, int symmetryMask,
                            int radialSymmetryRepeatCount, float worldPositionX,
                            float worldPositionZ, WorldSymmetryOrbitPoint* outPoints,
                            int maximumPoints) {
    // Same buffer ceiling every other BuildSymmetryOrbit caller uses (Placement_Accept_PROC.cpp) —
    // a caller-supplied maximumPoints larger than the policy ceiling cannot overrun this stack array.
    const int clampedMaximumPoints = maximumPoints < Params::symmetryOrbitMaximum
                                    ? maximumPoints : Params::symmetryOrbitMaximum;
    if (clampedMaximumPoints <= 0) return 0;

    const float cellReciprocal = 1.0f / geometry.worldUnitsPerCell;
    const float cellPositionX  = worldPositionX * cellReciprocal;
    const float cellPositionZ  = worldPositionZ * cellReciprocal;
    const float extent         = static_cast<float>(geometry.VertexSize() - 1);

    Proc::SymmetryOrbitPoint cellOrbit[Params::symmetryOrbitMaximum];
    const Proc::PlacementConstants defaultConstants{};
    const int orbitCount = Proc::BuildSymmetryOrbit(symmetryMask, radialSymmetryRepeatCount, extent,
                                                     cellPositionX, cellPositionZ,
                                                     defaultConstants.symmetryDuplicateEpsilon,
                                                     cellOrbit, clampedMaximumPoints);

    for (int index = 0; index < orbitCount; ++index) {
        outPoints[index].worldPositionX = cellOrbit[index].positionX * geometry.worldUnitsPerCell;
        outPoints[index].worldPositionZ = cellOrbit[index].positionY * geometry.worldUnitsPerCell;
    }
    return orbitCount;
}

void ApplyHalfTurnYaw(float sourceRotationX, float sourceRotationY, float sourceRotationZ,
                      float sourceRotationW, float& outRotationX, float& outRotationY,
                      float& outRotationZ, float& outRotationW) {
    // Hardcoded 180-degree yaw quaternion about world Y (sin(90deg)=1, cos(90deg)=0 exactly).
    constexpr float halfTurnYawX = 0.0f, halfTurnYawY = 1.0f, halfTurnYawZ = 0.0f, halfTurnYawW = 0.0f;
    // World-space (extrinsic) composition: the source rotation applies first (locally), then the
    // 180-degree yaw rotates the whole result about the world's vertical axis -- QuaternionMultiply
    // applies its SECOND argument first, so the yaw is `first` here.
    Proc::SampledInstanceTransform composed;
    Proc::QuaternionMultiply(halfTurnYawX, halfTurnYawY, halfTurnYawZ, halfTurnYawW,
                             sourceRotationX, sourceRotationY, sourceRotationZ, sourceRotationW,
                             composed);
    outRotationX = composed.rotationX;
    outRotationY = composed.rotationY;
    outRotationZ = composed.rotationZ;
    outRotationW = composed.rotationW;
}

} // namespace Pipeline
} // namespace SanmapGen
