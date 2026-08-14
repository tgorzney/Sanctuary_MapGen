// FlowAccumulation_PROC.cpp — stage lifecycle: the parameter hash, the buffers both backends
// share, and the dispatch hand-off. The per-step work lives in FlowAccumulation_Fill_PROC.cpp
// (priority-flood), FlowAccumulation_Direction_PROC.cpp (routing), FlowAccumulation_
// Accumulate_PROC.cpp (the ordered DAG sweep), and the GPU pair FlowAccumulation_Gpu_PROC.cpp
// / FlowAccumulation_GpuRelax_PROC.cpp.
#include "FlowAccumulation_PROC.h"
#include <cstring>

namespace SanmapGen {
namespace Proc {
namespace {

constexpr std::size_t hashBasis = 1469598103934665603ull;   // FNV offset basis

inline std::size_t HashMix(std::size_t seed, std::size_t value) {
    return seed ^ (value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2));
}
inline std::size_t HashInteger(std::size_t seed, int value) {
    return HashMix(seed, static_cast<std::size_t>(static_cast<unsigned int>(value)));
}
inline std::size_t HashFloat(std::size_t seed, float value) {
    unsigned int bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return HashMix(seed, static_cast<std::size_t>(bits));
}

} // namespace

FlowAccumulationStage::FlowAccumulationStage(const Params::Geometry& geometrySettings,
                                             Data::MapFields& outputFields)
    : geometry(geometrySettings), mapFields(outputFields) {
    // ARCH §4.2: flow/accumulation shapes river placement and pathing, so the Output pass is
    // the Exact class on the Cpu; the preview keeps the Visual Gpu speed path.
    dispatchPolicy.outputBackend  = Sys::ComputeBackend::Cpu;
    dispatchPolicy.outputAccuracy = Sys::AccuracyClass::Exact;
    dispatchPolicy.previewBackend  = Sys::ComputeBackend::Gpu;
    dispatchPolicy.previewAccuracy = Sys::AccuracyClass::Visual;
}

std::size_t FlowAccumulationStage::ComputeParameterHash() const {
    std::size_t hash = HashInteger(hashBasis, geometry.mapSize);
    hash = HashInteger(hash, static_cast<int>(geometry.seed));
    hash = HashFloat(hash, constants.cellWeight);
    hash = HashFloat(hash, constants.flowNoiseImpact);
    hash = HashFloat(hash, constants.depressionFillEpsilon);
    hash = HashFloat(hash, constants.cardinalInverseDistance);
    hash = HashFloat(hash, constants.diagonalInverseDistance);
    hash = HashFloat(hash, constants.flowMagnitudeScale);
    hash = HashInteger(hash, static_cast<int>(constants.flowNoiseSeed));
    hash = HashInteger(hash, constants.gpuFillIterationLimit);
    hash = HashInteger(hash, constants.gpuAccumulationIterationLimit);
    hash = HashInteger(hash, constants.gpuConvergenceCheckInterval);
    hash = HashInteger(hash, constants.fillIterationsPerSide);
    hash = HashInteger(hash, constants.accumulationIterationsPerSide);
    hash = HashInteger(hash, constants.bFillDepressions ? 1 : 0);
    return HashInteger(hash, constants.bNormalizeAccumulation ? 1 : 0);
}

Sys::ComputeBackend FlowAccumulationStage::Run() {
    lastBackend = Sys::Dispatch(*this, dispatchPolicy, generationContext, globalBackend,
                                Sys::DataResidency::Either);
    return lastBackend;
}

// The heightfield upstream produced is the sizing authority; the two output fields follow it.
void FlowAccumulationStage::PrepareRun() {
    vertexSize = mapFields.heightfield.Width();
    if (vertexSize <= 0) {
        vertexSize = geometry.VertexSize();
        mapFields.Resize(vertexSize);
    }
    const std::size_t cellCount = static_cast<std::size_t>(vertexSize) * vertexSize;
    if (mapFields.flow.CellCount() != cellCount) mapFields.flow.Resize(vertexSize, vertexSize, 0.0f);
    if (mapFields.accumulation.CellCount() != cellCount)
        mapFields.accumulation.Resize(vertexSize, vertexSize, 0.0f);
    drainageSurface.assign(cellCount, 0.0f);
    flowDirections.assign(cellCount, flowSinkDirection);
    drainageOrder.clear();
    sinkCount = 0;
    bGpuFallbackUsed = false;
}

// The accuracy path: exact depression resolution, exact DAG order, no iteration budget.
void FlowAccumulationStage::RunOnCpu() {
    PrepareRun();
    if (vertexSize <= 0) return;
    BuildDrainageSurfaceCpu();
    BuildFlowDirectionsCpu();
    AccumulateDrainageCpu();
    CountSinks();
    NormalizeAccumulation();
}

int FlowAccumulationStage::ResolvedFillIterationLimit() const {
    if (constants.gpuFillIterationLimit > 0) return constants.gpuFillIterationLimit;
    return constants.fillIterationsPerSide * vertexSize;
}

int FlowAccumulationStage::ResolvedAccumulationIterationLimit() const {
    if (constants.gpuAccumulationIterationLimit > 0) return constants.gpuAccumulationIterationLimit;
    return constants.accumulationIterationsPerSide * vertexSize;
}

} // namespace Proc
} // namespace SanmapGen
