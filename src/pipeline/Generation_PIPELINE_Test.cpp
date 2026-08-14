// Generation_PIPELINE_Test.cpp — acceptance test for Generation_PIPELINE (M2-1).
//   g++ -O2 -std=c++17 -fsanitize=address,undefined Generation_PIPELINE_Test.cpp -o t && ./t
// Verifies dirty-hash propagation with three mock stages.
#include "Generation_PIPELINE.h"
#include <cstdio>
#include <functional>

using namespace SanmapGen::Pipeline;

int main() {
    int failures = 0;

    // Three stages, each with a mutable "param" and a run counter.
    std::size_t paramA = 1, paramB = 1, paramC = 1;
    int runsA = 0, runsB = 0, runsC = 0;

    GenerationPipeline pipeline;
    pipeline.AddStage("A", [&] { return std::hash<std::size_t>{}(paramA); }, [&] { ++runsA; });
    pipeline.AddStage("B", [&] { return std::hash<std::size_t>{}(paramB); }, [&] { ++runsB; });
    pipeline.AddStage("C", [&] { return std::hash<std::size_t>{}(paramC); }, [&] { ++runsC; });

    // First run: everything runs.
    auto ran = pipeline.Run();
    if (ran.size() != 3) { std::printf("FAIL first run count %zu\n", ran.size()); ++failures; }
    if (runsA != 1 || runsB != 1 || runsC != 1) { std::printf("FAIL first run\n"); ++failures; }

    // No change: nothing runs.
    ran = pipeline.Run();
    if (!ran.empty()) { std::printf("FAIL nothing should run (%zu)\n", ran.size()); ++failures; }
    if (runsA != 1 || runsB != 1 || runsC != 1) { std::printf("FAIL clean rerun\n"); ++failures; }

    // Change the MIDDLE stage: B and C run (downstream dirtied), A does not.
    paramB = 42;
    ran = pipeline.Run();
    if (ran.size() != 2 || ran[0] != "B" || ran[1] != "C") { std::printf("FAIL middle-change propagation\n"); ++failures; }
    if (runsA != 1 || runsB != 2 || runsC != 2) { std::printf("FAIL middle counts\n"); ++failures; }

    // Change the FIRST stage: all three run.
    paramA = 7;
    ran = pipeline.Run();
    if (ran.size() != 3) { std::printf("FAIL first-change all run\n"); ++failures; }
    if (runsA != 2 || runsB != 3 || runsC != 3) { std::printf("FAIL first-change counts\n"); ++failures; }

    // Change the LAST stage only: only C runs.
    paramC = 99;
    ran = pipeline.Run();
    if (ran.size() != 1 || ran[0] != "C") { std::printf("FAIL last-only\n"); ++failures; }

    // InvalidateAll forces a full re-run.
    pipeline.InvalidateAll();
    ran = pipeline.Run();
    if (ran.size() != 3) { std::printf("FAIL invalidate-all\n"); ++failures; }

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
