// ScenarioScript_Export_IO_Test.cpp — acceptance test for STEP71's Map Scenario export
// orchestrator. Scratch-directory pattern per MapExporter_IO_Test.cpp:25-31.
//
// LoadScenarioRuntimeText (WO6/STEP72) is a pure disk resolver over its own two path arguments --
// its real, shipped implementation (never a hand-rolled production stub) is what production code
// calls here. Tests "stub" its behavior at the test level exactly as
// ScenarioScript_RuntimeResource_IO_Test.cpp already does: by writing real scratch files at the
// bundled/override locations the resolver reads, never by re-defining the function.
#include "ScenarioScript_Export_IO.h"
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
        std::filesystem::temp_directory_path(pathError) / (std::string("SanGenScenarioExportTest_") + name);
    std::filesystem::remove_all(folder, pathError);
    return folder.string();
}

static void WriteTextFile(const std::string& filePath, const std::string& contents) {
    std::error_code folderError;
    std::filesystem::create_directories(std::filesystem::path(filePath).parent_path(), folderError);
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

// Both required subpaths, per GameInstallLocation_IO (STEP64).
static void MakeValidGameInstallRoot(const std::string& root) {
    std::error_code folderError;
    std::filesystem::create_directories(Io::JoinExportPath(root, "engine/LJ/lua"), folderError);
    std::filesystem::create_directories(Io::JoinExportPath(root, "engine/Sanctuary_Data/Maps"), folderError);
}

static std::string MapScriptDirectoryFor(const std::string& root, const std::string& mapName) {
    return Io::JoinExportPath(Io::JoinExportPath(Io::JoinExportPath(root, "engine"), "LJ/lua/maps"), mapName);
}

// A bundled runtime resource directory carrying a syntactically-valid, banner-prefixed
// SanGenScenarioRuntime.lua -- the real WO6 resolver reads it directly (no stub function).
static std::string MakeValidBundledRuntimeDirectory(const char* name) {
    const std::string directory = ScratchFolderPath(name);
    const std::string bundledText = std::string(Io::kScenarioGeneratedFileBannerLine) + "\nScenario = {}\n";
    WriteTextFile(Io::JoinExportPath(directory, "SanGenScenarioRuntime.lua"), bundledText);
    return directory;
}

static Params::MapRecipe MakeRecipe(const std::string& mapName) {
    Params::MapRecipe recipe;
    recipe.mapName = mapName;
    recipe.geometry.mapSize = 4;
    return recipe;
}

// 1. Invalid gameInstallRoot (missing both required subfolders).
static void TestInvalidRootReturnsAllDefaultsAndTouchesNothing() {
    const std::string root = ScratchFolderPath("InvalidRoot");
    std::error_code folderError;
    std::filesystem::create_directories(root, folderError); // the root itself exists; subpaths do not

    const Params::MapRecipe recipe = MakeRecipe("TestMap1");
    const Io::ScenarioExportResult result = Io::ExportMapScenario(root, recipe, "irrelevant", "");

    Check(!result.bDataLuaWritten, "invalid root: bDataLuaWritten stays false");
    Check(!result.bRuntimeCopied, "invalid root: bRuntimeCopied stays false");
    Check(!result.bOrchestratorPresent, "invalid root: bOrchestratorPresent stays false");
    Check(!result.bDataLuaCollisionDetected, "invalid root: bDataLuaCollisionDetected stays false");
    Check(!result.bRuntimeCollisionDetected, "invalid root: bRuntimeCollisionDetected stays false");
    Check(!result.bDataLuaSyntaxCheckFailed, "invalid root: bDataLuaSyntaxCheckFailed stays false");
    Check(!result.bRuntimeSyntaxCheckFailed, "invalid root: bRuntimeSyntaxCheckFailed stays false");
    Check(result.writtenFilePaths.empty(), "invalid root: writtenFilePaths is empty");
    Check(result.debugLog.find("LJ/lua") != std::string::npos
              && result.debugLog.find("Sanctuary_Data/Maps") != std::string::npos,
          "invalid root: debugLog contains rootValidation.reason naming both missing subpaths");
    Check(!FileExists(Io::JoinExportPath(root, "engine")), "invalid root: zero files/folders created");
}

// 2. Clean export, fresh folder.
static void TestCleanExportFreshFolder() {
    const std::string root = ScratchFolderPath("Clean_Root");
    MakeValidGameInstallRoot(root);
    const std::string bundledDirectory = MakeValidBundledRuntimeDirectory("Clean_Bundled");
    const Params::MapRecipe recipe = MakeRecipe("TestMap2");

    const Io::ScenarioExportResult result = Io::ExportMapScenario(root, recipe, bundledDirectory, "");

    Check(result.bDataLuaWritten, "clean export: Data.lua written");
    Check(result.bRuntimeCopied, "clean export: Runtime.lua written");
    Check(!result.bOrchestratorPresent, "clean export: no _data.lua present");
    Check(result.debugLog.find("files written but inert") != std::string::npos,
          "clean export: log carries the inert phrasing");

    const std::string mapScriptDirectory = MapScriptDirectoryFor(root, recipe.mapName);
    const std::string dataLuaPath = Io::JoinExportPath(mapScriptDirectory, recipe.mapName + "_Scenarios_Data.lua");
    const std::string runtimeLuaPath =
        Io::JoinExportPath(mapScriptDirectory, recipe.mapName + "_Scenarios_Runtime.lua");
    Check(FileExists(dataLuaPath), "clean export: Data.lua exists on disk");
    Check(FileExists(runtimeLuaPath), "clean export: Runtime.lua exists on disk");
    const std::string banner(Io::kScenarioGeneratedFileBannerLine);
    Check(ReadTextFile(dataLuaPath).compare(0, banner.size(), banner) == 0,
          "clean export: Data.lua opens with the banner");
    Check(ReadTextFile(runtimeLuaPath).compare(0, banner.size(), banner) == 0,
          "clean export: Runtime.lua opens with the banner");
}

// 3. Re-export over its own prior output.
static void TestReExportOverOwnOutputOverwritesCleanly() {
    const std::string root = ScratchFolderPath("ReExport_Root");
    MakeValidGameInstallRoot(root);
    const std::string bundledDirectory = MakeValidBundledRuntimeDirectory("ReExport_Bundled");
    const Params::MapRecipe recipe = MakeRecipe("TestMap3");

    const Io::ScenarioExportResult first = Io::ExportMapScenario(root, recipe, bundledDirectory, "");
    Check(first.bDataLuaWritten && first.bRuntimeCopied, "re-export: first export writes both files");

    const Io::ScenarioExportResult second = Io::ExportMapScenario(root, recipe, bundledDirectory, "");
    Check(second.bDataLuaWritten, "re-export: Data.lua overwrites cleanly");
    Check(second.bRuntimeCopied, "re-export: Runtime.lua overwrites cleanly");
    Check(!second.bDataLuaCollisionDetected, "re-export: zero Data.lua collision flag");
    Check(!second.bRuntimeCollisionDetected, "re-export: zero Runtime.lua collision flag");
}

// 4. Synthetic banner collision -- Data.lua.
static void TestDataLuaBannerCollision() {
    const std::string root = ScratchFolderPath("DataCollision_Root");
    MakeValidGameInstallRoot(root);
    const std::string bundledDirectory = MakeValidBundledRuntimeDirectory("DataCollision_Bundled");
    const Params::MapRecipe recipe = MakeRecipe("TestMap4");

    const std::string mapScriptDirectory = MapScriptDirectoryFor(root, recipe.mapName);
    const std::string dataLuaPath = Io::JoinExportPath(mapScriptDirectory, recipe.mapName + "_Scenarios_Data.lua");
    const std::string preSeededContent = "-- unrecognized hand-authored content, no banner\n";
    WriteTextFile(dataLuaPath, preSeededContent);

    const Io::ScenarioExportResult result = Io::ExportMapScenario(root, recipe, bundledDirectory, "");

    Check(!result.bDataLuaWritten, "data collision: bDataLuaWritten stays false");
    Check(result.bDataLuaCollisionDetected, "data collision: bDataLuaCollisionDetected fires");
    const std::string pendingPath =
        Io::JoinExportPath(mapScriptDirectory, recipe.mapName + "_Scenarios_Data.sangen-pending.lua");
    Check(FileExists(pendingPath), "data collision: the pending sibling exists");
    Check(ReadTextFile(pendingPath) == Io::BuildScenarioDataLuaText(recipe),
          "data collision: the pending sibling carries the freshly-rendered text");
    Check(ReadTextFile(dataLuaPath) == preSeededContent,
          "data collision: the original is byte-identical to what was pre-seeded");
}

// 5. Synthetic banner collision -- Runtime.lua (symmetric with test 4).
static void TestRuntimeLuaBannerCollision() {
    const std::string root = ScratchFolderPath("RuntimeCollision_Root");
    MakeValidGameInstallRoot(root);
    const std::string bundledDirectory = MakeValidBundledRuntimeDirectory("RuntimeCollision_Bundled");
    const Params::MapRecipe recipe = MakeRecipe("TestMap5");

    const std::string mapScriptDirectory = MapScriptDirectoryFor(root, recipe.mapName);
    const std::string runtimeLuaPath =
        Io::JoinExportPath(mapScriptDirectory, recipe.mapName + "_Scenarios_Runtime.lua");
    const std::string preSeededContent = "-- unrecognized hand-authored runtime content, no banner\n";
    WriteTextFile(runtimeLuaPath, preSeededContent);

    const Io::ScenarioExportResult result = Io::ExportMapScenario(root, recipe, bundledDirectory, "");

    Check(result.bDataLuaWritten, "runtime collision: Data.lua still writes cleanly");
    Check(!result.bRuntimeCopied, "runtime collision: bRuntimeCopied stays false");
    Check(result.bRuntimeCollisionDetected, "runtime collision: bRuntimeCollisionDetected fires");
    const std::string pendingPath =
        Io::JoinExportPath(mapScriptDirectory, recipe.mapName + "_Scenarios_Runtime.sangen-pending.lua");
    Check(FileExists(pendingPath), "runtime collision: the pending sibling exists");
    Check(ReadTextFile(runtimeLuaPath) == preSeededContent,
          "runtime collision: the original is byte-identical to what was pre-seeded");
}

// 6. Legacy-map migration -- distinct from tests 4/5: filename disjointness, not a banner check.
static void TestLegacyMapFilenameDisjointness() {
    const std::string root = ScratchFolderPath("Legacy_Root");
    MakeValidGameInstallRoot(root);
    const std::string bundledDirectory = MakeValidBundledRuntimeDirectory("Legacy_Bundled");
    const Params::MapRecipe recipe = MakeRecipe("Pandemonium Isthmus");

    const std::string mapScriptDirectory = MapScriptDirectoryFor(root, recipe.mapName);
    const std::string legacyPath =
        Io::JoinExportPath(mapScriptDirectory, recipe.mapName + "_Scenarios_Script.lua");
    const std::string legacyContent = "-- legacy hand-authored orchestrator script content\n";
    WriteTextFile(legacyPath, legacyContent);
    WriteTextFile(Io::JoinExportPath(mapScriptDirectory, recipe.mapName + "_data.lua"),
                 "-- real hand-authored orchestrator\n");

    const Io::ScenarioExportResult result = Io::ExportMapScenario(root, recipe, bundledDirectory, "");

    Check(result.bDataLuaWritten, "legacy migration: Data.lua written cleanly");
    Check(result.bRuntimeCopied, "legacy migration: Runtime.lua written cleanly");
    Check(!result.bDataLuaCollisionDetected, "legacy migration: no Data.lua collision -- filename disjointness");
    Check(!result.bRuntimeCollisionDetected, "legacy migration: no Runtime.lua collision -- filename disjointness");
    Check(ReadTextFile(legacyPath) == legacyContent,
          "legacy migration: the legacy file's bytes are unchanged -- SanGen never opened it");
    Check(result.bOrchestratorPresent, "legacy migration: bOrchestratorPresent is true");
}

// 7. Syntax-check refusal on the runtime side never blocks the Data.lua write.
static void TestRuntimeSyntaxCheckRefusalDoesNotBlockDataLua() {
    const std::string root = ScratchFolderPath("SyntaxRefusal_Root");
    MakeValidGameInstallRoot(root);
    const std::string garbageBundledDirectory = ScratchFolderPath("SyntaxRefusal_GarbageBundled");
    const std::string overridePath = Io::JoinExportPath(ScratchFolderPath("SyntaxRefusal_Override"), "Bad.lua");
    // A readable override with deliberately malformed Lua (an unterminated function) -- the real
    // resolver succeeds (bSucceeded == true, sourceDescription == "override"), and this ticket's
    // pre-write syntax check is what must refuse the write.
    WriteTextFile(overridePath, std::string(Io::kScenarioGeneratedFileBannerLine) +
                                "\nfunction Scenario.Broken(\n");
    const Params::MapRecipe recipe = MakeRecipe("TestMap7");

    const Io::ScenarioExportResult result =
        Io::ExportMapScenario(root, recipe, garbageBundledDirectory, overridePath);

    Check(result.bRuntimeSyntaxCheckFailed, "syntax refusal: bRuntimeSyntaxCheckFailed fires");
    Check(!result.bRuntimeCopied, "syntax refusal: bRuntimeCopied stays false");
    const std::string mapScriptDirectory = MapScriptDirectoryFor(root, recipe.mapName);
    Check(!FileExists(Io::JoinExportPath(mapScriptDirectory, recipe.mapName + "_Scenarios_Runtime.lua")),
          "syntax refusal: no file written to the Runtime path");
    Check(result.bDataLuaWritten, "syntax refusal: Data.lua from the same export still succeeds");
}

// 8. LoadScenarioRuntimeText hard failure never blocks the Data.lua write.
static void TestRuntimeResolutionFailureDoesNotBlockDataLua() {
    const std::string root = ScratchFolderPath("RuntimeFailure_Root");
    MakeValidGameInstallRoot(root);
    // Neither bundled nor override readable: an empty override skips that leg entirely, and the
    // bundled directory below deliberately carries no SanGenScenarioRuntime.lua.
    const std::string unreadableBundledDirectory = ScratchFolderPath("RuntimeFailure_Bundled");
    const Params::MapRecipe recipe = MakeRecipe("TestMap8");

    const Io::ScenarioExportResult result =
        Io::ExportMapScenario(root, recipe, unreadableBundledDirectory, "");

    Check(!result.bRuntimeCopied, "runtime failure: bRuntimeCopied stays false");
    Check(!result.debugLog.empty(), "runtime failure: no crash, debugLog is populated");
    Check(result.debugLog.find("could not be read") != std::string::npos,
          "runtime failure: debugLog contains the resolver's errorMessage");
    Check(result.bDataLuaWritten, "runtime failure: Data.lua from the same call still succeeds");
}

// 9. Folder auto-creation.
static void TestFolderAutoCreation() {
    const std::string root = ScratchFolderPath("AutoCreate_Root");
    MakeValidGameInstallRoot(root);
    const std::string bundledDirectory = MakeValidBundledRuntimeDirectory("AutoCreate_Bundled");
    const Params::MapRecipe recipe = MakeRecipe("TestMap9");
    const std::string mapScriptDirectory = MapScriptDirectoryFor(root, recipe.mapName);
    Check(!FileExists(mapScriptDirectory), "auto-creation: the map script subfolder does not exist yet");

    const Io::ScenarioExportResult result = Io::ExportMapScenario(root, recipe, bundledDirectory, "");

    Check(result.bDataLuaWritten && result.bRuntimeCopied,
          "auto-creation: export still succeeds end-to-end");
    Check(FileExists(mapScriptDirectory), "auto-creation: the folder now exists");
}

int main() {
    TestInvalidRootReturnsAllDefaultsAndTouchesNothing();
    TestCleanExportFreshFolder();
    TestReExportOverOwnOutputOverwritesCleanly();
    TestDataLuaBannerCollision();
    TestRuntimeLuaBannerCollision();
    TestLegacyMapFilenameDisjointness();
    TestRuntimeSyntaxCheckRefusalDoesNotBlockDataLua();
    TestRuntimeResolutionFailureDoesNotBlockDataLua();
    TestFolderAutoCreation();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
