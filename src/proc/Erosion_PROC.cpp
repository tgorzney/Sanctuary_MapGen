// Erosion_PROC.cpp — stage lifecycle: the parameter hash, the flattened configuration both
// backends consume, and the dispatch hand-off. The per-backend work lives in
// Erosion_Droplet_PROC.cpp (Cpu trace), Erosion_Gpu_PROC.cpp (Gpu speed path),
// Erosion_Rain_PROC.cpp (rain + spawns), Erosion_Field_PROC.cpp (DATA round-trip) and
// Erosion_Accumulation_PROC.cpp (the Cpu-only ordered spillover DAG).
#include "Erosion_PROC.h"
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
// Every settings field is hashed as raw words: no field can change without dirtying the
// stage, and adding a field cannot silently escape the hash.
std::size_t HashRawRecord(std::size_t seed, const void* record, std::size_t byteSize) {
    const unsigned char* bytes = static_cast<const unsigned char*>(record);
    for (std::size_t index = 0; index < byteSize; ++index)
        seed = HashMix(seed, static_cast<std::size_t>(bytes[index]));
    return seed;
}

} // namespace

ErosionStage::ErosionStage(const Params::Geometry& geometrySettings, Data::MapFields& fields)
    : geometry(geometrySettings), mapFields(fields) {}

std::size_t ErosionStage::ComputeParameterHash() const {
    std::size_t hash = HashInteger(hashBasis, geometry.mapSize);
    hash = HashInteger(hash, static_cast<int>(geometry.seed));
    hash = HashFloat(hash, constants.heightFixedPointScale);
    hash = HashFloat(hash, constants.boundaryMargin);
    hash = HashInteger(hash, constants.spawnRejectionSafetyLimit);
    for (int stratum = 0; stratum < stratumCount; ++stratum) {
        hash = HashRawRecord(hash, &materials[stratum], sizeof(MaterialPhysics));
        if (!layerSettings[stratum].bEnabled) { hash = HashInteger(hash, stratum); continue; }
        hash = HashRawRecord(hash, &layerSettings[stratum], sizeof(ErosionLayerSettings));
    }
    return hash;
}

Sys::ComputeBackend ErosionStage::Run() {
    lastBackend = Sys::Dispatch(*this, dispatchPolicy, generationContext, globalBackend,
                                Sys::DataResidency::Either);
    return lastBackend;
}

void ErosionStage::BuildMaterialPhysicsBuffer() {
    materialPhysicsBuffer.assign(static_cast<std::size_t>(stratumCount) * materialPhysicsStride, 0.0f);
    for (int stratum = 0; stratum < stratumCount; ++stratum) {
        float* record = materialPhysicsBuffer.data() + stratum * materialPhysicsStride;
        record[materialPhysicsHardnessOffset]   = materials[stratum].hardness;
        record[materialPhysicsFrictionOffset]   = materials[stratum].friction;
        record[materialPhysicsCohesionOffset]   = materials[stratum].cohesion;
        record[materialPhysicsCapacityOffset]   = materials[stratum].capacityMultiplier;
        record[materialPhysicsAbsorptionOffset] = materials[stratum].absorptionRate;
        record[materialPhysicsErodableOffset]   = materials[stratum].bErodable ? 1.0f : 0.0f;
    }
}

void ErosionStage::PrepareRun() {
    vertexSize = mapFields.IsSized() ? mapFields.VertexSize() : geometry.VertexSize();
    cellCount = vertexSize * vertexSize;
    thicknessFixedPoint.assign(static_cast<std::size_t>(stratumCount) * cellCount, 0);
    rainMap.assign(static_cast<std::size_t>(cellCount), 1.0f);
    BuildMaterialPhysicsBuffer();
    processedLayerCount = 0;
    lastDropletCount = 0;
    ReadThicknessFromFields();
}

ErosionKernelConfiguration ErosionStage::BuildConfiguration(int stratumIndex) const {
    const ErosionLayerSettings& settings = layerSettings[stratumIndex];
    ErosionKernelConfiguration configuration;
    configuration.dropletCount           = settings.dropletCount < 0 ? 0 : settings.dropletCount;
    configuration.maximumLifetime        = settings.maximumLifetime;
    configuration.vertexSize             = vertexSize;
    configuration.stratumCount           = stratumCount;
    configuration.depositStratum         = stratumIndex;
    configuration.highestErodableStratum = settings.bErodeBeneath ? stratumCount - 1 : stratumIndex;
    configuration.bDepositionMode        = settings.bDepositionMode ? 1 : 0;
    configuration.randomSeed             = static_cast<int>(geometry.seed) + constants.meanderSeedOffset
                                         + stratumIndex;
    configuration.heightFixedPointScale   = constants.heightFixedPointScale;
    configuration.heightFixedPointInverse = constants.HeightFixedPointInverse();
    configuration.baseErosionRate         = settings.baseErosionRate;
    configuration.baseDepositionRate      = settings.baseDepositionRate;
    configuration.evaporationRate         = settings.evaporationRate;
    configuration.gravity                 = settings.gravity;
    configuration.capacityBaseMultiplier  = settings.capacityBaseMultiplier;
    configuration.carryingCapacityScale   = settings.carryingCapacityScale;
    configuration.capacityMinimum         = settings.capacityMinimum;
    configuration.inertiaBase             = settings.inertiaBase;
    configuration.inertiaFrictionScale    = settings.inertiaFrictionScale;
    const float viscosity = settings.fluidViscosity > settings.minimumViscosity
                          ? settings.fluidViscosity : settings.minimumViscosity;
    configuration.viscosityReciprocal     = 1.0f / viscosity;
    configuration.meanderStrength         = settings.meanderStrength;
    configuration.divergenceFactor        = 1.0f - settings.slopeAdherence;
    configuration.divergenceThreshold     = settings.divergenceThreshold;
    configuration.waterMinimum            = settings.waterMinimum;
    configuration.sedimentMinimum         = settings.sedimentMinimum;
    configuration.thicknessEpsilon        = settings.thicknessEpsilon;
    configuration.initialSedimentLoad     = settings.bDepositionMode ? settings.initialSedimentLoad : 0.0f;
    configuration.boundaryMargin          = constants.boundaryMargin;
    configuration.depositionModeCapacityGain = settings.depositionModeCapacityGain;
    return configuration;
}

long long ErosionStage::TotalVolumeFixedPoint() const {
    long long total = 0;
    for (int ticks : thicknessFixedPoint) total += ticks;
    return total;
}

void ErosionStage::RunOnCpu() {
    PrepareRun();
    for (int stratum = 0; stratum < stratumCount; ++stratum) {
        if (!layerSettings[stratum].bEnabled) continue;
        BuildRainMap(stratum);
        BuildDropletSpawns(stratum);
        TraceDropletsCpu(BuildConfiguration(stratum));
        if (layerSettings[stratum].bAccurateSimultaneousAccumulation) ApplyAccumulationDagCpu(stratum);
        ++processedLayerCount;
        lastDropletCount += static_cast<int>(dropletSpawns.size() / 2);
    }
    WriteThicknessToFields();
}

} // namespace Proc
} // namespace SanmapGen
