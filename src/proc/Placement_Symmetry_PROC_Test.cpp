// Placement_Symmetry_PROC_Test.cpp — acceptance test for symmetry clones and the basic
// AI-analyzability check (spawns reachable). Build with MSVC from src/proc:
//   cl /EHsc /std:c++17 /O2 Placement_Symmetry_PROC_Test.cpp Placement_PROC.cpp
//      Placement_Hash_PROC.cpp Placement_Rules_PROC.cpp Placement_Fields_PROC.cpp
//      Placement_Metrics_PROC.cpp Placement_Scatter_PROC.cpp Placement_Accept_PROC.cpp
//      Placement_Emit_PROC.cpp Placement_Gpu_PROC.cpp ..\sys\GpuResource_Program_SYS.cpp
//      ..\sys\GpuResource_Buffer_SYS.cpp ..\sys\GpuResource_ProgramParts_SYS.cpp
//      ..\sys\GpuGlFunctions_SYS.cpp opengl32.lib
#include "Placement_PROC.h"
#include "Placement_Test_Terrain.h"
#include <cstdio>
#include <cmath>
#include <vector>

using namespace SanmapGen;

static int failures = 0;
static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failures; }
}

static Params::MapRecipe MakeSymmetricRecipe(int symmetryMask) {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = PlacementTest::mapSize;
    recipe.geometry.seed = 4242u;
    recipe.geometry.terrainMaxHeight = 128.0f;
    recipe.globalSymmetryMask = symmetryMask;

    Params::MarkerRule spawnRule;
    spawnRule.category = Params::MarkerCategory::Spawn;
    spawnRule.count = 4;
    spawnRule.clearanceSpacing = 24.0f;
    spawnRule.mapEdgePadding = 10;
    spawnRule.minHeight = 0.4f; spawnRule.maxHeight = 0.6f;
    spawnRule.maxSlope = 10.0f;
    spawnRule.bRandomSelection = true;
    spawnRule.bSymmetryUseGlobal = true;
    spawnRule.transform = PlacementTest::MakeTransform("m002", 1.0f, 1.0f);
    recipe.markerRules.push_back(spawnRule);
    return recipe;
}

// Is there an instance at (targetX, targetY), within half a cell?
static bool HasInstanceAt(const Data::PlacementInstances& instances, float targetX, float targetY) {
    for (std::size_t index = 0; index < instances.Count(); ++index) {
        const float deltaX = instances.positionX[index] - targetX;
        const float deltaY = instances.positionZ[index] - targetY;
        if (deltaX * deltaX + deltaY * deltaY < 0.25f) return true;
    }
    return false;
}

// The basic AI-analyzability probe (AI_HOSTCLIENT_SPEC §A): flood-fill the pathable region
// (slope under the limit, above water) from the first spawn and require every other spawn to be
// inside it — a map that seals a spawn off is not playable. Full validation is a later work-order.
static bool AllSpawnsReachable(const Data::PlacementInstances& markers,
                               const Data::MapFields& fields, float maxSlopeDegrees) {
    if (markers.Count() == 0) return false;
    const int side = PlacementTest::vertexSize;
    const float tangent = std::tan(maxSlopeDegrees * 3.14159265f / 180.0f);
    const float gradientLimitSquared = tangent * tangent;
    std::vector<unsigned char> bVisited(static_cast<std::size_t>(side) * side, 0u);
    std::vector<int> stack;
    const int startX = static_cast<int>(markers.positionX[0] + 0.5f);
    const int startY = static_cast<int>(markers.positionZ[0] + 0.5f);
    stack.push_back(startY * side + startX);
    bVisited[startY * side + startX] = 1u;
    while (!stack.empty()) {
        const int cell = stack.back(); stack.pop_back();
        const int cellX = cell % side, cellY = cell / side;
        const int offsetsX[4] = { 1, -1, 0, 0 };
        const int offsetsY[4] = { 0, 0, 1, -1 };
        for (int direction = 0; direction < 4; ++direction) {
            const int nextX = cellX + offsetsX[direction], nextY = cellY + offsetsY[direction];
            if (nextX < 0 || nextY < 0 || nextX >= side || nextY >= side) continue;
            const int nextCell = nextY * side + nextX;
            if (bVisited[nextCell] != 0u) continue;
            const float slopeGradient = fields.slope.Get(nextX, nextY);   // the Mask stage's bake
            if (slopeGradient * slopeGradient > gradientLimitSquared) continue;
            bVisited[nextCell] = 1u;
            stack.push_back(nextCell);
        }
    }
    for (std::size_t index = 0; index < markers.Count(); ++index) {
        const int cellX = static_cast<int>(markers.positionX[index] + 0.5f);
        const int cellY = static_cast<int>(markers.positionZ[index] + 0.5f);
        if (bVisited[static_cast<std::size_t>(cellY) * side + cellX] == 0u) return false;
    }
    return true;
}

int main() {
    Data::MapFields fields;
    PlacementTest::BuildTestFields(fields);
    const float extent = static_cast<float>(PlacementTest::vertexSize - 1);

    // --- four-fold symmetry: mirror across both ground axes.
    Params::MapRecipe recipe = MakeSymmetricRecipe(Params::SymmetryAxis::MirrorAcrossX
                                                 | Params::SymmetryAxis::MirrorAcrossZ);
    Data::PlacementResults results;
    Proc::PlacementStage stage(recipe, fields, results);
    stage.Run();
    const Data::PlacementInstances& markers = results.markers;
    std::printf("symmetric markers=%zu\n", markers.Count());
    Check(markers.Count() == 4, "mirror-X|Z produces a 4-member orbit");

    bool bOneSymmetryGroup = markers.Count() > 0;
    for (std::size_t index = 1; index < markers.Count(); ++index)
        if (markers.symmetryIdentifier[index] != markers.symmetryIdentifier[0]) bOneSymmetryGroup = false;
    Check(bOneSymmetryGroup, "clones share one symmetry id");

    bool bClosedUnderMirror = markers.Count() > 0;
    for (std::size_t index = 0; index < markers.Count(); ++index) {
        const float positionX = markers.positionX[index], positionY = markers.positionZ[index];
        if (!HasInstanceAt(markers, extent - positionX, positionY)) bClosedUnderMirror = false;
        if (!HasInstanceAt(markers, positionX, extent - positionY)) bClosedUnderMirror = false;
    }
    Check(bClosedUnderMirror, "clone positions are exact mirrors");

    // A mirrored yaw is pi - yaw, i.e. the quaternion's y and w swap. Orbit order is
    // [source, mirrorX(source), ...], so entry 1 is entry 0 mirrored across X.
    if (markers.Count() == 4) {
        Check(std::fabs(markers.rotationY[1] - markers.rotationW[0]) < 1e-5f
              && std::fabs(markers.rotationW[1] - markers.rotationY[0]) < 1e-5f,
              "clone rotation is the mirrored rotation");
        Check(std::fabs(markers.positionY[1] - markers.positionY[0]) < 1e-4f,
              "clones sit at the same height on symmetric terrain");
    }

    // --- basic AI-analyzability: every spawn is in one connected pathable region.
    Check(AllSpawnsReachable(markers, fields, 20.0f), "all spawns are mutually reachable");

    // --- point symmetry: a 2-member orbit, so a count of 4 yields two groups.
    Data::PlacementResults pointResults;
    Params::MapRecipe pointRecipe = MakeSymmetricRecipe(Params::SymmetryAxis::RotateHalfTurn);
    Proc::PlacementStage pointStage(pointRecipe, fields, pointResults);
    pointStage.Run();
    std::printf("half-turn markers=%zu\n", pointResults.markers.Count());
    Check(pointResults.markers.Count() == 4, "half-turn symmetry fills the requested count");
    bool bClosedUnderRotation = pointResults.markers.Count() > 0;
    for (std::size_t index = 0; index < pointResults.markers.Count(); ++index)
        if (!HasInstanceAt(pointResults.markers, extent - pointResults.markers.positionX[index],
                           extent - pointResults.markers.positionZ[index])) bClosedUnderRotation = false;
    Check(bClosedUnderRotation, "half-turn clones are exact point reflections");
    Check(AllSpawnsReachable(pointResults.markers, fields, 20.0f), "half-turn spawns reachable");

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
