// MapExporter_ScenarioRecord_IO.h — the shared `<ScenarioRecord>` body builder
// (`BuildScenarioRecordJson`), composed by MapExporter_Scenarios_IO.cpp's PatternScenarios/
// CountScenarios/DefaultScenario call sites. Split out under the ARCH §1.5 ceiling
// (MapExporter_Scenarios_IO.cpp would exceed the hard 150-line cap with this inlined) — mirrors the
// exact precedent already established on the import side, MapImporter_ScenarioRecord_IO.h/.cpp.
// Declares no new public type (ARCH §8.4).
#pragma once
#include <nlohmann/json.hpp>
#include <vector>

namespace SanmapGen {
namespace Params { struct ScenarioBody; struct MapArea; }
namespace Io {

// The exact inverse of MapImporter_ScenarioRecord_IO.h's ReadScenarioBodyJson. `mapSize` is needed
// for the SpawnIds/Alloys/AlloysToAdd Position coordinate flip (see the .cpp's own ATTENTION comment).
nlohmann::ordered_json BuildScenarioRecordJson(const Params::ScenarioBody& body, int mapSize,
                                               const std::vector<Params::MapArea>& areas);

} // namespace Io
} // namespace SanmapGen
