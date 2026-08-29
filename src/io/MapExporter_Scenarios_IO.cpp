// MapExporter_Scenarios_IO.cpp — `recipe.scenarios` -> the top-level `.sanmap` `Scenarios` object.
// Layer: IO. Ordinary per-domain pair (DESIGN_MapScenarioIO_R1.md §0) — structurally nothing like
// the Lua-rendering leg (WO5/ScenarioScript_*_IO). Mirrors BuildArmiesJson's layered-builder shape:
// one private BuildScenarioRecordJson composed by three call sites (PatternScenarios/
// CountScenarios/DefaultScenario). Field shape per STEP69_ParamsScenariosRoundTrip_IO.md §1/§3/§5/
// §6 (this ticket's own inline tables are the binding source of truth — no live SANMAP_FORMAT_SPEC
// "Correction 17" exists to cite instead, per that ticket's 2026-08-22 correction).
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// Index == the enum's own declaration order (ARCH_15_05_ParamsScenariosType.md §15.5) — do not
// reorder. Domain-local, mirrors markerCategoryCount-style per-domain constants.
constexpr const char* kAlloyModeSpellings[4]  = { "explicit", "occupancy", "keepAll", "delta" };
constexpr const char* kCountFieldSpellings[3] = { "Total", "HumanCount", "AiCount" };
constexpr const char* kComparatorSpellings[6] =
    { "Equal", "NotEqual", "GreaterThan", "GreaterOrEqual", "LessThan", "LessOrEqual" };

// ⚠️ ATTENTION — COORDINATE FLIP UNCONFIRMED FOR SCENARIOS. Applies the same `mapSize - z - 1`
// flip every other InstancedTransform-shaped position field uses (Armies/Markers/Props/Decals);
// NOT independently ratified for Scenarios — chosen for consistency, human's 2026-08-21 ruling to
// build now, verify later. IF SPAWNS/ALLOYS APPEAR MIRRORED ALONG Z IN-GAME, THIS IS THE FIRST
// PLACE TO LOOK: remove the flip here AND at the matching import call site. Round-trip tests pass
// either way (export/import are inverses) — only in-game verification catches a wrong choice.
nlohmann::ordered_json BuildPositionJson(float x, float y, float z, int mapSize) {
    return { { "x", x }, { "y", y }, { "z", static_cast<float>(mapSize) - z - 1.0f } };
}

nlohmann::ordered_json BuildSpawnsJson(const std::vector<Params::ScenarioSpawn>& spawns, int mapSize) {
    nlohmann::ordered_json array = nlohmann::ordered_json::array();
    for (const Params::ScenarioSpawn& spawn : spawns) {
        array.push_back({ { "ArmyName", spawn.armyName },
                          { "Position", BuildPositionJson(spawn.positionX, spawn.positionY, spawn.positionZ, mapSize) } });
    }
    return array;
}

nlohmann::ordered_json BuildAlloyOverridesJson(const std::vector<Params::ScenarioAlloyOverride>& overrides,
                                               int mapSize) {
    nlohmann::ordered_json array = nlohmann::ordered_json::array();
    for (const Params::ScenarioAlloyOverride& entry : overrides) {
        array.push_back({ { "ArmyName", entry.armyName }, { "MarkerName", entry.markerName },
                          { "Position", BuildPositionJson(entry.positionX, entry.positionY, entry.positionZ, mapSize) } });
    }
    return array;
}

nlohmann::ordered_json BuildAlloyRemovalsJson(const std::vector<Params::ScenarioAlloyRemoval>& removals) {
    nlohmann::ordered_json array = nlohmann::ordered_json::array();
    for (const Params::ScenarioAlloyRemoval& removal : removals)
        array.push_back({ { "ArmyName", removal.armyName }, { "MarkerName", removal.markerName } });
    return array;
}

// Resolves body.areaName against `areas` (first-match by .name, mirroring this exact file family's
// own established idiom -- AreasTab_List_UI.h's ResolveAreaColor, UniqueNameList_UI.h's
// NameIsTakenBefore -- both resolve by first/earliest match, never last-wins). Empty areaName or an
// unresolvable (stale) name both fall back to body.area unchanged -- never crash, never emit garbage
// (ARCH_15_05_ParamsScenariosType.md §15.5 AMENDED 2026-08-28).
Params::MapArea ResolveScenarioAreaRect(const Params::ScenarioBody& body,
                                        const std::vector<Params::MapArea>& areas) {
    if (body.areaName.empty()) return body.area;
    for (const Params::MapArea& area : areas)
        if (area.name == body.areaName) return area;
    return body.area;
}

// The shared 9-field `<ScenarioRecord>` body, in the wire's own listed field order — composed by
// all three of PatternScenarios/CountScenarios/DefaultScenario below (mirrors BuildArmiesJson).
nlohmann::ordered_json BuildScenarioRecordJson(const Params::ScenarioBody& body, int mapSize,
                                               const std::vector<Params::MapArea>& areas) {
    nlohmann::ordered_json json;
    json["Name"] = body.name;
    // Resolved against recipe.areas when body.areaName names a live entry; falls back to body.area
    // otherwise (empty areaName, or a stale/renamed/deleted reference). See ResolveScenarioAreaRect.
    const Params::MapArea resolvedArea = ResolveScenarioAreaRect(body, areas);
    json["Area"] = { { "x", resolvedArea.originX }, { "y", resolvedArea.originZ },
                     { "width", resolvedArea.width }, { "height", resolvedArea.length } };
    // Sibling of Area, always emitted even when empty -- matches SpawnsUnits/AuthoringNote's own
    // "every scalar field always present" convention (ARCH_15_05_ParamsScenariosType.md §15.5
    // AMENDED 2026-08-28: round-trip the reference, never export-only bake).
    json["AreaName"] = body.areaName;
    // RETIRED 2026-08-28 (STEP204): "Navy" + "NavalFleet" are gone. "SpawnsUnits" is always
    // present, even when false — matches how every other scalar scenario field is emitted.
    json["SpawnsUnits"] = body.spawnsUnits;
    json["AlloyMode"]   = kAlloyModeSpellings[static_cast<int>(body.alloyMode)];
    json["Spawns"]         = BuildSpawnsJson(body.spawns, mapSize);
    json["Alloys"]         = BuildAlloyOverridesJson(body.alloys, mapSize);
    json["AlloysToAdd"]    = BuildAlloyOverridesJson(body.alloysToAdd, mapSize);
    json["AlloysToRemove"] = BuildAlloyRemovalsJson(body.alloysToRemove);
    json["AuthoringNote"]  = body.authoringNote;
    return json;
}

} // namespace

nlohmann::ordered_json BuildScenariosJson(const Params::MapRecipe& recipe) {
    const int mapSize = recipe.geometry.mapSize;
    const Params::Scenarios& scenarios = recipe.scenarios;

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
            conditions.push_back({ { "Field", kCountFieldSpellings[static_cast<int>(condition.field)] },
                                   { "Comparator", kComparatorSpellings[static_cast<int>(condition.comparator)] },
                                   { "Value", condition.value } });
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
    return document;
}

} // namespace Io
} // namespace SanmapGen
