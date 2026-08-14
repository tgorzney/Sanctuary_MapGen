// Placement_PROC_Test.cpp — acceptance test for the placement stage (M3-6): determinism,
// Poisson spacing, the per-rule gates, and that the stage plugs into the conductor. Build
// with MSVC from src/proc: cl /EHsc /std:c++17 /O2 with this file, every Placement_*_PROC.cpp,
// ..\sys\GpuResource_*_SYS.cpp, ..\sys\GpuGlFunctions_SYS.cpp and opengl32.lib.
#include "Placement_PROC.h"
#include "Placement_Test_Terrain.h"
#include "../pipeline/Generation_PIPELINE.h"
#include <cstdio>
#include <cstring>

using namespace SanmapGen;

static int failures = 0;
static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failures; }
}

// One spawn rule (spaced, gated onto the flat plain) plus one mask-gated prop rule.
static Params::MapRecipe MakeRecipe(unsigned int seed) {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = PlacementTest::mapSize;
    recipe.geometry.seed = seed;
    recipe.geometry.terrainMaxHeight = 128.0f;

    Params::MarkerRule spawnRule;
    spawnRule.category = Params::MarkerCategory::Spawn;
    spawnRule.count = 6;
    spawnRule.clearanceSpacing = 20.0f;
    spawnRule.mapEdgePadding = 8;
    spawnRule.minHeight = 0.4f; spawnRule.maxHeight = 0.6f;
    spawnRule.maxSlope = 10.0f;
    spawnRule.bRandomSelection = true;      // hashed order: no clearance scoring needed
    spawnRule.transform = PlacementTest::MakeTransform("m002", 1.0f, 1.0f);
    recipe.markerRules.push_back(spawnRule);

    Params::PropRule propRule;              // biome/mask gate: stratum 3 lives in the left half
    propRule.density = 0.35f;
    propRule.spacingMinimum = 6.0f;
    propRule.mapEdgePadding = 4;
    propRule.minHeight = 0.4f; propRule.maxHeight = 0.6f;
    propRule.maxSlope = 20.0f;
    propRule.maskStratumIndex = PlacementTest::maskStratumIndex;
    propRule.maskWeightMinimum = 0.5f;
    propRule.transform = PlacementTest::MakeTransform("edbm014", 0.8f, 1.4f);
    propRule.transform.bCollidable = true;
    recipe.propRules.push_back(propRule);
    return recipe;
}

// FNV-1a over every emitted float — printed so two PROCESSES (and two machines) can be
// compared, which is the shared-generation requirement, not just two objects in one run.
static unsigned long long PlacementChecksum(const Data::PlacementInstances& instances) {
    unsigned long long checksum = 1469598103934665603ull;
    for (std::size_t index = 0; index < instances.Count(); ++index) {
        const float values[5] = { instances.positionX[index], instances.positionY[index],
                                  instances.positionZ[index], instances.rotationY[index],
                                  instances.scaleX[index] };
        for (int value = 0; value < 5; ++value) {
            unsigned int bits = 0;
            std::memcpy(&bits, &values[value], sizeof(bits));
            checksum = (checksum ^ bits) * 1099511628211ull;
        }
    }
    return checksum;
}

static bool InstancesEqual(const Data::PlacementInstances& first, const Data::PlacementInstances& second) {
    if (first.Count() != second.Count()) return false;
    for (std::size_t index = 0; index < first.Count(); ++index)
        if (first.positionX[index] != second.positionX[index]
            || first.positionZ[index] != second.positionZ[index]
            || first.positionY[index] != second.positionY[index]
            || first.rotationY[index] != second.rotationY[index]
            || first.scaleX[index]    != second.scaleX[index]
            || first.symmetryIdentifier[index] != second.symmetryIdentifier[index]) return false;
    return true;
}

int main() {
    Data::MapFields fields;
    PlacementTest::BuildTestFields(fields);

    Params::MapRecipe recipe = MakeRecipe(1234u);
    Data::PlacementResults resultsFirst, resultsSecond, resultsOtherSeed, resultsGpuPath;
    Proc::PlacementStage stageFirst(recipe, fields, resultsFirst);
    stageFirst.Run();
    Check(stageFirst.LastBackend() == Sys::ComputeBackend::Cpu, "placement resolves to the Cpu path");

    // --- determinism: same recipe, a second independent stage -> identical instances.
    Proc::PlacementStage stageSecond(recipe, fields, resultsSecond);
    stageSecond.Run();
    Check(InstancesEqual(resultsFirst.markers, resultsSecond.markers), "same seed -> identical markers");
    Check(InstancesEqual(resultsFirst.props, resultsSecond.props), "same seed -> identical props");

    // Re-running the SAME stage must also reproduce itself (no accumulated state).
    Proc::PlacementStage stageRepeat(recipe, fields, resultsGpuPath);
    stageRepeat.RunOnGpu();     // no Gpu manager -> gate falls back to the Cpu twin
    Check(!stageRepeat.WasGpuGateUsed(), "Gpu gate reports fallback when no manager is set");
    Check(InstancesEqual(resultsFirst.markers, resultsGpuPath.markers), "Gpu fallback == Cpu result");

    Params::MapRecipe otherSeedRecipe = MakeRecipe(9876u);
    Proc::PlacementStage stageOtherSeed(otherSeedRecipe, fields, resultsOtherSeed);
    stageOtherSeed.Run();
    Check(!InstancesEqual(resultsFirst.markers, resultsOtherSeed.markers), "different seed -> different map");
    Check(stageFirst.ComputeParameterHash() != stageOtherSeed.ComputeParameterHash(),
          "seed change dirties the stage hash");

    const Data::PlacementInstances& markers = resultsFirst.markers;
    const Data::PlacementInstances& props = resultsFirst.props;
    std::printf("markers=%zu props=%zu candidates=%d accepted=%d\nchecksum markers=%016llx props=%016llx\n",
                markers.Count(), props.Count(), stageFirst.EvaluatedCandidateCount(),
                stageFirst.AcceptedCandidateCount(), PlacementChecksum(markers), PlacementChecksum(props));
    Check(markers.Count() == 6, "spawn rule places exactly its count");
    Check(props.Count() > 20, "prop rule scatters a population");

    // --- Poisson spacing: no two instances of a rule closer than its minimum.
    Check(PlacementTest::MinimumSeparation(markers) >= 20.0f - 1e-3f, "marker spacing >= 20 cells");
    Check(PlacementTest::MinimumSeparation(props) >= 6.0f - 1e-3f, "prop spacing >= 6 cells");

    // --- gates honoured (height / slope / edge padding / biome mask / collision flag).
    Check(PlacementTest::AllWithinGates(markers, fields, stageFirst, 0.4f, 0.6f, 10.0f, 8),
          "markers honour height, slope and edge padding");
    Check(PlacementTest::AllWithinGates(props, fields, stageFirst, 0.4f, 0.6f, 20.0f, 4),
          "props honour the same gates");
    bool bMaskGateHeld = true, bCollidableSet = true;
    for (std::size_t index = 0; index < props.Count(); ++index) {
        const int cellX = static_cast<int>(props.positionX[index] + 0.5f);
        const int cellY = static_cast<int>(props.positionZ[index] + 0.5f);
        if (fields.materialMasks[PlacementTest::maskStratumIndex].Get(cellX, cellY) < 0.5f) bMaskGateHeld = false;
        if (props.bCollidable[index] == 0) bCollidableSet = false;
        if (props.scaleX[index] < 0.8f - 1e-4f || props.scaleX[index] > 1.4f + 1e-4f) bCollidableSet = false;
    }
    Check(bMaskGateHeld, "prop biome/mask gate honoured");
    Check(bCollidableSet, "prop collision flag and scale range round-trip");
    // --- the stage plugs into the dirty-hash conductor as-is (M3-8 does the real wiring).
    Pipeline::GenerationPipeline pipeline;
    pipeline.AddStage("Placement", [&] { return stageFirst.ComputeParameterHash(); },
                      [&] { stageFirst.Run(); });
    Check(pipeline.Run().size() == 1, "stage registers and runs under GenerationPipeline");
    Check(pipeline.Run().empty(), "unchanged params skip the stage");

    // --- round-trip identity fields survive into the SoA.
    Check(markers.Count() > 0 && markers.templateIdentifier[0].characters[0] == 'm', "marker tpId stored");
    Check(markers.Count() > 0 && markers.category[0] == static_cast<int>(Params::MarkerCategory::Spawn),
          "marker category stored");

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
