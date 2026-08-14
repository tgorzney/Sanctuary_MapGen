// Placement_Scatter_PROC.cpp — the Poisson-disk candidate pass.
// One candidate per grid cell of size spacing/sqrt(2), jittered by the position hash, gated
// by the rule's gate field, then ordered. Ordering is the blue-noise trick: a hashed
// priority (or the rule's area priority) decides who gets to claim its disk first, so the
// accepted set is spaced AND unbiased — a raster-order walk would bias every rule towards
// the top-left corner. No rand(), no global state: (seed, rule, position) is the only input.
#include "Placement_PROC.h"
#include "Placement_Gate_PROC.h"
#include <algorithm>
#include <cmath>

namespace SanmapGen {
namespace Proc {
namespace {

inline float ClampToRange(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}
inline int NearestCell(float position, int vertexSize) {
    int cell = static_cast<int>(position + 0.5f);
    if (cell < 0) cell = 0;
    if (cell >= vertexSize) cell = vertexSize - 1;
    return cell;
}

struct CandidateGridLayout { float cellSize = 1.0f; int gridSide = 1; };

// Cell size spacing/sqrt(2) guarantees at most one accepted point per cell; the count cap is
// the safety valve for a rule that asks for a pathological candidate density.
CandidateGridLayout MakeCandidateGridLayout(const PlacementConstants& constants,
                                            const ScatterRuleConfiguration& configuration, float extent) {
    CandidateGridLayout layout;
    layout.cellSize = configuration.spacingMinimum > constants.spacingEpsilon
                    ? configuration.spacingMinimum * constants.candidateCellSizeFactor
                    : constants.candidateCellSizeMinimum;
    if (layout.cellSize < constants.candidateCellSizeMinimum)
        layout.cellSize = constants.candidateCellSizeMinimum;
    layout.gridSide = static_cast<int>(extent / layout.cellSize) + 1;
    if (layout.gridSide < 1) layout.gridSide = 1;
    if (layout.gridSide > constants.candidateCountMaximum / layout.gridSide) {
        layout.gridSide = static_cast<int>(std::sqrt(static_cast<float>(constants.candidateCountMaximum)));
        layout.cellSize = extent / static_cast<float>(layout.gridSide > 1 ? layout.gridSide - 1 : 1);
    }
    return layout;
}

// Position + hash only; the gates and the sort key are applied by the caller.
ScatterCandidate MakeJitteredCandidate(const PlacementConstants& constants,
                                       const ScatterRuleConfiguration& configuration,
                                       const CandidateGridLayout& layout, int gridX, int gridY, float extent) {
    ScatterCandidate candidate;
    candidate.positionHash = HashPosition(constants, configuration.ruleSeed, gridX, gridY);
    candidate.candidateIndex = gridY * layout.gridSide + gridX;
    const float jitterX = (HashToUnitFloat(HashStream(constants, candidate.positionHash,
                                                      ScatterHashStream::JitterX)) - 0.5f)
                        * constants.candidateJitterStrength;
    const float jitterY = (HashToUnitFloat(HashStream(constants, candidate.positionHash,
                                                      ScatterHashStream::JitterY)) - 0.5f)
                        * constants.candidateJitterStrength;
    candidate.positionX = ClampToRange((static_cast<float>(gridX) + 0.5f + jitterX) * layout.cellSize,
                                       0.0f, extent);
    candidate.positionY = ClampToRange((static_cast<float>(gridY) + 0.5f + jitterY) * layout.cellSize,
                                       0.0f, extent);
    return candidate;
}

} // namespace

void PlacementStage::ScatterRule(std::size_t configurationIndex) {
    std::vector<ScatterCandidate> candidates;
    CollectCandidates(configurationIndex, candidates);
    std::sort(candidates.begin(), candidates.end(), ScatterCandidateOrder);
    AcceptCandidates(configurationIndex, candidates);
}

void PlacementStage::CollectCandidates(std::size_t configurationIndex,
                                       std::vector<ScatterCandidate>& outCandidates) {
    const ScatterRuleConfiguration& configuration = ruleConfigurations[configurationIndex];
    const int vertexSize = mapFields.VertexSize();
    const float extent = static_cast<float>(vertexSize - 1);
    const CandidateGridLayout layout = MakeCandidateGridLayout(constants, configuration, extent);
    const bool bUseDensity = (configuration.selectionFlags & ScatterSelectionFlag::UseDensity) != 0;

    outCandidates.clear();
    for (int gridY = 0; gridY < layout.gridSide; ++gridY)
        for (int gridX = 0; gridX < layout.gridSide; ++gridX) {
            ScatterCandidate candidate = MakeJitteredCandidate(constants, configuration, layout,
                                                               gridX, gridY, extent);
            const int cellX = NearestCell(candidate.positionX, vertexSize);
            const int cellY = NearestCell(candidate.positionY, vertexSize);
            candidate.gateWeight = gateWeightField.Get(cellX, cellY);
            ++evaluatedCandidateCount;
            if (candidate.gateWeight <= 0.0f) continue;
            if (bUseDensity) {
                const float draw = HashToUnitFloat(HashStream(constants, candidate.positionHash,
                                                              ScatterHashStream::Density));
                if (draw >= configuration.density * candidate.gateWeight) continue;
            }
            bool bRejected = false;
            candidate.sortKey = ComputeCandidateSortKey(configuration, cellX, cellY, candidate, bRejected);
            if (!bRejected) outCandidates.push_back(candidate);
        }
}

// Lower sorts first. Area priorities rank by radial clearance / height variance; everything
// else uses a hashed draw divided by the gate weight, so a heavier-weighted position (focus
// gradient) statistically wins the disk without ever breaking determinism.
float PlacementStage::ComputeCandidateSortKey(const ScatterRuleConfiguration& configuration,
                                              int cellX, int cellY, const ScatterCandidate& candidate,
                                              bool& bRejected) const {
    bRejected = false;
    const int selectionFlags = configuration.selectionFlags;
    const bool bCheckMaximum = (selectionFlags & ScatterSelectionFlag::CheckMaximumRadius) != 0;
    const bool bClearanceGate = configuration.clearanceRadiusMinimum > 0.0f || bCheckMaximum;
    const bool bAreaPriority = configuration.collectionIndex == 0
        && (selectionFlags & (ScatterSelectionFlag::RandomSelection | ScatterSelectionFlag::UseDensity
                              | ScatterSelectionFlag::UseAllPositions)) == 0;
    float clearanceRadius = 0.0f;
    if (bClearanceGate || (bAreaPriority && configuration.priorityMode != 2))
        clearanceRadius = SampleClearanceRadius(configuration, cellX, cellY);
    if (bClearanceGate) {
        if (clearanceRadius < configuration.clearanceRadiusMinimum) { bRejected = true; return 0.0f; }
        if (bCheckMaximum && configuration.clearanceRadiusMaximum > 0.0f
            && clearanceRadius > configuration.clearanceRadiusMaximum) { bRejected = true; return 0.0f; }
    }
    if (bAreaPriority) {
        if (configuration.priorityMode == 0) return -clearanceRadius;   // LargestArea
        if (configuration.priorityMode == 1) return clearanceRadius;    // SmallestArea
        return SampleHeightVariance(cellX, cellY);                      // LeastVariance
    }
    const float weight = candidate.gateWeight > 1.0e-6f ? candidate.gateWeight : 1.0e-6f;
    return HashToUnitFloat(HashStream(constants, candidate.positionHash, ScatterHashStream::Priority))
         / weight;
}

} // namespace Proc
} // namespace SanmapGen
