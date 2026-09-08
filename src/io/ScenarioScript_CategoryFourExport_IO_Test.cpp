// ScenarioScript_CategoryFourExport_IO_Test.cpp — acceptance test for STEP251's scaffold-once-if-
// missing category-4 generator-file writer. Scratch-folder pattern per
// ScenarioScript_Export_IO_Test.cpp:26-32.
#include "ScenarioScript_CategoryFourExport_IO.h"
#include "FilesystemPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../sys/LuaSyntaxCheck_SYS.h"
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
        std::filesystem::temp_directory_path(pathError) /
        (std::string("SanGenScenarioCategoryFourExportTest_") + name);
    std::filesystem::remove_all(folder, pathError);
    std::filesystem::create_directories(folder, pathError);
    return folder.string();
}

static void WriteTextFile(const std::string& filePath, const std::string& contents) {
    std::ofstream outputStream(filePath, std::ios::binary | std::ios::trunc);
    outputStream << contents;
}

static std::string ReadTextFile(const std::string& filePath) {
    std::string text;
    Io::ReadTextFileBytes(filePath, text);
    return text;
}

static bool FileExists(const std::string& filePath) {
    std::error_code pathError;
    return std::filesystem::exists(std::filesystem::path(filePath), pathError);
}

static Params::MapRecipe MakeRecipe(const std::string& mapName) {
    Params::MapRecipe recipe;
    recipe.mapName = mapName;
    return recipe;
}

// 9. Scaffold created when missing.
static void TestScaffoldCreatedWhenMissing() {
    const std::string directory = ScratchFolderPath("Missing");
    Params::MapRecipe recipe = MakeRecipe("TestMap");
    recipe.scenarios.defaultScenario.name = "MyScenario";
    recipe.scenarios.defaultScenario.spawnsUnits = true;

    const Io::ScenarioCategoryFourExportReport report =
        Io::ExportScenarioCategoryFourScaffolds(directory, recipe);

    const std::string expectedPath = Io::JoinExportPath(directory, "TestMap_Scenarios_MyScenario.lua");
    Check(FileExists(expectedPath), "the scaffold file now exists");
    const std::string text = ReadTextFile(expectedPath);
    const std::string banner(Io::kScenarioCategoryFourScaffoldBannerLine);
    Check(text.compare(0, banner.size(), banner) == 0, "the file opens with the scaffold banner");
    Check(text.find("function GenerateScenarioUnits(area)") != std::string::npos,
          "the scaffold defines GenerateScenarioUnits(area)");
    Check(report.scaffoldedFilePaths.size() == 1 && report.scaffoldedFilePaths[0] == expectedPath,
          "report.scaffoldedFilePaths contains exactly that path");
    Check(report.writeRefusals.empty(), "no refusals for a clean, valid scaffold");
}

// 10. Scaffold skipped when present, in any state (untouched prior scaffold, and arbitrary
// hand-edited content with no banner at all).
static void TestScaffoldSkippedWhenPresentInAnyState() {
    {
        const std::string directory = ScratchFolderPath("PresentScaffold");
        Params::MapRecipe recipe = MakeRecipe("TestMap");
        recipe.scenarios.defaultScenario.name = "MyScenario";
        recipe.scenarios.defaultScenario.spawnsUnits = true;
        const std::string filePath = Io::JoinExportPath(directory, "TestMap_Scenarios_MyScenario.lua");
        const std::string priorScaffoldText =
            std::string(Io::kScenarioCategoryFourScaffoldBannerLine) + "\nfunction GenerateScenarioUnits(area) return {} end\n";
        WriteTextFile(filePath, priorScaffoldText);

        const Io::ScenarioCategoryFourExportReport report =
            Io::ExportScenarioCategoryFourScaffolds(directory, recipe);

        Check(ReadTextFile(filePath) == priorScaffoldText,
              "an untouched prior scaffold's bytes are unchanged");
        Check(report.scaffoldedFilePaths.empty(), "nothing scaffolded this export");
    }
    {
        const std::string directory = ScratchFolderPath("PresentHandEdited");
        Params::MapRecipe recipe = MakeRecipe("TestMap");
        recipe.scenarios.defaultScenario.name = "MyScenario";
        recipe.scenarios.defaultScenario.spawnsUnits = true;
        const std::string filePath = Io::JoinExportPath(directory, "TestMap_Scenarios_MyScenario.lua");
        const std::string handEditedContent = "-- arbitrary hand-edited content, no banner at all\n";
        WriteTextFile(filePath, handEditedContent);

        const Io::ScenarioCategoryFourExportReport report =
            Io::ExportScenarioCategoryFourScaffolds(directory, recipe);

        Check(ReadTextFile(filePath) == handEditedContent,
              "arbitrary hand-edited content's bytes are unchanged");
        Check(report.scaffoldedFilePaths.empty(), "nothing scaffolded this export");
    }
}

// 11. spawnsUnits == false scenarios are never written, valid or invalid name alike.
static void TestSpawnsUnitsFalseScenariosAreNeverWritten() {
    const std::string directory = ScratchFolderPath("SpawnsUnitsFalse");
    Params::MapRecipe recipe = MakeRecipe("TestMap");

    Params::PatternScenario validButNotSpawning;
    validButNotSpawning.body.name = "ValidName";
    validButNotSpawning.body.spawnsUnits = false;
    recipe.scenarios.patternScenarios.push_back(validButNotSpawning);

    Params::CountScenario invalidAndNotSpawning;
    invalidAndNotSpawning.body.name = "Bad Name!";
    invalidAndNotSpawning.body.spawnsUnits = false;
    recipe.scenarios.countScenarios.push_back(invalidAndNotSpawning);

    const Io::ScenarioCategoryFourExportReport report =
        Io::ExportScenarioCategoryFourScaffolds(directory, recipe);

    Check(report.scaffoldedFilePaths.empty(), "no scaffold is written for any spawnsUnits==false scenario");
    Check(report.writeRefusals.empty(), "and no refusal is logged either -- they are never considered");
    Check(!FileExists(Io::JoinExportPath(directory, "TestMap_Scenarios_ValidName.lua")),
          "no file appears for the valid-but-non-spawning scenario");
}

// 12. Invalid name refuses only that scenario's write; the other scenario's write still succeeds.
static void TestInvalidNameRefusesOnlyThatScenariosWrite() {
    const std::string directory = ScratchFolderPath("InvalidNameRefusal");
    Params::MapRecipe recipe = MakeRecipe("TestMap");

    Params::PatternScenario validScenario;
    validScenario.body.name = "ValidOne";
    validScenario.body.spawnsUnits = true;
    recipe.scenarios.patternScenarios.push_back(validScenario);

    Params::CountScenario invalidScenario;
    invalidScenario.body.name = "My Scenario!";
    invalidScenario.body.spawnsUnits = true;
    recipe.scenarios.countScenarios.push_back(invalidScenario);

    const Io::ScenarioCategoryFourExportReport report =
        Io::ExportScenarioCategoryFourScaffolds(directory, recipe);

    Check(FileExists(Io::JoinExportPath(directory, "TestMap_Scenarios_ValidOne.lua")),
          "the valid scenario's file is written");
    Check(report.scaffoldedFilePaths.size() == 1, "exactly one scaffold written");
    Check(report.writeRefusals.size() == 1, "exactly one refusal logged");
    if (!report.writeRefusals.empty())
        Check(report.writeRefusals[0].find("My Scenario!") != std::string::npos,
              "the refusal names the invalid scenario");
}

// 13. Default scenario is exported symmetrically -- defense-in-depth.
static void TestDefaultScenarioExportedSymmetrically() {
    const std::string directory = ScratchFolderPath("DefaultSymmetric");
    Params::MapRecipe recipe = MakeRecipe("TestMap");
    recipe.scenarios.defaultScenario.name = "DefaultGen";
    recipe.scenarios.defaultScenario.spawnsUnits = true;

    const Io::ScenarioCategoryFourExportReport report =
        Io::ExportScenarioCategoryFourScaffolds(directory, recipe);

    const std::string expectedPath = Io::JoinExportPath(directory, "TestMap_Scenarios_DefaultGen.lua");
    Check(FileExists(expectedPath), "the default scenario's own scaffold is written");
    Check(report.scaffoldedFilePaths.size() == 1 && report.scaffoldedFilePaths[0] == expectedPath,
          "reported exactly like a pattern/count entry would be");
}

// 14. The scaffold's generated Lua text is syntactically valid.
static void TestScaffoldTextIsSyntacticallyValidLua() {
    const std::string directory = ScratchFolderPath("SyntaxCheck");
    Params::MapRecipe recipe = MakeRecipe("TestMap");
    recipe.scenarios.defaultScenario.name = "SyntaxScenario";
    recipe.scenarios.defaultScenario.spawnsUnits = true;

    Io::ExportScenarioCategoryFourScaffolds(directory, recipe);
    const std::string filePath = Io::JoinExportPath(directory, "TestMap_Scenarios_SyntaxScenario.lua");
    const std::string text = ReadTextFile(filePath);
    Check(Sys::CheckLuaSyntax(text).bSucceeded, "the written scaffold file is syntactically valid Lua");
}

int main() {
    TestScaffoldCreatedWhenMissing();
    TestScaffoldSkippedWhenPresentInAnyState();
    TestSpawnsUnitsFalseScenariosAreNeverWritten();
    TestInvalidNameRefusesOnlyThatScenariosWrite();
    TestDefaultScenarioExportedSymmetrically();
    TestScaffoldTextIsSyntacticallyValidLua();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
