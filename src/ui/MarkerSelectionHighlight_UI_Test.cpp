// MarkerSelectionHighlight_UI_Test.cpp — acceptance test for ComputeManualMarkerSelectionHighlight
// (ARCH §19.19). Pure logic, no imgui, no window — mirrors MarkerDragGesture_UI_Test.cpp's own
// fixture style (a small Params::Geometry, hand-built MarkerInstanceGroup/MarkerTransform fixtures).
#include "MarkerSelectionHighlight_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

bool Contains(const std::vector<int>& values, int target) {
    for (int value : values) if (value == target) return true;
    return false;
}

// extent = mapSize = 10 (VertexSize() - 1), worldUnitsPerCell = 1 -> world units == cell units, so
// the mirror math below (`extent - position`) is exact and easy to hand-verify — same fixture as
// MarkerDragGesture_UI_Test.cpp's own MakeTestGeometry.
Params::Geometry MakeTestGeometry() {
    Params::Geometry geometry;
    geometry.mapSize = 10;
    geometry.worldUnitsPerCell = 1.0f;
    return geometry;
}

Params::MarkerTransform MakeTransform(int instanceIdentifier, float x, float z,
                                      int symmetryGroupIdentifier = 0, int layerIndex = 0) {
    Params::MarkerTransform transform;
    transform.instanceIdentifier = instanceIdentifier;
    transform.transform.positionX = x;
    transform.transform.positionZ = z;
    transform.symmetryGroupIdentifier = symmetryGroupIdentifier;
    transform.layerIndex = layerIndex;
    return transform;
}

const std::vector<Params::MarkerInstanceLayer> kNoLayers;

void RunNoSelectionChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform(1, 2.0f, 2.0f));
    const Params::Geometry geometry = MakeTestGeometry();

    const std::vector<int> result = ComputeManualMarkerSelectionHighlight(
        markers, kNoLayers, geometry, Params::SymmetryAxis::MirrorAcrossX, 3, 0.5f, -1);
    Check(result.empty(), "selectedInstanceIdentifier == -1 returns an empty vector");
}

void RunStaleIdentifierChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform(1, 2.0f, 2.0f));
    const Params::Geometry geometry = MakeTestGeometry();

    const std::vector<int> result = ComputeManualMarkerSelectionHighlight(
        markers, kNoLayers, geometry, Params::SymmetryAxis::MirrorAcrossX, 3, 0.5f, /*stale=*/999);
    Check(result.empty(), "a stale identifier (not present in markers) returns an empty vector, never a crash");
}

// Also exercises the "orbitCount <= 1 returns only the selected instance" rule (SymmetryAxis::None
// never produces a sibling), checked by SIZE alone (no mock of BuildWorldSymmetryOrbit exists or is
// needed — this codebase has no mocking convention; a real Params::Geometry fixture is used).
void RunNoneAxisSingleResultChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform(5, 2.0f, 2.0f));
    const Params::Geometry geometry = MakeTestGeometry();

    const std::vector<int> result = ComputeManualMarkerSelectionHighlight(
        markers, kNoLayers, geometry, Params::SymmetryAxis::None, 3, 0.5f, 5);
    Check(static_cast<int>(result.size()) == 1, "SymmetryAxis::None (orbitCount == 1) returns exactly one element");
    Check(!result.empty() && result[0] == 5, "and that element is the selected instance's own identifier");
}

// The freshly-authored, never-dragged case (ARCH §19.19's own explicit callout): a transform with
// symmetryGroupIdentifier == 0 (the "Add Marker" default) under a mirrored layer, with a SECOND
// transform in the SAME group already sitting at the exact mirrored position (also
// symmetryGroupIdentifier == 0) — position-driven orbit matching finds it, an equality-on-
// symmetryGroupIdentifier approach would have missed it entirely.
void RunNeverDraggedSiblingMatchChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeTransform(10, 3.0f, 5.0f, /*symmetryGroupIdentifier=*/0));
    markers[0].transforms.push_back(MakeTransform(11, 7.0f, 5.0f, /*symmetryGroupIdentifier=*/0));   // exact mirror: 10 - 3 = 7
    const Params::Geometry geometry = MakeTestGeometry();

    const std::vector<int> result = ComputeManualMarkerSelectionHighlight(
        markers, kNoLayers, geometry, Params::SymmetryAxis::MirrorAcrossX, 3, 0.5f, 10);
    Check(Contains(result, 10) && Contains(result, 11),
          "position-driven orbit matching finds a never-dragged, symmetryGroupIdentifier==0 sibling");
    Check(static_cast<int>(result.size()) == 2, "exactly the selected instance plus its one sibling");
}

// A sibling sitting at the exact mirrored position but in a DIFFERENT MarkerInstanceGroup is never
// matched — the same-group scoping rule.
void RunDifferentGroupNotMatchedChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(2);
    markers[0].transforms.push_back(MakeTransform(20, 3.0f, 5.0f));
    markers[1].transforms.push_back(MakeTransform(21, 7.0f, 5.0f));   // exact mirror, but a different group
    const Params::Geometry geometry = MakeTestGeometry();

    const std::vector<int> result = ComputeManualMarkerSelectionHighlight(
        markers, kNoLayers, geometry, Params::SymmetryAxis::MirrorAcrossX, 3, 0.5f, 20);
    Check(!Contains(result, 21), "a sibling in a DIFFERENT MarkerInstanceGroup is never matched");
    Check(static_cast<int>(result.size()) == 1, "leaving only the selected instance itself");
}

// Tolerance boundary: strictly inside matches, strictly outside does not, exactly at the boundary
// matches (`<=`, mirroring HitTestManualMarkers' own boundary convention).
void RunToleranceBoundaryChecks() {
    const Params::Geometry geometry = MakeTestGeometry();
    const float distanceTolerance = 0.5f;

    {
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].transforms.push_back(MakeTransform(30, 3.0f, 5.0f));
        markers[0].transforms.push_back(MakeTransform(31, 7.0f + 0.3f, 5.0f));   // 0.3 inside 0.5
        const std::vector<int> result = ComputeManualMarkerSelectionHighlight(
            markers, kNoLayers, geometry, Params::SymmetryAxis::MirrorAcrossX, 3, distanceTolerance, 30);
        Check(Contains(result, 31), "a sibling strictly within tolerance is matched");
    }
    {
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].transforms.push_back(MakeTransform(32, 3.0f, 5.0f));
        markers[0].transforms.push_back(MakeTransform(33, 7.0f + 0.6f, 5.0f));   // 0.6 outside 0.5
        const std::vector<int> result = ComputeManualMarkerSelectionHighlight(
            markers, kNoLayers, geometry, Params::SymmetryAxis::MirrorAcrossX, 3, distanceTolerance, 32);
        Check(!Contains(result, 33), "a sibling strictly outside tolerance is not matched");
    }
    {
        std::vector<Params::MarkerInstanceGroup> markers(1);
        markers[0].transforms.push_back(MakeTransform(34, 3.0f, 5.0f));
        markers[0].transforms.push_back(MakeTransform(35, 7.0f + 0.5f, 5.0f));   // exactly at the boundary
        const std::vector<int> result = ComputeManualMarkerSelectionHighlight(
            markers, kNoLayers, geometry, Params::SymmetryAxis::MirrorAcrossX, 3, distanceTolerance, 34);
        Check(Contains(result, 35), "a sibling exactly at the tolerance boundary IS matched (<=)");
    }
}

} // namespace

int main() {
    RunNoSelectionChecks();
    RunStaleIdentifierChecks();
    RunNoneAxisSingleResultChecks();
    RunNeverDraggedSiblingMatchChecks();
    RunDifferentGroupNotMatchedChecks();
    RunToleranceBoundaryChecks();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
