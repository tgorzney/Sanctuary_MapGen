// MapExporter_ScenarioAreaNameValidation_IO.h -- `ScenarioAreaNameValidationReport` + the export-time
// ScenarioBody::areaName -> recipe.areas membership scan (STEP209). Layer: IO. Modelled directly on
// the sibling MapExporter_ArmySpawnMarkerValidation_IO.h: a report struct with a one-wording
// SummaryText(), plus a pure Validate* free function, same tier as recipe.IsValid(), never called
// from inside BuildSanmapJsonText/BuildScenarioDataLuaText.
//
// SHARED by both export legs (the .sanmap JSON leg, MapExporter_IO.cpp, and the Lua-rendering leg,
// ScenarioScript_Export_IO.cpp) -- unlike the per-leg spelling-table duplication precedent elsewhere
// in this file family, this validator carries no wire-format-specific content (no JSON, no Lua text),
// so there is no reason to fork it; it is pure Params-level logic.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

// One export-time areaName -> recipe.areas membership pass. WARN-ONLY: a stale reference (the named
// Area was renamed or deleted after a scenario picked it) is a legal, tolerated state
// (ARCH_15_05_ParamsScenariosType.md §15.5 AMENDED 2026-08-28) -- never auto-cleared, never blocking.
struct ScenarioAreaNameValidationReport {
    std::vector<std::string> staleReferences;   // one entry per scenario with a non-empty areaName
                                                 // not found in recipe.areas, in tier-then-vector
                                                 // order (pattern, then count, then default)
    bool AllReferencesResolve() const { return staleReferences.empty(); }
    std::string SummaryText() const;   // ONE wording -- shared by every call site
};

// Pure/read-only, touches no disk, never called from inside BuildSanmapJsonText/
// BuildScenarioDataLuaText -- same tier as recipe.IsValid().
ScenarioAreaNameValidationReport ValidateScenarioAreaNameReferences(const Params::MapRecipe& recipe);

} // namespace Io
} // namespace SanmapGen
