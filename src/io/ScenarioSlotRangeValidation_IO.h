// ScenarioSlotRangeValidation_IO.h -- `ScenarioSlotRangeValidationReport` + the export-time
// ScenarioCountCondition::slotRangeStart/slotRangeEnd bounds scan (STEP253). Layer: IO. Modelled
// directly on the sibling MapExporter_ScenarioAreaNameValidation_IO.h: a report struct with a
// one-wording SummaryText(), plus a pure Validate* free function, same tier as recipe.IsValid(),
// never called from inside BuildSanmapJsonText/BuildScenarioDataLuaText.
//
// SHARED by both export legs (the .sanmap JSON leg, MapExporter_IO.cpp, and the Lua-rendering leg,
// ScenarioScript_Export_IO.cpp) -- this validator carries no wire-format-specific content, so there
// is no reason to fork it; it is pure Params-level logic (same posture as
// ValidateScenarioAreaNameReferences).
//
// WARN-ONLY at this reporting tier: a violation is refused (skipped) only at the actual serialization
// point -- MapExporter_Scenarios_IO.cpp's BuildScenariosJson and ScenarioScript_DataLua_IO.cpp's
// BuildCountScenariosTable each call the shared ScenarioSlotRangeIsValid predicate below and silently
// omit (never clamp/reorder) the one offending condition row, per ARCH_15_05_ParamsScenariosType.md
// §15.5 AMENDED 2026-09-04. This report exists so the refusal is also loud/logged, named by scenario,
// at the same two export orchestrators ValidateScenarioAreaNameReferences already warns from.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

// 1 <= slotRangeStart <= slotRangeEnd <= maxArmySlotCount, validated against the map's own authored
// slot count (ARCH_15_10), never a hardcoded 16. Meaningless (unused) for every field other than
// SlotRangeOccupiedCount. A small, header-only shared predicate (unlike ResolveScenarioAreaRect's
// own deliberate per-leg duplication elsewhere in this file family) -- kept in ONE place specifically
// because both export legs AND this validator need the exact same boolean, and it carries no
// wire-format-specific content to fork.
inline bool ScenarioSlotRangeIsValid(int slotRangeStart, int slotRangeEnd, int maxArmySlotCount) {
    return slotRangeStart >= 1 && slotRangeStart <= slotRangeEnd && slotRangeEnd <= maxArmySlotCount;
}

struct ScenarioSlotRangeValidationReport {
    std::vector<std::string> violations;   // one entry per invalid SlotRangeOccupiedCount condition,
                                            // named by scenario, in countScenarios vector order
    bool AllRangesValid() const { return violations.empty(); }
    std::string SummaryText() const;   // ONE wording -- shared by every call site
};

// Pure/read-only, touches no disk, never called from inside BuildSanmapJsonText/
// BuildScenarioDataLuaText -- same tier as ValidateScenarioAreaNameReferences.
ScenarioSlotRangeValidationReport ValidateScenarioSlotRanges(const Params::MapRecipe& recipe);

} // namespace Io
} // namespace SanmapGen
