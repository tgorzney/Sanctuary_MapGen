// ScenarioScript_AreaImport_IO_Test.cpp -- acceptance test for STEP215's disk-touching entry point.
// Scratch-directory pattern per ScenarioScript_Export_IO_Test.cpp's own precedent.
#include "ScenarioScript_AreaImport_IO.h"
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
        std::filesystem::temp_directory_path(pathError) / (std::string("SanGenAreaImportTest_") + name);
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

static const char* kRealNamedKeyBlock =
    "local AREA_356 = { x = 846, y = 846, width = 356, height = 356 }\n"
    "local AREA_169 = { x = 668.4444444444445, y = 824, width = 711.1111111111111, height = 400 }\n";

// 1. Filename refusal -- "_Scenarios_Runtime.lua" suffix, regardless of content.
static void TestRefusesSanGenOwnedFilenameRuntime() {
    const std::string folder = ScratchFolderPath("RefuseRuntimeFilename");
    const std::string filePath = WriteScratchFile(folder, "SomeMap_Scenarios_Runtime.lua", kRealNamedKeyBlock);
    Params::MapRecipe recipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, recipe);
    Check(result.bRefusedGeneratedFile, "RefuseRuntimeFilename: refused");
    Check(recipe.areas.empty(), "RefuseRuntimeFilename: recipe.areas untouched");
}

// 2. Filename refusal -- "_Scenarios_Data.lua" suffix, regardless of content.
static void TestRefusesSanGenOwnedFilenameData() {
    const std::string folder = ScratchFolderPath("RefuseDataFilename");
    const std::string filePath = WriteScratchFile(folder, "SomeMap_Scenarios_Data.lua", kRealNamedKeyBlock);
    Params::MapRecipe recipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, recipe);
    Check(result.bRefusedGeneratedFile, "RefuseDataFilename: refused");
    Check(recipe.areas.empty(), "RefuseDataFilename: recipe.areas untouched");
}

// 3. THE REQUIRED ACCEPTANCE TEST (ARCH §15.11 item 1): export a real _Scenarios_Data.lua via the
//    REAL renderer (BuildScenarioDataLuaText -- never a hand-typed banner string), feed it back in
//    under its OWN real filename, and confirm refusal.
static void TestAcceptanceExportedDataLuaIsRefusedUnderRealFilename() {
    Params::MapRecipe exportRecipe;
    exportRecipe.mapName = "AcceptanceMap";
    exportRecipe.geometry.mapSize = 4;
    const std::string realExportedText = Io::BuildScenarioDataLuaText(exportRecipe);

    const std::string folder = ScratchFolderPath("AcceptanceRealFilename");
    const std::string filePath = WriteScratchFile(folder, "AcceptanceMap_Scenarios_Data.lua", realExportedText);

    Params::MapRecipe importRecipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, importRecipe);
    Check(result.bRefusedGeneratedFile, "Acceptance/RealFilename: refused");
    Check(importRecipe.areas.empty(), "Acceptance/RealFilename: recipe.areas untouched");
}

// 4. Isolates the BANNER-LINE guard specifically (independent of the filename guard): the same real
//    exported text, written under a filename that does NOT match either owned suffix. Refusal here
//    can only be the banner-line check firing -- proving the guard is a checked property of the
//    FILE'S CONTENT, not merely a filename convention.
static void TestAcceptanceExportedDataLuaIsRefusedByBannerAloneUnderADifferentFilename() {
    Params::MapRecipe exportRecipe;
    exportRecipe.mapName = "AcceptanceMap";
    exportRecipe.geometry.mapSize = 4;
    const std::string realExportedText = Io::BuildScenarioDataLuaText(exportRecipe);

    const std::string folder = ScratchFolderPath("AcceptanceBannerOnly");
    const std::string filePath = WriteScratchFile(folder, "renamed_copy_of_export.lua", realExportedText);

    Params::MapRecipe importRecipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, importRecipe);
    Check(result.bRefusedGeneratedFile, "Acceptance/BannerOnly: refused by banner-line alone");
    Check(importRecipe.areas.empty(), "Acceptance/BannerOnly: recipe.areas untouched");
}

// 5. Unreadable/missing file.
static void TestRefusesUnreadableFile() {
    const std::string folder = ScratchFolderPath("RefuseUnreadable");
    const std::string filePath = Io::JoinExportPath(folder, "does_not_exist.lua");
    Params::MapRecipe recipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, recipe);
    Check(result.bRefusedUnreadableFile, "RefuseUnreadable: refused");
    Check(recipe.areas.empty(), "RefuseUnreadable: recipe.areas untouched");
}

// 6. Oversized file (item 10) -- refused before any rectangle could possibly be extracted, and the
//    stat-then-refuse path never needed to load the oversized content into a std::string at all.
static void TestRefusesOversizedFile() {
    const std::string folder = ScratchFolderPath("RefuseOversized");
    std::string oversizedText;
    oversizedText.reserve(Io::kMaxScenarioAreaImportSourceBytes + 1024);
    while (oversizedText.size() <= Io::kMaxScenarioAreaImportSourceBytes) oversizedText += "-- padding line\n";
    const std::string filePath = WriteScratchFile(folder, "oversized.lua", oversizedText);
    Params::MapRecipe recipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, recipe);
    Check(result.bRefusedOversizedFile, "RefuseOversized: refused");
    Check(recipe.areas.empty(), "RefuseOversized: recipe.areas untouched");
}

// 7. Successful import adds new areas additively into an empty recipe.
static void TestSuccessfulImportAddsNewAreas() {
    const std::string folder = ScratchFolderPath("SuccessfulImport");
    const std::string filePath = WriteScratchFile(folder, "ForeignMap_Scenarios_Script.lua", kRealNamedKeyBlock);
    Params::MapRecipe recipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, recipe);
    Check(!result.bRefusedGeneratedFile && !result.bRefusedUnreadableFile && !result.bRefusedOversizedFile,
          "SuccessfulImport: no refusal");
    Check(recipe.areas.size() == 2, "SuccessfulImport: both areas landed in recipe.areas");
    Check(result.writtenNames.size() == 2, "SuccessfulImport: both names reported written");
    const Params::MapArea* area356 = FindAreaByName(recipe, "AREA_356");
    Check(area356 != nullptr && area356->width == 356.0f, "SuccessfulImport: AREA_356 field values correct");
}

// 8. Name collision against an EXISTING recipe.areas entry is skipped and reported -- never a
//    silent overwrite -- while a non-colliding area from the same file is still imported (item 9).
static void TestNameCollisionSkippedAndReportedAdditively() {
    const std::string folder = ScratchFolderPath("NameCollision");
    const std::string filePath = WriteScratchFile(folder, "ForeignMap_Scenarios_Script.lua", kRealNamedKeyBlock);

    Params::MapRecipe recipe;
    Params::MapArea preExistingArea;
    preExistingArea.name = "AREA_356";
    preExistingArea.originX = 1.0f; preExistingArea.originZ = 1.0f;
    preExistingArea.width = 1.0f; preExistingArea.length = 1.0f;
    recipe.areas.push_back(preExistingArea);

    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, recipe);
    Check(result.skippedCollisionNames.size() == 1 && result.skippedCollisionNames[0] == "AREA_356",
          "NameCollision: AREA_356 reported skipped");
    Check(result.writtenNames.size() == 1 && result.writtenNames[0] == "AREA_169",
          "NameCollision: AREA_169 still imported (additive, not all-or-nothing)");
    Check(recipe.areas.size() == 2, "NameCollision: recipe now has the pre-existing area plus the new one");
    const Params::MapArea* stillPreExisting = FindAreaByName(recipe, "AREA_356");
    Check(stillPreExisting != nullptr && stillPreExisting->width == 1.0f,
          "NameCollision: pre-existing AREA_356 was never overwritten");
}

// 9. In-file collisions and grammar near-misses are carried through the wrapper's result verbatim.
static void TestNearMissesAndInFileCollisionsCarriedThrough() {
    const std::string source =
        "local AREA_DUP = { x = 1, y = 1, width = 1, height = 1 }\n"
        "local AREA_DUP = { x = 2, y = 2, width = 2, height = 2 }\n"
        "local AREA_BAD = { x = 1, y = 1, width = 1 }\n";   // missing 'height' -- a near-miss
    const std::string folder = ScratchFolderPath("PassThrough");
    const std::string filePath = WriteScratchFile(folder, "ForeignMap_Scenarios_Script.lua", source);
    Params::MapRecipe recipe;
    const Io::ScenarioAreaImportResult result = Io::ImportAreaRectanglesFromScenarioScriptFile(filePath, recipe);
    Check(result.collisionIdentifiers.size() == 1 && result.collisionIdentifiers[0] == "AREA_DUP",
          "PassThrough: in-file collision carried through");
    Check(result.nearMisses.size() == 1 && result.nearMisses[0].identifier == "AREA_BAD",
          "PassThrough: near-miss carried through");
    Check(recipe.areas.size() == 1, "PassThrough: only the resolved AREA_DUP landed");
}

int main() {
    TestRefusesSanGenOwnedFilenameRuntime();
    TestRefusesSanGenOwnedFilenameData();
    TestAcceptanceExportedDataLuaIsRefusedUnderRealFilename();
    TestAcceptanceExportedDataLuaIsRefusedByBannerAloneUnderADifferentFilename();
    TestRefusesUnreadableFile();
    TestRefusesOversizedFile();
    TestSuccessfulImportAddsNewAreas();
    TestNameCollisionSkippedAndReportedAdditively();
    TestNearMissesAndInFileCollisionsCarriedThrough();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
