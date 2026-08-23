// AppSettings_IO_Test.cpp — acceptance test for the app's first persistent file outside `.sanmap`
// and the asset atlas cache (STEP19_AppSettings_IO). Drives the real Save/Load round trip against a
// scratch folder under the platform temp directory — never the real `%APPDATA%\SanGen\` — plus the
// Constitution §6 degrade-gracefully paths: a missing directory, a missing file, and corrupt JSON.
#include "AppSettings_IO.h"
#include "AppSettingsLocation_IO.h"
#include "FilesystemPrimitives_IO.h"
#include <cstdio>
#include <filesystem>
#include <fstream>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static std::string ScratchFolderPath() {
    std::error_code pathError;
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path(pathError) / "SanGenAppSettingsTest";
    std::filesystem::remove_all(folder, pathError);
    return folder.string();
}

static void TestRoundTripSurvivesExactly() {
    const std::string scratchFolder = ScratchFolderPath();
    Io::AppSettings written;
    written.sanpackPath         = "D:/Fixtures/roundtrip.sanpack";
    written.assetCacheDirectory = "D:/Fixtures/AtlasCache";
    written.environmentPackPath = "D:/Fixtures/environment.sanpack";
    written.gameInstallRoot               = "D:/Fixtures/GameInstall";
    written.scenarioRuntimeOverridePath   = "D:/Fixtures/CustomRuntime.lua";
    written.lastTemplateIngestTimestamp   = "2026-08-23T12:34:56Z";
    written.lastTemplateIngestEntryCount  = 217;
    written.bTemplateIngestEnabled        = false;
    written.bUseGpuTerrain = false;
    written.bUseGpuFlow    = true;
    written.bWysiwygBaking = true;
    written.bUseGpuMarkers = true;

    Check(Io::SaveAppSettings(scratchFolder, written), "a populated AppSettings saves cleanly");
    const Io::AppSettings read = Io::LoadAppSettings(scratchFolder);
    Check(read.sanpackPath == written.sanpackPath, "sanpackPath survives Save->Load exactly");
    Check(read.assetCacheDirectory == written.assetCacheDirectory,
          "assetCacheDirectory survives Save->Load exactly");
    Check(read.environmentPackPath == written.environmentPackPath,
          "environmentPackPath survives Save->Load exactly");
    Check(read.gameInstallRoot == written.gameInstallRoot,
          "gameInstallRoot survives Save->Load exactly");
    Check(read.scenarioRuntimeOverridePath == written.scenarioRuntimeOverridePath,
          "scenarioRuntimeOverridePath survives Save->Load exactly");
    Check(read.lastTemplateIngestTimestamp == written.lastTemplateIngestTimestamp,
          "lastTemplateIngestTimestamp survives Save->Load exactly");
    Check(read.lastTemplateIngestEntryCount == written.lastTemplateIngestEntryCount,
          "lastTemplateIngestEntryCount survives Save->Load exactly");
    Check(read.bTemplateIngestEnabled == written.bTemplateIngestEnabled,
          "bTemplateIngestEnabled survives Save->Load exactly");
    Check(read.bUseGpuTerrain == written.bUseGpuTerrain, "bUseGpuTerrain survives Save->Load exactly");
    Check(read.bUseGpuFlow == written.bUseGpuFlow, "bUseGpuFlow survives Save->Load exactly");
    Check(read.bWysiwygBaking == written.bWysiwygBaking, "bWysiwygBaking survives Save->Load exactly");
    Check(read.bUseGpuMarkers == written.bUseGpuMarkers, "bUseGpuMarkers survives Save->Load exactly");
}

static void TestMissingDirectoryDegradesToDefaults() {
    const Io::AppSettings defaults;
    const std::string missingFolder =
        Io::JoinExportPath(ScratchFolderPath(), "ThisFolderWasNeverCreated");
    const Io::AppSettings read = Io::LoadAppSettings(missingFolder);
    Check(read.sanpackPath.empty() && read.assetCacheDirectory.empty() &&
              read.environmentPackPath.empty() && read.gameInstallRoot.empty() &&
              read.scenarioRuntimeOverridePath.empty() && read.lastTemplateIngestTimestamp.empty(),
          "a missing directory degrades to default-constructed strings, never a crash");
    Check(read.lastTemplateIngestEntryCount == defaults.lastTemplateIngestEntryCount,
          "and lastTemplateIngestEntryCount keeps its compiled default (0)");
    Check(read.bUseGpuTerrain == defaults.bUseGpuTerrain && read.bUseGpuFlow == defaults.bUseGpuFlow &&
              read.bWysiwygBaking == defaults.bWysiwygBaking && read.bUseGpuMarkers == defaults.bUseGpuMarkers &&
              read.bTemplateIngestEnabled == defaults.bTemplateIngestEnabled,
          "and every bool keeps its compiled default");
}

static void TestMissingFileInAnExistingDirectoryDegradesToDefaults() {
    const std::string scratchFolder = ScratchFolderPath();
    std::error_code folderError;
    std::filesystem::create_directories(scratchFolder, folderError);
    const Io::AppSettings read = Io::LoadAppSettings(scratchFolder);
    Check(read.sanpackPath.empty(), "an existing directory with no settings file also degrades quietly");
}

static void TestCorruptJsonDegradesToDefaults() {
    const std::string scratchFolder = ScratchFolderPath();
    std::error_code folderError;
    std::filesystem::create_directories(scratchFolder, folderError);
    const std::string filePath = Io::JoinExportPath(scratchFolder, Io::kAppSettingsFileName);
    std::ofstream corruptFile(filePath, std::ios::binary | std::ios::trunc);
    corruptFile << "{ this is not valid json";
    corruptFile.close();

    const Io::AppSettings read = Io::LoadAppSettings(scratchFolder);
    const Io::AppSettings defaults;
    Check(read.sanpackPath == defaults.sanpackPath && read.bUseGpuTerrain == defaults.bUseGpuTerrain,
          "malformed JSON degrades to compiled defaults, never a crash");
    Check(read.gameInstallRoot == defaults.gameInstallRoot &&
              read.scenarioRuntimeOverridePath == defaults.scenarioRuntimeOverridePath,
          "including the two new fields");
    Check(read.lastTemplateIngestTimestamp == defaults.lastTemplateIngestTimestamp &&
              read.lastTemplateIngestEntryCount == defaults.lastTemplateIngestEntryCount &&
              read.bTemplateIngestEnabled == defaults.bTemplateIngestEnabled,
          "including the three template-ingest fields (STEP90)");
}

static void TestDefaultDirectoryIsPlausible() {
    const std::string directory = Io::DefaultAppSettingsDirectory();
#ifdef _WIN32
    Check(!directory.empty(), "DefaultAppSettingsDirectory resolves a non-empty path on Windows");
    Check(directory.find("SanGen") != std::string::npos,
          "and it names this application's own folder somewhere in the path");
#else
    (void)directory;
#endif
}

int main() {
    TestRoundTripSurvivesExactly();
    TestMissingDirectoryDegradesToDefaults();
    TestMissingFileInAnExistingDirectoryDegradesToDefaults();
    TestCorruptJsonDegradesToDefaults();
    TestDefaultDirectoryIsPlausible();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
