// MapCanvas_IconLayer_Microbenchmark_UI_Test.cpp — STEP59: real wall-clock numbers for §14.9's
// placeholder OverlayRenderingSettings::visibleInstanceBudget, at N in {100k, 300k, 600k} x
// {0%-culled, ~5%-visible}, for the three operations ARCH_14_09_RenderingPerformance.md §14.9 names:
// (1) STEP53's REAL world->screen transform + AABB cull + compaction candidate-list build path
// (Ui::ResolveVisibleCandidates), (2) STEP53's REAL FlushIconLayerBucket-shaped bulk ImDrawList
// write, (3) a naive per-instance ImDrawList::AddImage() path — both (2) and (3) timed in the
// sibling MapCanvas_IconLayer_MicrobenchmarkFrameOps_UI_Test.cpp, which also owns the headless-imgui-
// frame harness (Constitution §1.5 split); the synthetic-instance scene builder is a second sibling,
// MapCanvas_IconLayer_MicrobenchmarkScenarios_UI_Test.cpp.
//
// Test-only: ships no production code. Mirrors Placement_Symmetry_PROC_Test.cpp's own "Acceptance
// test 9" precedent — std::chrono::steady_clock + printf + loose Check()s that never gate on the
// timing numbers themselves, no formal benchmark harness.
//
// Explicitly out of scope (STEP59 work-order): updating visibleInstanceBudget's default value,
// adding a SIMD transform backend, a reusable benchmark framework, CPU/machine auto-detection, and
// any change to STEP53's shipped production files.
#include "MapCanvas_IconLayer_CullInternal_UI.h"
#include "MapCanvas_IconLayer_TestFixture_UI.h"
#include <chrono>
#include <cstdio>

namespace SanmapGen {
namespace Ui {

struct ScenarioSpec { const char* label; bool bZeroCulled; };

// MapCanvas_IconLayer_MicrobenchmarkScenarios_UI_Test.cpp
void BuildMicrobenchmarkScene(IconLayerTestFixture& fixture, int instanceCount, bool bZeroCulledScenario,
                              unsigned int randomSeed);
// MapCanvas_IconLayer_MicrobenchmarkFrameOps_UI_Test.cpp
void RunOperationTwoBulkWrite(const std::vector<OverlayVisibleInstance>& candidates, int instanceCount,
                              const ScenarioSpec& scenario);
void RunOperationThreeNaiveAddImage(const std::vector<OverlayVisibleInstance>& candidates, int instanceCount,
                                    const ScenarioSpec& scenario);

namespace {

double ElapsedMillis(const std::chrono::steady_clock::time_point& start,
                     const std::chrono::steady_clock::time_point& end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

void PrintBasisTagHeader() {
#ifdef NDEBUG
    const char* buildConfiguration = "Release (NDEBUG)";
#else
    const char* buildConfiguration = "Debug";
#endif
    std::printf("==== STEP59 overlay vertex-gen microbenchmark -- basis tag (Constitution Section 7) ====\n");
    std::printf("build configuration : %s\n", buildConfiguration);
    std::printf("compiled             : %s %s\n", __DATE__, __TIME__);
    std::printf("machine              : <manually record CPU/GPU/OS model here when reporting these numbers>\n");
    std::printf("===========================================================================================\n");
}

// Operation 1 — STEP53's real §1 candidate-list build path, called directly. One untimed warm-up
// call primes the per-layer AABB cache first (production builds this once and reuses it every
// frame; ResolveVisibleCandidates itself defensively rebuilds it on a cold/first call), so the timed
// call below measures only the transform+cull+compaction work a real subsequent frame would pay.
std::vector<OverlayVisibleInstance> RunOperationOne(const DrawOverlayIconLayersInput& input,
                                                     IconLayerAabbCache_UI& aabbCache,
                                                     int instanceCount, const ScenarioSpec& scenario) {
    std::vector<OverlayVisibleInstance> warmupCandidates;
    ResolveVisibleCandidates(input, aabbCache, nullptr, warmupCandidates);

    std::vector<OverlayVisibleInstance> candidates;
    const auto start = std::chrono::steady_clock::now();
    ResolveVisibleCandidates(input, aabbCache, nullptr, candidates);
    const auto end = std::chrono::steady_clock::now();
    std::printf("[op1 transform+cull+compaction] N=%7d scenario=%-12s elapsed=%9.3fms candidates=%9zu\n",
               instanceCount, scenario.label, ElapsedMillis(start, end), candidates.size());

    if (scenario.bZeroCulled)
        check(candidates.size() == static_cast<std::size_t>(instanceCount),
              "operation 1: the 0%-culled scenario keeps exactly N candidates");
    else
        check(candidates.size() <= static_cast<std::size_t>(0.10 * instanceCount),
              "operation 1: the ~5%-visible scenario's surviving candidates fall within [0, 0.10*N]");
    return candidates;
}

void RunOneMeasurementSet(int instanceCount, const ScenarioSpec& scenario, unsigned int randomSeed) {
    IconLayerTestFixture fixture;
    BuildMicrobenchmarkScene(fixture, instanceCount, scenario.bZeroCulled, randomSeed);
    const DrawOverlayIconLayersInput input = fixture.Input();

    const std::vector<OverlayVisibleInstance> candidates =
        RunOperationOne(input, fixture.aabbCache, instanceCount, scenario);
    RunOperationTwoBulkWrite(candidates, instanceCount, scenario);
    RunOperationThreeNaiveAddImage(candidates, instanceCount, scenario);
}

} // namespace

void RunMapCanvasIconLayerMicrobenchmark() {
    PrintBasisTagHeader();
    const int instanceCounts[] = { 100000, 300000, 600000 };
    const ScenarioSpec scenarios[] = { { "0pct-culled", true }, { "5pct-visible", false } };
    unsigned int randomSeed = 1u;
    for (int instanceCount : instanceCounts)
        for (const ScenarioSpec& scenario : scenarios)
            RunOneMeasurementSet(instanceCount, scenario, randomSeed++);
}

} // namespace Ui
} // namespace SanmapGen

int main() {
    SanmapGen::Ui::RunMapCanvasIconLayerMicrobenchmark();
    if (SanmapGen::Ui::previewTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", SanmapGen::Ui::previewTestFailureCount);
    return 1;
}
