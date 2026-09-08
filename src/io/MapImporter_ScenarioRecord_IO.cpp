// MapImporter_ScenarioRecord_IO.cpp — see the header for the split rationale. The exact inverse of
// MapExporter_ScenarioRecord_IO.cpp's per-record builders. Field shape per
// STEP69_ParamsScenariosRoundTrip_IO.md §1/§3/§5/§6/§7 (this ticket's own inline tables are the
// binding source of truth — no live SANMAP_FORMAT_SPEC "Correction 17" exists to cite instead).
// "SpawnIds"/"SpawnPoints" per ARCH_15_12_ScenarioSpawnIdentity.md §15.12 (STEP252).
#include "MapImporter_ScenarioRecord_IO.h"
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
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

// RETIRED 2026-09-08 (STEP252, ARCH_15_12_ScenarioSpawnIdentity.md §15.12): ReadSpawnsJson (the old
// array-of-{ArmyName,Position} shape) is gone, replaced by ReadSpawnIdsJson below (per-record) and
// ReadSpawnPointsJson (Scenarios-level, exposed via the header — see its own doc comment there).
void ReadSpawnIdsJson(const nlohmann::json& parent, const char* key,
                     std::vector<std::string>& outSpawnIds) {
    if (!parent.contains(key) || !parent[key].is_array()) return;
    outSpawnIds.clear();
    for (const nlohmann::json& entry : parent[key])
        if (entry.is_string()) outSpawnIds.push_back(entry.get<std::string>());
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

// ARCH_15_12_ScenarioSpawnIdentity.md §15.12's own required migration posture for the "Spawns" ->
// "SpawnIds" breaking shape change: "if a scenario record's Spawns key is present with
// object-shaped elements ... log one loud warning ... never converted, never populated into
// spawnIds." The old shape's own structural signature is an array whose first element (if any) is
// a JSON object; the new shape's elements are plain strings.
void WarnIfLegacySpawnsShapePresent(const nlohmann::json& json, const Params::ScenarioBody& body,
                                    MapImportResult& result) {
    if (!json.contains("Spawns") || !json["Spawns"].is_array()) return;
    const nlohmann::json& spawnsArray = json["Spawns"];
    if (spawnsArray.empty() || !spawnsArray.front().is_object()) return;
    result.Warn("scenario \"" + body.name + "\" carries the RETIRED \"Spawns\" array-of-objects "
               "shape (pre-STEP252) -- NOT read, NOT converted. Hand-convert it to \"SpawnIds\" "
               "(a flat array of spawnId strings) per ARCH_15_12_ScenarioSpawnIdentity.md §15.12.");
}

} // namespace

// §15.12 (STEP252) — exposed (not anonymous-namespace-private) so MapImporter_Scenarios_IO.cpp can
// call it once, at the Scenarios-level parse site — see the header's own doc comment.
void ReadSpawnPointsJson(const nlohmann::json& parent, const char* key,
                        std::vector<Params::ScenarioSpawnPoint>& outSpawnPoints, int mapSize) {
    if (!parent.contains(key) || !parent[key].is_array()) return;
    outSpawnPoints.clear();
    for (const nlohmann::json& pointJson : parent[key]) {
        if (!pointJson.is_object()) continue;
        Params::ScenarioSpawnPoint point;
        ReadJsonText(pointJson, "SpawnId", point.spawnId);
        ReadJsonText(pointJson, "ArmyName", point.armyName);
        ReadPositionJson(pointJson, point.positionX, point.positionY, point.positionZ, mapSize);
        outSpawnPoints.push_back(point);
    }
}

// RETIRED 2026-08-28 (STEP204, human ruling): a pre-STEP204 `.sanmap`'s "Navy"/"NavalFleet" (and
// any "PondSide"/"PondAssignment" nested keys) are deprecated data — SILENTLY DROPPED here, never
// migrated, never warned, never an error. This function simply never reads them: `ReadJsonBoolean`/
// `ReadJson*` only ever look up the keys named below, so an old file's now-unread keys fall through
// exactly like any other unrecognized field at this nesting level (there is no strict/reject-
// unknown-key mode here to work around — confirmed by reading this function; see MapImporter_
// ScenariosRecord_IO_Test.cpp's legacy-fixture coverage).
void ReadScenarioBodyJson(const nlohmann::json& json, Params::ScenarioBody& body, int mapSize,
                          MapImportResult& result) {
    ReadJsonText(json, "Name", body.name);
    if (json.contains("Area") && json["Area"].is_object()) {
        const nlohmann::json& area = json["Area"];
        ReadJsonFloat(area, "x", body.area.originX);
        ReadJsonFloat(area, "y", body.area.originZ);
        ReadJsonFloat(area, "width", body.area.width);
        ReadJsonFloat(area, "height", body.area.length);
    }
    // Absent key (every pre-STEP209 .sanmap) -> stays at the struct default, empty. Never an error --
    // same idiom as every other plain string field in this function (e.g. AuthoringNote, below).
    ReadJsonText(json, "AreaName", body.areaName);
    // Absent key (every pre-STEP204 .sanmap) -> stays at the struct default, false. Never an error.
    ReadJsonBoolean(json, "SpawnsUnits", body.spawnsUnits);
    // `body` is pre-loaded (default-constructed = Occupancy) before this call — an absent/
    // unrecognized AlloyMode leaves it untouched, exactly ReadArmyJson's `faction` idiom (§7).
    int alloyModeValue = static_cast<int>(body.alloyMode);
    if (ReadJsonEnumerationText(json, "AlloyMode", kAlloyModeSpellings, kAlloyModeCount, alloyModeValue))
        body.alloyMode = static_cast<Params::ScenarioAlloyMode>(alloyModeValue);
    WarnIfLegacySpawnsShapePresent(json, body, result);
    ReadSpawnIdsJson(json, "SpawnIds", body.spawnIds);
    ReadAlloyOverridesJson(json, "Alloys", body.alloys, mapSize);
    ReadAlloyOverridesJson(json, "AlloysToAdd", body.alloysToAdd, mapSize);
    ReadAlloyRemovalsJson(json, "AlloysToRemove", body.alloysToRemove);
    ReadJsonText(json, "AuthoringNote", body.authoringNote);
}

} // namespace Io
} // namespace SanmapGen
