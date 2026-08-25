// LayerStack_PARAMS_Test.cpp — acceptance test for M1-3 (GeoLayer + LayerStack) and STEP152's
// bDisabled generation-inclusion gate + HasActiveProceduralLayer().
//   g++ -O2 -std=c++17 -fsanitize=address,undefined LayerStack_PARAMS_Test.cpp -o t && ./t
#include "LayerStack_PARAMS.h"
#include <cstdio>

using namespace SanmapGen::Params;

int main() {
    int failures = 0;
    LayerStack stack;

    GeoLayer groupA; groupA.name = "A"; groupA.layers.resize(2);
    groupA.layers[0].frequency = 1.0f;
    groupA.layers[1].bDisabled = true;                 // disabled layer -> skipped
    groupA.layers[1].bEnabled  = true;                 // bEnabled is UI-only; must have NO effect

    GeoLayer groupB; groupB.name = "B"; groupB.bDisabled = true; groupB.layers.resize(1); // whole group off
    groupB.bEnabled = true;                             // same: bEnabled alone must not re-include it

    GeoLayer groupC; groupC.name = "C"; groupC.layers.resize(1);
    groupC.layers[0].frequency = 3.0f;
    groupC.layers[0].bEnabled  = false;                 // hidden (UI) but NOT disabled -> still flat

    stack.geoLayers = { groupA, groupB, groupC };

    std::vector<const Layer*> flat = stack.GetFlatLayers();
    if (flat.size() != 2) { std::printf("FAIL flat count %zu\n", flat.size()); ++failures; }
    else {
        if (flat[0]->frequency != 1.0f) { std::printf("FAIL flat order 0\n"); ++failures; }
        if (flat[1]->frequency != 3.0f) { std::printf("FAIL flat order 1\n"); ++failures; }
    }
    if (stack.TotalLayerCount() != 4) { std::printf("FAIL total count\n"); ++failures; }
    if (stack.simulationGrouping != SimulationGrouping::Unified) { std::printf("FAIL default grouping\n"); ++failures; }

    // --- HasActiveProceduralLayer(): true the moment at least one flattened layer is
    // generation-included, unbaked and not the always-flat NoiseType::None sentinel.
    {
        LayerStack activeStack;
        GeoLayer group; group.layers.resize(1);
        group.layers[0].noiseType = NoiseType::OpenSimplex2;
        group.layers[0].bBaked    = false;
        activeStack.geoLayers = { group };
        if (!activeStack.HasActiveProceduralLayer())
            { std::printf("FAIL HasActiveProceduralLayer true case\n"); ++failures; }
    }
    {
        // Baked -> not active, even with a real recipe still attached.
        LayerStack bakedStack;
        GeoLayer group; group.layers.resize(1);
        group.layers[0].noiseType = NoiseType::OpenSimplex2;
        group.layers[0].bBaked    = true;
        bakedStack.geoLayers = { group };
        if (bakedStack.HasActiveProceduralLayer())
            { std::printf("FAIL HasActiveProceduralLayer baked case\n"); ++failures; }
    }
    {
        // NoiseType::None -> the always-flat, recipe-less sentinel; never counts as active.
        LayerStack noneStack;
        GeoLayer group; group.layers.resize(1);
        group.layers[0].noiseType = NoiseType::None;
        group.layers[0].bBaked    = false;
        noneStack.geoLayers = { group };
        if (noneStack.HasActiveProceduralLayer())
            { std::printf("FAIL HasActiveProceduralLayer NoiseType::None case\n"); ++failures; }
    }
    {
        // Disabled -> excluded from GetFlatLayers() entirely, so it cannot count as active either,
        // even though its own recipe would otherwise qualify.
        LayerStack disabledStack;
        GeoLayer group; group.layers.resize(1);
        group.layers[0].noiseType = NoiseType::OpenSimplex2;
        group.layers[0].bBaked    = false;
        group.layers[0].bDisabled = true;
        disabledStack.geoLayers = { group };
        if (disabledStack.HasActiveProceduralLayer())
            { std::printf("FAIL HasActiveProceduralLayer disabled case\n"); ++failures; }
    }
    {
        // Every layer baked-or-disabled-or-recipe-less across a multi-layer stack -> still false.
        LayerStack everyKindInactive;
        GeoLayer group; group.layers.resize(3);
        group.layers[0].bBaked    = true;                       // baked
        group.layers[1].bDisabled = true;                       // disabled
        group.layers[2].noiseType = NoiseType::None;             // recipe-less
        everyKindInactive.geoLayers = { group };
        if (everyKindInactive.HasActiveProceduralLayer())
            { std::printf("FAIL HasActiveProceduralLayer mixed-inactive case\n"); ++failures; }
    }

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
