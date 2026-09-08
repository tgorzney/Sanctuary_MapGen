// ScenarioSpawnIdValidation_IO_Test.cpp — STEP252 acceptance for
// ARCH_15_12_ScenarioSpawnIdentity.md §15.12's Validator + Resolve algorithm. Pure, disk-free; no
// imgui, no window, no GL context.
#include "ScenarioSpawnIdValidation_IO.h"
#include "../params/Scenario_PARAMS.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

Params::ScenarioSpawnPoint MakePoolEntry(const char* spawnId, const char* armyName) {
    Params::ScenarioSpawnPoint point;
    point.spawnId = spawnId; point.armyName = armyName;
    return point;
}

bool ReportHasViolation(const Io::ScenarioSpawnIdValidationReport& report, const std::string& descriptor,
                        const std::string& detail) {
    for (const Io::ScenarioSpawnIdValidationReport::Violation& violation : report.violations)
        if (violation.descriptor == descriptor && violation.detail == detail) return true;
    return false;
}

// (1) pool spawnId "ARMY_01"/"army_01"/"Army_01" all flagged reserved-namespace-forbidden;
// "North_1v1" not flagged.
void RunReservedNamespaceChecks() {
    Params::Scenarios scenarios;
    scenarios.spawnPoints = { MakePoolEntry("ARMY_01", "ARMY_01"), MakePoolEntry("army_01", "ARMY_02"),
                              MakePoolEntry("Army_01", "ARMY_03"), MakePoolEntry("North_1v1", "ARMY_04") };
    const Io::ScenarioSpawnIdValidationReport report = Io::ValidateScenarioSpawnIds(scenarios);
    Check(ReportHasViolation(report, "pool", "ARMY_01"), "\"ARMY_01\" pool spawnId flagged reserved");
    Check(ReportHasViolation(report, "pool", "army_01"), "\"army_01\" pool spawnId flagged reserved");
    Check(ReportHasViolation(report, "pool", "Army_01"), "\"Army_01\" pool spawnId flagged reserved");
    Check(!ReportHasViolation(report, "pool", "North_1v1"), "\"North_1v1\" pool spawnId NOT flagged");
}

// (2) duplicate pool spawnId flagged, first-authored kept on fix-up.
void RunDuplicatePoolSpawnIdChecks() {
    Params::Scenarios scenarios;
    Params::ScenarioSpawnPoint first = MakePoolEntry("North_1v1", "ARMY_01");
    first.positionX = 1.0f;
    Params::ScenarioSpawnPoint duplicate = MakePoolEntry("North_1v1", "ARMY_02");
    duplicate.positionX = 99.0f;
    scenarios.spawnPoints = { first, duplicate };

    const Io::ScenarioSpawnIdValidationReport report = Io::ValidateScenarioSpawnIds(scenarios);
    Check(ReportHasViolation(report, "pool", "North_1v1"), "duplicate pool spawnId flagged");

    const Params::Scenarios fixedUp = Io::ApplyScenarioSpawnIdFixups(scenarios);
    Check(fixedUp.spawnPoints.size() == 1, "fix-up drops the later duplicate pool row");
    if (fixedUp.spawnPoints.size() == 1) {
        Check(fixedUp.spawnPoints[0].armyName == "ARMY_01" && fixedUp.spawnPoints[0].positionX == 1.0f,
              "the FIRST-authored pool row is kept, not the later duplicate");
    }
}

// (3) two spawnIds in one scenario resolving to the same armyName flagged; the SAME two armyNames
// across DIFFERENT scenarios NOT flagged.
void RunSameScenarioDuplicateArmyNameChecks() {
    Params::Scenarios scenarios;
    scenarios.spawnPoints = { MakePoolEntry("North_1v1", "ARMY_01"), MakePoolEntry("South_1v1", "ARMY_01") };
    Params::CountScenario oneScenario;
    oneScenario.body.name = "OneScenario";
    oneScenario.body.spawnIds = { "North_1v1", "South_1v1" };   // both resolve to ARMY_01
    scenarios.countScenarios.push_back(oneScenario);

    const Io::ScenarioSpawnIdValidationReport report = Io::ValidateScenarioSpawnIds(scenarios);
    Check(ReportHasViolation(report, "OneScenario", "South_1v1"),
          "the SECOND spawnId resolving to an already-claimed armyName in the SAME scenario is flagged");

    const Params::Scenarios fixedUp = Io::ApplyScenarioSpawnIdFixups(scenarios);
    Check(fixedUp.countScenarios.size() == 1 && fixedUp.countScenarios[0].body.spawnIds.size() == 1
          && fixedUp.countScenarios[0].body.spawnIds[0] == "North_1v1",
          "fix-up drops the later same-scenario duplicate, keeping the first-authored spawnId");

    // Cross-scenario reuse of the same armyName is explicitly fine -- only checked per-scenario.
    Params::Scenarios crossScenario;
    crossScenario.spawnPoints = { MakePoolEntry("North_1v1", "ARMY_01") };
    Params::CountScenario scenarioA; scenarioA.body.name = "ScenarioA"; scenarioA.body.spawnIds = { "North_1v1" };
    Params::CountScenario scenarioB; scenarioB.body.name = "ScenarioB"; scenarioB.body.spawnIds = { "North_1v1" };
    crossScenario.countScenarios = { scenarioA, scenarioB };
    const Io::ScenarioSpawnIdValidationReport crossReport = Io::ValidateScenarioSpawnIds(crossScenario);
    Check(!ReportHasViolation(crossReport, "ScenarioA", "North_1v1")
          && !ReportHasViolation(crossReport, "ScenarioB", "North_1v1"),
          "the SAME armyName reused across DIFFERENT scenarios is never flagged");
}

// (4) an unresolvable spawnId (matches neither pool nor a real ARMY_XX-shaped string) flagged
// warn-only, never dropped by the fix-up.
void RunUnresolvableSpawnIdChecks() {
    Params::Scenarios scenarios;
    Params::CountScenario scenario;
    scenario.body.name = "HasGarbage";
    scenario.body.spawnIds = { "totally_bogus_spawn_id" };
    scenarios.countScenarios.push_back(scenario);

    const Io::ScenarioSpawnIdValidationReport report = Io::ValidateScenarioSpawnIds(scenarios);
    Check(ReportHasViolation(report, "HasGarbage", "totally_bogus_spawn_id"),
          "an unresolvable spawnId is flagged");

    const Params::Scenarios fixedUp = Io::ApplyScenarioSpawnIdFixups(scenarios);
    Check(fixedUp.countScenarios.size() == 1 && fixedUp.countScenarios[0].body.spawnIds.size() == 1
          && fixedUp.countScenarios[0].body.spawnIds[0] == "totally_bogus_spawn_id",
          "an unresolvable spawnId is left AS-AUTHORED by the fix-up, never dropped");
}

// (5) ResolveSpawnId pool-hit, literal-fallback-hit, and miss, each independently.
struct FakeArmyIdentityTransformLookup : Io::ArmyIdentityTransformLookup {
    bool Find(const std::string& armyName, float& outX, float& outY, float& outZ) const override {
        if (armyName != "ARMY_01") return false;
        outX = 7.0f; outY = 8.0f; outZ = 9.0f;
        return true;
    }
};

void RunResolveSpawnIdChecks() {
    std::vector<Params::ScenarioSpawnPoint> pool = { MakePoolEntry("North_1v1", "ARMY_02") };
    pool[0].positionX = 1.0f; pool[0].positionY = 2.0f; pool[0].positionZ = 3.0f;

    // Pool-hit: resolves via the pool, regardless of lookup.
    {
        std::string armyName; float x = 0.0f, y = 0.0f, z = 0.0f;
        const bool bResolved = Io::ResolveSpawnId("North_1v1", pool, nullptr, armyName, x, y, z);
        Check(bResolved && armyName == "ARMY_02" && x == 1.0f && y == 2.0f && z == 3.0f,
              "ResolveSpawnId pool-hit resolves to the pool row's own armyName/position");
    }
    // Literal-fallback-hit: a real lookup resolves an ARMY_XX name the pool doesn't carry.
    {
        FakeArmyIdentityTransformLookup lookup;
        std::string armyName; float x = 0.0f, y = 0.0f, z = 0.0f;
        const bool bResolved = Io::ResolveSpawnId("ARMY_01", pool, &lookup, armyName, x, y, z);
        Check(bResolved && armyName == "ARMY_01" && x == 7.0f && y == 8.0f && z == 9.0f,
              "ResolveSpawnId literal-fallback-hit resolves via the real lookup");
    }
    // Miss: matches neither the pool nor the lookup.
    {
        FakeArmyIdentityTransformLookup lookup;
        std::string armyName; float x = 0.0f, y = 0.0f, z = 0.0f;
        const bool bResolved = Io::ResolveSpawnId("ARMY_99", pool, &lookup, armyName, x, y, z);
        Check(!bResolved, "ResolveSpawnId miss: matches neither the pool nor a real lookup");
    }
}

} // namespace

int main() {
    RunReservedNamespaceChecks();
    RunDuplicatePoolSpawnIdChecks();
    RunSameScenarioDuplicateArmyNameChecks();
    RunUnresolvableSpawnIdChecks();
    RunResolveSpawnIdChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
