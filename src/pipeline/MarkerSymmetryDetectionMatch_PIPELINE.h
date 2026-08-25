// MarkerSymmetryDetectionMatch_PIPELINE.h — the per-seed matching primitives behind
// FindMarkerSymmetryMatches (MarkerSymmetryDetection_PIPELINE.h). Layer: PIPELINE. Split out of
// MarkerSymmetryDetection_PIPELINE.cpp purely to land both files under the ARCH §1.5 150-line
// ceiling — no behavior change, same posture as Placement_Symmetry_PROC.h's own split into
// Placement_SymmetryOrbit_PROC.h (STEP33)'s `SymmetryDetail` namespace. Internal to the PIPELINE
// implementation only — not included by the public MarkerSymmetryDetection_PIPELINE.h.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "SymmetryOrbitQuery_PIPELINE.h"
#include "../data/SpatialGrid_DATA.h"

namespace SanmapGen {
namespace Pipeline {
namespace MatchDetail {

// Cell width should stay comfortably larger than distanceTolerance (>= 4x) so the single-cell
// bucket lookup rarely misses a genuine match across a cell boundary — Data::SpatialGrid::
// defaultChunkResolution (32) is the starting point, reduced only if that guidance would be
// violated at the map's own world size.
inline int ResolvedChunkResolution(float worldSize, float distanceTolerance) {
    if (!(worldSize > 0.0f)) return 1;
    const float minimumCellWidth = distanceTolerance > 0.0f ? distanceTolerance * 4.0f : worldSize;
    int resolution = Data::SpatialGrid::defaultChunkResolution;
    while (resolution > 1 && (worldSize / static_cast<float>(resolution)) < minimumCellWidth) --resolution;
    return resolution;
}

// The canonical, insertion-order-independent ordering: (quantized X, quantized Z, original index).
struct CanonicalKey {
    long long quantizedX     = 0;
    long long quantizedZ     = 0;
    int       candidateIndex = -1;
};

inline std::vector<CanonicalKey> BuildCanonicalOrder(const float* candidatePositionX,
                                                      const float* candidatePositionZ, int candidateCount,
                                                      float quantizationStep) {
    std::vector<CanonicalKey> keys(static_cast<std::size_t>(candidateCount));
    for (int index = 0; index < candidateCount; ++index) {
        keys[static_cast<std::size_t>(index)] = CanonicalKey{
            static_cast<long long>(std::lround(candidatePositionX[index] / quantizationStep)),
            static_cast<long long>(std::lround(candidatePositionZ[index] / quantizationStep)),
            index };
    }
    std::sort(keys.begin(), keys.end(), [](const CanonicalKey& a, const CanonicalKey& b) {
        if (a.quantizedX != b.quantizedX) return a.quantizedX < b.quantizedX;
        if (a.quantizedZ != b.quantizedZ) return a.quantizedZ < b.quantizedZ;
        return a.candidateIndex < b.candidateIndex;
    });
    return keys;
}

struct MatchTuple {
    float distanceSquared = 0.0f;
    int   slotIndex       = -1;
    int   candidateIndex  = -1;
    int   canonicalRank   = 0;
};

// Every in-tolerance (slot, candidate) tuple across `seedIndex`'s orbit's non-seed slots, via one
// grid bucket lookup per slot — the ONE bucket-lookup-per-query-point pattern Picking_UI::PickMarker
// already uses.
inline std::vector<MatchTuple> GatherSeedMatchTuples(const Data::SpatialGrid& grid,
        const std::vector<bool>& consumed, const std::vector<int>& canonicalRank,
        const WorldSymmetryOrbitPoint* orbitPoints, int orbitCount, int seedIndex,
        const float* candidatePositionX, const float* candidatePositionZ, float distanceTolerance) {
    const float toleranceSquared = distanceTolerance * distanceTolerance;
    std::vector<MatchTuple> tuples;
    for (int slotIndex = 1; slotIndex < orbitCount; ++slotIndex) {
        const float slotX = orbitPoints[slotIndex].worldPositionX;
        const float slotZ = orbitPoints[slotIndex].worldPositionZ;
        const int cellIndex = grid.CellIndexAt(slotX, slotZ);
        for (std::int32_t position = grid.BucketBegin(cellIndex); position < grid.BucketEnd(cellIndex);
            ++position) {
            const std::int32_t candidateIndex = grid.InstanceIndexAt(position);
            if (candidateIndex == seedIndex || consumed[static_cast<std::size_t>(candidateIndex)]) continue;
            const float deltaX = candidatePositionX[candidateIndex] - slotX;
            const float deltaZ = candidatePositionZ[candidateIndex] - slotZ;
            const float distanceSquared = deltaX * deltaX + deltaZ * deltaZ;
            if (distanceSquared > toleranceSquared) continue;
            tuples.push_back(MatchTuple{ distanceSquared, slotIndex, candidateIndex,
                                         canonicalRank[static_cast<std::size_t>(candidateIndex)] });
        }
    }
    return tuples;
}

// Claims `tuples` greedily (smallest distance first, canonical-order tie-break): each slot claimed
// at most once, each candidate claimed at most once. `outMatchedBySlot` is sized `orbitCount`, slot
// 0 always -1 (never a match target). Returns whether every slot (1..orbitCount-1) got claimed.
inline bool ClaimSeedMatchTuples(std::vector<MatchTuple> tuples, int orbitCount,
                                 std::vector<int>& outMatchedBySlot) {
    std::sort(tuples.begin(), tuples.end(), [](const MatchTuple& a, const MatchTuple& b) {
        if (a.distanceSquared != b.distanceSquared) return a.distanceSquared < b.distanceSquared;
        return a.canonicalRank < b.canonicalRank;
    });
    std::vector<bool> slotClaimed(static_cast<std::size_t>(orbitCount), false);
    std::vector<int>  claimedCandidates;   // small — bounded by tuples.size(), not candidateCount
    outMatchedBySlot.assign(static_cast<std::size_t>(orbitCount), -1);
    int claimedCount = 0;
    for (const MatchTuple& tuple : tuples) {
        if (slotClaimed[static_cast<std::size_t>(tuple.slotIndex)]) continue;
        if (std::find(claimedCandidates.begin(), claimedCandidates.end(), tuple.candidateIndex)
            != claimedCandidates.end()) continue;
        slotClaimed[static_cast<std::size_t>(tuple.slotIndex)] = true;
        claimedCandidates.push_back(tuple.candidateIndex);
        outMatchedBySlot[static_cast<std::size_t>(tuple.slotIndex)] = tuple.candidateIndex;
        ++claimedCount;
    }
    return claimedCount == orbitCount - 1;
}

} // namespace MatchDetail
} // namespace Pipeline
} // namespace SanmapGen
