// MarkerSymmetryDetection_PIPELINE.h — pure, read-only detection of already-mirrored candidate
// positions (STEP107's "Fix Symmetry" command). Layer: PIPELINE, same category as
// SymmetryOrbitQuery_PIPELINE.h (ARCH_16_03_ModuleBoundaryChain.md §16.3): a stateless query with no
// DAG/dirty-hash participation, callable on demand from UI. Wraps
// Pipeline::BuildWorldSymmetryOrbit (itself a wrapper over Proc::BuildSymmetryOrbit) as the SOLE
// source of mirror geometry — this file adds no independent mirror math, only the
// candidate-to-orbit-slot matching STEP94's MatchCorrespondenceToOrbit already established the shape
// of, scaled to run once per candidate seed across a whole pool instead of once per drag gesture.
// Writes nothing to PARAMS — see MarkerSymmetryFixCommand_UI.h for the UI-layer function that
// performs the actual symmetryGroupIdentifier writes this query's results feed.
#pragma once
#include <vector>
#include "../params/Geometry_PARAMS.h"

namespace SanmapGen {
namespace Pipeline {

// One fully-confirmed orbit found in the candidate pool. `seedCandidateIndex` plus one entry in
// `matchedCandidateIndices` per non-seed orbit slot the seed's orbit produced — every index is an
// offset into the CALLER's own candidatePositionX/candidatePositionZ arrays, not a PARAMS index of
// any kind (this file knows nothing about MarkerTransform).
struct MarkerSymmetryOrbitMatch {
    int              seedCandidateIndex = -1;
    std::vector<int> matchedCandidateIndices;
};

// The quantization step (world units) used to build the canonical, insertion-order-independent
// candidate/tie-break ordering below. Deliberately much finer than any realistic position
// difference (so it never conflates two genuinely-distinct positions), and independent of
// `distanceTolerance` (a bucketing/ordering constant, not a match-acceptance one).
constexpr float positionQuantizationStep = 0.01f;

// Finds every FULLY-matched symmetry orbit among `candidateCount` candidate positions (parallel
// arrays `candidatePositionX`/`candidatePositionZ`, world space, already restricted by the caller to
// one layer's own in-scope markers) under `symmetryMask`/`radialSymmetryRepeatCount`.
//
// Processes candidates as seeds in a CANONICAL order independent of array/insertion order: sorted by
// (round(positionX / positionQuantizationStep), round(positionZ / positionQuantizationStep),
// original candidate index). For each seed not yet consumed by an earlier-confirmed orbit:
//   1. Compute its orbit via BuildWorldSymmetryOrbit(geometry, symmetryMask,
//      radialSymmetryRepeatCount, seed.x, seed.z, orbitPoints, symmetryOrbitMaximum). Slot 0 is
//      always the seed itself and is never a match target. If the returned count is <= 1 (no
//      mirrors under this mask), skip — nothing to detect for this seed, no unmatched-slot
//      contribution.
//   2. For each slot 1..orbitCount-1: ONE Data::SpatialGrid bucket lookup at that slot's world point
//      (grid built once, up front, over the full unconsumed candidate pool), collecting every
//      unconsumed candidate in that bucket within `distanceTolerance` (squared-distance compare, no
//      sqrt) as a (slotIndex, candidateIndex, distanceSquared) tuple. This is the ONE
//      bucket-lookup-per-query-point pattern Picking_UI::PickMarker already uses — same accepted
//      single-cell-lookup posture, including its edge case (a genuine in-tolerance match that falls
//      just across a cell boundary from the query point can be missed).
//   3. Sort THIS seed's own tuples by distanceSquared ascending, tied-broken by the same canonical
//      (quantized position, index) order as step 1's seed ordering. Claim greedily, smallest first:
//      each slot claimed at most once, each candidate claimed at most once — IDENTICAL claim loop
//      shape to MarkerOrbitCorrespondence_UI.cpp's MatchCorrespondenceToOrbit, reimplemented locally
//      (PIPELINE may not depend on UI, ARCH §3.1).
//   4. If every slot got claimed: this seed's orbit is CONFIRMED. Record a MarkerSymmetryOrbitMatch
//      {seed, claimed candidates in slot order}. Remove the seed and every claimed candidate from
//      the unconsumed pool.
//   5. Otherwise: record nothing, add (orbitCount - 1 - claimedCount) to the running unmatched-slot
//      total, and leave every considered candidate back in the unconsumed pool for a later seed's
//      consideration.
//
// Correctness note on WHY confirmed-only consumption avoids double-detection: two markers A, B that
// mirror each other are each other's sole match. Whichever of the two sorts first in the canonical
// seed order is processed first, confirms {A, B}, and consumes BOTH — so the second one is never
// later reprocessed as its own seed. This makes seed order matter only for WHICH candidate is
// nominally "the seed" of a confirmed pair/set (an arbitrary, deterministic choice with no
// observable effect) while still guaranteeing every physical orbit is detected and reported exactly
// once, never twice.
//
// `outUnmatchedSlotCount`, if non-null, receives the total from step 5 across every attempted
// (orbitCount > 1) but not-fully-confirmed seed.
std::vector<MarkerSymmetryOrbitMatch> FindMarkerSymmetryMatches(
    const Params::Geometry& geometry, int symmetryMask, int radialSymmetryRepeatCount,
    const float* candidatePositionX, const float* candidatePositionZ, int candidateCount,
    float distanceTolerance, int* outUnmatchedSlotCount);

} // namespace Pipeline
} // namespace SanmapGen
