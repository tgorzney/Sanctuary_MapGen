// Erosion_Column_PROC.h — the per-column operations the droplet trace is built from.
// Layer: PROC (CPU side; Erosion_Column_PROC.glsl is the GPU twin, same expressions).
// The stack is stratum-major fixed-point ticks: thicknessFixedPoint[stratum * cellCount + cell].
// Every mutation is clamped so a thickness can never go negative and so a droplet can only
// pick up what the column actually held — that is what makes the volume book balance
// (LAYER_SYSTEM_SPEC additive-thickness model).
#pragma once
#include "Erosion_Kernel_PROC.h"

namespace SanmapGen {
namespace Proc {

struct ColumnHeightSample {
    float height    = 0.0f;
    float gradientX = 0.0f;
    float gradientY = 0.0f;
};

inline int ClampInteger(int value, int lowest, int highest) {
    return value < lowest ? lowest : (value > highest ? highest : value);
}

// Total column height in ticks at one integer cell (all strata, base included).
inline int ColumnTotalFixedPoint(const int* thicknessFixedPoint, int stratumCount, int cellCount,
                                 int cellIndex) {
    int total = 0;
    for (int stratum = 0; stratum < stratumCount; ++stratum)
        total += thicknessFixedPoint[stratum * cellCount + cellIndex];
    return total;
}

// Bilinear height plus the bilinear gradient, in height units. Sample position is clamped
// to the grid exactly as the legacy CalculateGradient did, so droplet paths match.
inline ColumnHeightSample SampleColumnHeight(const int* thicknessFixedPoint, int stratumCount,
                                             int vertexSize, float fixedPointInverse,
                                             float sampleX, float sampleY) {
    const int cellCount = vertexSize * vertexSize;
    const int coordinateX = static_cast<int>(sampleX);
    const int coordinateY = static_cast<int>(sampleY);
    const float fractionX = sampleX - static_cast<float>(coordinateX);
    const float fractionY = sampleY - static_cast<float>(coordinateY);
    const int lowX  = ClampInteger(coordinateX, 0, vertexSize - 1);
    const int lowY  = ClampInteger(coordinateY, 0, vertexSize - 1);
    const int highX = ClampInteger(coordinateX + 1, 0, vertexSize - 1);
    const int highY = ClampInteger(coordinateY + 1, 0, vertexSize - 1);

    const float heightLowLow   = FixedPointToHeight(ColumnTotalFixedPoint(thicknessFixedPoint, stratumCount, cellCount, lowY * vertexSize + lowX),  fixedPointInverse);
    const float heightHighLow  = FixedPointToHeight(ColumnTotalFixedPoint(thicknessFixedPoint, stratumCount, cellCount, lowY * vertexSize + highX), fixedPointInverse);
    const float heightLowHigh  = FixedPointToHeight(ColumnTotalFixedPoint(thicknessFixedPoint, stratumCount, cellCount, highY * vertexSize + lowX), fixedPointInverse);
    const float heightHighHigh = FixedPointToHeight(ColumnTotalFixedPoint(thicknessFixedPoint, stratumCount, cellCount, highY * vertexSize + highX), fixedPointInverse);

    ColumnHeightSample sample;
    sample.gradientX = (heightHighLow - heightLowLow) * (1.0f - fractionY) + (heightHighHigh - heightLowHigh) * fractionY;
    sample.gradientY = (heightLowHigh - heightLowLow) * (1.0f - fractionX) + (heightHighHigh - heightHighLow) * fractionX;
    sample.height = heightLowLow * (1.0f - fractionX) * (1.0f - fractionY) + heightHighLow * fractionX * (1.0f - fractionY)
                  + heightLowHigh * (1.0f - fractionX) * fractionY + heightHighHigh * fractionX * fractionY;
    return sample;
}

// Topmost stratum with material at this cell, scanning down from `highestStratum`.
// Returns -1 when the column is empty down to stratum 0.
inline int FindTopMaterialStratum(const int* thicknessFixedPoint, int cellCount, int cellIndex,
                                  int highestStratum, int thicknessEpsilonTicks) {
    for (int stratum = highestStratum; stratum >= 0; --stratum)
        if (thicknessFixedPoint[stratum * cellCount + cellIndex] > thicknessEpsilonTicks) return stratum;
    return -1;
}

// Carve `requestedTicks` out of the column top-down, skipping non-erodable strata and never
// taking more than a stratum holds. Returns the ticks ACTUALLY removed — the caller adds
// exactly that to the droplet's sediment, so nothing is created out of nothing.
inline int ErodeColumnClamped(int* thicknessFixedPoint, int cellCount, int cellIndex,
                              int highestStratum, const float* materialPhysics, int requestedTicks) {
    int remaining = requestedTicks;
    int removed = 0;
    for (int stratum = highestStratum; stratum >= 0 && remaining > 0; --stratum) {
        if (materialPhysics[stratum * materialPhysicsStride + materialPhysicsErodableOffset] <= 0.0f) continue;
        int& thickness = thicknessFixedPoint[stratum * cellCount + cellIndex];
        const int taken = thickness < remaining ? thickness : remaining;
        if (taken <= 0) continue;
        thickness -= taken;
        remaining -= taken;
        removed += taken;
    }
    return removed;
}

// Add sediment to one stratum of one column. Returns the ticks actually added.
inline int DepositColumn(int* thicknessFixedPoint, int cellCount, int cellIndex, int stratum,
                         int amountTicks) {
    if (amountTicks <= 0) return 0;
    thicknessFixedPoint[stratum * cellCount + cellIndex] += amountTicks;
    return amountTicks;
}

} // namespace Proc
} // namespace SanmapGen
