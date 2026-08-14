// Erosion_DropletTransfer_PROC.cpp — what a droplet does TO the column: steer, erode, deposit,
// settle. Layer: PROC (Cpu twin of the same-named functions in Erosion_Column_PROC.glsl).
// Every transfer is measured in fixed-point ticks and reports back what actually moved, so a
// droplet can never take material a thin column did not have — the mass-conservation property
// the acceptance test leans on (LAYER_SYSTEM_SPEC additive-thickness model).
#include "Erosion_Droplet_PROC.h"
#include <cmath>

namespace SanmapGen {
namespace Proc {
namespace {

// The four cell indices and bilinear weights of the cell the droplet is standing in.
struct SplatFootprint {
    int   cellIndices[4];
    float weights[4];
};

SplatFootprint BuildSplatFootprint(int vertexSize, int nodeX, int nodeY, float fractionX, float fractionY) {
    SplatFootprint footprint;
    footprint.weights[0] = (1.0f - fractionX) * (1.0f - fractionY);
    footprint.weights[1] = fractionX * (1.0f - fractionY);
    footprint.weights[2] = (1.0f - fractionX) * fractionY;
    footprint.weights[3] = fractionX * fractionY;
    footprint.cellIndices[0] = nodeY * vertexSize + nodeX;
    footprint.cellIndices[1] = footprint.cellIndices[0] + 1;
    footprint.cellIndices[2] = footprint.cellIndices[0] + vertexSize;
    footprint.cellIndices[3] = footprint.cellIndices[2] + 1;
    return footprint;
}

} // namespace

float DepositSplat(const DropletContext& context, int nodeX, int nodeY, float fractionX, float fractionY,
                   float amountHeight) {
    const ErosionKernelConfiguration& configuration = *context.configuration;
    const SplatFootprint footprint = BuildSplatFootprint(configuration.vertexSize, nodeX, nodeY, fractionX, fractionY);
    int addedTicks = 0;
    for (int corner = 0; corner < 4; ++corner)
        addedTicks += DepositColumn(context.thicknessFixedPoint, context.cellCount, footprint.cellIndices[corner],
                                    configuration.depositStratum,
                                    HeightToFixedPoint(amountHeight * footprint.weights[corner],
                                                       configuration.heightFixedPointScale));
    return FixedPointToHeight(addedTicks, configuration.heightFixedPointInverse);
}

float ErodeSplat(const DropletContext& context, int nodeX, int nodeY, float fractionX, float fractionY,
                 float amountHeight) {
    const ErosionKernelConfiguration& configuration = *context.configuration;
    const SplatFootprint footprint = BuildSplatFootprint(configuration.vertexSize, nodeX, nodeY, fractionX, fractionY);
    int removedTicks = 0;
    for (int corner = 0; corner < 4; ++corner)
        removedTicks += ErodeColumnClamped(context.thicknessFixedPoint, context.cellCount,
                                           footprint.cellIndices[corner], configuration.highestErodableStratum,
                                           context.materialPhysics,
                                           HeightToFixedPoint(amountHeight * footprint.weights[corner],
                                                              configuration.heightFixedPointScale));
    return FixedPointToHeight(removedTicks, configuration.heightFixedPointInverse);
}

// Blend the old direction with the (meandered) gradient. Meander/divergence is the term the
// old Gpu path lacked; it now runs on both backends off the shared integer hash stream.
void SteerDroplet(const ErosionKernelConfiguration& configuration, DropletState& state,
                  float gradientX, float gradientY, float friction, unsigned int stepSeed) {
    const float slopeLength = std::sqrt(gradientX * gradientX + gradientY * gradientY);
    const float divergence = configuration.divergenceFactor
                           * (1.0f - ClampUnit(slopeLength * configuration.divergenceThreshold));
    const float steerX = gradientX + (HashRandomUnitFloat(stepSeed) - 0.5f) * configuration.meanderStrength * divergence;
    const float steerY = gradientY + (HashRandomUnitFloat(stepSeed + 1u) - 0.5f) * configuration.meanderStrength * divergence;

    const float inertia = (configuration.inertiaBase + (1.0f - friction) * configuration.inertiaFrictionScale)
                        * configuration.viscosityReciprocal;
    state.directionX = state.directionX * inertia - steerX * (1.0f - inertia);
    state.directionY = state.directionY * inertia - steerY * (1.0f - inertia);
    const float length = std::sqrt(state.directionX * state.directionX + state.directionY * state.directionY);
    if (length != 0.0f) {
        const float lengthReciprocal = 1.0f / length;   // reciprocal multiply, never divide twice
        state.directionX *= lengthReciprocal;
        state.directionY *= lengthReciprocal;
    }
}

// Deposit when overloaded or climbing, otherwise carve.
void ExchangeSediment(const DropletContext& context, DropletState& state, float deltaHeight,
                      float capacity, float hardness, int nodeX, int nodeY, float fractionX, float fractionY) {
    const ErosionKernelConfiguration& configuration = *context.configuration;
    if (state.sediment > capacity || deltaHeight > 0.0f) {
        const float requested = deltaHeight > 0.0f
                              ? (deltaHeight < state.sediment ? deltaHeight : state.sediment)
                              : (state.sediment - capacity) * configuration.baseDepositionRate;
        if (requested > 0.0f)
            state.sediment -= DepositSplat(context, nodeX, nodeY, fractionX, fractionY, requested);
    } else if (configuration.bDepositionMode == 0) {
        const float wanted = (capacity - state.sediment) * configuration.baseErosionRate * (1.0f - hardness);
        const float requested = wanted < -deltaHeight ? wanted : -deltaHeight;
        if (requested > 0.0f)
            state.sediment += ErodeSplat(context, nodeX, nodeY, fractionX, fractionY, requested);
    }
}

// Dump whatever the droplet still carries at its last valid cell, so eroded volume returns to
// the map instead of vanishing (the conservation sanity the acceptance test checks).
void SettleDroplet(const DropletContext& context, float sediment, int nodeX, int nodeY) {
    const ErosionKernelConfiguration& configuration = *context.configuration;
    if (configuration.bConserveSedimentAtExit == 0 || sediment <= configuration.sedimentMinimum) return;
    const int cellIndex = nodeY * configuration.vertexSize + nodeX;
    if (cellIndex < 0 || cellIndex >= context.cellCount) return;
    DepositColumn(context.thicknessFixedPoint, context.cellCount, cellIndex, configuration.depositStratum,
                  HeightToFixedPoint(sediment, configuration.heightFixedPointScale));
}

} // namespace Proc
} // namespace SanmapGen
