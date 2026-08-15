// Placement_Emit_PROC.cpp — one accepted position -> one fully-resolved SoA instance.
// Everything a round-trip needs is written here: world position (absolute game units, Y read
// from the map's terrainMaxHeight and never a baked 128), the sampled quaternion/scale, the
// tpId, the dominant biome stratum, the collision flag and the symmetry id.
// The transform is sampled from the SOURCE candidate's hash, so a mirrored clone is the exact
// mirror of its source rather than an independent draw.
#include "Placement_PROC.h"
#include "Placement_Transform_PROC.h"

namespace SanmapGen {
namespace Proc {
namespace {

inline int ClampCell(int cell, int vertexSize) {
    if (cell < 0) return 0;
    return cell >= vertexSize ? vertexSize - 1 : cell;
}

} // namespace

// The biome stamped on an instance is the stratum most VISIBLE under it, so this reads the
// surface weights for the same reason the gate does (ARCH §7.2.6).
int PlacementStage::DominantStratumIndex(int cellX, int cellY) const {
    int dominantIndex = 0;
    float dominantWeight = -1.0f;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        const Data::FloatField& stratumWeights = mapFields.surfaceStratumWeights[stratum];
        if (stratumWeights.IsEmpty()) continue;
        const float weight = stratumWeights.Get(cellX, cellY);
        if (weight > dominantWeight) { dominantWeight = weight; dominantIndex = stratum; }
    }
    return dominantIndex;
}

void PlacementStage::EmitInstance(std::size_t configurationIndex, const SymmetryOrbitPoint& point,
                                  uint32_t sourcePositionHash, int symmetryIdentifier) {
    const ScatterRuleConfiguration& configuration = ruleConfigurations[configurationIndex];
    const int vertexSize = mapFields.VertexSize();
    const int cellX = ClampCell(static_cast<int>(point.positionX + 0.5f), vertexSize);
    const int cellY = ClampCell(static_cast<int>(point.positionY + 0.5f), vertexSize);

    float normalX = 0.0f, normalY = 1.0f, normalZ = 0.0f;
    if ((configuration.selectionFlags & ScatterSelectionFlag::AlignToNormal) != 0) {
        const Data::FloatField& heightfield = mapFields.heightfield;
        const int lowX = ClampCell(cellX - 1, vertexSize), highX = ClampCell(cellX + 1, vertexSize);
        const int lowY = ClampCell(cellY - 1, vertexSize), highY = ClampCell(cellY + 1, vertexSize);
        const float heightScale = recipe.geometry.terrainMaxHeight / constants.worldUnitsPerCell;
        const float gradientX = (heightfield.Get(highX, cellY) - heightfield.Get(lowX, cellY))
                              * heightScale / static_cast<float>(highX - lowX > 0 ? highX - lowX : 1);
        const float gradientZ = (heightfield.Get(cellX, highY) - heightfield.Get(cellX, lowY))
                              * heightScale / static_cast<float>(highY - lowY > 0 ? highY - lowY : 1);
        TerrainNormalFromGradient(gradientX, gradientZ, normalX, normalY, normalZ);
    }

    const SampledInstanceTransform sampled =
        SampleInstanceTransform(constants, configuration, sourcePositionHash,
                                point.yawScale, point.yawOffsetRadians, normalX, normalY, normalZ);

    Data::PlacementInstance instance;
    instance.positionX = point.positionX * constants.worldUnitsPerCell;
    instance.positionZ = point.positionY * constants.worldUnitsPerCell;
    instance.positionY = mapFields.heightfield.SampleBilinear(point.positionX, point.positionY)
                       * recipe.geometry.terrainMaxHeight;
    instance.rotationX = sampled.rotationX; instance.rotationY = sampled.rotationY;
    instance.rotationZ = sampled.rotationZ; instance.rotationW = sampled.rotationW;
    instance.scaleX = sampled.scaleX; instance.scaleY = sampled.scaleY; instance.scaleZ = sampled.scaleZ;
    instance.templateIdentifier = ruleTemplateIdentifiers[configurationIndex];
    instance.ruleIndex          = configuration.ruleIndex;
    instance.category           = configuration.category;
    instance.symmetryIdentifier = symmetryIdentifier;
    instance.biomeStratumIndex  = DominantStratumIndex(cellX, cellY);
    instance.armyIndex          = configuration.armyIndex;
    instance.bCollidable        = (configuration.selectionFlags & ScatterSelectionFlag::Collidable) != 0;
    CollectionFor(configuration.collectionIndex).Append(instance);
}

} // namespace Proc
} // namespace SanmapGen
