// Thermal_Relax_PROC.cpp — the Cpu accuracy path's driver and its spread-factor pass.
// Twin of the THERMAL_PASS_PREPARE branch in Thermal_PROC.glsl.
// The pass answers two questions per cell, both from read-only state: what talus threshold does
// this cell's material mix impose, and is the cell unstable (any drop past that threshold)?
// Publishing that per cell is what lets the apply pass GATHER instead of scatter — so the
// relaxation is order-independent, race-free, and identical on both backends (the legacy
// in-place scatter was neither).
#include "Thermal_PROC.h"
#include "../sys/ThreadPool_SYS.h"

namespace SanmapGen {
namespace Proc {

// Talus threshold for one cell: its material masks weight the per-stratum thresholds. With no
// mask coverage at all the cell falls back to stratum 0 (MASKING_SPEC's bottom-layer fallback).
static float BlendCellTalusThreshold(const Data::MapFields& fields, const std::vector<float>& thresholds,
                                     int x, int y, float maskWeightEpsilon) {
    float weightSum = 0.0f;
    float weightedThreshold = 0.0f;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        const float weight = fields.materialMasks[stratum].Get(x, y);
        weightSum += weight;
        weightedThreshold += weight * thresholds[stratum];
    }
    if (weightSum > maskWeightEpsilon) return weightedThreshold * Math::Reciprocal(weightSum);
    return thresholds[0];
}

void ThermalStage::PrepareIterationCpu() {
    const int vertexSize = mapFields.VertexSize();
    const float spreadFactorActive = kernelConstantBlock[ThermalConstantSlot::spreadFactorActive];
    const float movementEpsilon    = kernelConstantBlock[ThermalConstantSlot::movementEpsilon];
    const float maskWeightEpsilon  = kernelConstantBlock[ThermalConstantSlot::maskWeightEpsilon];

    const auto prepareRow = [&](int y) {
        for (int x = 0; x < vertexSize; ++x) {
            const float threshold = BlendCellTalusThreshold(mapFields, resolvedTalusThresholds,
                                                            x, y, maskWeightEpsilon);
            cellTalusThreshold.Set(x, y, threshold);
            const float height = mapFields.heightfield.Get(x, y);
            float totalExcess = 0.0f;
            for (int step = 0; step < thermalNeighbourCount; ++step) {
                const int neighbourX = x + thermalNeighbourOffsetX[step];
                const int neighbourY = y + thermalNeighbourOffsetY[step];
                if (neighbourX < 0 || neighbourX >= vertexSize) continue;
                if (neighbourY < 0 || neighbourY >= vertexSize) continue;
                totalExcess += ExcessDrop(height, mapFields.heightfield.Get(neighbourX, neighbourY), threshold);
            }
            cellSpreadFactor.Set(x, y, totalExcess > movementEpsilon ? spreadFactorActive : 0.0f);
        }
    };
    if (threadPool != nullptr) threadPool->ParallelFor(0, vertexSize, prepareRow);
    else for (int y = 0; y < vertexSize; ++y) prepareRow(y);
}

// The Cpu accuracy path (ARCH §4.2 Output default for Thermal). Every sweep is prepare ->
// apply -> commit; the commit is what makes the sweep a Jacobi step rather than a Gauss-Seidel
// one, which is exactly what the Gpu twin does.
void ThermalStage::RunOnCpu() {
    PrepareRun();
    completedIterationCount = 0;
    lastBackend = Sys::ComputeBackend::Cpu;
    if (!mapFields.IsSized()) return;
    for (int iteration = 0; iteration < constants.iterationCount; ++iteration) {
        PrepareIterationCpu();
        ApplyIterationCpu();
        CommitIterationCpu();
        ++completedIterationCount;
    }
}

} // namespace Proc
} // namespace SanmapGen
