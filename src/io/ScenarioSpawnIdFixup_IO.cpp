// ScenarioSpawnIdFixup_IO.cpp — `ApplyScenarioSpawnIdFixups`, split out of ScenarioSpawnIdValidation_IO.cpp
// purely for the ARCH §1.5 file-size ceiling (both share ScenarioSpawnIdValidation_IO.h). STEP252
// (ARCH_15_12_ScenarioSpawnIdentity.md §15.12).
#include "ScenarioSpawnIdValidation_IO.h"
#include "Sanmap_ArmyIdentity_IO.h"
#include "../params/Scenario_PARAMS.h"
#include <algorithm>

namespace SanmapGen {
namespace Io {
namespace {

bool PoolEntryIsReservedOrDuplicate(const std::vector<Params::ScenarioSpawnPoint>& keptSoFar,
                                    const std::string& spawnId) {
    if (ResemblesArmyIdentityCaseInsensitive(spawnId)) return true;
    for (const Params::ScenarioSpawnPoint& kept : keptSoFar)
        if (kept.spawnId == spawnId) return true;
    return false;
}

void ApplyBodySpawnIdFixups(Params::ScenarioBody& body,
                            const std::vector<Params::ScenarioSpawnPoint>& fixedUpPool) {
    std::vector<std::string> keptSpawnIds;
    std::vector<std::string> resolvedArmyNamesSoFar;
    keptSpawnIds.reserve(body.spawnIds.size());
    for (const std::string& spawnId : body.spawnIds) {
        std::string armyName; float x = 0.0f, y = 0.0f, z = 0.0f;
        if (ResolveSpawnId(spawnId, fixedUpPool, nullptr, armyName, x, y, z)) {
            const bool bDuplicate = std::find(resolvedArmyNamesSoFar.begin(), resolvedArmyNamesSoFar.end(),
                                              armyName) != resolvedArmyNamesSoFar.end();
            if (bDuplicate) continue;   // later entry resolving to an already-used armyName: dropped
            resolvedArmyNamesSoFar.push_back(armyName);
        }
        // Unresolvable entries are left AS-AUTHORED, warn-only (never dropped here).
        keptSpawnIds.push_back(spawnId);
    }
    body.spawnIds = keptSpawnIds;
}

} // namespace

Params::Scenarios ApplyScenarioSpawnIdFixups(const Params::Scenarios& scenarios) {
    Params::Scenarios fixedUp = scenarios;

    std::vector<Params::ScenarioSpawnPoint> keptPool;
    keptPool.reserve(fixedUp.spawnPoints.size());
    for (const Params::ScenarioSpawnPoint& point : fixedUp.spawnPoints) {
        if (PoolEntryIsReservedOrDuplicate(keptPool, point.spawnId)) continue;
        keptPool.push_back(point);
    }
    fixedUp.spawnPoints = keptPool;

    for (Params::PatternScenario& scenario : fixedUp.patternScenarios)
        ApplyBodySpawnIdFixups(scenario.body, fixedUp.spawnPoints);
    for (Params::CountScenario& scenario : fixedUp.countScenarios)
        ApplyBodySpawnIdFixups(scenario.body, fixedUp.spawnPoints);
    ApplyBodySpawnIdFixups(fixedUp.defaultScenario, fixedUp.spawnPoints);

    return fixedUp;
}

} // namespace Io
} // namespace SanmapGen
