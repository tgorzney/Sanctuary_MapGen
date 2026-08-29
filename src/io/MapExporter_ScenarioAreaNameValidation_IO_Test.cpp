// MapExporter_ScenarioAreaNameValidation_IO_Test.cpp — acceptance test for STEP209: the export-time,
// warn-only ScenarioBody::areaName -> recipe.areas membership scan. Modelled on
// MapExporter_ArmySpawnMarkerValidation_IO_Test.cpp's own standalone Check()/main() shape and
// scratch-directory export pattern.
#include "MapExporter_ScenarioAreaNameValidation_IO.h"
#include "MapExporter_IO.h"
#include "FilesystemPrimitives_IO.h"
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
        std::filesystem::temp_directory_path(pathError) / "SanGenScenarioAreaNameValidationTest";
    std::filesystem::remove_all(folder, pathError);
    return folder.string();
}

namespace {

Params::MapArea MakeArea(const std::string& name) {
    Params::MapArea area;
    area.name = name;
    return area;
}

} // namespace

// 8a. Hit case: a resolvable areaName produces a clean report.
static void TestHitCaseIsClean() {
    Params::MapRecipe recipe;
    recipe.areas.push_back(MakeArea("Foo"));
    recipe.scenarios.defaultScenario.areaName = "Foo";

    const Io::ScenarioAreaNameValidationReport report = Io::ValidateScenarioAreaNameReferences(recipe);
    Check(report.AllReferencesResolve(), "a resolvable areaName reports clean");
    Check(report.SummaryText().empty(), "and the summary is empty");
}

// 8b. Empty-areaName case: also clean.
static void TestEmptyAreaNameIsClean() {
    Params::MapRecipe recipe;
    recipe.scenarios.defaultScenario.areaName.clear();

    const Io::ScenarioAreaNameValidationReport report = Io::ValidateScenarioAreaNameReferences(recipe);
    Check(report.AllReferencesResolve(), "an empty areaName reports clean");
}

// 9. Stale case: exactly one entry; SummaryText names both the scenario and the missing areaName.
static void TestStaleCaseIsNamed() {
    Params::MapRecipe recipe;
    recipe.scenarios.defaultScenario.name = "MyDefault";
    recipe.scenarios.defaultScenario.areaName = "Ghost";

    const Io::ScenarioAreaNameValidationReport report = Io::ValidateScenarioAreaNameReferences(recipe);
    Check(!report.AllReferencesResolve(), "a stale areaName is reported");
    Check(report.staleReferences.size() == 1, "exactly one stale entry");
    Check(report.SummaryText().find("MyDefault") != std::string::npos,
          "the summary names the scenario");
    Check(report.SummaryText().find("Ghost") != std::string::npos,
          "the summary names the missing areaName");
}

// 10. Multiple stale scenarios across all three tiers produce three distinct entries, tier order.
static void TestMultipleStaleAcrossTiersTierOrder() {
    Params::MapRecipe recipe;

    Params::PatternScenario pattern;
    pattern.body.name = "PatternOne";
    pattern.body.areaName = "MissingPattern";
    recipe.scenarios.patternScenarios.push_back(pattern);

    Params::CountScenario countScenario;
    countScenario.body.name = "CountOne";
    countScenario.body.areaName = "MissingCount";
    recipe.scenarios.countScenarios.push_back(countScenario);

    recipe.scenarios.defaultScenario.name = "DefaultOne";
    recipe.scenarios.defaultScenario.areaName = "MissingDefault";

    const Io::ScenarioAreaNameValidationReport report = Io::ValidateScenarioAreaNameReferences(recipe);
    Check(report.staleReferences.size() == 3, "three distinct stale entries, one per tier");
    if (report.staleReferences.size() == 3) {
        Check(report.staleReferences[0].find("PatternOne") != std::string::npos
              && report.staleReferences[0].find("MissingPattern") != std::string::npos,
              "entry 0 is the pattern-tier scenario");
        Check(report.staleReferences[1].find("CountOne") != std::string::npos
              && report.staleReferences[1].find("MissingCount") != std::string::npos,
              "entry 1 is the count-tier scenario");
        Check(report.staleReferences[2].find("DefaultOne") != std::string::npos
              && report.staleReferences[2].find("MissingDefault") != std::string::npos,
              "entry 2 is the default-tier scenario");
    }
}

// 11. Wiring test: a stale areaName exported via ExportSanmapOnly never blocks, but does warn.
static void TestWiringNeverBlocksButWarns() {
    Params::MapRecipe recipe;
    recipe.mapName = "scratch";
    recipe.scenarios.defaultScenario.name = "WiredDefault";
    recipe.scenarios.defaultScenario.areaName = "WiredGhost";

    const std::string scratchFolder = ScratchFolderPath();
    const Io::MapExportResult result = Io::MapExporter::ExportSanmapOnly(scratchFolder, recipe);

    Check(result.bSucceeded, "a stale areaName never blocks the export");
    Check(result.warningCount >= 1, "and increments warningCount by at least one");
    Check(result.debugLog.find("WiredDefault") != std::string::npos,
          "debugLog names the stale scenario");
    Check(result.debugLog.find("WiredGhost") != std::string::npos,
          "debugLog names the missing areaName");
}

int main() {
    TestHitCaseIsClean();
    TestEmptyAreaNameIsClean();
    TestStaleCaseIsNamed();
    TestMultipleStaleAcrossTiersTierOrder();
    TestWiringNeverBlocksButWarns();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
