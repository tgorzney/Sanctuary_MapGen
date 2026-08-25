// MarkerSymmetryDetection_PIPELINE_Test.cpp — acceptance coverage for
// Pipeline::FindMarkerSymmetryMatches (STEP107 §3a): basic confirmed-pair detection, unmatched-slot
// reporting with no force-assignment (acceptance test 3), determinism under candidate reordering
// (acceptance test 7), and O(n) performance shape at a synthetic stress size (acceptance test 8) —
// the query-level half of STEP107's acceptance test. The command-level (UI-layer, id-allocation)
// half lives in MarkerSymmetryFixCommand_UI_Test.cpp. Pure: no imgui/GL.
#include "MarkerSymmetryDetection_PIPELINE.h"
#include "../params/Symmetry_PARAMS.h"
#include <chrono>
#include <cstdio>
#include <vector>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

// worldUnitsPerCell = 1 -> world units == cell units, extent = mapSize, mirror(x) = mapSize - x,
// hand-verifiable (same convention MarkerDragGesture_UI_Test.cpp's MakeTestGeometry uses).
Params::Geometry MakeTestGeometry(int mapSize = 10) {
    Params::Geometry geometry;
    geometry.mapSize = mapSize;
    geometry.worldUnitsPerCell = 1.0f;
    return geometry;
}

// Test 1 — a basic exact-mirror pair is confirmed as one match; the seed/matched indices resolve
// back to the two candidate array slots, in either role.
void RunBasicPairMatchChecks() {
    const Params::Geometry geometry = MakeTestGeometry();
    const float positionX[2] = { 2.0f, 8.0f };   // mirror(2) = 10 - 2 = 8
    const float positionZ[2] = { 3.0f, 3.0f };
    int unmatchedSlotCount = -1;
    const std::vector<Pipeline::MarkerSymmetryOrbitMatch> matches = Pipeline::FindMarkerSymmetryMatches(
        geometry, Params::SymmetryAxis::MirrorAcrossX, 3, positionX, positionZ, 2, 0.5f, &unmatchedSlotCount);
    Check(matches.size() == 1, "an exact mirror pair confirms exactly one orbit");
    Check(unmatchedSlotCount == 0, "and reports zero unmatched slots");
    if (matches.empty()) return;
    Check(matches[0].matchedCandidateIndices.size() == 1, "the confirmed orbit has one matched slot");
    const int seed = matches[0].seedCandidateIndex;
    const int matched = matches[0].matchedCandidateIndices[0];
    Check((seed == 0 && matched == 1) || (seed == 1 && matched == 0),
          "the seed/matched pair is exactly the two candidates, in either role");
}

// Test 2 (acceptance test 3) — a lone candidate with no mirror partner is left unmatched, never
// force-assigned: zero confirmed matches, at least one unmatched slot reported.
void RunUnmatchedSlotReportingChecks() {
    const Params::Geometry geometry = MakeTestGeometry();
    const float positionX[1] = { 2.0f };
    const float positionZ[1] = { 3.0f };
    int unmatchedSlotCount = -1;
    const std::vector<Pipeline::MarkerSymmetryOrbitMatch> matches = Pipeline::FindMarkerSymmetryMatches(
        geometry, Params::SymmetryAxis::MirrorAcrossX, 3, positionX, positionZ, 1, 0.5f, &unmatchedSlotCount);
    Check(matches.empty(), "a lone candidate with no mirror partner confirms nothing");
    Check(unmatchedSlotCount >= 1, "and reports at least one unmatched slot");
}

// Test 3 (acceptance test 3) — a partner exists but sits just OUTSIDE distanceTolerance: still
// never force-matched, still reports an unmatched slot.
void RunOutOfToleranceChecks() {
    const Params::Geometry geometry = MakeTestGeometry();
    // True mirror of (2, 3) is (8, 3); this partner sits at (8.6, 3), 0.6 away — outside tolerance 0.5.
    const float positionX[2] = { 2.0f, 8.6f };
    const float positionZ[2] = { 3.0f, 3.0f };
    int unmatchedSlotCount = -1;
    const std::vector<Pipeline::MarkerSymmetryOrbitMatch> matches = Pipeline::FindMarkerSymmetryMatches(
        geometry, Params::SymmetryAxis::MirrorAcrossX, 3, positionX, positionZ, 2, 0.5f, &unmatchedSlotCount);
    Check(matches.empty(), "a candidate just outside tolerance is never force-matched");
    Check(unmatchedSlotCount >= 1, "and still reports an unmatched slot");
}

// Test 4 (acceptance test 7 — determinism) — the SAME positions/mask/tolerance, fed with the
// candidate array reversed, produce the same confirmed-group and unmatched-slot counts: results
// cannot depend on candidate array order.
void RunDeterminismChecks() {
    const Params::Geometry geometry = MakeTestGeometry();
    const int mask = Params::SymmetryAxis::MirrorAcrossX;
    const std::vector<float> forwardX = { 2.0f, 8.0f, 4.0f, 6.0f };   // two exact mirror pairs
    const std::vector<float> forwardZ = { 3.0f, 3.0f, 5.0f, 5.0f };
    const std::vector<float> reversedX(forwardX.rbegin(), forwardX.rend());
    const std::vector<float> reversedZ(forwardZ.rbegin(), forwardZ.rend());

    int forwardUnmatched = -1, reversedUnmatched = -1;
    const std::vector<Pipeline::MarkerSymmetryOrbitMatch> forwardMatches = Pipeline::FindMarkerSymmetryMatches(
        geometry, mask, 3, forwardX.data(), forwardZ.data(), 4, 0.5f, &forwardUnmatched);
    const std::vector<Pipeline::MarkerSymmetryOrbitMatch> reversedMatches = Pipeline::FindMarkerSymmetryMatches(
        geometry, mask, 3, reversedX.data(), reversedZ.data(), 4, 0.5f, &reversedUnmatched);

    Check(forwardMatches.size() == 2, "both mirror pairs are confirmed in forward order");
    Check(forwardMatches.size() == reversedMatches.size(),
          "reordering the candidate array does not change the confirmed group count");
    Check(forwardUnmatched == reversedUnmatched, "nor the unmatched slot count");
    Check(forwardUnmatched == 0, "and neither run leaves an unmatched slot for this fully-paired pool");
}

// Test 5 (acceptance test 8 — performance shape) — several hundred exact mirror pairs complete well
// within a generous wall-clock ceiling. A regression to an O(n^2) implementation at this input size
// would land in the seconds range (STEP107 §6's own performance note); 2 seconds is a loose,
// non-flaky ceiling for the intended O(n) shape.
void RunPerformanceShapeChecks() {
    const Params::Geometry geometry = MakeTestGeometry(2000);
    const int pairCount = 300;
    std::vector<float> positionX(static_cast<std::size_t>(pairCount) * 2);
    std::vector<float> positionZ(static_cast<std::size_t>(pairCount) * 2);
    for (int pairIndex = 0; pairIndex < pairCount; ++pairIndex) {
        const float x = static_cast<float>(pairIndex) * 3.0f + 1.0f;
        const float z = static_cast<float>(pairIndex % 17) * 5.0f + 1.0f;
        positionX[static_cast<std::size_t>(pairIndex) * 2]     = x;
        positionZ[static_cast<std::size_t>(pairIndex) * 2]     = z;
        positionX[static_cast<std::size_t>(pairIndex) * 2 + 1] = 2000.0f - x;   // exact mirror
        positionZ[static_cast<std::size_t>(pairIndex) * 2 + 1] = z;
    }
    int unmatchedSlotCount = -1;
    const auto startTime = std::chrono::steady_clock::now();
    const std::vector<Pipeline::MarkerSymmetryOrbitMatch> matches = Pipeline::FindMarkerSymmetryMatches(
        geometry, Params::SymmetryAxis::MirrorAcrossX, 3, positionX.data(), positionZ.data(),
        static_cast<int>(positionX.size()), 0.5f, &unmatchedSlotCount);
    const double elapsedSeconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();

    Check(static_cast<int>(matches.size()) == pairCount, "every synthetic pair is confirmed");
    Check(unmatchedSlotCount == 0, "with no unmatched slots at this stress size");
    Check(elapsedSeconds < 2.0, "an O(n) implementation completes this stress size in well under 2 seconds");
}

} // namespace

int main() {
    RunBasicPairMatchChecks();
    RunUnmatchedSlotReportingChecks();
    RunOutOfToleranceChecks();
    RunDeterminismChecks();
    RunPerformanceShapeChecks();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
