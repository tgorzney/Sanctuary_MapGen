// MapExporter_Scenarios_IO.cpp — `recipe.scenarios` -> the top-level `.sanmap` `Scenarios` object.
// Layer: IO. Ordinary per-domain pair (DESIGN_MapScenarioIO_R1.md §0) — structurally nothing like
// the Lua-rendering leg (WO5/ScenarioScript_*_IO). The shared `<ScenarioRecord>` builder
// (`BuildScenarioRecordJson`) lives in MapExporter_ScenarioRecord_IO.cpp (ARCH §1.5 split, mirrors
// the import side's own MapImporter_ScenarioRecord_IO split); this file composes it for
// PatternScenarios/CountScenarios/DefaultScenario, plus the shared spawn-point pool (§15.12) and
// `MaxArmySlotCount`. Field shape per STEP69_ParamsScenariosRoundTrip_IO.md §1/§3/§5/§6 (this
// ticket's own inline tables are the binding source of truth — no live SANMAP_FORMAT_SPEC
// "Correction 17" exists to cite instead, per that ticket's 2026-08-22 correction).
#include "MapExporter_Recipe_IO.h"
#include "MapExporter_ScenarioRecord_IO.h"
#include "ScenarioSlotRangeValidation_IO.h"
#include "ScenarioSpawnIdValidation_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

constexpr const char* kCountFieldSpellings[4] = { "Total", "HumanCount", "AiCount", "SlotRangeOccupiedCount" };
constexpr const char* kComparatorSpellings[6] =
    { "Equal", "NotEqual", "GreaterThan", "GreaterOrEqual", "LessThan", "LessOrEqual" };

// Deliberately duplicated in MapExporter_ScenarioRecord_IO.cpp's own BuildPositionJson — see that
// file's own ATTENTION comment for the full coordinate-flip rationale.
nlohmann::ordered_json BuildPositionJson(float x, float y, float z, int mapSize) {
    return { { "x", x }, { "y", y }, { "z", static_cast<float>(mapSize) - z - 1.0f } };
}

// §15.12 (STEP252) — the shared, `Scenarios`-level custom-spawn-point pool, one per
// `Scenarios::spawnPoints` row.
nlohmann::ordered_json BuildSpawnPointsJson(const std::vector<Params::ScenarioSpawnPoint>& spawnPoints,
                                            int mapSize) {
    nlohmann::ordered_json array = nlohmann::ordered_json::array();
    for (const Params::ScenarioSpawnPoint& point : spawnPoints) {
        array.push_back({ { "SpawnId", point.spawnId }, { "ArmyName", point.armyName },
                         { "Position", BuildPositionJson(point.positionX, point.positionY,
                                                         point.positionZ, mapSize) } });
    }
    return array;
}

} // namespace

nlohmann::ordered_json BuildScenariosJson(const Params::MapRecipe& recipe) {
    const int mapSize = recipe.geometry.mapSize;
    // Export never trusts the roster is already clean (mirrors STEP76's AssignArmyIdentities
    // re-mint-defensively pattern) -- ValidateScenarioSpawnIds's fix-ups (ARCH_15_12 §15.12) are
    // applied to a COPY before any of the three tiers below render, so a reserved-namespace/
    // duplicate pool spawnId or a same-scenario duplicate armyName reference never reaches the wire.
    const Params::Scenarios scenarios = ApplyScenarioSpawnIdFixups(recipe.scenarios);

    nlohmann::ordered_json patternScenarios = nlohmann::ordered_json::array();
    for (const Params::PatternScenario& pattern : scenarios.patternScenarios) {
        nlohmann::ordered_json json = BuildScenarioRecordJson(pattern.body, mapSize, recipe.areas);
        json["Pattern"] = pattern.slotPattern;
        patternScenarios.push_back(json);
    }

    // LOAD-BEARING: its own `ordered_json::array()`, iterated in `countScenarios`'s own vector
    // order — never routed through a std::map/std::unordered_map that could reorder (§15.6).
    nlohmann::ordered_json countScenarios = nlohmann::ordered_json::array();
    for (const Params::CountScenario& countScenario : scenarios.countScenarios) {
        nlohmann::ordered_json json = BuildScenarioRecordJson(countScenario.body, mapSize, recipe.areas);
        nlohmann::ordered_json conditions = nlohmann::ordered_json::array();
        for (const Params::ScenarioCountCondition& condition : countScenario.conditions) {
            // An invalid SlotRangeOccupiedCount range refuses only THIS row, never the rest of the
            // scenario (ARCH_15_05 §15.5 AMENDED 2026-09-04; loud/logged via ScenarioSlotRangeValidation_IO).
            if (condition.field == Params::ScenarioCountField::SlotRangeOccupiedCount
                && !ScenarioSlotRangeIsValid(condition.slotRangeStart, condition.slotRangeEnd, scenarios.maxArmySlotCount))
                continue;
            // SlotRangeStart/SlotRangeEnd always emitted, meaningless-but-present otherwise (as Value is).
            conditions.push_back({ { "Field", kCountFieldSpellings[static_cast<int>(condition.field)] },
                                   { "Comparator", kComparatorSpellings[static_cast<int>(condition.comparator)] },
                                   { "Value", condition.value },
                                   { "SlotRangeStart", condition.slotRangeStart },
                                   { "SlotRangeEnd", condition.slotRangeEnd } });
        }
        json["Conditions"] = conditions;
        countScenarios.push_back(json);
    }

    nlohmann::ordered_json document;
    document["PatternScenarios"] = patternScenarios;
    document["CountScenarios"]   = countScenarios;
    document["DefaultScenario"]  = BuildScenarioRecordJson(scenarios.defaultScenario, mapSize, recipe.areas);
    // §15.10 amendment — top-level, map-wide slotPattern length; a sibling of the three above.
    document["MaxArmySlotCount"] = scenarios.maxArmySlotCount;
    // §15.12 amendment — the fifth top-level sibling key: the shared custom-spawn-point pool,
    // already fixed-up above.
    document["SpawnPoints"] = BuildSpawnPointsJson(scenarios.spawnPoints, mapSize);
    return document;
}

} // namespace Io
} // namespace SanmapGen
