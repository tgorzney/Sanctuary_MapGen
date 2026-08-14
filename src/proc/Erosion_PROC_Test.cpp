// Erosion_PROC_Test.cpp — acceptance test for the erosion stage, Cpu/Exact path (M3-3).
// Build with MSVC from src/proc:
//   cl /EHsc /std:c++17 /O2 Erosion_PROC_Test.cpp Erosion_PROC.cpp Erosion_Field_PROC.cpp \
//      Erosion_Rain_PROC.cpp Erosion_Spawn_PROC.cpp Erosion_Droplet_PROC.cpp \
//      Erosion_DropletTransfer_PROC.cpp Erosion_Accumulation_PROC.cpp Erosion_Gpu_PROC.cpp \
//      ..\sys\GpuResource_*.cpp ..\sys\GpuGlFunctions_SYS.cpp opengl32.lib gdi32.lib user32.lib
// Covers: erosion carves; volume conservation (no runaway height); the fixed-point stack never
// goes negative; deterministic mode is bit-identical across two runs; the ordered accumulation
// DAG conserves too; and a rate change re-runs the stage through the PIPELINE dirty hash.
// Cpu/Gpu parity lives in Erosion_Gpu_PROC_Test.cpp (it needs a GL context).
#include "Erosion_TestFixture_PROC.h"
#include "../pipeline/Generation_PIPELINE.h"
#include <cstring>

using namespace SanmapGen;
using namespace SanmapGen::ErosionTest;

static Sys::DispatchPolicy DeterministicPolicy(const Proc::ErosionStage& stage) {
    Sys::DispatchPolicy policy = stage.ActiveDispatchPolicy();
    policy.bDeterministic = true;
    return policy;
}

// Runs one deterministic Output-context pass and returns the resulting heightfield.
static std::vector<float> RunDeterministicPass(const Params::Geometry& geometry, Data::MapFields& fields,
                                               Proc::ErosionStage& stage) {
    ConfigureStage(stage);
    stage.SetDispatchPolicy(DeterministicPolicy(stage));
    stage.SetGenerationContext(Sys::GenerationContext::Output);
    Check(stage.Run() == Sys::ComputeBackend::Cpu, "deterministic Exact-class erosion resolves to Cpu");
    (void)geometry;
    return HeightfieldCopy(fields);
}

static void CheckErodedField(const std::vector<float>& before, const std::vector<float>& after,
                             const Proc::ErosionStage& stage) {
    int changedCells = 0;
    bool bFinite = true, bNonNegative = true;
    float highest = 0.0f;
    for (std::size_t index = 0; index < after.size(); ++index) {
        if (after[index] != before[index]) ++changedCells;
        if (!(after[index] == after[index]) || after[index] > 1.0e6f) bFinite = false;
        if (after[index] < 0.0f) bNonNegative = false;
        if (after[index] > highest) highest = after[index];
    }
    for (int ticks : stage.ThicknessFixedPoint()) if (ticks < 0) bNonNegative = false;

    const double drift = (TotalVolume(after) - TotalVolume(before)) / TotalVolume(before);
    Check(changedCells > static_cast<int>(after.size()) / 20, "erosion reshaped a meaningful part of the map");
    Check(bFinite, "no NaN / runaway value in the eroded heightfield");
    Check(bNonNegative, "no negative height and no stratum driven below zero");
    Check(highest < 2.0f, "no runaway peak (conservation sanity)");
    Check(drift < 0.001 && drift > -0.001, "volume conserved to 0.1% (mass-conserving transport)");
    std::printf("volume drift %+.6f%%, cells changed %d/%zu, peak %.4f\n",
                drift * 100.0, changedCells, after.size(), highest);
}

// A rate change must dirty the stage; an unchanged recipe must skip it (M2-1 dirty hash).
static void CheckDirtyHashRerun(const Params::Geometry& geometry) {
    Data::MapFields fields;
    BuildTestTerrain(fields, geometry.VertexSize());
    Proc::ErosionStage stage(geometry, fields);
    ConfigureStage(stage);
    stage.LayerSettings(0).dropletCount = 200;            // keep the dirty-hash test quick
    int runCount = 0;
    Pipeline::GenerationPipeline pipeline;
    pipeline.AddStage("Erosion", [&] { return stage.ComputeParameterHash(); },
                      [&] { stage.Run(); ++runCount; });
    pipeline.Run();
    Check(runCount == 1, "first pipeline pass runs erosion");
    pipeline.Run();
    Check(runCount == 1, "unchanged settings skip erosion");
    stage.LayerSettings(0).baseErosionRate = 0.45f;
    pipeline.Run();
    Check(runCount == 2, "changing the erosion RATE re-runs the stage");
    stage.Material(0).hardness = 0.42f;
    pipeline.Run();
    Check(runCount == 3, "changing a material's hardness re-runs the stage");
}

// The Cpu-only ordered spillover DAG must move material, not create it.
static void CheckAccumulationDag(const Params::Geometry& geometry) {
    Data::MapFields fields;
    BuildTestTerrain(fields, geometry.VertexSize());
    const std::vector<float> before = HeightfieldCopy(fields);
    Proc::ErosionStage stage(geometry, fields);
    ConfigureStage(stage);
    stage.LayerSettings(0).bAccurateSimultaneousAccumulation = true;
    stage.SetDispatchPolicy(DeterministicPolicy(stage));
    stage.SetGenerationContext(Sys::GenerationContext::Output);
    stage.Run();
    const std::vector<float> after = HeightfieldCopy(fields);
    const double drift = (TotalVolume(after) - TotalVolume(before)) / TotalVolume(before);
    Check(drift < 0.001 && drift > -0.001, "accumulation DAG spillover conserves volume");
}

int main() {
    Params::Geometry geometry;
    geometry.mapSize = 128;

    Data::MapFields fields;
    BuildTestTerrain(fields, geometry.VertexSize());
    const std::vector<float> before = HeightfieldCopy(fields);
    Proc::ErosionStage stage(geometry, fields);
    const std::vector<float> after = RunDeterministicPass(geometry, fields, stage);
    Check(stage.ProcessedLayerCount() == 1, "exactly the one enabled erosion layer ran");
    Check(stage.LastDropletCount() == 6000, "rejection sampling produced exactly DropletCount drops");
    CheckErodedField(before, after, stage);

    Data::MapFields secondFields;
    BuildTestTerrain(secondFields, geometry.VertexSize());
    Proc::ErosionStage secondStage(geometry, secondFields);
    const std::vector<float> secondAfter = RunDeterministicPass(geometry, secondFields, secondStage);
    Check(std::memcmp(after.data(), secondAfter.data(), after.size() * sizeof(float)) == 0,
          "two deterministic runs are BIT-identical");
    Check(secondStage.ThicknessFixedPoint() == stage.ThicknessFixedPoint(),
          "the fixed-point erosion state itself is bit-identical");

    CheckAccumulationDag(geometry);
    CheckDirtyHashRerun(geometry);

    if (FailureCount() == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", FailureCount());
    return 1;
}
