// ScenarioScript_CategoryFourExport_IO.h -- category-4 on-disk shape (ARCH_15_04_ThreeFileOnDiskShape.md
// "AMENDED 2026-09-03", MAP_SCENARIO_SPEC.md §11.2/§14 point 4): the per-scenario hand-authored
// unit-spawn generator file, <MapName>_Scenarios_<ScenarioName>.lua. Layer: IO. Scaffold-once-if-missing,
// then permanently hands-off -- a THIRD overwrite-safety class, distinct from both
// ScenarioScript_Export_IO's existing banner-gated always-regenerate class (categories 2-3) and
// category 1's never-write-ever posture. Sole caller: ScenarioScript_Export_IO::ExportMapScenario,
// called once per export, after the Data.lua/Runtime.lua legs.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

// The weaker banner every category-4 scaffold opens with. UNLIKE kScenarioGeneratedFileBannerLine
// (categories 2-3), this banner is NEVER read back or compared against to decide whether to overwrite
// -- once a category-4 file exists in ANY state (this banner, hand-edited, no banner at all), SanGen
// never touches it again. It exists purely so the scaffold's own text carries a consistent,
// machine-greppable literal a human can search for across every map.
inline constexpr const char* kScenarioCategoryFourScaffoldBannerLine =
    "-- SANGEN-CREATED STARTING POINT -- freely hand-edit -- never regenerated";

struct ScenarioCategoryFourExportReport {
    std::vector<std::string> scaffoldedFilePaths;   // new generator-file scaffolds written this export
                                                     // -- one entry per spawnsUnits==true scenario whose
                                                     // file did not yet exist
    std::vector<std::string> writeRefusals;         // spawnsUnits==true scenarios whose own scaffold
                                                     // write was refused this export (an invalid name,
                                                     // or -- structurally near-unreachable given
                                                     // upstream charset validation -- a syntax-check
                                                     // failure on the rendered scaffold text), each a
                                                     // fully-formatted message
};

// mapScriptDirectory: LJ/lua/maps/<MapName>/, already created by the caller (ExportMapScenario's own
// step 2) -- this function never creates it itself. For every ScenarioBody across
// recipe.scenarios.patternScenarios/countScenarios/defaultScenario with spawnsUnits == true:
//   1. Validate its own name via Io::ValidateScenarioNames(recipe.scenarios) (computed ONCE per call,
//      not once per scenario). An invalid name refuses THAT scenario's write only -- appends a message
//      to writeRefusals and moves on; every other scenario's export is unaffected.
//   2. std::filesystem::exists on the literal path mapScriptDirectory/<MapName>_Scenarios_<Name>.lua --
//      present in ANY state -> skip silently (no read, no comparison, never touched again, not even
//      logged as a no-op -- this is the expected common case on every export after the first). Absent
//      -> render the minimal scaffold, run it through Sys::CheckLuaSyntax (defensive -- structurally
//      unreachable given every rendered placeholder is charset-restricted to [A-Za-z0-9_] and only ever
//      appears inside `--` comment lines, but Constitution §6 asks SanGen to validate what it itself
//      writes too), write it, append its path to scaffoldedFilePaths.
// Total: never throws, never touches a category-4 file already on disk in any state.
ScenarioCategoryFourExportReport ExportScenarioCategoryFourScaffolds(
    const std::string& mapScriptDirectory, const Params::MapRecipe& recipe);

} // namespace Io
} // namespace SanmapGen
