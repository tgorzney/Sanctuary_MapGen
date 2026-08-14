// Placement_Metrics_PROC.cpp — the per-candidate area metrics the marker priorities rank by.
// Radial clearance comes from RadialClearance_MATH (M0-5): the deterministic Bresenham
// perimeter walk, so "largest flat area" means the same thing here as everywhere else. Only
// evaluated for rules that actually need it — it is the expensive part of a candidate.
#include "Placement_PROC.h"
#include "../math/RadialClearance_MATH.h"

namespace SanmapGen {
namespace Proc {

float PlacementStage::SampleClearanceRadius(const ScatterRuleConfiguration& configuration,
                                            int cellX, int cellY) const {
    const int vertexSize = mapFields.VertexSize();
    int searchRadius = constants.clearanceSearchRadiusMaximum;
    if (configuration.clearanceRadiusMaximum > 0.0f) {
        const int ruleRadius = static_cast<int>(configuration.clearanceRadiusMaximum) + 1;
        if (ruleRadius < searchRadius) searchRadius = ruleRadius;
    }
    if (searchRadius < 1) searchRadius = 1;
    const int scored = Math::ScoreRadialClearance(mapFields.heightfield.Data(), vertexSize, vertexSize,
                                                  cellX, cellY,
                                                  configuration.heightMinimum, configuration.heightMaximum,
                                                  configuration.clearanceHeightTolerance, searchRadius, 1);
    return static_cast<float>(scored);
}

// Height variance in a square window — the LeastVariance priority's "flattest spot" measure.
float PlacementStage::SampleHeightVariance(int cellX, int cellY) const {
    const int vertexSize = mapFields.VertexSize();
    int windowRadius = static_cast<int>(constants.varianceSampleRadius);
    if (windowRadius < 1) windowRadius = 1;
    const int lowX = cellX - windowRadius < 0 ? 0 : cellX - windowRadius;
    const int lowY = cellY - windowRadius < 0 ? 0 : cellY - windowRadius;
    const int highX = cellX + windowRadius >= vertexSize ? vertexSize - 1 : cellX + windowRadius;
    const int highY = cellY + windowRadius >= vertexSize ? vertexSize - 1 : cellY + windowRadius;

    float sum = 0.0f;
    float sumOfSquares = 0.0f;
    int sampleCount = 0;
    for (int y = lowY; y <= highY; ++y)
        for (int x = lowX; x <= highX; ++x) {
            const float sample = mapFields.heightfield.Get(x, y);
            sum += sample;
            sumOfSquares += sample * sample;
            ++sampleCount;
        }
    if (sampleCount == 0) return 0.0f;
    const float countReciprocal = 1.0f / static_cast<float>(sampleCount);
    const float mean = sum * countReciprocal;
    const float variance = sumOfSquares * countReciprocal - mean * mean;
    return variance > 0.0f ? variance : 0.0f;
}

} // namespace Proc
} // namespace SanmapGen
