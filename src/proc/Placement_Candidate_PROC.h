// Placement_Candidate_PROC.h — one Poisson-disk candidate and its deterministic order.
// Layer: PROC. Candidates are drawn on a grid of cell size spacing/sqrt(2) (so one cell can
// hold at most one accepted point) and jittered by the position hash — the blue-noise dart
// throw. Acceptance walks them in sort-key order; ties break on the candidate index, which
// is unique, so the ordering is TOTAL and the sort result cannot depend on the sort
// implementation (DETERMINISM_SPEC: same seed -> same map, on any machine).
#pragma once
#include <cstdint>

namespace SanmapGen {
namespace Proc {

struct ScatterCandidate {
    float        positionX = 0.0f;      // cell coordinates (not world units)
    float        positionY = 0.0f;
    float        sortKey   = 0.0f;      // lower is placed first
    float        gateWeight = 0.0f;
    uint32_t     positionHash = 0u;
    int          candidateIndex = 0;    // candidate-grid index: the unique tiebreak
};

inline bool ScatterCandidateOrder(const ScatterCandidate& first, const ScatterCandidate& second) {
    if (first.sortKey < second.sortKey) return true;
    if (second.sortKey < first.sortKey) return false;
    return first.candidateIndex < second.candidateIndex;
}

} // namespace Proc
} // namespace SanmapGen
