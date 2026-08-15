// Placement_Fields_PROC.cpp — the derived fields every rule reads: the squared height
// gradient (slope), the Jump-Flood exclusion distance field, and the per-rule gate field.
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

// Squared height gradient in game units per cell — squared because every slope gate compares
// against a squared tangent, so neither backend ever needs a square root or an arc-tangent.
void PlacementStage::BuildSlopeGradientField() {
    const int vertexSize = mapFields.VertexSize();
    const Data::FloatField& heightfield = mapFields.heightfield;
    const float heightScale = recipe.geometry.terrainMaxHeight;
    const float cellReciprocal = 1.0f / constants.worldUnitsPerCell;         // one-sided edge
    const float spanReciprocal = 0.5f * cellReciprocal;                      // central difference
    const int lastIndex = vertexSize - 1;

    auto computeSlopeRow = [&](int y) {
        const int lowY = y > 0 ? y - 1 : 0;
        const int highY = y < lastIndex ? y + 1 : lastIndex;
        const float reciprocalY = (highY - lowY) == 2 ? spanReciprocal : cellReciprocal;
        for (int x = 0; x < vertexSize; ++x) {
            const int lowX = x > 0 ? x - 1 : 0;
            const int highX = x < lastIndex ? x + 1 : lastIndex;
            const float reciprocalX = (highX - lowX) == 2 ? spanReciprocal : cellReciprocal;
            const float gradientX = (heightfield.Get(highX, y) - heightfield.Get(lowX, y))
                                  * heightScale * reciprocalX;
            const float gradientY = (heightfield.Get(x, highY) - heightfield.Get(x, lowY))
                                  * heightScale * reciprocalY;
            slopeGradientField.Set(x, y, gradientX * gradientX + gradientY * gradientY);
        }
    };
    if (threadPool != nullptr) threadPool->ParallelFor(0, vertexSize, computeSlopeRow);
    else for (int y = 0; y < vertexSize; ++y) computeSlopeRow(y);
}

void PlacementStage::BuildDerivedFields() {
    const int vertexSize = mapFields.VertexSize();
    slopeGradientField.Resize(vertexSize, vertexSize, 0.0f);
    gateWeightField.Resize(vertexSize, vertexSize, 0.0f);
    BuildSlopeGradientField();

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
                                           slopeGradientField.Get(x, y),
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
