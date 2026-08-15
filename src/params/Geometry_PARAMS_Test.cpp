// Geometry_PARAMS_Test.cpp — acceptance test for Geometry_PARAMS (M1-1).
//   g++ -O2 -std=c++17 Geometry_PARAMS_Test.cpp -o t && ./t
#include "Geometry_PARAMS.h"
#include <cstdio>

using namespace SanmapGen::Params;

int main() {
    int failures = 0;
    Geometry geometry;
    if (geometry.VertexSize() != 257) { std::printf("FAIL VertexSize\n"); ++failures; }
    if (geometry.VertexCount() != 257ull * 257ull) { std::printf("FAIL VertexCount\n"); ++failures; }
    if (!geometry.IsValid()) { std::printf("FAIL default valid\n"); ++failures; }
    // M5-0a: worldUnitsPerCell relocated here from Proc::PlacementConstants, same default.
    if (geometry.worldUnitsPerCell != 1.0f) { std::printf("FAIL worldUnitsPerCell default\n"); ++failures; }
    geometry.mapSize = 0;
    if (geometry.IsValid()) { std::printf("FAIL invalid mapSize\n"); ++failures; }
    geometry.mapSize = 512; geometry.terrainMaxHeight = 0.0f;
    if (geometry.IsValid()) { std::printf("FAIL invalid height\n"); ++failures; }

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
