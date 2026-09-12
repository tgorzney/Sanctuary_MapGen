// ScenarioScript_FullDataImport_IO_Test.cpp -- acceptance test for STEP265's disk-touching
// orchestrator. Scratch-directory pattern per ScenarioScript_AreaImport_IO_Test.cpp's own precedent.
#include "ScenarioScript_FullDataImport_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "ScenarioScript_DataLua_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static std::string ScratchFolderPath(const char* name) {
    std::error_code pathError;
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path(pathError) / (std::string("SanGenFullDataImportTest_") + name);
    std::filesystem::remove_all(folder, pathError);
    std::filesystem::create_directories(folder, pathError);
    return folder.string();
}

static std::string WriteScratchFile(const std::string& folder, const char* fileName, const std::string& contents) {
    const std::string filePath = Io::JoinExportPath(folder, fileName);
    std::ofstream outputStream(filePath, std::ios::binary | std::ios::trunc);
    outputStream << contents;
    outputStream.close();
    return filePath;
}

static const Params::MapArea* FindAreaByName(const Params::MapRecipe& recipe, const std::string& name) {
    for (const Params::MapArea& area : recipe.areas) if (area.name == name) return &area;
    return nullptr;
}

// One of each of the four ratified shapes in a single synthetic file: an area rectangle
// (§15.11), a Template-A match-condition function (Shape 1), a PATTERN_SCENARIOS entry (Shape 2), and
// a Shape-3 unit-placement row.
static const char* kOneOfEachShapeSource =
    "local AREA_356 = { x = 846, y = 846, width = 356, height = 356 }\n"
    "\n"
    "local COUNT_SCENARIOS = {\n"
    "    { name = \"1v1\", match = function(t, h, a, pattern) return t == 2 end, area = AREA_356 },\n"
    "}\n"
    "\n"
    "local PATTERN_SCENARIOS = {\n"
    "    { name = \"4human-slots5-8\", pattern = \"----hhhh--------\" },\n"
    "}\n"
    "\n"
    "local UNIT_PLACEMENTS = {\n"
    "    { armyName = \"ARMY_01\", templateIdentifier = \"ucn3001\", x = 10, y = 0, z = 20 },\n"
    "}\n";

// 1. Filename refusal -- "_Scenarios_Data.lua" suffix, regardless of content; all four sub-results
//    stay empty, recipe untouched.
static void TestRefusesSanGenOwnedFilename() {
    const std::string folder = ScratchFolderPath("RefuseFilename");
    const std::string filePath = WriteScratchFile(folder, "SomeMap_Scenarios_Data.lua", kOneOfEachShapeSource);
    Params::MapRecipe recipe;
    const Io::ScenarioFullDataImportResult result =
        Io::ImportFullScenarioDataFromScenarioScriptFile(filePath, 100, recipe);
    Check(result.bRefusedAsSanGenOwnedFile, "RefuseFilename: refused");
    Check(result.areas.areas.empty() && result.matchConditions.conditionSets.empty()
              && result.slotPatterns.entries.empty() && result.unitPlacements.placements.empty(),
          "RefuseFilename: all four sub-results empty");
    Check(recipe.areas.empty() && recipe.scenarios.patternScenarios.empty()
              && recipe.scenarios.countScenarios.empty(),
          "RefuseFilename: recipe completely untouched");
}

// 2. Banner-line refusal alone (a real exported _Scenarios_Data.lua fed back in under a DIFFERENT
//    filename) -- proves the guard is a checked property of the file's content, matching
//    ScenarioScript_AreaImport_IO_Test.cpp's own isolating acceptance test.
static void TestRefusesRealExportedBannerUnderADifferentFilename() {
    Params::MapRecipe exportRecipe;
    exportRecipe.mapName = "AcceptanceMap";
    exportRecipe.geometry.mapSize = 4;
    const std::string realExportedText = Io::BuildScenarioDataLuaText(exportRecipe);

    const std::string folder = ScratchFolderPath("BannerOnly");
    const std::string filePath = WriteScratchFile(folder, "renamed_copy_of_export.lua", realExportedText);

    Params::MapRecipe importRecipe;
    const Io::ScenarioFullDataImportResult result =
        Io::ImportFullScenarioDataFromScenarioScriptFile(filePath, 4, importRecipe);
    Check(result.bRefusedAsSanGenOwnedFile, "BannerOnly: refused by banner-line alone");
    Check(importRecipe.areas.empty(), "BannerOnly: recipe.areas untouched");
}

// 3. Unreadable/missing file.
static void TestRefusesUnreadableFile() {
    const std::string folder = ScratchFolderPath("RefuseUnreadable");
    const std::string filePath = Io::JoinExportPath(folder, "does_not_exist.lua");
    Params::MapRecipe recipe;
    const Io::ScenarioFullDataImportResult result =
        Io::ImportFullScenarioDataFromScenarioScriptFile(filePath, 100, recipe);
    Check(result.bRefusedUnreadableFile, "RefuseUnreadable: refused");
}

// 4. Byte cap: a source file exceeding kMaxScenarioFullDataImportSourceBytes (4 MiB, same cap as the
//    area importer) is refused before any content is scanned -- confirms and reuses the existing
//    area-importer's own established behavior rather than reinventing it.
static void TestRefusesOversizedFile() {
    const std::string folder = ScratchFolderPath("RefuseOversized");
    std::string oversizedText;
    oversizedText.reserve(Io::kMaxScenarioFullDataImportSourceBytes + 1024);
    while (oversizedText.size() <= Io::kMaxScenarioFullDataImportSourceBytes) oversizedText += "-- padding line\n";
    const std::string filePath = WriteScratchFile(folder, "oversized.lua", oversizedText);
    Params::MapRecipe recipe;
    const Io::ScenarioFullDataImportResult result =
        Io::ImportFullScenarioDataFromScenarioScriptFile(filePath, 100, recipe);
    Check(result.bRefusedOversizedFile, "RefuseOversized: refused");
    Check(recipe.areas.empty(), "RefuseOversized: recipe.areas untouched");
}

// 5. THE ACCEPTANCE TEST (work-order test 4): one of each shape in one pass populates all four result
//    fields correctly; ONLY recipe.areas is mutated -- the other three candidate lists are returned
//    but leave recipe.scenarios completely untouched.
static void TestOrchestratorComposesAllFourShapesInOnePass() {
    const std::string folder = ScratchFolderPath("OneOfEachShape");
    const std::string filePath = WriteScratchFile(folder, "ForeignMap_Scenarios_Script.lua", kOneOfEachShapeSource);

    Params::MapRecipe recipe;
    const Io::ScenarioFullDataImportResult result =
        Io::ImportFullScenarioDataFromScenarioScriptFile(filePath, 100, recipe);

    Check(!result.bRefusedAsSanGenOwnedFile && !result.bRefusedUnreadableFile && !result.bRefusedOversizedFile,
          "OneOfEach: no refusal");

    Check(result.areas.areas.size() == 1 && result.areas.areas[0].name == "AREA_356",
          "OneOfEach: area rectangle extracted");
    Check(result.matchConditions.conditionSets.size() == 1
              && result.matchConditions.conditionSets[0].conditions.size() == 1,
          "OneOfEach: Template-A match condition extracted");
    Check(result.slotPatterns.entries.size() == 1
              && result.slotPatterns.entries[0].slotPattern == "----hhhh--------",
          "OneOfEach: slot pattern extracted");
    Check(result.unitPlacements.placements.size() == 1
              && result.unitPlacements.placements[0].armyName == "ARMY_01"
              && result.unitPlacements.placements[0].positionZ == 79.0f,   // 100 - 20 - 1
          "OneOfEach: unit placement extracted with the mapSize-flipped z");

    // Only recipe.areas is mutated.
    Check(recipe.areas.size() == 1, "OneOfEach: recipe.areas received exactly the one extracted area");
    const Params::MapArea* area = FindAreaByName(recipe, "AREA_356");
    Check(area != nullptr && area->width == 356.0f, "OneOfEach: AREA_356 field values correct in recipe.areas");
    Check(result.writtenAreaNames.size() == 1 && result.writtenAreaNames[0] == "AREA_356",
          "OneOfEach: area reported written");

    // recipe.scenarios is never touched -- match-conditions/slot-patterns/unit-placements are surfaced
    // as raw candidates only, never auto-attached to any scenario record.
    Check(recipe.scenarios.patternScenarios.empty(), "OneOfEach: recipe.scenarios.patternScenarios untouched");
    Check(recipe.scenarios.countScenarios.empty(), "OneOfEach: recipe.scenarios.countScenarios untouched");
    Check(recipe.scenarios.defaultScenario.unitPlacements.empty(),
          "OneOfEach: recipe.scenarios.defaultScenario.unitPlacements untouched");
}

// 6. Name collision against an EXISTING recipe.areas entry is skipped and reported, exactly
//    ScenarioScript_AreaImport_IO's own item-9 policy -- the other three candidate lists are still
//    populated regardless.
static void TestAreaNameCollisionSkippedButOtherCandidatesStillSurfaced() {
    const std::string folder = ScratchFolderPath("AreaCollision");
    const std::string filePath = WriteScratchFile(folder, "ForeignMap_Scenarios_Script.lua", kOneOfEachShapeSource);

    Params::MapRecipe recipe;
    Params::MapArea preExistingArea;
    preExistingArea.name = "AREA_356";
    preExistingArea.originX = 1.0f; preExistingArea.originZ = 1.0f;
    preExistingArea.width = 1.0f; preExistingArea.length = 1.0f;
    recipe.areas.push_back(preExistingArea);

    const Io::ScenarioFullDataImportResult result =
        Io::ImportFullScenarioDataFromScenarioScriptFile(filePath, 100, recipe);
    Check(result.skippedCollisionAreaNames.size() == 1 && result.skippedCollisionAreaNames[0] == "AREA_356",
          "AreaCollision: AREA_356 reported skipped");
    Check(recipe.areas.size() == 1, "AreaCollision: pre-existing area never duplicated/overwritten");
    const Params::MapArea* stillPreExisting = FindAreaByName(recipe, "AREA_356");
    Check(stillPreExisting != nullptr && stillPreExisting->width == 1.0f,
          "AreaCollision: pre-existing AREA_356 was never overwritten");
    Check(result.unitPlacements.placements.size() == 1 && result.slotPatterns.entries.size() == 1
              && result.matchConditions.conditionSets.size() == 1,
          "AreaCollision: the other three candidate lists are still fully populated regardless of the area collision");
}

int main() {
    TestRefusesSanGenOwnedFilename();
    TestRefusesRealExportedBannerUnderADifferentFilename();
    TestRefusesUnreadableFile();
    TestRefusesOversizedFile();
    TestOrchestratorComposesAllFourShapesInOnePass();
    TestAreaNameCollisionSkippedButOtherCandidatesStillSurfaced();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
