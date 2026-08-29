// MapExporter_ScenarioAreaNameValidation_IO.cpp -- `ValidateScenarioAreaNameReferences` and
// `ScenarioAreaNameValidationReport::SummaryText`. Layer: IO. STEP209.
//
// Design ruling (ARCH_15_05_ParamsScenariosType.md §15.5 AMENDED 2026-08-28): this function only
// REPORTS -- it never mutates `recipe`, touches no disk, and stays a sibling pre-flight step, the
// same tier as `recipe.IsValid()`/`ValidateArmiesHaveSpawnMarkers`, never called from inside
// BuildSanmapJsonText/BuildScenarioDataLuaText.
#include "MapExporter_ScenarioAreaNameValidation_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// Never-blank scenario descriptor, mirroring ScenarioRowLabel's "never blank" posture without
// depending on the UI layer (downward-only deps, Constitution §3) -- IO owns its own copy.
std::string ScenarioAreaNameDescriptor(const Params::ScenarioBody& body, const char* tierLabel, int index) {
    if (!body.name.empty()) return body.name;
    return std::string(tierLabel) + " Scenario #" + std::to_string(index + 1);
}

// First-match scan against recipe.areas -- same idiom as MapExporter_Scenarios_IO.cpp's own
// ResolveScenarioAreaRect (STEP209 §3a); this file owns its own copy of the lookup rather than
// including that file (per-file-duplication precedent this file family already established).
bool AreaNameResolves(const std::string& areaName, const std::vector<Params::MapArea>& areas) {
    for (const Params::MapArea& area : areas)
        if (area.name == areaName) return true;
    return false;
}

void CheckOneBody(const Params::ScenarioBody& body, const char* tierLabel, int index,
                  const std::vector<Params::MapArea>& areas, ScenarioAreaNameValidationReport& report) {
    if (body.areaName.empty()) return;
    if (AreaNameResolves(body.areaName, areas)) return;
    report.staleReferences.push_back(ScenarioAreaNameDescriptor(body, tierLabel, index)
                                     + " -> \"" + body.areaName + "\"");
}

} // namespace

ScenarioAreaNameValidationReport ValidateScenarioAreaNameReferences(const Params::MapRecipe& recipe) {
    ScenarioAreaNameValidationReport report;

    // Tier order: pattern, then count, then default (same tier order §6.1/§14 of
    // MAP_SCENARIO_SPEC.md already establish elsewhere in this file family).
    for (std::size_t index = 0u; index < recipe.scenarios.patternScenarios.size(); ++index)
        CheckOneBody(recipe.scenarios.patternScenarios[index].body, "Pattern", static_cast<int>(index),
                    recipe.areas, report);
    for (std::size_t index = 0u; index < recipe.scenarios.countScenarios.size(); ++index)
        CheckOneBody(recipe.scenarios.countScenarios[index].body, "Count", static_cast<int>(index),
                    recipe.areas, report);
    CheckOneBody(recipe.scenarios.defaultScenario, "Default", 0, recipe.areas, report);

    return report;
}

// ONE wording, shared by every call site -- do not restate the phrasing elsewhere.
std::string ScenarioAreaNameValidationReport::SummaryText() const {
    if (AllReferencesResolve()) return std::string();
    std::string text = std::to_string(staleReferences.size())
        + " scenario(s) reference an Area name not found in recipe.areas:";
    for (const std::string& entry : staleReferences)
        text += "\n  " + entry;
    text += "\nThe exported rectangle falls back to that scenario's own last-known Area values; the "
            "reference itself is kept as-authored (never silently cleared). Re-pick the Area in the "
            "Scenarios tab, or re-add an Area with that name, to resolve it.";
    return text;
}

} // namespace Io
} // namespace SanmapGen
