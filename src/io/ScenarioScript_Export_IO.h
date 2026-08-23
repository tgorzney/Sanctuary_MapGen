// ScenarioScript_Export_IO.h — the Map Scenario export orchestrator: given a validated game install
// root and a Params::MapRecipe, writes <MapName>_Scenarios_Runtime.lua and
// <MapName>_Scenarios_Data.lua into LJ/lua/maps/<MapName>/ under the banner-gated overwrite-safety
// rule (`ARCH_15_04_ThreeFileOnDiskShape.md` §15.4, MAP_SCENARIO_SPEC.md §2.1). Layer: IO. The ONE entry point UI calls for the
// scenario export leg.
//
// SEPARATE from MapExporter_IO::ExportAll/ExportSanmapOnly, by design (DESIGN_MapScenarioIO_R1.md
// §5) -- a scenario-leg failure NEVER sets MapExportResult::bSucceeded = false, and a .sanmap/asset
// export failure never blocks this. Two independent calls, two independent result types, never
// merged.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

// Mirrors MapExportResult's shape but is a DISTINCT type -- never merged with it.
struct ScenarioExportResult {
    bool bDataLuaWritten           = false;  // <MapName>_Scenarios_Data.lua written this export
    bool bRuntimeCopied            = false;  // <MapName>_Scenarios_Runtime.lua written this export
    bool bOrchestratorPresent      = false;  // <MapName>_data.lua exists; false => warn, files inert
    bool bDataLuaCollisionDetected = false;  // unrecognized content occupied the Data.lua path
    bool bRuntimeCollisionDetected = false;  // unrecognized content occupied the Runtime.lua path
                                             // (symmetric with the data flag -- see correction 1)
    bool bDataLuaSyntaxCheckFailed = false;  // LuaSyntaxCheck_SYS rejected the RENDERED data text;
                                             // write refused -- separate from a collision
    bool bRuntimeSyntaxCheckFailed = false;  // LuaSyntaxCheck_SYS rejected the resolved runtime
                                             // text (bundled or override); write refused
    std::vector<std::string> writtenFilePaths;
    std::string              debugLog;

    void Log(const std::string& line) { debugLog += line; debugLog += '\n'; }
};

// gameInstallRoot: validated by this function itself via
// GameInstallLocation_IO::ValidateGameInstallRoot (STEP64). recipe.mapName names the target
// subfolder and the three files' shared filename prefix. runtimeResourceDirectory/
// runtimeOverridePathOrEmpty pass through verbatim to WO6's LoadScenarioRuntimeText (§0).
ScenarioExportResult ExportMapScenario(const std::string& gameInstallRoot,
                                       const Params::MapRecipe& recipe,
                                       const std::string& runtimeResourceDirectory,
                                       const std::string& runtimeOverridePathOrEmpty);

} // namespace Io
} // namespace SanmapGen
