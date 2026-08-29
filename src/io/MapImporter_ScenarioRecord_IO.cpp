// MapImporter_ScenarioRecord_IO.cpp — see the header for the split rationale. The exact inverse of
// MapExporter_Scenarios_IO.cpp's per-record builders. Field shape per
// STEP69_ParamsScenariosRoundTrip_IO.md §1/§3/§5/§6/§7 (this ticket's own inline tables are the
// binding source of truth — no live SANMAP_FORMAT_SPEC "Correction 17" exists to cite instead).
#include "MapImporter_ScenarioRecord_IO.h"
#include "JsonPrimitives_IO.h"
#include "../params/Scenario_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// Index == the enum's own declaration order (ARCH_15_05_ParamsScenariosType.md §15.5) — mirrors
// the exporter's own copy (each domain file owns its own, per existing per-domain precedent).
constexpr const char* kAlloyModeSpellings[4] = { "explicit", "occupancy", "keepAll", "delta" };
constexpr int         kAlloyModeCount        = 4;

// ⚠️ ATTENTION — COORDINATE FLIP UNCONFIRMED FOR SCENARIOS. Inverts the same `mapSize - z - 1`
// flip MapExporter_Scenarios_IO.cpp's BuildPositionJson applies — see that file's own ATTENTION
// comment for the full rationale/unconfirmed-choice framing; the two sites are the entire blast
// radius if a future in-game check finds this wrong.
void ReadPositionJson(const nlohmann::json& parent, float& x, float& y, float& z, int mapSize) {
    if (!parent.contains("Position") || !parent["Position"].is_object()) return;
    const nlohmann::json& position = parent["Position"];
    ReadJsonFloat(position, "x", x);
    ReadJsonFloat(position, "y", y);
    float jsonZ = static_cast<float>(mapSize) - z - 1.0f;
    if (ReadJsonFloat(position, "z", jsonZ)) z = static_cast<float>(mapSize) - jsonZ - 1.0f;
}

void ReadSpawnsJson(const nlohmann::json& parent, const char* key,
                    std::vector<Params::ScenarioSpawn>& outSpawns, int mapSize) {
    if (!parent.contains(key) || !parent[key].is_array()) return;
    outSpawns.clear();
    for (const nlohmann::json& spawnJson : parent[key]) {
        if (!spawnJson.is_object()) continue;
        Params::ScenarioSpawn spawn;
        ReadJsonText(spawnJson, "ArmyName", spawn.armyName);
        ReadPositionJson(spawnJson, spawn.positionX, spawn.positionY, spawn.positionZ, mapSize);
        outSpawns.push_back(spawn);
    }
}

void ReadAlloyOverridesJson(const nlohmann::json& parent, const char* key,
                            std::vector<Params::ScenarioAlloyOverride>& outOverrides, int mapSize) {
    if (!parent.contains(key) || !parent[key].is_array()) return;
    outOverrides.clear();
    for (const nlohmann::json& entryJson : parent[key]) {
        if (!entryJson.is_object()) continue;
        Params::ScenarioAlloyOverride entry;
        ReadJsonText(entryJson, "ArmyName", entry.armyName);
        ReadJsonText(entryJson, "MarkerName", entry.markerName);
        ReadPositionJson(entryJson, entry.positionX, entry.positionY, entry.positionZ, mapSize);
        outOverrides.push_back(entry);
    }
}

void ReadAlloyRemovalsJson(const nlohmann::json& parent, const char* key,
                           std::vector<Params::ScenarioAlloyRemoval>& outRemovals) {
    if (!parent.contains(key) || !parent[key].is_array()) return;
    outRemovals.clear();
    for (const nlohmann::json& entryJson : parent[key]) {
        if (!entryJson.is_object()) continue;
        Params::ScenarioAlloyRemoval entry;
        ReadJsonText(entryJson, "ArmyName", entry.armyName);
        ReadJsonText(entryJson, "MarkerName", entry.markerName);
        outRemovals.push_back(entry);
    }
}

} // namespace

// RETIRED 2026-08-28 (STEP204, human ruling): a pre-STEP204 `.sanmap`'s "Navy"/"NavalFleet" (and
// any "PondSide"/"PondAssignment" nested keys) are deprecated data — SILENTLY DROPPED here, never
// migrated, never warned, never an error. This function simply never reads them: `ReadJsonBoolean`/
// `ReadJson*` only ever look up the keys named below, so an old file's now-unread keys fall through
// exactly like any other unrecognized field at this nesting level (there is no strict/reject-
// unknown-key mode here to work around — confirmed by reading this function; see MapImporter_
// ScenariosRecord_IO_Test.cpp's legacy-fixture coverage).
void ReadScenarioBodyJson(const nlohmann::json& json, Params::ScenarioBody& body, int mapSize) {
    ReadJsonText(json, "Name", body.name);
    if (json.contains("Area") && json["Area"].is_object()) {
        const nlohmann::json& area = json["Area"];
        ReadJsonFloat(area, "x", body.area.originX);
        ReadJsonFloat(area, "y", body.area.originZ);
        ReadJsonFloat(area, "width", body.area.width);
        ReadJsonFloat(area, "height", body.area.length);
    }
    // Absent key (every pre-STEP204 .sanmap) -> stays at the struct default, false. Never an error.
    ReadJsonBoolean(json, "SpawnsUnits", body.spawnsUnits);
    // `body` is pre-loaded (default-constructed = Occupancy) before this call — an absent/
    // unrecognized AlloyMode leaves it untouched, exactly ReadArmyJson's `faction` idiom (§7).
    int alloyModeValue = static_cast<int>(body.alloyMode);
    if (ReadJsonEnumerationText(json, "AlloyMode", kAlloyModeSpellings, kAlloyModeCount, alloyModeValue))
        body.alloyMode = static_cast<Params::ScenarioAlloyMode>(alloyModeValue);
    ReadSpawnsJson(json, "Spawns", body.spawns, mapSize);
    ReadAlloyOverridesJson(json, "Alloys", body.alloys, mapSize);
    ReadAlloyOverridesJson(json, "AlloysToAdd", body.alloysToAdd, mapSize);
    ReadAlloyRemovalsJson(json, "AlloysToRemove", body.alloysToRemove);
    ReadJsonText(json, "AuthoringNote", body.authoringNote);
}

} // namespace Io
} // namespace SanmapGen
