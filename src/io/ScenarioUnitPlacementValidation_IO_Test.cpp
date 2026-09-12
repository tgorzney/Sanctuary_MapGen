// ScenarioUnitPlacementValidation_IO_Test.cpp — STEP260 acceptance for
// ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md §15.14's unit-placement validator.
// Pure, disk-free; no imgui, no window, no GL context.
#include "ScenarioUnitPlacementValidation_IO.h"
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

bool ReportHasViolation(const Io::ScenarioUnitPlacementValidationReport& report,
                        const std::string& descriptor, const std::string& detail) {
    for (const Io::ScenarioUnitPlacementValidationReport::Violation& violation : report.violations)
        if (violation.descriptor == descriptor && violation.detail == detail) return true;
    return false;
}

// (1) an unresolvable armyName (no live lookup -> shape-only check) produces a named, logged
// warning and does NOT drop the placement row.
void RunUnresolvableArmyNameChecks() {
    Params::Scenarios scenarios;
    Params::CountScenario scenario;
    scenario.body.name = "HasBadArmy";
    Params::ScenarioUnitPlacement placement;
    placement.armyName = "totally_bogus_army";
    placement.templateIdentifier = "ucn3001";
    scenario.body.unitPlacements.push_back(placement);
    scenarios.countScenarios.push_back(scenario);

    const Io::ScenarioUnitPlacementValidationReport report =
        Io::ValidateScenarioUnitPlacements(scenarios, nullptr);
    Check(ReportHasViolation(report, "HasBadArmy", "totally_bogus_army"),
          "an unresolvable armyName is flagged");

    // Never dropped -- the placement is still present, unchanged, after validation (validation is
    // report-only; there is no fix-up unit for this type, unlike ScenarioSpawnIdValidation).
    Check(scenarios.countScenarios[0].body.unitPlacements.size() == 1,
          "the offending placement row is NOT dropped by validation");
}

// A well-formed ARMY_XX-shaped name (no live lookup) resolves via the shape-only fallback and is
// NOT flagged.
void RunWellFormedArmyNameNotFlaggedTest() {
    Params::Scenarios scenarios;
    Params::CountScenario scenario;
    scenario.body.name = "GoodArmy";
    Params::ScenarioUnitPlacement placement;
    placement.armyName = "ARMY_01";
    placement.templateIdentifier = "ucn3001";
    scenario.body.unitPlacements.push_back(placement);
    scenarios.countScenarios.push_back(scenario);

    const Io::ScenarioUnitPlacementValidationReport report =
        Io::ValidateScenarioUnitPlacements(scenarios, nullptr);
    Check(!ReportHasViolation(report, "GoodArmy", "ARMY_01"),
          "a well-formed ARMY_XX armyName is NOT flagged (shape-only fallback)");
}

// (2) an empty templateIdentifier produces a named, logged warning and does NOT drop the row.
void RunEmptyTemplateIdentifierChecks() {
    Params::Scenarios scenarios;
    Params::CountScenario scenario;
    scenario.body.name = "HasBadTemplate";
    Params::ScenarioUnitPlacement placement;
    placement.armyName = "ARMY_01";
    placement.templateIdentifier = "";   // deliberately empty
    scenario.body.unitPlacements.push_back(placement);
    scenarios.countScenarios.push_back(scenario);

    const Io::ScenarioUnitPlacementValidationReport report =
        Io::ValidateScenarioUnitPlacements(scenarios, nullptr);
    Check(ReportHasViolation(report, "HasBadTemplate", "ARMY_01"),
          "an empty templateIdentifier is flagged");
    Check(scenarios.countScenarios[0].body.unitPlacements.size() == 1,
          "the offending placement row is NOT dropped by validation");
}

// (3) a real ArmyIdentityTransformLookup resolves a non-ARMY_XX-shaped name, and rejects an
// unknown one -- exercising the lookup branch, not just the shape-only fallback.
struct FakeArmyIdentityTransformLookup : Io::ArmyIdentityTransformLookup {
    bool Find(const std::string& armyName, float& outX, float& outY, float& outZ) const override {
        if (armyName != "NorthArmy") return false;
        outX = 1.0f; outY = 2.0f; outZ = 3.0f;
        return true;
    }
};

void RunRealLookupChecks() {
    FakeArmyIdentityTransformLookup lookup;
    Params::Scenarios scenarios;
    Params::CountScenario scenario;
    scenario.body.name = "LookupScenario";
    Params::ScenarioUnitPlacement resolvable;
    resolvable.armyName = "NorthArmy"; resolvable.templateIdentifier = "ucn3001";
    Params::ScenarioUnitPlacement unresolvable;
    unresolvable.armyName = "SouthArmy"; unresolvable.templateIdentifier = "ucn3001";
    scenario.body.unitPlacements.push_back(resolvable);
    scenario.body.unitPlacements.push_back(unresolvable);
    scenarios.countScenarios.push_back(scenario);

    const Io::ScenarioUnitPlacementValidationReport report =
        Io::ValidateScenarioUnitPlacements(scenarios, &lookup);
    Check(!ReportHasViolation(report, "LookupScenario", "NorthArmy"),
          "a real lookup resolves a non-ARMY_XX-shaped armyName it knows about");
    Check(ReportHasViolation(report, "LookupScenario", "SouthArmy"),
          "a real lookup flags an armyName it does not know about");
}

// No duplicate/uniqueness check -- two placements at the same or different position, or even the
// same army/template, are both legal and neither is flagged.
void RunNoDuplicateCheckTest() {
    Params::Scenarios scenarios;
    Params::CountScenario scenario;
    scenario.body.name = "MultiUnit";
    Params::ScenarioUnitPlacement a; a.armyName = "ARMY_01"; a.templateIdentifier = "ucn3001";
    Params::ScenarioUnitPlacement b; b.armyName = "ARMY_01"; b.templateIdentifier = "ucn3001";
    scenario.body.unitPlacements = { a, b };
    scenarios.countScenarios.push_back(scenario);

    const Io::ScenarioUnitPlacementValidationReport report =
        Io::ValidateScenarioUnitPlacements(scenarios, nullptr);
    Check(report.AllValid(), "two identical, independent placement rows are never flagged as duplicates");
}

} // namespace

int main() {
    RunUnresolvableArmyNameChecks();
    RunWellFormedArmyNameNotFlaggedTest();
    RunEmptyTemplateIdentifierChecks();
    RunRealLookupChecks();
    RunNoDuplicateCheckTest();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
