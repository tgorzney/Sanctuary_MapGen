// Erosion_Field_PROC.cpp — the DATA round-trip: MapFields <-> the fixed-point thickness stack.
// Layer: PROC. LAYER_SYSTEM_SPEC simulates in THICKNESS, not in height: the authored stack
// arrives as a heightfield plus per-stratum material proportions, split into per-material
// thickness columns (thickness_s = height * weight_s, so the column sums back to the height
// exactly), eroded mass-conservingly, then read back as height + refreshed material proportions.
// Converting on the way in/out is what lets the sim move material instead of just height.
#include "Erosion_PROC.h"

namespace SanmapGen {
namespace Proc {

void ErosionStage::ReadThicknessFromFields() {
    if (!mapFields.IsSized()) return;
    const float fixedPointScale = constants.heightFixedPointScale;
    for (int y = 0; y < vertexSize; ++y) {
        for (int x = 0; x < vertexSize; ++x) {
            const int cellIndex = y * vertexSize + x;
            const float height = mapFields.heightfield.Get(x, y);
            float weightSum = 0.0f;
            for (int stratum = 0; stratum < stratumCount; ++stratum)
                weightSum += mapFields.materialProportions[stratum].Get(x, y);

            if (height <= 0.0f) continue;
            if (weightSum <= 0.0f) {   // nothing authored yet: the column is all base
                thicknessFixedPoint[cellIndex] = HeightToFixedPoint(height, fixedPointScale);
                continue;
            }
            // Split by weight, then hand the rounding remainder to the base stratum so the
            // column total is bit-exactly the input height in ticks (no volume invented).
            const int totalTicks = HeightToFixedPoint(height, fixedPointScale);
            const float weightReciprocal = 1.0f / weightSum;
            int assignedTicks = 0;
            for (int stratum = stratumCount - 1; stratum >= 1; --stratum) {
                const float share = mapFields.materialProportions[stratum].Get(x, y) * weightReciprocal;
                const int ticks = HeightToFixedPoint(height * share, fixedPointScale);
                const int clamped = ticks > totalTicks - assignedTicks ? totalTicks - assignedTicks : ticks;
                thicknessFixedPoint[stratum * cellCount + cellIndex] = clamped < 0 ? 0 : clamped;
                assignedTicks += clamped < 0 ? 0 : clamped;
            }
            thicknessFixedPoint[cellIndex] = totalTicks - assignedTicks;
        }
    }
}

void ErosionStage::WriteThicknessToFields() {
    if (!mapFields.IsSized()) return;
    const float fixedPointInverse = constants.HeightFixedPointInverse();
    for (int y = 0; y < vertexSize; ++y) {
        for (int x = 0; x < vertexSize; ++x) {
            const int cellIndex = y * vertexSize + x;
            const int totalTicks = ColumnTotalFixedPointAt(cellIndex);
            const float height = FixedPointToHeight(totalTicks, fixedPointInverse);
            mapFields.heightfield.Set(x, y, height);
            if (totalTicks <= 0) {
                mapFields.materialProportions[0].Set(x, y, 1.0f);
                for (int stratum = 1; stratum < stratumCount; ++stratum)
                    mapFields.materialProportions[stratum].Set(x, y, 0.0f);
                continue;
            }
            const float totalReciprocal = 1.0f / static_cast<float>(totalTicks);
            for (int stratum = 0; stratum < stratumCount; ++stratum) {
                const int ticks = thicknessFixedPoint[stratum * cellCount + cellIndex];
                mapFields.materialProportions[stratum].Set(x, y, static_cast<float>(ticks) * totalReciprocal);
            }
        }
    }
}

int ErosionStage::ColumnTotalFixedPointAt(int cellIndex) const {
    return ColumnTotalFixedPoint(thicknessFixedPoint.data(), stratumCount, cellCount, cellIndex);
}

} // namespace Proc
} // namespace SanmapGen
