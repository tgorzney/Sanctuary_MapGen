// Placement_Fields_PROC.cpp — the derived fields every rule reads: the Jump-Flood exclusion
// distance field and the per-rule gate field.
// Slope is NOT among them: `MapFields.slope` is baked by the Mask stage, the single writer of
// the one slope formula (ARCH §3.4.1, M5-0c). This stage reads that field and squares it at the
// read site, because the gate compares against a squared tangent — the unit and the gate math
// are unchanged; only the shadow recompute is gone.
// The gate field is the Cpu twin of Placement_PROC.glsl — same ScatterGateWeight call, so
// the preview gate and the authoritative bake evaluate one expression, not two.
#include "Placement_PROC.h"
#include "Placement_Gate_PROC.h"
#include "../math/JumpFloodDistanceField_MATH.h"
#include "../sys/ThreadPool_SYS.h"
#include <cmath>

namespace SanmapGen {
namespace Proc {
namespace {

bool NeedsObstacleDistanceField(const std::vector<ScatterRuleConfiguration>& configurations) {
    for (const ScatterRuleConfiguration& configuration : configurations)
        if (configuration.obstacleDistanceMinimum > 0.0f
            || (configuration.selectionFlags & ScatterSelectionFlag::NearCliffs) != 0)
            return true;
    return false;
}

} // namespace

// The baked slope at one vertex, squared for the gate. The unsized case answers a flat 0 rather
// than reading past a field the Mask stage never wrote (Constitution §6).
float PlacementStage::SampleSlopeGradientSquared(int cellX, int cellY) const {
    if (!bSlopeFieldAvailable) return 0.0f;
    const float slopeGradient = mapFields.slope.Get(cellX, cellY);
    return slopeGradient * slopeGradient;
}

void PlacementStage::BuildDerivedFields() {
    const int vertexSize = mapFields.VertexSize();
    gateWeightField.Resize(vertexSize, vertexSize, 0.0f);
    bSlopeFieldAvailable = mapFields.slope.Width() == vertexSize
                        && mapFields.slope.Height() == vertexSize;

    // The Jump-Flood exclusion field is only paid for when some rule actually gates on it.
    if (!NeedsObstacleDistanceField(ruleConfigurations)) return;
    const Data::FloatField& heightfield = mapFields.heightfield;
    obstacleDistanceField.Resize(vertexSize, vertexSize, constants.obstacleDistanceMaximum);
    Math::ComputeJumpFloodDistanceField(heightfield.Data(), vertexSize, vertexSize,
                                        constants.playableHeightMinimum, constants.playableHeightMaximum,
                                        constants.obstacleGradientTolerance,
                                        constants.obstacleDistanceMaximum,
                                        obstacleDistanceField.Data());
    bObstacleFieldBuilt = true;
}

// The stratum gate is a VISIBILITY statement — "scatter trees where the grass shows" — so it
// reads the Mask stage's surface weights, never the physical proportions (ARCH §7.2.6). This
// is what makes placement WYSIWYG with the preview.
float PlacementStage::SampleSurfaceStratumWeight(int stratumIndex, int cellX, int cellY) const {
    if (stratumIndex < 0 || stratumIndex >= Data::MapFields::stratumCount) return 1.0f;
    const Data::FloatField& stratumWeights = mapFields.surfaceStratumWeights[stratumIndex];
    if (stratumWeights.IsEmpty()) return 0.0f;
    return stratumWeights.Get(cellX, cellY);
}

void PlacementStage::BuildGateFieldCpu(std::size_t configurationIndex) {
    const ScatterRuleConfiguration& configuration = ruleConfigurations[configurationIndex];
    const int vertexSize = mapFields.VertexSize();
    const float mapCenter = static_cast<float>(vertexSize - 1) * 0.5f;
    const int padding = configuration.mapEdgePadding;
    const float defaultObstacleDistance = constants.obstacleDistanceMaximum;

    auto computeGateRow = [&](int y) {
        for (int x = 0; x < vertexSize; ++x) {
            float weight = 0.0f;
            if (x >= padding && y >= padding && x < vertexSize - padding && y < vertexSize - padding) {
                const float offsetX = static_cast<float>(x) - mapCenter;
                const float offsetY = static_cast<float>(y) - mapCenter;
                const float focusDistance = std::sqrt(offsetX * offsetX + offsetY * offsetY);
                const float obstacleDistance = bObstacleFieldBuilt ? obstacleDistanceField.Get(x, y)
                                                                   : defaultObstacleDistance;
                weight = ScatterGateWeight(configuration, mapFields.heightfield.Get(x, y),
                                           SampleSlopeGradientSquared(x, y),
                                           SampleSurfaceStratumWeight(configuration.maskStratumIndex, x, y),
                                           obstacleDistance, focusDistance);
            }
            gateWeightField.Set(x, y, weight);
        }
    };
    if (threadPool != nullptr) threadPool->ParallelFor(0, vertexSize, computeGateRow);
    else for (int y = 0; y < vertexSize; ++y) computeGateRow(y);
}

} // namespace Proc
} // namespace SanmapGen
