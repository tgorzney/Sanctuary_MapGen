// MapExporter_ArmySpawnMarkerValidation_IO_Test.cpp — acceptance test for STEP82: the export-time,
// warn-only army->Spawn-marker membership scan. Modelled on MapExporter_IO_Test.cpp's own
// standalone Check()/main() shape and MapExporter_BlueprintValidation_IO_Test.cpp's
// scratch-directory export pattern.
#include "MapExporter_ArmySpawnMarkerValidation_IO.h"
#include "MapExporter_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "../data/MapFields_DATA.h"
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
        std::filesystem::temp_directory_path(pathError) / "SanGenArmySpawnMarkerValidationTest";
    std::filesystem::remove_all(folder, pathError);
    return folder.string();
}

namespace {

Params::Army MakeArmy(const std::string& name) {
    Params::Army army;
    army.name = name;
    return army;
}

Params::MarkerTransform MakeSpawnTransform(const std::string& name, const std::string& alias = "") {
    Params::MarkerTransform markerTransform;
    markerTransform.name  = name;
    markerTransform.alias = alias;
    return markerTransform;
}

Params::MarkerInstanceGroup MakeSpawnGroup(const std::vector<Params::MarkerTransform>& transforms) {
    Params::MarkerInstanceGroup group;
    group.name       = Io::spawnMarkerGroupName;
    group.transforms = transforms;
    return group;
}

} // namespace

// 1. Clean map: every army has a matching Spawn transform.
static void TestCleanMapReportsNothing() {
    Params::MapRecipe recipe;
    recipe.armies.push_back(MakeArmy("ARMY_01"));
    recipe.armies.push_back(MakeArmy("ARMY_02"));
    recipe.markers.push_back(MakeSpawnGroup({MakeSpawnTransform("ARMY_01"), MakeSpawnTransform("ARMY_02")}));

    const Io::ArmySpawnMarkerValidationReport report = Io::ValidateArmiesHaveSpawnMarkers(recipe);
    Check(report.AllArmiesHaveSpawnMarkers(), "two armies each with a matching Spawn transform: clean");
    Check(report.SummaryText().empty(), "and the summary is empty");
    Check(report.bSpawnMarkerGroupPresent, "the Spawn group was found");
}

// 2. One orphan among two armies.
static void TestOneOrphanIsNamed() {
    Params::MapRecipe recipe;
    recipe.armies.push_back(MakeArmy("ARMY_01"));
    recipe.armies.push_back(MakeArmy("ARMY_02"));
    recipe.markers.push_back(MakeSpawnGroup({MakeSpawnTransform("ARMY_01")}));

    const Io::ArmySpawnMarkerValidationReport report = Io::ValidateArmiesHaveSpawnMarkers(recipe);
    Check(!report.AllArmiesHaveSpawnMarkers(), "ARMY_02 has no matching Spawn transform");
    Check(report.armyNamesWithoutSpawnMarker.size() == 1
          && report.armyNamesWithoutSpawnMarker[0] == "ARMY_02",
          "exactly ARMY_02 is listed");
    Check(report.SummaryText().find("ARMY_02") != std::string::npos,
          "the summary names the orphaned army");
}

// 3. No Spawn group at all: every army is orphaned, the subsumed per-group case.
static void TestNoSpawnGroupOrphansEveryArmy() {
    Params::MapRecipe recipe;
    recipe.armies.push_back(MakeArmy("ARMY_01"));
    recipe.armies.push_back(MakeArmy("ARMY_02"));
    recipe.armies.push_back(MakeArmy("ARMY_03"));

    const Io::ArmySpawnMarkerValidationReport report = Io::ValidateArmiesHaveSpawnMarkers(recipe);
    Check(report.armyNamesWithoutSpawnMarker.size() == 3, "all three armies are orphaned");
    Check(!report.bSpawnMarkerGroupPresent, "no Spawn group was found at all");
    Check(report.SummaryText().find("marker group at all") != std::string::npos,
          "the summary states the group is entirely absent");
}

// 4. Zero armies: a clean, empty report even with no Spawn group either.
static void TestZeroArmiesIsCleanNotAWarning() {
    Params::MapRecipe recipe;
    const Io::ArmySpawnMarkerValidationReport report = Io::ValidateArmiesHaveSpawnMarkers(recipe);
    Check(report.AllArmiesHaveSpawnMarkers(), "no armies means nothing can be orphaned");
    Check(report.SummaryText().empty(), "and the summary has nothing to say about an empty map");
}

// 5. alias must NOT satisfy the match — only `name` counts.
static void TestAliasNeverSatisfiesTheMatch() {
    Params::MapRecipe recipe;
    recipe.armies.push_back(MakeArmy("ARMY_01"));
    recipe.markers.push_back(MakeSpawnGroup({MakeSpawnTransform("SpawnPoint 0", "ARMY_01")}));

    const Io::ArmySpawnMarkerValidationReport report = Io::ValidateArmiesHaveSpawnMarkers(recipe);
    Check(!report.AllArmiesHaveSpawnMarkers(), "a transform whose alias equals the army name is NOT a match");
    Check(report.armyNamesWithoutSpawnMarker.size() == 1 && report.armyNamesWithoutSpawnMarker[0] == "ARMY_01",
          "ARMY_01 is still reported orphaned");
}

// 6. Case-sensitive, byte-for-byte — no case folding.
static void TestMatchIsCaseSensitive() {
    Params::MapRecipe recipe;
    recipe.armies.push_back(MakeArmy("ARMY_01"));
    recipe.markers.push_back(MakeSpawnGroup({MakeSpawnTransform("army_01")}));

    const Io::ArmySpawnMarkerValidationReport report = Io::ValidateArmiesHaveSpawnMarkers(recipe);
    Check(!report.AllArmiesHaveSpawnMarkers(), "a differently-cased name does not match");
}

// 7. Non-blocking, end to end, through both public export actions.
static void TestExportIsNonBlockingEndToEnd() {
    Params::MapRecipe recipe;
    recipe.mapName = "scratch";
    recipe.armies.push_back(MakeArmy("ARMY_01"));
    recipe.armies.push_back(MakeArmy("ARMY_02"));
    recipe.markers.push_back(MakeSpawnGroup({MakeSpawnTransform("ARMY_01")}));   // ARMY_02 orphaned

    const std::string scratchFolder = ScratchFolderPath();
    const Io::MapExportResult sanmapOnlyResult = Io::MapExporter::ExportSanmapOnly(scratchFolder, recipe);
    Check(sanmapOnlyResult.bSucceeded, "ExportSanmapOnly still succeeds with an orphaned army");
    std::error_code pathError;
    Check(std::filesystem::exists(std::filesystem::path(Io::JoinExportPath(scratchFolder, "scratch.sanmap")),
                                  pathError),
          "and the .sanmap is on disk");
    Check(sanmapOnlyResult.debugLog.find("WARNING: ") != std::string::npos,
          "the debugLog carries a WARNING: line");
    Check(sanmapOnlyResult.debugLog.find("ARMY_02") != std::string::npos,
          "naming the orphaned army");

    Data::MapFields fields;   // deliberately unsized: ExportAll still logs "no baked fields" and succeeds
    const Io::MapExportResult exportAllResult =
        Io::MapExporter::ExportAll(scratchFolder, recipe, fields);
    Check(exportAllResult.bSucceeded, "ExportAll also still succeeds with an orphaned army");
    Check(exportAllResult.debugLog.find("WARNING: ") != std::string::npos
          && exportAllResult.debugLog.find("ARMY_02") != std::string::npos,
          "and ExportAll's debugLog carries the same warning");
}

// 8. Non-mutating: the recipe passed to ExportSanmapOnly is untouched.
static void TestExportDoesNotMutateTheRecipe() {
    Params::MapRecipe recipe;
    recipe.mapName = "scratch";
    recipe.armies.push_back(MakeArmy("ARMY_01"));
    recipe.armies.push_back(MakeArmy("ARMY_02"));
    recipe.markers.push_back(MakeSpawnGroup({MakeSpawnTransform("ARMY_01")}));
    const std::size_t armyCountBefore   = recipe.armies.size();
    const std::size_t markerCountBefore = recipe.markers.size();

    const std::string scratchFolder = ScratchFolderPath();
    Io::MapExporter::ExportSanmapOnly(scratchFolder, recipe);

    Check(recipe.armies.size() == armyCountBefore, "recipe.armies is untouched by the export");
    Check(recipe.markers.size() == markerCountBefore, "recipe.markers is untouched by the export");
    bool bSpawnGroupWasAdded = false;
    for (const Params::MarkerInstanceGroup& group : recipe.markers)
        if (group.name == Io::spawnMarkerGroupName) bSpawnGroupWasAdded = true;
    Check(bSpawnGroupWasAdded, "the one Spawn group that was already there is still there (not duplicated)");
    Check(recipe.markers.size() == 1, "and no NEW Spawn group was added");
}

// 9. One aggregate warning, not one per orphan.
static void TestOneAggregateWarningNotOnePerArmy() {
    Params::MapRecipe recipe;
    recipe.mapName = "scratch";
    recipe.armies.push_back(MakeArmy("ARMY_01"));
    recipe.armies.push_back(MakeArmy("ARMY_02"));
    recipe.armies.push_back(MakeArmy("ARMY_03"));
    // No Spawn group at all: all three are orphaned.

    const std::string scratchFolder = ScratchFolderPath();
    const Io::MapExportResult result = Io::MapExporter::ExportSanmapOnly(scratchFolder, recipe);
    Check(result.warningCount == 1, "three orphaned armies still increment warningCount by exactly 1");
}

// 10. Duplicate army names are reported once.
static void TestDuplicateArmyNamesReportedOnce() {
    Params::MapRecipe recipe;
    recipe.armies.push_back(MakeArmy("ARMY_01"));
    recipe.armies.push_back(MakeArmy("ARMY_01"));

    const Io::ArmySpawnMarkerValidationReport report = Io::ValidateArmiesHaveSpawnMarkers(recipe);
    Check(report.armyNamesWithoutSpawnMarker.size() == 1,
          "two Army entries sharing a name appear exactly once in the report");
}

// 11. Union across duplicate Spawn groups.
static void TestUnionAcrossDuplicateSpawnGroups() {
    Params::MapRecipe recipe;
    recipe.armies.push_back(MakeArmy("ARMY_01"));
    recipe.markers.push_back(MakeSpawnGroup({MakeSpawnTransform("ARMY_00")}));   // unrelated
    recipe.markers.push_back(MakeSpawnGroup({MakeSpawnTransform("ARMY_01")}));   // second "Spawn" group

    const Io::ArmySpawnMarkerValidationReport report = Io::ValidateArmiesHaveSpawnMarkers(recipe);
    Check(report.AllArmiesHaveSpawnMarkers(),
          "the match in the SECOND duplicate Spawn group still counts (union, not first-only)");
}

// 12. MapExportResult::Warn parity with MapImportResult::Warn.
static void TestWarnMatchesImporterShape() {
    Io::MapExportResult result;
    result.Warn("x");
    Check(result.debugLog == "WARNING: x\n", "Warn appends the exact \"WARNING: \" + line + newline shape");
    Check(result.warningCount == 1, "and increments warningCount by exactly one");
    result.Warn("y");
    Check(result.warningCount == 2, "a second Warn call increments again");
}

int main() {
    TestCleanMapReportsNothing();
    TestOneOrphanIsNamed();
    TestNoSpawnGroupOrphansEveryArmy();
    TestZeroArmiesIsCleanNotAWarning();
    TestAliasNeverSatisfiesTheMatch();
    TestMatchIsCaseSensitive();
    TestExportIsNonBlockingEndToEnd();
    TestExportDoesNotMutateTheRecipe();
    TestOneAggregateWarningNotOnePerArmy();
    TestDuplicateArmyNamesReportedOnce();
    TestUnionAcrossDuplicateSpawnGroups();
    TestWarnMatchesImporterShape();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
