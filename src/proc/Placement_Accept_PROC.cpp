// Placement_Accept_PROC.cpp — dart-throw acceptance: spacing, symmetry, and the count limit.
// A candidate is accepted as a WHOLE symmetry orbit or not at all: if any clone would fail
// the gates or violate the minimum spacing, the source is skipped too. That is what makes a
// symmetric map fair — a spawn never exists without its mirror (AI_HOSTCLIENT_SPEC "start
// positions ... symmetric ground").
#include "Placement_PROC.h"
#include "Placement_SpacingGrid_PROC.h"

namespace SanmapGen {
namespace Proc {

void PlacementStage::AcceptCandidates(std::size_t configurationIndex,
                                      const std::vector<ScatterCandidate>& candidates) {
    const ScatterRuleConfiguration& configuration = ruleConfigurations[configurationIndex];
    const int vertexSize = mapFields.VertexSize();
    const float extent = static_cast<float>(vertexSize - 1);
    const bool bSpacingActive = configuration.spacingMinimum > constants.spacingEpsilon;

    // Seed the accelerator with everything already placed in this collection, so a rule also
    // keeps its distance from the markers/props earlier rules put down.
    SpacingGrid spacingGrid;
    spacingGrid.Configure(bSpacingActive ? configuration.spacingMinimum : constants.candidateCellSizeMinimum,
                          vertexSize);
    Data::PlacementInstances& collection = CollectionFor(configuration.collectionIndex);
    const float cellReciprocal = 1.0f / recipe.geometry.worldUnitsPerCell;
    for (std::size_t index = 0; index < collection.Count(); ++index)
        spacingGrid.Insert(collection.positionX[index] * cellReciprocal,
                           collection.positionZ[index] * cellReciprocal);

    const bool bLimitCount = (configuration.selectionFlags
                              & (ScatterSelectionFlag::UseDensity | ScatterSelectionFlag::UseAllPositions)) == 0
                           && configuration.targetCount > 0;
    SymmetryOrbitPoint orbit[Params::symmetryOrbitMaximum];
    int placedCount = 0;
    for (const ScatterCandidate& candidate : candidates) {
        if (bLimitCount && placedCount >= configuration.targetCount) break;
        const int orbitCount = BuildSymmetryOrbit(configuration.symmetryMask,
                                                  ruleRadialSymmetryRepeatCounts[configurationIndex],
                                                  extent, candidate.positionX, candidate.positionY,
                                                  constants.symmetryDuplicateEpsilon,
                                                  orbit, Params::symmetryOrbitMaximum);
        if (!IsOrbitPlaceable(configuration, orbit, orbitCount, spacingGrid)) continue;
        const int symmetryIdentifier = nextSymmetryIdentifier++;
        for (int pointIndex = 0; pointIndex < orbitCount; ++pointIndex) {
            EmitInstance(configurationIndex, orbit[pointIndex], candidate.positionHash, symmetryIdentifier);
            spacingGrid.Insert(orbit[pointIndex].positionX, orbit[pointIndex].positionY);
        }
        placedCount            += orbitCount;
        acceptedCandidateCount += orbitCount;
    }
}

// Every clone must be in bounds, pass the SAME gate field as its source, and respect the
// minimum spacing against both the already-accepted set and its own siblings.
bool PlacementStage::IsOrbitPlaceable(const ScatterRuleConfiguration& configuration,
                                      const SymmetryOrbitPoint* orbit, int orbitCount,
                                      const SpacingGrid& spacingGrid) const {
    const int vertexSize = mapFields.VertexSize();
    const float extent = static_cast<float>(vertexSize - 1);
    const float spacing = configuration.spacingMinimum;
    const bool bSpacingActive = spacing > constants.spacingEpsilon;
    const float spacingSquared = spacing * spacing;

    for (int index = 0; index < orbitCount; ++index) {
        const float positionX = orbit[index].positionX;
        const float positionY = orbit[index].positionY;
        if (positionX < 0.0f || positionY < 0.0f || positionX > extent || positionY > extent) return false;
        int cellX = static_cast<int>(positionX + 0.5f);
        int cellY = static_cast<int>(positionY + 0.5f);
        if (cellX >= vertexSize) cellX = vertexSize - 1;
        if (cellY >= vertexSize) cellY = vertexSize - 1;
        if (gateWeightField.Get(cellX, cellY) <= 0.0f) return false;
        if (!bSpacingActive) continue;
        if (spacingGrid.HasPointWithin(positionX, positionY, spacing)) return false;
        for (int sibling = 0; sibling < index; ++sibling) {
            const float deltaX = orbit[sibling].positionX - positionX;
            const float deltaY = orbit[sibling].positionY - positionY;
            if (deltaX * deltaX + deltaY * deltaY < spacingSquared) return false;
        }
    }
    return true;
}

} // namespace Proc
} // namespace SanmapGen
