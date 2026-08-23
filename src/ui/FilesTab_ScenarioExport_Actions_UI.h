// FilesTab_ScenarioExport_Actions_UI.h — MODULE-INTERNAL seam for STEP77's
// FilesTabAction::ExportScenarioScript action, split out of FilesTab_Actions_UI.cpp for the
// Constitution §1.5 file-size ceiling. Nothing outside the FilesTab module includes this header;
// it declares no new public type (ARCH §8.4) — FilesTabState/FilesTabAction stay in FilesTab_UI.h.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct MapRecipe; struct Scenarios; }
namespace Ui {

struct FilesTabState;

// STEP77: a SEPARATE leg from RunRecipeExport — Io::ExportMapScenario (STEP71) never sets/reads
// Io::MapExportResult, and a scenario-leg failure never blocks a sanmap export
// (DESIGN_MapScenarioIO_R1.md §5, mirrored one layer up here). Headless — no imgui frame.
bool RunScenarioScriptExport(FilesTabState& state, const Params::MapRecipe& recipe);

// The mandatory-spawns confirm gate's pre-check (Fix §3): every `patternScenarios`/
// `countScenarios` entry STEP74's own `ScenarioNeedsSpawnsAcknowledgment` flags, reused rather
// than re-derived — `defaultScenario` is never included (STEP74's own exemption). Empty == the
// gate should not open; non-empty names every affected scenario, in list order. Pure/headless.
std::vector<std::string> ScenariosNeedingSpawnsAcknowledgment(const Params::Scenarios& scenarios);

} // namespace Ui
} // namespace SanmapGen
