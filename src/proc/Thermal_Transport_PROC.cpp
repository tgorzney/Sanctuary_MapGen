// Thermal_Transport_PROC.cpp — the Cpu apply pass: the gather that actually moves material,
// plus the material that travels with it. Twin of the THERMAL_PASS_APPLY branch in
// Thermal_PROC.glsl.
// Gather, never scatter: a cell subtracts what IT owes its neighbours and adds what EACH
// neighbour owes it, both from the same published spread factor / threshold — so donor and
// receiver compute the identical number, mass is conserved exactly, and no cell is ever written
// twice (the legacy scatter admitted races on the Gpu and was order-dependent on the Cpu).
#include "Thermal_PROC.h"
#include "../sys/ThreadPool_SYS.h"
#include <cstring>

namespace SanmapGen {
namespace Proc {
namespace {

struct NeighbourInflow { int x; int y; float amount; };

struct RelaxContext {
    const Data::FloatField& heightfield;
    const Data::FloatField& spreadFactor;
    const Data::FloatField& talusThreshold;
    int vertexSize;
};

// Returns the cell's relaxed height and records what each neighbour contributed to it.
float RelaxCellHeight(const RelaxContext& context, int x, int y,
                      NeighbourInflow* inflows, int& inflowCount, float& totalInflow) {
    const float height = context.heightfield.Get(x, y);
    const float threshold = context.talusThreshold.Get(x, y);
    const float spreadFactor = context.spreadFactor.Get(x, y);
    float outflow = 0.0f;
    totalInflow = 0.0f;
    inflowCount = 0;
    for (int step = 0; step < thermalNeighbourCount; ++step) {
        const int neighbourX = x + thermalNeighbourOffsetX[step];
        const int neighbourY = y + thermalNeighbourOffsetY[step];
        if (neighbourX < 0 || neighbourX >= context.vertexSize) continue;
        if (neighbourY < 0 || neighbourY >= context.vertexSize) continue;
        const float neighbourHeight = context.heightfield.Get(neighbourX, neighbourY);
        outflow += spreadFactor * ExcessDrop(height, neighbourHeight, threshold);
        const float received = context.spreadFactor.Get(neighbourX, neighbourY)
                             * ExcessDrop(neighbourHeight, height,
                                          context.talusThreshold.Get(neighbourX, neighbourY));
        totalInflow += received;
        inflows[inflowCount].x = neighbourX;
        inflows[inflowCount].y = neighbourY;
        inflows[inflowCount].amount = received;
        ++inflowCount;
    }
    return height - outflow + totalInflow;
}

// Material that slides carries its donor's mix. The receiving cell is treated as a homogeneous
// column of depth `columnDepth`, so the arriving volume remixes it by volume weight.
void MixCellMaterial(const Data::MapFields& fields, Data::FloatField* scratch,
                     const NeighbourInflow* inflows, int inflowCount,
                     int x, int y, float columnDepth, float totalInflow) {
    const float inverseTotal = Math::Reciprocal(columnDepth + totalInflow);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        float donorProportion = 0.0f;   // donors first, in neighbour order, exactly as the shader does
        for (int entry = 0; entry < inflowCount; ++entry)
            donorProportion += inflows[entry].amount
                       * fields.materialProportions[stratum].Get(inflows[entry].x, inflows[entry].y);
        scratch[stratum].Set(x, y,
            (fields.materialProportions[stratum].Get(x, y) * columnDepth + donorProportion) * inverseTotal);
    }
}

} // namespace

void ThermalStage::ApplyIterationCpu() {
    const int vertexSize = mapFields.VertexSize();
    const RelaxContext context{ mapFields.heightfield, cellSpreadFactor, cellTalusThreshold, vertexSize };
    const float movementEpsilon    = kernelConstantBlock[ThermalConstantSlot::movementEpsilon];
    const float minimumColumnDepth = kernelConstantBlock[ThermalConstantSlot::minimumColumnDepth];
    const bool bTransport = constants.bTransportMaterialProportions;

    const auto applyRow = [&](int y) {
        NeighbourInflow inflows[thermalNeighbourCount];
        for (int x = 0; x < vertexSize; ++x) {
            int inflowCount = 0;
            float totalInflow = 0.0f;
            const float height = mapFields.heightfield.Get(x, y);
            heightScratch.Set(x, y, RelaxCellHeight(context, x, y, inflows, inflowCount, totalInflow));
            if (!bTransport) continue;
            if (totalInflow <= movementEpsilon) {
                for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
                    materialProportionScratch[stratum].Set(x, y, mapFields.materialProportions[stratum].Get(x, y));
                continue;
            }
            const float columnDepth = height > minimumColumnDepth ? height : minimumColumnDepth;
            MixCellMaterial(mapFields, materialProportionScratch, inflows, inflowCount,
                            x, y, columnDepth, totalInflow);
        }
    };
    if (threadPool != nullptr) threadPool->ParallelFor(0, vertexSize, applyRow);
    else for (int y = 0; y < vertexSize; ++y) applyRow(y);
}

// Scratch -> live fields. Doing this between sweeps is what makes each sweep a Jacobi step,
// matching the Gpu's ping-pong buffers exactly.
void ThermalStage::CommitIterationCpu() {
    const std::size_t byteCount = mapFields.heightfield.CellCount() * sizeof(float);
    std::memcpy(mapFields.heightfield.Data(), heightScratch.Data(), byteCount);
    if (!constants.bTransportMaterialProportions) return;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        std::memcpy(mapFields.materialProportions[stratum].Data(), materialProportionScratch[stratum].Data(), byteCount);
}

} // namespace Proc
} // namespace SanmapGen
