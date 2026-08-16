// Placement_Test_Terrain.h — test-only scaffolding shared by the placement acceptance tests
// (not part of the layer graph; nothing in src/ includes it outside a *_Test.cpp).
// Builds a deliberately symmetric probe terrain: a flat, pathable plain with one steep cone
// in the middle, plus a stratum mask that covers the left half — so slope, height, mask and
// symmetry gates all have something to bite on.
// The scene stands in for the UPSTREAM Mask stage, so it authors both of Mask's outputs:
// `surfaceStratumWeights` and the baked `slope` field Placement now reads (M5-0c). The slope is
// produced by the one authority, `Proc::SlopeGradientMagnitude`, never a second formula.
#pragma once
#include "Placement_PROC.h"
#include "Mask_Kernel_PROC.h"
#include "Mask_Slope_PROC.h"     // the ONE slope formula; the scene stands in for the Mask stage
#include <cmath>

namespace PlacementTest {

using namespace SanmapGen;

constexpr int mapSize          = 128;
constexpr int vertexSize       = mapSize + 1;
constexpr int maskStratumIndex = 3;
constexpr float plainHeight    = 0.5f;
constexpr float coneRadius     = 18.0f;
constexpr float terrainMaxHeight = 128.0f;   // the value every placement test's recipe carries

// Mask's slope output, authored by the scene through the ONE slope authority
// (Proc::SlopeGradientMagnitude) with Mask's own default constants, so the fixture can never
// drift from the stage that writes this field in a real run. The gradient's run is the caller's
// `worldUnitsPerCell` — the same `Params::Geometry` value the recipe carries (ARCH §7.1, M5-0d),
// so a scaled fixture and the Placement stage reading it cannot disagree about cell world-size.
inline void BakeSlopeField(Data::MapFields& fields, float worldUnitsPerCell = 1.0f) {
    const Proc::MaskConstants constants;
    const float cellWorldSize = worldUnitsPerCell > 0.0f ? worldUnitsPerCell : 1.0f;
    Proc::MaskStratumConfiguration configuration;
    configuration.heightScale       = terrainMaxHeight;
    configuration.inverseSingleSpan = 1.0f / cellWorldSize;
    configuration.inverseDoubleSpan = 1.0f / (constants.centralDifferenceSpan * cellWorldSize);
    const float* const heightValues = fields.heightfield.Data();
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x)
            fields.slope.Set(x, y, Proc::SlopeGradientMagnitude(heightValues, x, y, vertexSize,
                                                                configuration));
}

inline void BuildTestFields(Data::MapFields& fields, float worldUnitsPerCell = 1.0f) {
    fields.Resize(vertexSize, 0.0f);
    const float center = static_cast<float>(vertexSize - 1) * 0.5f;
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x) {
            const float offsetX = static_cast<float>(x) - center;
            const float offsetY = static_cast<float>(y) - center;
            const float distance = std::sqrt(offsetX * offsetX + offsetY * offsetY);
            float height = plainHeight;
            if (distance < coneRadius) height = plainHeight + 0.5f * (1.0f - distance / coneRadius);
            fields.heightfield.Set(x, y, height);
            // Placement gates on the VISIBLE weights the Mask stage resolves (ARCH 7.2.6), so
            // the scene authors those; the proportions behind them are deliberately the mirror
            // image, so a stage reading the wrong field fails every gate assertion.
            fields.surfaceStratumWeights[maskStratumIndex].Set(x, y, x < vertexSize / 2 ? 1.0f : 0.0f);
            fields.surfaceStratumWeights[0].Set(x, y, x < vertexSize / 2 ? 0.0f : 1.0f);
            fields.materialProportions[maskStratumIndex].Set(x, y, x < vertexSize / 2 ? 0.0f : 1.0f);
            fields.materialProportions[0].Set(x, y, x < vertexSize / 2 ? 1.0f : 0.0f);
        }
    BakeSlopeField(fields, worldUnitsPerCell);   // Mask's other output, which Placement READS (M5-0c)
}

inline Params::ScatterTransform MakeTransform(const char* templateIdentifier,
                                              float scaleMinimum, float scaleMaximum) {
    Params::ScatterTransform transform;
    transform.scaleMinimum = scaleMinimum;
    transform.scaleMaximum = scaleMaximum;
    for (int index = 0; index < 7 && templateIdentifier[index] != '\0'; ++index)
        transform.templateIdentifier[index] = templateIdentifier[index];
    return transform;
}

// Smallest distance (in cells) between any two instances of a collection.
inline float MinimumSeparation(const Data::PlacementInstances& instances) {
    float minimum = 1.0e9f;
    for (std::size_t first = 0; first < instances.Count(); ++first)
        for (std::size_t second = first + 1; second < instances.Count(); ++second) {
            const float deltaX = instances.positionX[first] - instances.positionX[second];
            const float deltaY = instances.positionZ[first] - instances.positionZ[second];
            const float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
            if (distance < minimum) minimum = distance;
        }
    return minimum;
}

// The slope check reads the BAKED field and squares it exactly as the gate does — the stage no
// longer exposes a slope field of its own, because it no longer owns one (M5-0c).
// Instances carry WORLD positions, so they are divided back to cells by the same
// `worldUnitsPerCell` the stage multiplied by (Placement_Accept_PROC does the identical
// conversion) — one owner of cell world-size on both sides of the check (M5-0d).
inline bool AllWithinGates(const Data::PlacementInstances& instances, const Data::MapFields& fields,
                           float heightMinimum, float heightMaximum,
                           float maxSlopeDegrees, int mapEdgePadding,
                           float worldUnitsPerCell = 1.0f) {
    const float tangent = std::tan(maxSlopeDegrees * 3.14159265f / 180.0f);
    const float gradientLimitSquared = tangent * tangent;
    const float cellReciprocal = 1.0f / (worldUnitsPerCell > 0.0f ? worldUnitsPerCell : 1.0f);
    for (std::size_t index = 0; index < instances.Count(); ++index) {
        const int cellX = static_cast<int>(instances.positionX[index] * cellReciprocal + 0.5f);
        const int cellY = static_cast<int>(instances.positionZ[index] * cellReciprocal + 0.5f);
        if (cellX < mapEdgePadding || cellY < mapEdgePadding) return false;
        if (cellX >= vertexSize - mapEdgePadding || cellY >= vertexSize - mapEdgePadding) return false;
        const float height = fields.heightfield.Get(cellX, cellY);
        if (height < heightMinimum || height > heightMaximum) return false;
        const float slopeGradient = fields.slope.Get(cellX, cellY);
        if (slopeGradient * slopeGradient > gradientLimitSquared * 1.001f) return false;
    }
    return true;
}

} // namespace PlacementTest
