// SymmetryOrbitQuery_PIPELINE.cpp — see SymmetryOrbitQuery_PIPELINE.h for the full rationale.
#include "SymmetryOrbitQuery_PIPELINE.h"
#include "../params/Symmetry_PARAMS.h"
#include "../proc/Placement_Symmetry_PROC.h"
#include "../proc/Placement_Kernel_PROC.h"

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

} // namespace Pipeline
} // namespace SanmapGen
