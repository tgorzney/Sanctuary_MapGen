// MapExporter_ScenarioRecord_IO.cpp — see the header for the split rationale. The exact inverse of
// MapImporter_ScenarioRecord_IO.cpp's per-record readers. Field shape per
// STEP69_ParamsScenariosRoundTrip_IO.md §1/§3/§5/§6 (this ticket's own inline tables are the
// binding source of truth), `SpawnIds` reshaped per ARCH_15_12_ScenarioSpawnIdentity.md §15.12
// (STEP252).
#include "MapExporter_ScenarioRecord_IO.h"
#include "../params/MapArea_PARAMS.h"
#include "../params/Scenario_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// Index == the enum's own declaration order (ARCH_15_05_ParamsScenariosType.md §15.5) — do not
// reorder. Domain-local, mirrors markerCategoryCount-style per-domain constants. Deliberately
// duplicated in MapExporter_Scenarios_IO.cpp's own kAlloyModeSpellings (each file owns its own
// copy, STEP69 §5 precedent) — this file never #includes that one.
constexpr const char* kAlloyModeSpellings[4] = { "explicit", "occupancy", "keepAll", "delta" };

// ⚠️ ATTENTION — COORDINATE FLIP UNCONFIRMED FOR SCENARIOS. Applies the same `mapSize - z - 1`
// flip every other InstancedTransform-shaped position field uses (Armies/Markers/Props/Decals);
// NOT independently ratified for Scenarios — chosen for consistency, human's 2026-08-21 ruling to
// build now, verify later. IF SPAWNS/ALLOYS APPEAR MIRRORED ALONG Z IN-GAME, THIS IS THE FIRST
// PLACE TO LOOK: remove the flip here AND at the matching import call site. Round-trip tests pass
// either way (export/import are inverses) — only in-game verification catches a wrong choice.
// Deliberately duplicated in MapExporter_Scenarios_IO.cpp's own BuildPositionJson (tiny, and each
// half of this ARCH §1.5 split stays self-contained rather than sharing a header for 3 lines).
nlohmann::ordered_json BuildPositionJson(float x, float y, float z, int mapSize) {
    return { { "x", x }, { "y", y }, { "z", static_cast<float>(mapSize) - z - 1.0f } };
}

nlohmann::ordered_json BuildSpawnIdsJson(const std::vector<std::string>& spawnIds) {
    nlohmann::ordered_json array = nlohmann::ordered_json::array();
    for (const std::string& spawnId : spawnIds) array.push_back(spawnId);
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

} // namespace

// The shared 9-field `<ScenarioRecord>` body, in the wire's own listed field order — composed by
// all three of PatternScenarios/CountScenarios/DefaultScenario (mirrors BuildArmiesJson).
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
    // RESHAPED 2026-09-08 (STEP252, was "Spawns"/BuildSpawnsJson, array-of-{ArmyName,Position}) —
    // ARCH_15_12_ScenarioSpawnIdentity.md §15.12: a flat array of strings.
    json["SpawnIds"]       = BuildSpawnIdsJson(body.spawnIds);
    json["Alloys"]         = BuildAlloyOverridesJson(body.alloys, mapSize);
    json["AlloysToAdd"]    = BuildAlloyOverridesJson(body.alloysToAdd, mapSize);
    json["AlloysToRemove"] = BuildAlloyRemovalsJson(body.alloysToRemove);
    json["AuthoringNote"]  = body.authoringNote;
    return json;
}

} // namespace Io
} // namespace SanmapGen
