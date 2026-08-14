// FlowAccumulation_Pipeline_PROC_Test.cpp — the dirty-hash and stochastic-routing halves of
// the M3-5 acceptance list: the stage skips when nothing it consumes changed, re-runs when a
// constant moves or an upstream stage dirties, and the stochastic single-flow-direction term
// stays reproducible from its seed while still conserving drainage.
#include "FlowAccumulation_TestSupport_PROC.h"
#include "../pipeline/Generation_PIPELINE.h"

using namespace SanmapGen;

namespace FlowAccumulationTest {

void RunDirtyHashChecks(int side) {
    Params::Geometry geometry;
    geometry.mapSize = side - 1;
    Data::MapFields fields;
    BuildTiltedBowlTerrain(fields, side);
    Proc::FlowAccumulationStage stage(geometry, fields);

    std::size_t upstreamParameter = 1;
    int upstreamRunCount = 0;
    int flowRunCount = 0;
    Pipeline::GenerationPipeline pipeline;
    pipeline.AddStage("Upstream", [&] { return upstreamParameter; }, [&] { ++upstreamRunCount; });
    pipeline.AddStage("FlowAccumulation", [&] { return stage.ComputeParameterHash(); },
                      [&] { stage.Run(); ++flowRunCount; });

    pipeline.Run();
    Check(flowRunCount == 1, "dirty-hash: the flow stage runs on the first pass");
    pipeline.Run();
    Check(flowRunCount == 1, "dirty-hash: an unchanged flow stage is skipped");

    stage.Constants().cellWeight = 2.0f;
    pipeline.Run();
    Check(flowRunCount == 2, "dirty-hash: changing a stage constant re-runs the stage");
    Check(upstreamRunCount == 1, "dirty-hash: a downstream change never re-runs the upstream stage");
    Check(fields.accumulation.Get(0, side / 2) == 2.0f * static_cast<float>(side) * side,
          "the re-run honours the new cell weight (conservation scales with it)");

    stage.Constants().cellWeight = 1.0f;
    pipeline.Run();
    Check(flowRunCount == 3, "dirty-hash: reverting a constant is still a change and re-runs");
    upstreamParameter = 2;
    pipeline.Run();
    Check(upstreamRunCount == 2 && flowRunCount == 4,
          "dirty-hash: an upstream change dirties the flow stage through the combined hash");
}

void RunStochasticRoutingChecks(int side) {
    Params::Geometry geometry;
    geometry.mapSize = side - 1;
    Data::MapFields fields;
    BuildTiltedBowlTerrain(fields, side);
    Proc::FlowAccumulationStage stage(geometry, fields);
    stage.Constants().flowNoiseImpact = 0.35f;
    stage.Constants().flowNoiseSeed = 1234u;

    stage.RunOnCpu();
    const std::vector<int> firstDirections = stage.FlowDirections();
    const double firstSinkTotal = SinkAccumulationTotal(stage, fields);
    stage.RunOnCpu();
    Check(firstDirections == stage.FlowDirections(),
          "stochastic routing is reproducible: same seed, same directions");
    Check(firstSinkTotal == static_cast<double>(side) * side,
          "conservation still holds with the stochastic tie-breaker on");

    stage.Constants().flowNoiseSeed = 4321u;
    stage.RunOnCpu();
    Check(firstDirections != stage.FlowDirections(),
          "the stochastic term is live: a different seed scatters the routing");
    Check(SinkAccumulationTotal(stage, fields) == static_cast<double>(side) * side,
          "conservation holds for the re-seeded routing too");
}

} // namespace FlowAccumulationTest
