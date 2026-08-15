// Thermal_PROC.cpp — stage lifecycle: the parameter hash PIPELINE registers, the once-per-run
// resolve of talus angles into height thresholds (shared verbatim by both backends), scratch
// sizing, and the dispatch hand-off. The per-backend work lives in Thermal_Relax_PROC.cpp and
// Thermal_Transport_PROC.cpp (Cpu) and Thermal_Gpu_PROC.cpp (Gpu).
#include "Thermal_PROC.h"
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

ThermalStage::ThermalStage(const Params::Geometry& geometrySettings, Data::MapFields& fields)
    : geometry(geometrySettings), mapFields(fields) {}

std::size_t ThermalStage::ComputeParameterHash() const {
    std::size_t hash = HashInteger(hashBasis, geometry.mapSize);
    hash = HashFloat(hash, geometry.terrainMaxHeight);
    hash = HashInteger(hash, constants.iterationCount);
    hash = HashInteger(hash, constants.bTransportMaterialProportions ? 1 : 0);
    hash = HashFloat(hash, constants.relaxationRate);
    hash = HashFloat(hash, constants.cellWorldSize);
    hash = HashFloat(hash, constants.minimumColumnDepth);
    hash = HashFloat(hash, constants.movementEpsilon);
    hash = HashFloat(hash, constants.proportionWeightEpsilon);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        hash = HashFloat(hash, constants.talusAngleDegrees[stratum]);
    return hash;
}

// Angles -> thresholds, the constant block both backends read, and the scratch fields the
// gather formulation needs. Cheap and idempotent; every run calls it so a tweak lands at once.
void ThermalStage::PrepareRun() {
    resolvedTalusThresholds.resize(Data::MapFields::stratumCount);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        resolvedTalusThresholds[stratum] = TalusThresholdFromAngle(
            constants.talusAngleDegrees[stratum], constants.cellWorldSize, geometry.terrainMaxHeight);

    float relaxationRate = constants.relaxationRate;
    if (relaxationRate < 0.0f) relaxationRate = 0.0f;
    if (relaxationRate > 1.0f) relaxationRate = 1.0f;

    kernelConstantBlock.assign(ThermalConstantSlot::totalCount, 0.0f);
    kernelConstantBlock[ThermalConstantSlot::spreadFactorActive] =
        relaxationRate * thermalInverseNeighbourCount;
    kernelConstantBlock[ThermalConstantSlot::movementEpsilon]    = constants.movementEpsilon;
    kernelConstantBlock[ThermalConstantSlot::minimumColumnDepth] = constants.minimumColumnDepth;
    kernelConstantBlock[ThermalConstantSlot::proportionWeightEpsilon]  = constants.proportionWeightEpsilon;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        kernelConstantBlock[ThermalConstantSlot::talusThresholdBase + stratum] =
            resolvedTalusThresholds[stratum];

    if (!mapFields.IsSized()) return;
    const int vertexSize = mapFields.VertexSize();
    if (cellSpreadFactor.Width() == vertexSize && cellSpreadFactor.Height() == vertexSize) return;
    cellSpreadFactor.Resize(vertexSize, vertexSize, 0.0f);
    cellTalusThreshold.Resize(vertexSize, vertexSize, 0.0f);
    heightScratch.Resize(vertexSize, vertexSize, 0.0f);
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        materialProportionScratch[stratum].Resize(vertexSize, vertexSize, 0.0f);
}

Sys::ComputeBackend ThermalStage::Run() {
    // RunOnCpu/RunOnGpu record the backend they actually completed on (RunOnGpu falls back to
    // the Cpu path when no GL program is available), so lastBackend stays honest.
    Sys::Dispatch(*this, dispatchPolicy, generationContext, globalBackend, Sys::DataResidency::Either);
    return lastBackend;
}

} // namespace Proc
} // namespace SanmapGen
