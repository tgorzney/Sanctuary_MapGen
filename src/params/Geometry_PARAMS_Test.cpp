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

    // Tab rebuild B: terrainMinHeight is the promoted vertical floor (Constitution §8).
    Geometry band;
    if (band.terrainMinHeight != 0.0f) { std::printf("FAIL terrainMinHeight default\n"); ++failures; }
    if (band.TerrainHeightSpan() != 128.0f) { std::printf("FAIL default span\n"); ++failures; }
    band.terrainMinHeight = 32.0f;
    if (band.TerrainHeightSpan() != 96.0f) { std::printf("FAIL raised floor span\n"); ++failures; }
    if (!band.IsValid()) { std::printf("FAIL floor under ceiling is valid\n"); ++failures; }
    band.terrainMinHeight = 200.0f;
    if (band.IsValid()) { std::printf("FAIL floor above ceiling is invalid\n"); ++failures; }

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
