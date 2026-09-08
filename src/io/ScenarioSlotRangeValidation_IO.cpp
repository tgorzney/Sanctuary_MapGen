// ScenarioSlotRangeValidation_IO.cpp -- `ValidateScenarioSlotRanges` and
// `ScenarioSlotRangeValidationReport::SummaryText`. Layer: IO. STEP253.
//
// Design ruling (ARCH_15_05_ParamsScenariosType.md §15.5 AMENDED 2026-09-04): this function only
// REPORTS -- it never mutates `recipe`, touches no disk, and stays a sibling pre-flight step, the
// same tier as `recipe.IsValid()`/`ValidateScenarioAreaNameReferences`, never called from inside
// BuildSanmapJsonText/BuildScenarioDataLuaText.
#include "ScenarioSlotRangeValidation_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// Never-blank scenario descriptor, mirroring ScenarioAreaNameDescriptor's own "never blank" posture
// (MapExporter_ScenarioAreaNameValidation_IO.cpp) -- this file owns its own copy.
std::string ScenarioSlotRangeDescriptor(const Params::ScenarioBody& body, int index) {
    if (!body.name.empty()) return body.name;
    return "Count Scenario #" + std::to_string(index + 1);
}

void CheckOneCountScenario(const Params::CountScenario& scenario, int index, int maxArmySlotCount,
                           ScenarioSlotRangeValidationReport& report) {
    for (const Params::ScenarioCountCondition& condition : scenario.conditions) {
        if (condition.field != Params::ScenarioCountField::SlotRangeOccupiedCount) continue;
        if (ScenarioSlotRangeIsValid(condition.slotRangeStart, condition.slotRangeEnd, maxArmySlotCount)) continue;
        report.violations.push_back(
            ScenarioSlotRangeDescriptor(scenario.body, index) + " -> slot range ["
            + std::to_string(condition.slotRangeStart) + ", " + std::to_string(condition.slotRangeEnd)
            + "] invalid against maxArmySlotCount=" + std::to_string(maxArmySlotCount));
    }
}

} // namespace

// Only CountScenario carries ScenarioCountCondition (ARCH_15_05_ParamsScenariosType.md §15.5) --
// PatternScenario/defaultScenario have no conditions to check.
ScenarioSlotRangeValidationReport ValidateScenarioSlotRanges(const Params::MapRecipe& recipe) {
    ScenarioSlotRangeValidationReport report;
    for (std::size_t index = 0u; index < recipe.scenarios.countScenarios.size(); ++index)
        CheckOneCountScenario(recipe.scenarios.countScenarios[index], static_cast<int>(index),
                              recipe.scenarios.maxArmySlotCount, report);
    return report;
}

// ONE wording, shared by every call site -- do not restate the phrasing elsewhere.
std::string ScenarioSlotRangeValidationReport::SummaryText() const {
    if (AllRangesValid()) return std::string();
    std::string text = std::to_string(violations.size())
        + " scenario condition(s) have an invalid SlotRangeOccupiedCount range:";
    for (const std::string& entry : violations)
        text += "\n  " + entry;
    text += "\nEach violating condition is refused (omitted from the export) rather than silently "
            "clamped, reordered, or truncated; the rest of that scenario's fields/conditions are "
            "unaffected. Fix slotRangeStart/slotRangeEnd (or raise maxArmySlotCount) in the "
            "Scenarios tab and re-export.";
    return text;
}

} // namespace Io
} // namespace SanmapGen
