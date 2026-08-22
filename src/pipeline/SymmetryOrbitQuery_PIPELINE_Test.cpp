// SymmetryOrbitQuery_PIPELINE_Test.cpp — acceptance test for BuildWorldSymmetryOrbit (STEP68).
// Pure: no imgui/GL, no marker types involved — proving the function is genuinely domain-agnostic
// (it only ever sees Params::Geometry, a mask, and a world position).
#include "SymmetryOrbitQuery_PIPELINE.h"
#include "../params/Symmetry_PARAMS.h"
#include <cstdio>
#include <cmath>

using namespace SanmapGen;

namespace {

bool NearlyEqual(float a, float b) { return std::fabs(a - b) < 0.001f; }

}

int main() {
    int failures = 0;

    Params::Geometry geometry;
    geometry.mapSize          = 256;
    geometry.worldUnitsPerCell = 2.0f;

    // 1. A single mirror axis off the mirror line -> a 2-point orbit.
    //    cell space: 40/2=20, extent=256, mirrored=236, world=236*2=472.
    {
        Pipeline::WorldSymmetryOrbitPoint points[Params::symmetryOrbitMaximum];
        int count = Pipeline::BuildWorldSymmetryOrbit(geometry, Params::SymmetryAxis::MirrorAcrossX, 0,
                                                       40.0f, 100.0f, points, Params::symmetryOrbitMaximum);
        if (count != 2) { std::printf("FAIL single-axis count %d\n", count); ++failures; }
        if (!NearlyEqual(points[0].worldPositionX, 40.0f) || !NearlyEqual(points[0].worldPositionZ, 100.0f))
            { std::printf("FAIL single-axis source point\n"); ++failures; }
        if (!NearlyEqual(points[1].worldPositionX, 472.0f) || !NearlyEqual(points[1].worldPositionZ, 100.0f))
            { std::printf("FAIL single-axis mirrored point\n"); ++failures; }
    }

    // 2. A combined mask (MirrorAcrossX | MirrorAcrossZ) -> a 4-point orbit.
    {
        const int mask = Params::SymmetryAxis::MirrorAcrossX | Params::SymmetryAxis::MirrorAcrossZ;
        Pipeline::WorldSymmetryOrbitPoint points[Params::symmetryOrbitMaximum];
        int count = Pipeline::BuildWorldSymmetryOrbit(geometry, mask, 0, 40.0f, 100.0f, points,
                                                       Params::symmetryOrbitMaximum);
        if (count != 4) { std::printf("FAIL combined-mask count %d\n", count); ++failures; }
        const float expectedX[4] = { 40.0f, 472.0f, 40.0f, 472.0f };
        const float expectedZ[4] = { 100.0f, 100.0f, 412.0f, 412.0f };
        for (int index = 0; index < 4 && index < count; ++index) {
            if (!NearlyEqual(points[index].worldPositionX, expectedX[index])
                || !NearlyEqual(points[index].worldPositionZ, expectedZ[index])) {
                std::printf("FAIL combined-mask point %d\n", index);
                ++failures;
            }
        }
    }

    // 3. A position ON the mirror line collapses to a 1-point orbit (extent=256 cells,
    //    mirror line at cell 128, world 128*2=256).
    {
        Pipeline::WorldSymmetryOrbitPoint points[Params::symmetryOrbitMaximum];
        int count = Pipeline::BuildWorldSymmetryOrbit(geometry, Params::SymmetryAxis::MirrorAcrossX, 0,
                                                       256.0f, 100.0f, points, Params::symmetryOrbitMaximum);
        if (count != 1) { std::printf("FAIL mirror-line collapse count %d\n", count); ++failures; }
        if (!NearlyEqual(points[0].worldPositionX, 256.0f) || !NearlyEqual(points[0].worldPositionZ, 100.0f))
            { std::printf("FAIL mirror-line collapse point\n"); ++failures; }
    }

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
