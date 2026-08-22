// MapImporter_Scenarios_IO.cpp — the top-level `.sanmap` `Scenarios` object -> `recipe.scenarios`.
// Layer: IO. The exact inverse of MapExporter_Scenarios_IO.cpp. The shared `<ScenarioRecord>`
// reader (`ReadScenarioBodyJson`) lives in MapImporter_ScenarioRecord_IO.cpp (ARCH §1.5 split);
// this file composes it for PatternScenarios/CountScenarios/DefaultScenario, plus the
// malformed-array warning and `MaxArmySlotCount`. Absent/non-object `Scenarios` ->
// `outRecipe.scenarios` stays default-constructed; never an error, never a fabricated entry
// (Constitution §6). Field shape per STEP69_ParamsScenariosRoundTrip_IO.md §1/§3/§5/§6/§7 (this
// ticket's own inline tables are the binding source of truth).
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include "MapImporter_ScenarioRecord_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// Index == the enum's own declaration order (ARCH_15_05_ParamsScenariosType.md §15.5).
constexpr const char* kCountFieldSpellings[3] = { "Total", "HumanCount", "AiCount" };
constexpr int         kCountFieldCount        = 3;
constexpr const char* kComparatorSpellings[6] =
    { "Equal", "NotEqual", "GreaterThan", "GreaterOrEqual", "LessThan", "LessOrEqual" };
constexpr int         kComparatorCount        = 6;

// A present-but-non-array `PatternScenarios`/`CountScenarios` is malformed, never silently
// coerced/iterated as if ordered (§7). The common "guessing an order" tail is shared by both
// messages; the middle TIER-2-specific ordering clause is CountScenarios-only.
void WarnIfArrayKeyMalformed(const nlohmann::json& parent, const char* key, bool bOrderingLoadBearing,
                             MapImportResult& result) {
    if (!parent.contains(key) || parent[key].is_array()) return;
    std::string message = std::string("Scenarios.") + key + " is present but not a JSON array";
    message += bOrderingLoadBearing
        ? "; TIER 2 match priority depends on array order, which an object has none of."
        : ".";
    message += " Treated as empty rather than guessing an order.";
    result.Warn(message);
}

void ReadPatternScenariosJson(const nlohmann::json& parent, std::vector<Params::PatternScenario>& outPatterns,
                              int mapSize, MapImportResult& result) {
    WarnIfArrayKeyMalformed(parent, "PatternScenarios", /*bOrderingLoadBearing=*/false, result);
    if (!parent.contains("PatternScenarios") || !parent["PatternScenarios"].is_array()) return;
    outPatterns.clear();
    for (const nlohmann::json& patternJson : parent["PatternScenarios"]) {
        Params::PatternScenario pattern;
        if (patternJson.is_object()) {
            ReadScenarioBodyJson(patternJson, pattern.body, mapSize);
            ReadJsonText(patternJson, "Pattern", pattern.slotPattern);
        }
        outPatterns.push_back(pattern);
    }
}

// ⚠️ LOAD-BEARING: `push_back` in document order — never routed through anything that could
// reorder (§15.6; the ticket's own CountScenarios order-preservation acceptance test).
void ReadCountScenariosJson(const nlohmann::json& parent, std::vector<Params::CountScenario>& outCountScenarios,
                            int mapSize, MapImportResult& result) {
    WarnIfArrayKeyMalformed(parent, "CountScenarios", /*bOrderingLoadBearing=*/true, result);
    if (!parent.contains("CountScenarios") || !parent["CountScenarios"].is_array()) return;
    outCountScenarios.clear();
    for (const nlohmann::json& countJson : parent["CountScenarios"]) {
        Params::CountScenario countScenario;
        if (countJson.is_object()) {
            ReadScenarioBodyJson(countJson, countScenario.body, mapSize);
            if (countJson.contains("Conditions") && countJson["Conditions"].is_array()) {
                for (const nlohmann::json& conditionJson : countJson["Conditions"]) {
                    if (!conditionJson.is_object()) continue;
                    Params::ScenarioCountCondition condition;
                    int fieldValue = static_cast<int>(condition.field);
                    if (ReadJsonEnumerationText(conditionJson, "Field", kCountFieldSpellings, kCountFieldCount, fieldValue))
                        condition.field = static_cast<Params::ScenarioCountField>(fieldValue);
                    int comparatorValue = static_cast<int>(condition.comparator);
                    if (ReadJsonEnumerationText(conditionJson, "Comparator", kComparatorSpellings, kComparatorCount, comparatorValue))
                        condition.comparator = static_cast<Params::ScenarioComparator>(comparatorValue);
                    ReadJsonInteger(conditionJson, "Value", condition.value);
                    countScenario.conditions.push_back(condition);
                }
            }
        }
        outCountScenarios.push_back(countScenario);
    }
}

} // namespace

void ReadScenariosJson(const nlohmann::json& document, Params::MapRecipe& outRecipe, MapImportResult& result) {
    if (!document.contains("Scenarios") || !document["Scenarios"].is_object()) return;
    const nlohmann::json& scenariosJson = document["Scenarios"];
    const int mapSize = outRecipe.geometry.mapSize;
    Params::Scenarios& scenarios = outRecipe.scenarios;

    // §15.10: absent -> stays at the struct default (16). Never clamped/validated against the
    // army roster here — that export-time warning is STEP70's job, not the importer's.
    ReadJsonInteger(scenariosJson, "MaxArmySlotCount", scenarios.maxArmySlotCount);

    ReadPatternScenariosJson(scenariosJson, scenarios.patternScenarios, mapSize, result);
    ReadCountScenariosJson(scenariosJson, scenarios.countScenarios, mapSize, result);

    if (scenariosJson.contains("DefaultScenario") && scenariosJson["DefaultScenario"].is_object())
        ReadScenarioBodyJson(scenariosJson["DefaultScenario"], scenarios.defaultScenario, mapSize);
}

} // namespace Io
} // namespace SanmapGen
