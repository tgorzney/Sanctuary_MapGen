// LayerStack_PARAMS_Test.cpp — acceptance test for M1-3 (GeoLayer + LayerStack).
//   g++ -O2 -std=c++17 -fsanitize=address,undefined LayerStack_PARAMS_Test.cpp -o t && ./t
#include "LayerStack_PARAMS.h"
#include <cstdio>

using namespace SanmapGen::Params;

int main() {
    int failures = 0;
    LayerStack stack;

    GeoLayer groupA; groupA.name = "A"; groupA.layers.resize(2);
    groupA.layers[0].frequency = 1.0f;
    groupA.layers[1].bEnabled = false;                 // disabled layer -> skipped

    GeoLayer groupB; groupB.name = "B"; groupB.bEnabled = false; groupB.layers.resize(1); // whole group off

    GeoLayer groupC; groupC.name = "C"; groupC.layers.resize(1);
    groupC.layers[0].frequency = 3.0f;

    stack.geoLayers = { groupA, groupB, groupC };

    std::vector<const Layer*> flat = stack.GetFlatLayers();
    if (flat.size() != 2) { std::printf("FAIL flat count %zu\n", flat.size()); ++failures; }
    else {
        if (flat[0]->frequency != 1.0f) { std::printf("FAIL flat order 0\n"); ++failures; }
        if (flat[1]->frequency != 3.0f) { std::printf("FAIL flat order 1\n"); ++failures; }
    }
    if (stack.TotalLayerCount() != 4) { std::printf("FAIL total count\n"); ++failures; }
    if (stack.simulationGrouping != SimulationGrouping::Unified) { std::printf("FAIL default grouping\n"); ++failures; }

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
