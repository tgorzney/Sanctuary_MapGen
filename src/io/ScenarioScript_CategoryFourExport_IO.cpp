// ScenarioScript_CategoryFourExport_IO.cpp -- see the header for the file-level contract. STEP251.
#include "ScenarioScript_CategoryFourExport_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "ScenarioNameValidation_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../sys/LuaSyntaxCheck_SYS.h"
#include <filesystem>

namespace SanmapGen {
namespace Io {
namespace {

std::string BuildScaffoldText(const std::string& mapName, const std::string& scenarioName) {
    return std::string(kScenarioCategoryFourScaffoldBannerLine) + "\n"
        "--\n"
        "-- " + mapName + "_Scenarios_" + scenarioName + ".lua -- hand-authored unit-spawn generator for\n"
        "-- the \"" + scenarioName + "\" scenario (`ARCH_15_04_ThreeFileOnDiskShape.md` category 4,\n"
        "-- `MAP_SCENARIO_SPEC.md` \xC2\xA7 11.2/\xC2\xA7 14). SanGen wrote this file ONCE, because\n"
        "-- " + mapName + "_Scenarios_Data.lua's \"" + scenarioName + "\" scenario record has\n"
        "-- spawnsUnits = true and no generator file existed yet at export time. SanGen will NEVER\n"
        "-- overwrite this file again, in any state -- edit freely, it is entirely yours from here.\n"
        "--\n"
        "-- CONTRACT: must expose a GLOBAL function GenerateScenarioUnits(area) returning a flat array\n"
        "-- of rows shaped { armyIndex = <int>, templateIdentifier = \"<string>\", x = <num>, y = <num>,\n"
        "-- z = <num> }. Scenario.SpawnMatchedScenarioUnits (in the sibling\n"
        "-- " + mapName + "_Scenarios_Runtime.lua) Import()s this file LAZILY -- only when \""
        + scenarioName + "\"\n"
        "-- is the scenario that actually matched the current lobby -- and passes whatever it returns\n"
        "-- straight to Scenario.SpawnUnits, which calls CreateUnit for you. Returning an empty table is\n"
        "-- legal: it spawns nothing, exactly like this stub does until you fill it in.\n"
        "--\n"
        "-- See MAP_SCENARIO_SPEC.md \xC2\xA7 12 for a complete worked example, and MAP_UNIT_SPAWNING_SPEC.md\n"
        "-- for the CreateUnit contract: army indices come from pairs(Armies), never hardcoded; guard\n"
        "-- army.lobbyOptions before reading isEmptySlot; never call CreateUnit directly -- only through\n"
        "-- Scenario.SpawnUnits.\n"
        "\n"
        "function GenerateScenarioUnits(area)\n"
        "    return {}\n"
        "end\n";
}

std::string CategoryFourFilePath(const std::string& mapScriptDirectory, const std::string& mapName,
                                 const std::string& scenarioName) {
    return JoinExportPath(mapScriptDirectory, mapName + "_Scenarios_" + scenarioName + ".lua");
}

void ExportOneScenarioBody(const Params::ScenarioBody& body, const std::string& mapScriptDirectory,
                           const std::string& mapName, const ScenarioNameValidationReport& nameReport,
                           ScenarioCategoryFourExportReport& outReport) {
    if (!body.spawnsUnits) return;

    const std::string* invalidReason = nameReport.FindViolationReasonForName(body.name);
    if (invalidReason != nullptr) {
        outReport.writeRefusals.push_back(
            "category-4 generator file for scenario '" + body.name + "' was NOT written: its own name "
            "failed validation (" + *invalidReason + ") -- fix the name, then re-export.");
        return;
    }

    const std::string filePath = CategoryFourFilePath(mapScriptDirectory, mapName, body.name);
    std::error_code existenceError;
    if (std::filesystem::exists(std::filesystem::path(filePath), existenceError)) return; // hands-off

    const std::string scaffoldText = BuildScaffoldText(mapName, body.name);
    const Sys::LuaSyntaxCheckResult syntax = Sys::CheckLuaSyntax(scaffoldText);
    if (!syntax.bSucceeded) {
        outReport.writeRefusals.push_back(
            "category-4 scaffold for scenario '" + body.name + "' failed its own syntax pre-check -- "
            "not written. This should be structurally unreachable; report it if seen.");
        return;
    }

    WriteBinaryFileBytes(filePath, scaffoldText.data(), scaffoldText.size());
    outReport.scaffoldedFilePaths.push_back(filePath);
}

} // namespace

ScenarioCategoryFourExportReport ExportScenarioCategoryFourScaffolds(
        const std::string& mapScriptDirectory, const Params::MapRecipe& recipe) {
    ScenarioCategoryFourExportReport report;
    const ScenarioNameValidationReport nameReport = ValidateScenarioNames(recipe.scenarios);

    for (const Params::PatternScenario& entry : recipe.scenarios.patternScenarios)
        ExportOneScenarioBody(entry.body, mapScriptDirectory, recipe.mapName, nameReport, report);
    for (const Params::CountScenario& entry : recipe.scenarios.countScenarios)
        ExportOneScenarioBody(entry.body, mapScriptDirectory, recipe.mapName, nameReport, report);
    // Defense-in-depth: the UI never exposes a spawnsUnits checkbox for Tier 3 (ScenariosTab_Lists_UI.cpp's
    // own "Tier 3 is never spawns-flagged" comment) but the PARAMS field still exists and a hand-edited
    // or imported .sanmap could set it -- treat the default scenario symmetrically, never assume the
    // UI-level convention holds at the PARAMS/IO layer.
    ExportOneScenarioBody(recipe.scenarios.defaultScenario, mapScriptDirectory, recipe.mapName, nameReport, report);

    return report;
}

} // namespace Io
} // namespace SanmapGen
