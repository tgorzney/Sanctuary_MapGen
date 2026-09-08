// ScenarioSpawnIdValidation_IO.cpp — see the header for the file-level contract. STEP252
// (ARCH_15_12_ScenarioSpawnIdentity.md §15.12). `ResolveSpawnId`/`ValidateScenarioSpawnIds` here;
// `ApplyScenarioSpawnIdFixups` is split into its own translation unit,
// ScenarioSpawnIdFixup_IO.cpp, purely for the ARCH §1.5 file-size ceiling (both share this one
// header, per that ruling's own "one small header" guidance).
#include "ScenarioSpawnIdValidation_IO.h"
#include "Sanmap_ArmyIdentity_IO.h"
#include "../params/Scenario_PARAMS.h"
#include <algorithm>

namespace SanmapGen {
namespace Io {

bool ResolveSpawnId(const std::string& spawnId, const std::vector<Params::ScenarioSpawnPoint>& pool,
                    const ArmyIdentityTransformLookup* armyDefaults,
                    std::string& outArmyName, float& outX, float& outY, float& outZ) {
    // Step 1: the pool, first-match (mirrors this file family's own "first-match, never last-wins"
    // idiom -- areaName's ResolveScenarioAreaRect).
    for (const Params::ScenarioSpawnPoint& point : pool) {
        if (point.spawnId != spawnId) continue;
        outArmyName = point.armyName;
        outX = point.positionX; outY = point.positionY; outZ = point.positionZ;
        return true;
    }
    // Step 2: the literal ARMY_XX fallback.
    if (armyDefaults != nullptr) {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        if (armyDefaults->Find(spawnId, x, y, z)) {
            outArmyName = spawnId; outX = x; outY = y; outZ = z;
            return true;
        }
        return false;
    }
    // No live lookup available -- shape-only fallback, see the header's own doc comment for why
    // this never gives a false positive for the duplicate/unresolvable checks that consume it.
    if (IsArmyIdentityWellFormed(spawnId)) {
        outArmyName = spawnId; outX = 0.0f; outY = 0.0f; outZ = 0.0f;
        return true;
    }
    // Step 3: matches neither -- unresolvable.
    return false;
}

namespace {

std::string DescribeScenarioBody(const Params::ScenarioBody& body) {
    return body.name.empty() ? std::string("(unnamed scenario)") : body.name;
}

void CheckScenarioBodySpawnIds(const Params::ScenarioBody& body,
                               const std::vector<Params::ScenarioSpawnPoint>& pool,
                               ScenarioSpawnIdValidationReport& report) {
    const std::string descriptor = DescribeScenarioBody(body);
    std::vector<std::string> resolvedArmyNamesSoFar;
    for (const std::string& spawnId : body.spawnIds) {
        std::string armyName; float x = 0.0f, y = 0.0f, z = 0.0f;
        if (!ResolveSpawnId(spawnId, pool, nullptr, armyName, x, y, z)) {
            report.violations.push_back({ descriptor, spawnId,
                "spawnId matches neither the custom spawn-point pool nor a well-formed ARMY_XX "
                "identity; left as-authored (warn-only, never dropped)." });
            continue;
        }
        const bool bDuplicate = std::find(resolvedArmyNamesSoFar.begin(), resolvedArmyNamesSoFar.end(),
                                          armyName) != resolvedArmyNamesSoFar.end();
        if (bDuplicate) {
            report.violations.push_back({ descriptor, spawnId,
                "spawnId resolves to armyName \"" + armyName + "\", already claimed by an earlier "
                "spawnId in this same scenario; the later one is dropped on export." });
            continue;
        }
        resolvedArmyNamesSoFar.push_back(armyName);
    }
}

} // namespace

ScenarioSpawnIdValidationReport ValidateScenarioSpawnIds(const Params::Scenarios& scenarios) {
    ScenarioSpawnIdValidationReport report;

    std::vector<std::string> seenPoolSpawnIds;
    for (const Params::ScenarioSpawnPoint& point : scenarios.spawnPoints) {
        if (ResemblesArmyIdentityCaseInsensitive(point.spawnId)) {
            report.violations.push_back({ "pool", point.spawnId,
                "spawnId looks like an ARMY_XX identity, case-insensitively; that namespace is "
                "reserved and forbidden for a pool entry." });
            continue;
        }
        const bool bDuplicate = std::find(seenPoolSpawnIds.begin(), seenPoolSpawnIds.end(), point.spawnId)
                                != seenPoolSpawnIds.end();
        if (bDuplicate) {
            report.violations.push_back({ "pool", point.spawnId,
                "spawnId duplicates an earlier pool entry; only the first-authored row is kept." });
            continue;
        }
        seenPoolSpawnIds.push_back(point.spawnId);
    }

    for (const Params::PatternScenario& scenario : scenarios.patternScenarios)
        CheckScenarioBodySpawnIds(scenario.body, scenarios.spawnPoints, report);
    for (const Params::CountScenario& scenario : scenarios.countScenarios)
        CheckScenarioBodySpawnIds(scenario.body, scenarios.spawnPoints, report);
    CheckScenarioBodySpawnIds(scenarios.defaultScenario, scenarios.spawnPoints, report);

    return report;
}

// ONE wording, shared by every call site -- do not restate the phrasing elsewhere.
std::string ScenarioSpawnIdValidationReport::SummaryText() const {
    if (AllValid()) return std::string();
    std::string text = std::to_string(violations.size()) + " scenario spawnId issue(s) found:";
    for (const Violation& violation : violations)
        text += "\n  " + violation.descriptor + ": " + violation.detail + " -- " + violation.reason;
    return text;
}

} // namespace Io
} // namespace SanmapGen
