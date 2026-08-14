// Placement_Test_Terrain.h — test-only scaffolding shared by the placement acceptance tests
// (not part of the layer graph; nothing in src/ includes it outside a *_Test.cpp).
// Builds a deliberately symmetric probe terrain: a flat, pathable plain with one steep cone
// in the middle, plus a stratum mask that covers the left half — so slope, height, mask and
// symmetry gates all have something to bite on.
#pragma once
#include "Placement_PROC.h"
#include <cmath>

namespace PlacementTest {

using namespace SanmapGen;

constexpr int mapSize          = 128;
constexpr int vertexSize       = mapSize + 1;
constexpr int maskStratumIndex = 3;
constexpr float plainHeight    = 0.5f;
constexpr float coneRadius     = 18.0f;

inline void BuildTestFields(Data::MapFields& fields) {
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
            fields.materialMasks[maskStratumIndex].Set(x, y, x < vertexSize / 2 ? 1.0f : 0.0f);
            fields.materialMasks[0].Set(x, y, x < vertexSize / 2 ? 0.0f : 1.0f);
        }
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

inline bool AllWithinGates(const Data::PlacementInstances& instances, const Data::MapFields& fields,
                           const Proc::PlacementStage& stage, float heightMinimum, float heightMaximum,
                           float maxSlopeDegrees, int mapEdgePadding) {
    const float tangent = std::tan(maxSlopeDegrees * 3.14159265f / 180.0f);
    const float gradientLimitSquared = tangent * tangent;
    for (std::size_t index = 0; index < instances.Count(); ++index) {
        const int cellX = static_cast<int>(instances.positionX[index] + 0.5f);
        const int cellY = static_cast<int>(instances.positionZ[index] + 0.5f);
        if (cellX < mapEdgePadding || cellY < mapEdgePadding) return false;
        if (cellX >= vertexSize - mapEdgePadding || cellY >= vertexSize - mapEdgePadding) return false;
        const float height = fields.heightfield.Get(cellX, cellY);
        if (height < heightMinimum || height > heightMaximum) return false;
        if (stage.SlopeGradientField().Get(cellX, cellY) > gradientLimitSquared * 1.001f) return false;
    }
    return true;
}

} // namespace PlacementTest
