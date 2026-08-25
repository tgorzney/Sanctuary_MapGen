// MarkersTab_ManualLayers_UI_Test.cpp — STEP106 acceptance coverage for the two pure gate/
// quantize functions this ticket adds to MarkersTab_ManualLayers_UI.h: `IsMarkerInstanceLayerLocked`
// (§3) and `QuantizeMarkerPositionToLayerGrid` (§6). Pure logic only — no imgui frame, no window,
// no GL context. Mirrors MarkerLayerIndexRepair_UI_Test.cpp's assertion shape.
#include "MarkersTab_ManualLayers_UI.h"
#include <cmath>
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

bool NearlyEqual(float a, float b) { return std::fabs(a - b) <= 0.001f; }

// Empty vector, out-of-range indices against a non-empty vector, and the ordinary in-range cases.
void RunIsMarkerInstanceLayerLockedChecks() {
    std::vector<Params::MarkerInstanceLayer> emptyLayers;
    Check(!IsMarkerInstanceLayerLocked(emptyLayers, 0), "an empty vector resolves to false at any index");
    Check(!IsMarkerInstanceLayerLocked(emptyLayers, -1), "and at a negative index too");

    std::vector<Params::MarkerInstanceLayer> markerLayers(2);
    markerLayers[1].bLocked = true;
    Check(!IsMarkerInstanceLayerLocked(markerLayers, 0), "index 0 (unlocked) resolves to false");
    Check(IsMarkerInstanceLayerLocked(markerLayers, 1), "index 1 (locked) resolves to true");
    Check(!IsMarkerInstanceLayerLocked(markerLayers, -1),
          "a negative index against a non-empty vector still resolves to false");
    Check(!IsMarkerInstanceLayerLocked(markerLayers, 2),
          "an index at size() resolves to false, never trusted as 'locked'");
}

// bGridSnapEnabled == false leaves the position untouched regardless of value; a non-positive
// gridSnapSizeWorldUnits is a defensive no-op, not a divide-by-zero; an out-of-range layerIndex
// leaves the position unchanged too.
void RunQuantizeMarkerPositionToLayerGridChecks() {
    std::vector<Params::MarkerInstanceLayer> markerLayers(3);
    markerLayers[0].bGridSnapEnabled = false;
    markerLayers[0].gridSnapSizeWorldUnits = 4.0f;
    markerLayers[1].bGridSnapEnabled = true;
    markerLayers[1].gridSnapSizeWorldUnits = 4.0f;
    markerLayers[2].bGridSnapEnabled = true;
    markerLayers[2].gridSnapSizeWorldUnits = 0.0f;

    float worldX = 6.1f, worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, 0, worldX, worldZ);
    Check(NearlyEqual(worldX, 6.1f) && NearlyEqual(worldZ, -3.9f),
          "grid snap off on the layer leaves the position unchanged regardless of value");

    worldX = 6.1f; worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, 1, worldX, worldZ);
    Check(NearlyEqual(worldX, 8.0f) && NearlyEqual(worldZ, -4.0f),
          "(6.1, -3.9) snaps to the nearest 4.0-unit cell: (8.0, -4.0)");

    // Tie case: std::round is ties-away-from-zero, so 2.0 / 4.0 == 0.5 rounds to 1.0, landing on 4.0.
    worldX = 2.0f; worldZ = 0.0f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, 1, worldX, worldZ);
    Check(NearlyEqual(worldX, 4.0f), "an exact tie (2.0 against a 4.0 cell) rounds away from zero to 4.0");

    worldX = 6.1f; worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, 2, worldX, worldZ);
    Check(NearlyEqual(worldX, 6.1f) && NearlyEqual(worldZ, -3.9f),
          "a non-positive gridSnapSizeWorldUnits is a defensive no-op, not a divide-by-zero");

    worldX = 6.1f; worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, -1, worldX, worldZ);
    Check(NearlyEqual(worldX, 6.1f) && NearlyEqual(worldZ, -3.9f),
          "an out-of-range layerIndex leaves the position unchanged");
    worldX = 6.1f; worldZ = -3.9f;
    QuantizeMarkerPositionToLayerGrid(markerLayers, 3, worldX, worldZ);
    Check(NearlyEqual(worldX, 6.1f) && NearlyEqual(worldZ, -3.9f),
          "and so does an index at size()");
}

} // namespace

int main() {
    RunIsMarkerInstanceLayerLockedChecks();
    RunQuantizeMarkerPositionToLayerGridChecks();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
