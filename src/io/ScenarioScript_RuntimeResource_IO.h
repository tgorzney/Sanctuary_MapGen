// ScenarioScript_RuntimeResource_IO.h — resolves and reads the bundled Map Scenario runtime Lua
// text (`ARCH_15_04_ThreeFileOnDiskShape.md` §15.4 point 2, DESIGN_MapScenarioIO_R1.md §3): a designer-chosen override path, if
// set and readable, else the bundled default staged beside the executable
// (resources/lua/SanGenScenarioRuntime.lua at build time — CMakeLists.txt, mirrors the .glsl
// shader-staging pattern verbatim). Layer: IO. Pure resolve+read only — no rendering (that is
// ScenarioScript_DataLua_IO, STEP70), no write, no overwrite-safety (that is
// ScenarioScript_Export_IO, STEP71, the sole caller).
#pragma once
#include <string>

namespace SanmapGen {
namespace Io {

// ⚠️ AMENDS STEP71 §0's assumed doc comment on errorMessage ("populated only when bSucceeded ==
// false") — see this ticket's §0/"Amendment" section. errorMessage is a DIAGNOSTIC/ADVISORY
// string: EMPTY only on a fully clean resolution (override used cleanly, or bundled used with no
// override given); NON-EMPTY whenever there is something the human should see — including a
// successful degrade-to-bundled (bSucceeded == true) as well as a hard failure (bSucceeded ==
// false). ScenarioScript_Export_IO (STEP71) must log errorMessage whenever non-empty, never gated
// on bSucceeded alone — a silent degrade is exactly what Constitution §6 forbids.
struct ScenarioRuntimeResourceResult {
    bool        bSucceeded = false;
    std::string runtimeLuaText;      // full resolved text, INCLUDING its own
                                     // kScenarioGeneratedFileBannerLine first line — the bundled
                                     // resources/lua/SanGenScenarioRuntime.lua opens with it
                                     // verbatim (STEP70's banner literal, duplicated as plain text
                                     // in a non-C++ resource file, kept in lockstep manually).
    std::string sourceDescription;   // "bundled" or "override", for the caller's debugLog
    std::string errorMessage;        // see the amended contract note above
};

// runtimeResourceDirectory: the directory the bundled default is staged into (the staged
// `sangen_lua_resources` folder beside the executable, or the CMake-provided directory in tests)
// — never an absolute path baked into source. runtimeOverridePathOrEmpty:
// Io::AppSettings::scenarioRuntimeOverridePath (STEP64) verbatim, empty meaning "use the bundled
// default."
//
// Resolution order (DESIGN_MapScenarioIO_R1.md §3):
//   1. runtimeOverridePathOrEmpty non-empty and readable -> that text, sourceDescription="override".
//   2. Else <runtimeResourceDirectory>/SanGenScenarioRuntime.lua -> sourceDescription="bundled".
//   3. Override was given but unreadable -> DEGRADE to bundled (never a silent swap, never a hard
//      failure over an override problem alone) -- errorMessage carries the loud diagnostic even
//      though bSucceeded ends up true.
//   4. Neither readable -> bSucceeded = false, errorMessage names both attempted paths.
// Total: never throws.
ScenarioRuntimeResourceResult LoadScenarioRuntimeText(const std::string& runtimeResourceDirectory,
                                                      const std::string& runtimeOverridePathOrEmpty);

} // namespace Io
} // namespace SanmapGen
