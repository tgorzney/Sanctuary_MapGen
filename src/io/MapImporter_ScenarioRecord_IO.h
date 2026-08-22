// MapImporter_ScenarioRecord_IO.h — the shared `<ScenarioRecord>` body reader
// (`ReadScenarioBodyJson`), composed by MapImporter_Scenarios_IO.cpp's PatternScenarios/
// CountScenarios/DefaultScenario call sites. Split out under the ARCH §1.5 ceiling
// (MapImporter_Scenarios_IO.cpp would exceed the hard 150-line cap with this inlined) — same
// "shared plumbing gets its own small header" precedent as MapExporter_ScatterTransform_IO.h /
// MapImporter_ScatterTransform_IO.h. Declares no new public type (ARCH §8.4).
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Params { struct ScenarioBody; }
namespace Io {

// Inverse of MapExporter_Scenarios_IO.cpp's BuildScenarioRecordJson. `mapSize` is needed for the
// Spawns/Alloys/AlloysToAdd Position coordinate flip (see the .cpp's own ATTENTION comment).
void ReadScenarioBodyJson(const nlohmann::json& json, Params::ScenarioBody& body, int mapSize);

} // namespace Io
} // namespace SanmapGen
