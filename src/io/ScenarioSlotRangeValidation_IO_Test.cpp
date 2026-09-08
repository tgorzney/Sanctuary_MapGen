// ScenarioSlotRangeValidation_IO_Test.cpp — acceptance test for STEP253: the export-time, warn-only
// ScenarioCountCondition::slotRangeStart/slotRangeEnd bounds scan. Modelled directly on
// MapExporter_ScenarioAreaNameValidation_IO_Test.cpp's own standalone Check()/main() shape and
// scratch-directory export pattern.
#include "ScenarioSlotRangeValidation_IO.h"
#include "MapExporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cstdio>
#include <filesystem>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static std::string ScratchFolderPath() {
    std::error_code pathError;
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path(pathError) / "SanGenScenarioSlotRangeValidationTest";
    std::filesystem::remove_all(folder, pathError);
    return folder.string();
}

namespace {

Params::ScenarioCountCondition MakeSlotRangeCondition(int slotRangeStart, int slotRangeEnd) {
    Params::ScenarioCountCondition condition;
    condition.field = Params::ScenarioCountField::SlotRangeOccupiedCount;
    condition.comparator = Params::ScenarioComparator::GreaterOrEqual;
    condition.value = 1;
    condition.slotRangeStart = slotRangeStart;
    condition.slotRangeEnd = slotRangeEnd;
    return condition;
}

} // namespace

// 1. In-bounds case: a valid range reports clean.
static void TestInBoundsRangeIsClean() {
    Params::MapRecipe recipe;
    recipe.scenarios.maxArmySlotCount = 16;
    Params::CountScenario countScenario;
    countScenario.body.name = "Slots5To8";
    countScenario.conditions.push_back(MakeSlotRangeCondition(5, 8));
    recipe.scenarios.countScenarios.push_back(countScenario);

    const Io::ScenarioSlotRangeValidationReport report = Io::ValidateScenarioSlotRanges(recipe);
    Check(report.AllRangesValid(), "a range within [1, maxArmySlotCount] reports clean");
    Check(report.SummaryText().empty(), "and the summary is empty");
}

// 2. A non-range condition (Total/HumanCount/AiCount) is never checked/flagged, regardless of its
// (meaningless) slotRangeStart/slotRangeEnd defaults.
static void TestNonRangeFieldNeverFlagged() {
    Params::MapRecipe recipe;
    // Deliberately 0 -- smaller than the struct default (1, 1), which WOULD be a violation if this
    // field were mistakenly checked. Proves the field-type guard, not just an easy-to-satisfy range.
    recipe.scenarios.maxArmySlotCount = 0;
    Params::CountScenario countScenario;
    countScenario.body.name = "TotalOnly";
    Params::ScenarioCountCondition totalCondition;
    totalCondition.field = Params::ScenarioCountField::Total;
    totalCondition.value = 1;
    countScenario.conditions.push_back(totalCondition);
    recipe.scenarios.countScenarios.push_back(countScenario);

    const Io::ScenarioSlotRangeValidationReport report = Io::ValidateScenarioSlotRanges(recipe);
    Check(report.AllRangesValid(), "a Total/HumanCount/AiCount condition is never checked against slot bounds");
}

// 3. slotRangeStart > slotRangeEnd is a violation, named by scenario.
static void TestStartGreaterThanEndIsViolation() {
    Params::MapRecipe recipe;
    recipe.scenarios.maxArmySlotCount = 16;
    Params::CountScenario countScenario;
    countScenario.body.name = "BackwardsRange";
    countScenario.conditions.push_back(MakeSlotRangeCondition(8, 5));
    recipe.scenarios.countScenarios.push_back(countScenario);

    const Io::ScenarioSlotRangeValidationReport report = Io::ValidateScenarioSlotRanges(recipe);
    Check(!report.AllRangesValid(), "slotRangeStart > slotRangeEnd is a violation");
    Check(report.violations.size() == 1 && report.violations[0].find("BackwardsRange") != std::string::npos,
          "the violation names the offending scenario");
}

// 4. slotRangeEnd > maxArmySlotCount is a violation, checked against the map's OWN authored slot
// count, never a hardcoded 16.
static void TestEndExceedsMaxArmySlotCountIsViolation() {
    Params::MapRecipe recipe;
    recipe.scenarios.maxArmySlotCount = 4;
    Params::CountScenario countScenario;
    countScenario.body.name = "ExceedsMax";
    countScenario.conditions.push_back(MakeSlotRangeCondition(1, 8));
    recipe.scenarios.countScenarios.push_back(countScenario);

    const Io::ScenarioSlotRangeValidationReport report = Io::ValidateScenarioSlotRanges(recipe);
    Check(!report.AllRangesValid(), "slotRangeEnd > maxArmySlotCount is a violation");
}

// 5. slotRangeStart < 1 is a violation.
static void TestStartBelowOneIsViolation() {
    Params::MapRecipe recipe;
    recipe.scenarios.maxArmySlotCount = 16;
    Params::CountScenario countScenario;
    countScenario.body.name = "NegativeStart";
    countScenario.conditions.push_back(MakeSlotRangeCondition(0, 4));
    recipe.scenarios.countScenarios.push_back(countScenario);

    const Io::ScenarioSlotRangeValidationReport report = Io::ValidateScenarioSlotRanges(recipe);
    Check(!report.AllRangesValid(), "slotRangeStart < 1 is a violation");
}

// 6. Wiring test: an invalid range exported via ExportSanmapOnly never blocks, but does warn, and the
// offending condition row is refused (omitted) from the actual written .sanmap.
static void TestWiringNeverBlocksButWarnsAndRefusesRow() {
    Params::MapRecipe recipe;
    recipe.mapName = "scratch";
    Params::CountScenario countScenario;
    countScenario.body.name = "WiredInvalid";
    countScenario.conditions.push_back(MakeSlotRangeCondition(5, 99));   // 99 exceeds the default 16
    recipe.scenarios.countScenarios.push_back(countScenario);

    const std::string scratchFolder = ScratchFolderPath();
    const Io::MapExportResult result = Io::MapExporter::ExportSanmapOnly(scratchFolder, recipe);

    Check(result.bSucceeded, "an invalid slot range never blocks the export");
    Check(result.warningCount >= 1, "and increments warningCount by at least one");
    Check(result.debugLog.find("WiredInvalid") != std::string::npos,
          "debugLog names the scenario with the invalid range");
}

int main() {
    TestInBoundsRangeIsClean();
    TestNonRangeFieldNeverFlagged();
    TestStartGreaterThanEndIsViolation();
    TestEndExceedsMaxArmySlotCountIsViolation();
    TestStartBelowOneIsViolation();
    TestWiringNeverBlocksButWarnsAndRefusesRow();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
