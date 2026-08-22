// GameInstallLocation_IO_Test.cpp — acceptance test for ValidateGameInstallRoot (STEP64). Drives
// real scratch-folder trees under the platform temp directory — never a real game install — through
// every combination of the two required subpaths, plus the empty-candidate and file-not-directory
// refusal paths (Constitution §6: total, never-throwing, always an actionable reason).
#include "GameInstallLocation_IO.h"
#include "FilesystemPrimitives_IO.h"
#include <cstdio>
#include <filesystem>
#include <fstream>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static std::string ScratchFolderPath(const char* name) {
    std::error_code pathError;
    const std::filesystem::path folder = std::filesystem::temp_directory_path(pathError) / name;
    std::filesystem::remove_all(folder, pathError);
    return folder.string();
}

static void TestBothSubpathsPresentValidates() {
    const std::string root = ScratchFolderPath("SanGenGameInstallRootTest_Both");
    std::error_code folderError;
    std::filesystem::create_directories(Io::JoinExportPath(root, "engine/LJ/lua"), folderError);
    std::filesystem::create_directories(Io::JoinExportPath(root, "engine/Sanctuary_Data/Maps"), folderError);

    const Io::GameInstallRootValidation validation = Io::ValidateGameInstallRoot(root);
    Check(validation.bValid, "both subpaths present validates bValid == true");
    Check(validation.reason.empty(), "and reason is empty on success");
}

static void TestMissingLuaScriptPathOnly() {
    const std::string root = ScratchFolderPath("SanGenGameInstallRootTest_MissingLua");
    std::error_code folderError;
    std::filesystem::create_directories(Io::JoinExportPath(root, "engine/Sanctuary_Data/Maps"), folderError);

    const Io::GameInstallRootValidation validation = Io::ValidateGameInstallRoot(root);
    Check(!validation.bValid, "missing engine/LJ/lua alone refuses");
    Check(validation.reason.find("LJ/lua") != std::string::npos,
          "and reason names that specific subpath");
}

static void TestMissingMapAssetPathOnly() {
    const std::string root = ScratchFolderPath("SanGenGameInstallRootTest_MissingMaps");
    std::error_code folderError;
    std::filesystem::create_directories(Io::JoinExportPath(root, "engine/LJ/lua"), folderError);

    const Io::GameInstallRootValidation validation = Io::ValidateGameInstallRoot(root);
    Check(!validation.bValid, "missing engine/Sanctuary_Data/Maps alone refuses");
    Check(validation.reason.find("Sanctuary_Data/Maps") != std::string::npos,
          "and reason names that specific subpath");
}

static void TestMissingBothSubpathsNamesBoth() {
    const std::string root = ScratchFolderPath("SanGenGameInstallRootTest_MissingBoth");
    std::error_code folderError;
    std::filesystem::create_directories(root, folderError);

    const Io::GameInstallRootValidation validation = Io::ValidateGameInstallRoot(root);
    Check(!validation.bValid, "missing both subpaths refuses");
    Check(validation.reason.find("LJ/lua") != std::string::npos
              && validation.reason.find("Sanctuary_Data/Maps") != std::string::npos,
          "and reason names BOTH missing subpaths, never silently just one");
}

static void TestEmptyCandidateRootRefusesWithoutTouchingDisk() {
    const Io::GameInstallRootValidation validation = Io::ValidateGameInstallRoot("");
    Check(!validation.bValid, "an empty candidate root refuses");
    Check(validation.reason == "no game install root was given.",
          "with the exact fixed reason, no filesystem call attempted");
}

static void TestCandidateRootThatIsAFileRefuses() {
    const std::string root = ScratchFolderPath("SanGenGameInstallRootTest_IsFile");
    std::error_code folderError;
    std::filesystem::create_directories(
        std::filesystem::path(root).parent_path(), folderError);
    std::ofstream fileNotFolder(root, std::ios::binary | std::ios::trunc);
    fileNotFolder << "not a directory";
    fileNotFolder.close();

    const Io::GameInstallRootValidation validation = Io::ValidateGameInstallRoot(root);
    Check(!validation.bValid, "a candidate root that is itself a file refuses, not crashes");
}

int main() {
    TestBothSubpathsPresentValidates();
    TestMissingLuaScriptPathOnly();
    TestMissingMapAssetPathOnly();
    TestMissingBothSubpathsNamesBoth();
    TestEmptyCandidateRootRefusesWithoutTouchingDisk();
    TestCandidateRootThatIsAFileRefuses();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
