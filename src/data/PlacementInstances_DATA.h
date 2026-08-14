// PlacementInstances_DATA.h — the resolved instance buffer, a REAL struct-of-arrays.
// Layer: DATA (computed output). One parallel array per field, so a consumer that only
// needs positions (preview scatter, spatial queries) streams three contiguous float arrays
// instead of walking 64-byte interleaved records. Fixes the "documented SoA, implemented
// AoS" defect in PLACEMENT_SCATTER_SPEC. No behavior beyond append/clear/read.
#pragma once
#include <vector>
#include <cstddef>
#include "PlacementInstance_DATA.h"

namespace SanmapGen {
namespace Data {

class PlacementInstances {
public:
    // Position columns (hot: preview, spacing queries, export).
    std::vector<float> positionX, positionY, positionZ;
    // Transform columns.
    std::vector<float> rotationX, rotationY, rotationZ, rotationW;
    std::vector<float> scaleX, scaleY, scaleZ;
    // Identity / gameplay columns.
    std::vector<TemplateIdentifier> templateIdentifier;
    std::vector<int>  ruleIndex;
    std::vector<int>  category;
    std::vector<int>  symmetryIdentifier;
    std::vector<int>  biomeStratumIndex;
    std::vector<int>  armyIndex;
    std::vector<unsigned char> bCollidable;   // 1 byte per instance, addressable

    std::size_t Count() const { return positionX.size(); }
    bool IsEmpty() const { return positionX.empty(); }

    void Clear() {
        positionX.clear(); positionY.clear(); positionZ.clear();
        rotationX.clear(); rotationY.clear(); rotationZ.clear(); rotationW.clear();
        scaleX.clear(); scaleY.clear(); scaleZ.clear();
        templateIdentifier.clear(); ruleIndex.clear(); category.clear();
        symmetryIdentifier.clear(); biomeStratumIndex.clear(); armyIndex.clear();
        bCollidable.clear();
    }

    void Reserve(std::size_t instanceCount) {
        positionX.reserve(instanceCount); positionY.reserve(instanceCount); positionZ.reserve(instanceCount);
        rotationX.reserve(instanceCount); rotationY.reserve(instanceCount);
        rotationZ.reserve(instanceCount); rotationW.reserve(instanceCount);
        scaleX.reserve(instanceCount); scaleY.reserve(instanceCount); scaleZ.reserve(instanceCount);
        templateIdentifier.reserve(instanceCount); ruleIndex.reserve(instanceCount);
        category.reserve(instanceCount); symmetryIdentifier.reserve(instanceCount);
        biomeStratumIndex.reserve(instanceCount); armyIndex.reserve(instanceCount);
        bCollidable.reserve(instanceCount);
    }

    std::size_t Append(const PlacementInstance& instance) {
        positionX.push_back(instance.positionX);
        positionY.push_back(instance.positionY);
        positionZ.push_back(instance.positionZ);
        rotationX.push_back(instance.rotationX);
        rotationY.push_back(instance.rotationY);
        rotationZ.push_back(instance.rotationZ);
        rotationW.push_back(instance.rotationW);
        scaleX.push_back(instance.scaleX);
        scaleY.push_back(instance.scaleY);
        scaleZ.push_back(instance.scaleZ);
        templateIdentifier.push_back(instance.templateIdentifier);
        ruleIndex.push_back(instance.ruleIndex);
        category.push_back(instance.category);
        symmetryIdentifier.push_back(instance.symmetryIdentifier);
        biomeStratumIndex.push_back(instance.biomeStratumIndex);
        armyIndex.push_back(instance.armyIndex);
        bCollidable.push_back(instance.bCollidable ? 1u : 0u);
        return positionX.size() - 1;
    }

    PlacementInstance Get(std::size_t index) const {
        PlacementInstance instance;
        instance.positionX = positionX[index]; instance.positionY = positionY[index];
        instance.positionZ = positionZ[index];
        instance.rotationX = rotationX[index]; instance.rotationY = rotationY[index];
        instance.rotationZ = rotationZ[index]; instance.rotationW = rotationW[index];
        instance.scaleX = scaleX[index]; instance.scaleY = scaleY[index]; instance.scaleZ = scaleZ[index];
        instance.templateIdentifier = templateIdentifier[index];
        instance.ruleIndex = ruleIndex[index];
        instance.category = category[index];
        instance.symmetryIdentifier = symmetryIdentifier[index];
        instance.biomeStratumIndex = biomeStratumIndex[index];
        instance.armyIndex = armyIndex[index];
        instance.bCollidable = bCollidable[index] != 0u;
        return instance;
    }
};

} // namespace Data
} // namespace SanmapGen
