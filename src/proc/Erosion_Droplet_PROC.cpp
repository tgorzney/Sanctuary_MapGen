// Erosion_Droplet_PROC.cpp — one droplet's life, and the Cpu pass that runs them all.
// Layer: PROC. This is the accuracy path (ARCH §4.2 Erosion → Output = Cpu/Exact): droplets
// run in spawn order on the shared fixed-point stack, so every drop sees the deposits of the
// drops before it — the full feedback the Gpu approximates away. Sequential order + integer
// state means the run reproduces itself bit-for-bit with no reduction-order or thread-count
// caveat (DETERMINISM_SPEC). AdvanceDroplet() is the twin of advanceDroplet() in
// Erosion_PROC.glsl, split the same way so the two stay comparable at a glance.
#include "Erosion_PROC.h"
#include "Erosion_Droplet_PROC.h"
#include <cmath>

namespace SanmapGen {
namespace Proc {
namespace {

// Sediment the water may carry here; deposition mode holds on to its load while sliding down.
float DropletCapacity(const ErosionKernelConfiguration& configuration, const DropletState& state,
                      float deltaHeight, float materialCapacityMultiplier) {
    float capacity = -deltaHeight * state.speed * state.water * configuration.capacityBaseMultiplier
                   * materialCapacityMultiplier * configuration.carryingCapacityScale;
    if (capacity < configuration.capacityMinimum) capacity = configuration.capacityMinimum;
    if (configuration.bDepositionMode != 0) {
        const float held = state.sediment * ClampUnit(-deltaHeight * configuration.depositionModeCapacityGain);
        if (held > capacity) capacity = held;
    }
    return capacity;
}

// One droplet step; false means the droplet died (its load is already settled).
bool AdvanceDroplet(const DropletContext& context, DropletState& state, int step, unsigned int dropletSeed) {
    const ErosionKernelConfiguration& configuration = *context.configuration;
    const int nodeX = static_cast<int>(state.positionX);
    const int nodeY = static_cast<int>(state.positionY);
    const float fractionX = state.positionX - static_cast<float>(nodeX);
    const float fractionY = state.positionY - static_cast<float>(nodeY);
    const ColumnHeightSample sampled =
        SampleColumnHeight(context.thicknessFixedPoint, configuration.stratumCount, configuration.vertexSize,
                           configuration.heightFixedPointInverse, state.positionX, state.positionY);
    const int topStratum = FindTopMaterialStratum(context.thicknessFixedPoint, context.cellCount,
                                                  nodeY * configuration.vertexSize + nodeX,
                                                  configuration.highestErodableStratum,
                                                  HeightToFixedPoint(configuration.thicknessEpsilon,
                                                                     configuration.heightFixedPointScale));
    const float* physics = context.materialPhysics
                         + (topStratum >= 0 ? topStratum : configuration.depositStratum) * materialPhysicsStride;

    SteerDroplet(configuration, state, sampled.gradientX, sampled.gradientY, physics[materialPhysicsFrictionOffset],
                 HashRandomCombine(dropletSeed, static_cast<unsigned int>(step)));
    state.positionX += state.directionX;
    state.positionY += state.directionY;
    const float highestCoordinate = static_cast<float>(configuration.vertexSize - 2);
    if ((state.directionX == 0.0f && state.directionY == 0.0f)
        || state.positionX < configuration.boundaryMargin || state.positionX >= highestCoordinate
        || state.positionY < configuration.boundaryMargin || state.positionY >= highestCoordinate) {
        SettleDroplet(context, state.sediment, nodeX, nodeY);
        return false;
    }

    const ColumnHeightSample moved =
        SampleColumnHeight(context.thicknessFixedPoint, configuration.stratumCount, configuration.vertexSize,
                           configuration.heightFixedPointInverse, state.positionX, state.positionY);
    const float deltaHeight = moved.height - sampled.height;
    ExchangeSediment(context, state, deltaHeight,
                     DropletCapacity(configuration, state, deltaHeight, physics[materialPhysicsCapacityOffset]),
                     physics[materialPhysicsHardnessOffset], nodeX, nodeY, fractionX, fractionY);

    const float speedSquared = state.speed * state.speed + deltaHeight * configuration.gravity;
    state.speed = std::sqrt(speedSquared > 0.0f ? speedSquared : 0.0f);
    state.water *= (1.0f - configuration.evaporationRate) * (1.0f - physics[materialPhysicsAbsorptionOffset]);
    if (state.water <= configuration.waterMinimum) {
        SettleDroplet(context, state.sediment, nodeX, nodeY);
        return false;
    }
    return true;
}

} // namespace

void TraceSingleDroplet(const DropletContext& context, int dropletIndex) {
    const ErosionKernelConfiguration& configuration = *context.configuration;
    const unsigned int dropletSeed = HashRandomCombine(static_cast<unsigned int>(configuration.randomSeed),
                                                      static_cast<unsigned int>(dropletIndex));
    DropletState state;
    state.positionX = context.dropletSpawns[dropletIndex * 2];
    state.positionY = context.dropletSpawns[dropletIndex * 2 + 1];
    state.sediment  = configuration.initialSedimentLoad;

    for (int step = 0; step < configuration.maximumLifetime; ++step)
        if (!AdvanceDroplet(context, state, step, dropletSeed)) return;
    SettleDroplet(context, state.sediment, static_cast<int>(state.positionX), static_cast<int>(state.positionY));
}

void ErosionStage::TraceDropletsCpu(const ErosionKernelConfiguration& configuration) {
    DropletContext context;
    context.thicknessFixedPoint = thicknessFixedPoint.data();
    context.materialPhysics     = materialPhysicsBuffer.data();
    context.configuration       = &configuration;
    context.dropletSpawns       = dropletSpawns.data();
    context.cellCount           = cellCount;
    const int dropletCount = static_cast<int>(dropletSpawns.size() / 2);
    for (int dropletIndex = 0; dropletIndex < dropletCount; ++dropletIndex)
        TraceSingleDroplet(context, dropletIndex);
}

} // namespace Proc
} // namespace SanmapGen
