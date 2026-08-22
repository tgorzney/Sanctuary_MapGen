// ScenarioScript_RuntimeResource_IO_Test.cpp — acceptance test for the bundled/override Map
// Scenario runtime Lua resolver (STEP72). Scratch-directory pattern per MapExporter_IO_Test.cpp:25-31.
// argv[1] is the staged shader directory (unused here); argv[2] is the CMake-staged
// SANGEN_V2_LUA_RESOURCE_DIRECTORY holding the real bundled SanGenScenarioRuntime.lua.
#include "ScenarioScript_RuntimeResource_IO.h"
#include "FilesystemPrimitives_IO.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static std::string ScratchFolderPath(const char* suffix) {
    std::error_code pathError;
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path(pathError) /
        (std::string("SanGenScenarioRuntimeResourceTest_") + suffix);
    std::filesystem::remove_all(folder, pathError);
    std::filesystem::create_directories(folder, pathError);
    return folder.string();
}

static void WriteTextFile(const std::string& filePath, const std::string& contents) {
    std::ofstream outputStream(filePath, std::ios::binary | std::ios::trunc);
    outputStream << contents;
}

// 1. Override path set and readable.
static void TestOverridePathReadableWinsCleanly() {
    const std::string scratchFolder = ScratchFolderPath("Override");
    const std::string overridePath = Io::JoinExportPath(scratchFolder, "MyOverride.lua");
    WriteTextFile(overridePath, "-- override content\nScenario = {}\n");

    const Io::ScenarioRuntimeResourceResult result =
        Io::LoadScenarioRuntimeText("D:/never/read/this", overridePath);

    Check(result.bSucceeded, "an override path that exists and is readable succeeds");
    Check(result.sourceDescription == "override", "the source is reported as override");
    Check(result.errorMessage.empty(), "a clean override resolution carries no diagnostic");
    Check(result.runtimeLuaText == "-- override content\nScenario = {}\n",
          "the returned text matches the seeded override content exactly");
}

// 2. Override empty, valid bundled directory.
static void TestEmptyOverrideFallsThroughToBundled() {
    const std::string scratchFolder = ScratchFolderPath("Bundled");
    const std::string bundledPath = Io::JoinExportPath(scratchFolder, "SanGenScenarioRuntime.lua");
    WriteTextFile(bundledPath, "-- bundled content\nScenario = {}\n");

    const Io::ScenarioRuntimeResourceResult result = Io::LoadScenarioRuntimeText(scratchFolder, "");

    Check(result.bSucceeded, "an empty override resolves against a valid bundled directory");
    Check(result.sourceDescription == "bundled", "the source is reported as bundled");
}

// 3. Loud degrade proof.
static void TestUnreadableOverrideDegradesLoudlyToBundled() {
    const std::string scratchFolder = ScratchFolderPath("Degrade");
    const std::string bundledPath = Io::JoinExportPath(scratchFolder, "SanGenScenarioRuntime.lua");
    WriteTextFile(bundledPath, "-- bundled content\nScenario = {}\n");
    const std::string nonexistentOverridePath = Io::JoinExportPath(scratchFolder, "DoesNotExist.lua");

    const Io::ScenarioRuntimeResourceResult result =
        Io::LoadScenarioRuntimeText(scratchFolder, nonexistentOverridePath);

    Check(result.bSucceeded, "a bad override degrades to bundled rather than hard-failing");
    Check(result.sourceDescription == "bundled", "the degrade actually used the bundled text");
    Check(!result.errorMessage.empty(), "the degrade is never silent -- errorMessage is populated");
    Check(result.errorMessage.find(nonexistentOverridePath) != std::string::npos,
          "the diagnostic names the override path that failed");
}

// 4. Neither readable.
static void TestNeitherReadableFails() {
    const std::string scratchFolder = ScratchFolderPath("NeitherReadable");
    // scratchFolder deliberately has no SanGenScenarioRuntime.lua in it.
    const std::string nonexistentOverridePath = Io::JoinExportPath(scratchFolder, "DoesNotExist.lua");

    const Io::ScenarioRuntimeResourceResult result =
        Io::LoadScenarioRuntimeText(scratchFolder, nonexistentOverridePath);

    Check(!result.bSucceeded, "neither an override nor a bundled default readable fails outright");
    const std::string expectedBundledPath = Io::JoinExportPath(scratchFolder, "SanGenScenarioRuntime.lua");
    Check(result.errorMessage.find(expectedBundledPath) != std::string::npos,
          "the failure names the bundled path that was attempted");
    Check(result.runtimeLuaText.empty(), "no partial text is returned on a hard failure");
}

// 5. Readable override short-circuits before the bundled directory is ever required to exist.
static void TestReadableOverrideShortCircuitsBeforeTouchingBundledDirectory() {
    const std::string scratchFolder = ScratchFolderPath("ShortCircuit");
    const std::string overridePath = Io::JoinExportPath(scratchFolder, "MyOverride.lua");
    WriteTextFile(overridePath, "-- override content\n");
    const std::string garbageBundledDirectory =
        Io::JoinExportPath(scratchFolder, "no_such_subdirectory_at_all/deeper_still");

    const Io::ScenarioRuntimeResourceResult result =
        Io::LoadScenarioRuntimeText(garbageBundledDirectory, overridePath);

    Check(result.bSucceeded, "a readable override succeeds even with a garbage bundled directory");
    Check(result.sourceDescription == "override", "and the bundled directory was never consulted");
}

// 6. Real bundled resource self-check, driven off the CMake-staged directory (argv[2]).
// Correction 2026-08-22: items (a)/(d) of the ticket's original acceptance test depend on
// STEP65 (Sys::CheckLuaSyntax) and STEP70 (Io::kScenarioGeneratedFileBannerLine), neither of
// which exists in this worktree yet. (a) is replaced with a hardcoded literal comparison; (d) is
// skipped outright, both per the ticket's correction.
static void TestRealBundledResourceSelfCheck(const std::string& luaResourceDirectory) {
    if (luaResourceDirectory.empty()) {
        std::printf("SKIP TestRealBundledResourceSelfCheck: no lua resource directory given (argv[2])\n");
        return;
    }
    const Io::ScenarioRuntimeResourceResult result = Io::LoadScenarioRuntimeText(luaResourceDirectory, "");
    Check(result.bSucceeded, "the real staged bundled resource resolves");
    Check(result.sourceDescription == "bundled", "and it is reported as bundled");

    const std::string& text = result.runtimeLuaText;

    // (a) TODO(after STEP65+STEP70 land): replace this literal with Io::kScenarioGeneratedFileBannerLine
    const std::string firstLine = text.substr(0, text.find('\n'));
    Check(firstLine == "-- GENERATED BY SANGEN -- DO NOT HAND-EDIT (regenerated on "
                        "every export)",
          "the bundled resource opens with the generated-file banner literal");

    Check(text.find("Scenario = {}") != std::string::npos,
          "Scenario stays a GLOBAL table declaration");
    // "local Scenario" alone is NOT the right substring: it false-positive-matches the unrelated
    // "local ScenarioData = Import(...)" declaration a few lines below (ScenarioData begins with
    // "Scenario"). The real regression this guards against is the DECLARATION itself becoming
    // `local Scenario = {}` (Part 1's own comment: "MUST stay a GLOBAL table ... never `local`").
    Check(text.find("local Scenario = {}") == std::string::npos,
          "and is never accidentally localized (Import() would silently yield nothing)");
    Check(text.find("function Scenario.ResolveAndApply") != std::string::npos,
          "ResolveAndApply is defined");
    Check(text.find("function Scenario.SpawnNavalFleets") != std::string::npos,
          "SpawnNavalFleets is defined");

    // (d) TODO(after STEP65 lands): add Sys::CheckLuaSyntax(runtimeLuaText).bSucceeded == true

    // 7b. Missing-alloy-roster guard is present and loud (text assertions on the bundled resource).
    Check(text.find("bAlloyRosterAvailable") != std::string::npos,
          "the fail-loud alloy-roster guard is present");
    Check(text.find("pairs(ARMY_ID_TO_NAME or {})") == std::string::npos,
          "the roster iteration was not regressed to a silent-fallback form");
    Check(text.find("KNOWN_ALLOY_MARKERS or {}") == std::string::npos,
          "neither was the marker lookup");
    // Part 1's own Warn() call wraps this sentence across two adjacent Lua string literals
    // ("...WILL BE ".."REMOVED: ...") to stay under the line-length convention, so the raw file
    // text never contains "NO ALLOY MARKERS WILL BE REMOVED" as one contiguous run (only the
    // Lua-evaluated, concatenated string would). Pin both literal fragments instead -- this still
    // catches the sentence being quietly softened or dropped, without depending on how it happens
    // to be line-wrapped in source.
    Check(text.find("NO ALLOY MARKERS WILL BE") != std::string::npos,
          "the warning text itself is pinned so it cannot be quietly softened (fragment 1)");
    Check(text.find("REMOVED: every alloy on the map will remain visible for every composition.") !=
              std::string::npos,
          "the warning text itself is pinned so it cannot be quietly softened (fragment 2)");
}

// 7. Empty directory and empty override never crashes.
static void TestEmptyDirectoryAndEmptyOverrideNeverCrashes() {
    const Io::ScenarioRuntimeResourceResult result = Io::LoadScenarioRuntimeText("", "");
    Check(!result.bSucceeded, "an empty directory and an empty override cannot succeed");
    Check(!result.errorMessage.empty(), "but it fails with an actionable message, never a crash");
}

// 8. ReadTextFileBytes coverage.
static void TestReadTextFileBytesCoverage() {
    const std::string scratchFolder = ScratchFolderPath("ReadTextFileBytes");
    const std::string missingPath = Io::JoinExportPath(scratchFolder, "Missing.txt");
    std::string outText = "unchanged";
    Check(!Io::ReadTextFileBytes(missingPath, outText), "a missing file reads as false");
    Check(outText == "unchanged", "and leaves outText untouched on failure");

    const std::string existingPath = Io::JoinExportPath(scratchFolder, "Existing.txt");
    const std::string expectedBytes = std::string("before\0after", 12);
    {
        std::ofstream outputStream(existingPath, std::ios::binary | std::ios::trunc);
        outputStream.write(expectedBytes.data(), static_cast<std::streamsize>(expectedBytes.size()));
    }
    std::string readBackText;
    Check(Io::ReadTextFileBytes(existingPath, readBackText), "an existing file reads as true");
    Check(readBackText == expectedBytes, "and the bytes match exactly, including an embedded \\0");
}

int main(int argumentCount, char** arguments) {
    const std::string luaResourceDirectory = (argumentCount > 2) ? arguments[2] : "";

    TestOverridePathReadableWinsCleanly();
    TestEmptyOverrideFallsThroughToBundled();
    TestUnreadableOverrideDegradesLoudlyToBundled();
    TestNeitherReadableFails();
    TestReadableOverrideShortCircuitsBeforeTouchingBundledDirectory();
    TestRealBundledResourceSelfCheck(luaResourceDirectory);
    TestEmptyDirectoryAndEmptyOverrideNeverCrashes();
    TestReadTextFileBytesCoverage();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
