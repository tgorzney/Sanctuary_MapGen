// Scenario_PARAMS_Test.cpp — STEP252 (ARCH_15_12_ScenarioSpawnIdentity.md §15.12) acceptance:
// `Params::ScenarioSpawnPoint` default-constructs well-formed (empty strings, zero floats). Pure
// check; no imgui, no window, no GL context.
#include "Scenario_PARAMS.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

void RunScenarioSpawnPointDefaultConstructionChecks() {
    Params::ScenarioSpawnPoint point;
    Check(point.spawnId.empty(), "a default-constructed ScenarioSpawnPoint's spawnId is empty");
    Check(point.armyName.empty(), "a default-constructed ScenarioSpawnPoint's armyName is empty");
    Check(point.positionX == 0.0f && point.positionY == 0.0f && point.positionZ == 0.0f,
          "a default-constructed ScenarioSpawnPoint's position is zeroed, not indeterminate");
}

// The pool is empty by default; ScenarioBody::spawnIds is a flat array of strings, also empty.
void RunScenariosPoolDefaultConstructionChecks() {
    Params::Scenarios scenarios;
    Check(scenarios.spawnPoints.empty(), "a default-constructed Scenarios has an empty spawnPoints pool");
    Check(scenarios.defaultScenario.spawnIds.empty(),
          "a default-constructed ScenarioBody has an empty spawnIds list");
}

} // namespace

int main() {
    RunScenarioSpawnPointDefaultConstructionChecks();
    RunScenariosPoolDefaultConstructionChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
