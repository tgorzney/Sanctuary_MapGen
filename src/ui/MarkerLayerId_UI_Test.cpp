// MarkerLayerId_UI_Test.cpp — acceptance test for `NextMarkerLayerId` (STEP60_MarkerInstanceLayer_
// PARAMS §2). No imgui frame, no window, no GL: the decision is pure by construction
// (MarkerLayerId_UI.h). Mirrors STEP56's `NextPropLayerId` coverage shape.
#include "MarkerLayerId_UI.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

int main() {
    std::vector<Params::MarkerInstanceLayer> emptyLayers;
    Check(Ui::NextMarkerLayerId(emptyLayers) == 0, "an empty markerLayers vector yields 0");

    std::vector<Params::MarkerInstanceLayer> sparseLayers;
    Params::MarkerInstanceLayer layerZero;
    layerZero.layerId = 0;
    Params::MarkerInstanceLayer layerTwo;
    layerTwo.layerId = 2;
    sparseLayers.push_back(layerZero);
    sparseLayers.push_back(layerTwo);
    Check(Ui::NextMarkerLayerId(sparseLayers) == 3, "ids {0,2} yield 3 (max-plus-one, not count-based)");

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
