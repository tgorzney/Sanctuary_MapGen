// MapRecipe_PARAMS_Test.cpp — acceptance test for M1-5 (the recipe aggregate).
//   g++ -O2 -std=c++17 -fsanitize=address,undefined MapRecipe_PARAMS_Test.cpp -o t && ./t
#include "MapRecipe_PARAMS.h"
#include <cstdio>

using namespace SanmapGen::Params;

int main() {
    int failures = 0;
    MapRecipe recipe;
    recipe.geometry.mapSize = 512;
    recipe.geometry.seed = 99u;

    GeoLayer group; group.layers.resize(2);
    recipe.layerStack.geoLayers.push_back(group);
    recipe.markerRuleLayers.resize(3);
    recipe.propRules.resize(1);

    if (recipe.geometry.VertexSize() != 513) { std::printf("FAIL geometry\n"); ++failures; }
    if (!recipe.IsValid()) { std::printf("FAIL valid\n"); ++failures; }
    if (recipe.layerStack.GetFlatLayers().size() != 2) { std::printf("FAIL flat via recipe\n"); ++failures; }
    if (recipe.markerRuleLayers.size() != 3 || recipe.propRules.size() != 1) { std::printf("FAIL rule counts\n"); ++failures; }
    recipe.geometry.mapSize = 0;
    if (recipe.IsValid()) { std::printf("FAIL invalid geometry\n"); ++failures; }

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
