// MapImporter_ScenarioRecord_IO.h — the shared `<ScenarioRecord>` body reader
// (`ReadScenarioBodyJson`), composed by MapImporter_Scenarios_IO.cpp's PatternScenarios/
// CountScenarios/DefaultScenario call sites. Split out under the ARCH §1.5 ceiling
// (MapImporter_Scenarios_IO.cpp would exceed the hard 150-line cap with this inlined) — same
// "shared plumbing gets its own small header" precedent as MapExporter_ScatterTransform_IO.h /
// MapImporter_ScatterTransform_IO.h. Declares no new public type (ARCH §8.4).
#pragma once
#include <nlohmann/json.hpp>
#include <vector>

namespace SanmapGen {
namespace Params { struct ScenarioBody; struct ScenarioSpawnPoint; }
namespace Io {

struct MapImportResult;

// Inverse of MapExporter_ScenarioRecord_IO.cpp's BuildScenarioRecordJson. `mapSize` is needed for
// the SpawnIds/Alloys/AlloysToAdd Position coordinate flip (see the .cpp's own ATTENTION comment).
// `result` receives a WARN-ONLY notice (ARCH_15_12_ScenarioSpawnIdentity.md §15.12) when this
// record still carries the RETIRED "Spawns" array-of-objects shape — never read, never converted.
void ReadScenarioBodyJson(const nlohmann::json& json, Params::ScenarioBody& body, int mapSize,
                          MapImportResult& result);

// §15.12 (STEP252) — the shared, `Scenarios`-level custom-spawn-point pool. Called ONCE, at the
// `Scenarios`-level parse site (MapImporter_Scenarios_IO.cpp, the same tier as `MaxArmySlotCount`),
// never per-`ScenarioBody` — exposed here (rather than staying `ReadScenarioBodyJson`-anonymous-
// namespace-private) purely so that cross-translation-unit call site can reach it.
void ReadSpawnPointsJson(const nlohmann::json& parent, const char* key,
                        std::vector<Params::ScenarioSpawnPoint>& outSpawnPoints, int mapSize);

} // namespace Io
} // namespace SanmapGen
