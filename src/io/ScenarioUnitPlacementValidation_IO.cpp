// ScenarioUnitPlacementValidation_IO.cpp -- see the header for the file-level contract. STEP260
// (ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md §15.14).
#include "ScenarioUnitPlacementValidation_IO.h"
#include "Sanmap_ArmyIdentity_IO.h"
#include "../params/Scenario_PARAMS.h"

namespace SanmapGen {
namespace Io {

namespace {

std::string DescribeScenarioBody(const Params::ScenarioBody& body) {
    return body.name.empty() ? std::string("(unnamed scenario)") : body.name;
}

// True when `armyName` resolves against the live roster (via `armyDefaults`), or -- with no live
// lookup available (export-time validation, this file's own pure/disk-free posture) -- a
// SHAPE-ONLY check: a well-formed ARMY_XX-shaped name is treated as resolving to itself. Mirrors
// ResolveSpawnId's own no-lookup fallback (ScenarioSpawnIdValidation_IO.cpp).
bool ResolvesArmyName(const std::string& armyName, const ArmyIdentityTransformLookup* armyDefaults) {
    if (armyDefaults != nullptr) {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        return armyDefaults->Find(armyName, x, y, z);
    }
    return IsArmyIdentityWellFormed(armyName);
}

void CheckScenarioBodyUnitPlacements(const Params::ScenarioBody& body,
                                     const ArmyIdentityTransformLookup* armyDefaults,
                                     ScenarioUnitPlacementValidationReport& report) {
    const std::string descriptor = DescribeScenarioBody(body);
    for (const Params::ScenarioUnitPlacement& placement : body.unitPlacements) {
        if (placement.templateIdentifier.empty()) {
            report.violations.push_back({ descriptor, placement.armyName,
                "unit placement has an empty templateIdentifier; left as-authored "
                "(warn-only, never dropped)." });
        }
        if (!ResolvesArmyName(placement.armyName, armyDefaults)) {
            report.violations.push_back({ descriptor, placement.armyName,
                "unit placement's armyName does not resolve against the live roster; left "
                "as-authored (warn-only, never dropped)." });
        }
    }
}

} // namespace

ScenarioUnitPlacementValidationReport ValidateScenarioUnitPlacements(
    const Params::Scenarios& scenarios, const ArmyIdentityTransformLookup* armyDefaults) {
    ScenarioUnitPlacementValidationReport report;

    for (const Params::PatternScenario& scenario : scenarios.patternScenarios)
        CheckScenarioBodyUnitPlacements(scenario.body, armyDefaults, report);
    for (const Params::CountScenario& scenario : scenarios.countScenarios)
        CheckScenarioBodyUnitPlacements(scenario.body, armyDefaults, report);
    CheckScenarioBodyUnitPlacements(scenarios.defaultScenario, armyDefaults, report);

    return report;
}

// ONE wording, shared by every call site -- do not restate the phrasing elsewhere.
std::string ScenarioUnitPlacementValidationReport::SummaryText() const {
    if (AllValid()) return std::string();
    std::string text = std::to_string(violations.size()) + " scenario unit placement issue(s) found:";
    for (const Violation& violation : violations)
        text += "\n  " + violation.descriptor + ": " + violation.detail + " -- " + violation.reason;
    return text;
}

} // namespace Io
} // namespace SanmapGen
