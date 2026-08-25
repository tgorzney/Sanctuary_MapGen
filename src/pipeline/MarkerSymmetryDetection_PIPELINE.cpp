// MarkerSymmetryDetection_PIPELINE.cpp — see MarkerSymmetryDetection_PIPELINE.h for the full
// rationale and algorithm description. The per-seed matching primitives live in
// MarkerSymmetryDetectionMatch_PIPELINE.h (ARCH §1.5 — split to stay under the 150-line ceiling).
#include "MarkerSymmetryDetection_PIPELINE.h"
#include "MarkerSymmetryDetectionMatch_PIPELINE.h"
#include "../params/Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Pipeline {
namespace {

// One seed's full attempt: builds its orbit, gathers/claims tuples, and either appends a confirmed
// MarkerSymmetryOrbitMatch to `results` (consuming the seed + every matched candidate) or folds its
// unclaimed slot count into `unmatchedSlotTotal`. Returns nothing — every effect is via the
// out-parameters, since this is a single step of FindMarkerSymmetryMatches's own seed loop.
void ProcessSeed(const Params::Geometry& geometry, int symmetryMask, int radialSymmetryRepeatCount,
                 const Data::SpatialGrid& grid, const std::vector<int>& canonicalRank, int seedIndex,
                 const float* candidatePositionX, const float* candidatePositionZ,
                 float distanceTolerance, std::vector<bool>& consumed,
                 std::vector<MarkerSymmetryOrbitMatch>& results, int& unmatchedSlotTotal) {
    WorldSymmetryOrbitPoint orbitPoints[Params::symmetryOrbitMaximum];
    const int orbitCount = BuildWorldSymmetryOrbit(geometry, symmetryMask, radialSymmetryRepeatCount,
                                                    candidatePositionX[seedIndex], candidatePositionZ[seedIndex],
                                                    orbitPoints, Params::symmetryOrbitMaximum);
    if (orbitCount <= 1) return;   // no mirrors under this mask — nothing to detect for this seed

    std::vector<MatchDetail::MatchTuple> tuples = MatchDetail::GatherSeedMatchTuples(
        grid, consumed, canonicalRank, orbitPoints, orbitCount, seedIndex, candidatePositionX,
        candidatePositionZ, distanceTolerance);
    std::vector<int> matchedBySlot;
    const bool bFullyConfirmed = MatchDetail::ClaimSeedMatchTuples(std::move(tuples), orbitCount, matchedBySlot);

    if (!bFullyConfirmed) {
        int claimedCount = 0;
        for (int slotIndex = 1; slotIndex < orbitCount; ++slotIndex)
            if (matchedBySlot[static_cast<std::size_t>(slotIndex)] >= 0) ++claimedCount;
        unmatchedSlotTotal += (orbitCount - 1 - claimedCount);
        return;
    }
    MarkerSymmetryOrbitMatch match;
    match.seedCandidateIndex = seedIndex;
    for (int slotIndex = 1; slotIndex < orbitCount; ++slotIndex)
        match.matchedCandidateIndices.push_back(matchedBySlot[static_cast<std::size_t>(slotIndex)]);
    consumed[static_cast<std::size_t>(seedIndex)] = true;
    for (int matchedIndex : match.matchedCandidateIndices) consumed[static_cast<std::size_t>(matchedIndex)] = true;
    results.push_back(std::move(match));
}

} // namespace

std::vector<MarkerSymmetryOrbitMatch> FindMarkerSymmetryMatches(
        const Params::Geometry& geometry, int symmetryMask, int radialSymmetryRepeatCount,
        const float* candidatePositionX, const float* candidatePositionZ, int candidateCount,
        float distanceTolerance, int* outUnmatchedSlotCount) {
    std::vector<MarkerSymmetryOrbitMatch> results;
    if (outUnmatchedSlotCount != nullptr) *outUnmatchedSlotCount = 0;
    if (candidateCount <= 0 || candidatePositionX == nullptr || candidatePositionZ == nullptr) return results;

    const std::vector<MatchDetail::CanonicalKey> canonicalOrder = MatchDetail::BuildCanonicalOrder(
        candidatePositionX, candidatePositionZ, candidateCount, positionQuantizationStep);
    std::vector<int> canonicalRank(static_cast<std::size_t>(candidateCount), 0);
    for (std::size_t rank = 0; rank < canonicalOrder.size(); ++rank)
        canonicalRank[static_cast<std::size_t>(canonicalOrder[rank].candidateIndex)] = static_cast<int>(rank);

    const float worldSize = static_cast<float>(geometry.mapSize) * geometry.worldUnitsPerCell;
    Data::SpatialGrid grid;
    grid.Configure(worldSize, MatchDetail::ResolvedChunkResolution(worldSize, distanceTolerance));
    grid.Build(candidatePositionX, candidatePositionZ, candidateCount);

    std::vector<bool> consumed(static_cast<std::size_t>(candidateCount), false);
    int unmatchedSlotTotal = 0;
    for (const MatchDetail::CanonicalKey& seedKey : canonicalOrder) {
        if (consumed[static_cast<std::size_t>(seedKey.candidateIndex)]) continue;
        ProcessSeed(geometry, symmetryMask, radialSymmetryRepeatCount, grid, canonicalRank,
                   seedKey.candidateIndex, candidatePositionX, candidatePositionZ, distanceTolerance,
                   consumed, results, unmatchedSlotTotal);
    }

    if (outUnmatchedSlotCount != nullptr) *outUnmatchedSlotCount = unmatchedSlotTotal;
    return results;
}

} // namespace Pipeline
} // namespace SanmapGen
