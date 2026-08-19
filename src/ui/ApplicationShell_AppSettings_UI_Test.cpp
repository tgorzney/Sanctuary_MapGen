// ApplicationShell_AppSettings_UI_Test.cpp — tab-rebuild WO E acceptance, part 4
// (STEP19_AppSettings_IO): a pre-existing settings file seeds `Ui::Application` at construction,
// and the exact call `Run()`'s clean-exit path makes (SaveAppSettingsAtShutdown,
// Application_Frame_UI.cpp) writes the current values back out. Driven through a REAL
// Ui::Application against a scratch settings directory, never the real `%APPDATA%\SanGen\`.
// Headless: nothing below opens a window or runs a stage.
#include "ApplicationShell_TestSupport_UI.h"
#include "../io/AppSettings_IO.h"
#include <cstdio>
#include <filesystem>

namespace SanmapGen {
namespace Ui {
namespace {

std::string AppSettingsScratchDirectory() {
    std::error_code pathError;
    const std::filesystem::path folder =
        std::filesystem::temp_directory_path(pathError) / "SanGenAppSettingsShellTest";
    std::filesystem::remove_all(folder, pathError);
    return folder.string();
}

ApplicationSettings MakeShellSettings(const std::string& appSettingsDirectory) {
    ApplicationSettings settings;
    settings.previewResolution = shellTestPreviewResolution;
    settings.appSettingsDirectoryOverride = appSettingsDirectory;
    return settings;
}

void SetTruncated(char* destination, std::size_t destinationSize, const char* text) {
    std::snprintf(destination, destinationSize, "%s", text);
}

} // namespace

void RunShellAppSettingsChecks() {
    const std::string scratchDirectory = AppSettingsScratchDirectory();

    // Every field pinned away from its compiled default, so a seed that silently fell back to
    // defaults would still fail every check below.
    Io::AppSettings seeded;
    seeded.sanpackPath         = "C:/Fixtures/preseeded.sanpack";
    seeded.assetCacheDirectory = "C:/Fixtures/AtlasCache";
    seeded.environmentPackPath = "C:/Fixtures/environment.sanpack";
    seeded.bUseGpuTerrain = false;
    seeded.bUseGpuFlow    = false;
    seeded.bWysiwygBaking = true;
    seeded.bUseGpuMarkers = true;
    Check(Io::SaveAppSettings(scratchDirectory, seeded), "the fixture file itself writes cleanly");

    Application application(MakeShellSettings(scratchDirectory));
    Check(std::string(application.SanpackPath()) == seeded.sanpackPath,
          "construction seeds sanpackPath from the pre-existing file");
    Check(std::string(application.TabState().system.assetCacheDirectory) == seeded.assetCacheDirectory,
          "and the SystemTab's asset cache directory");
    Check(application.TabState().stratums.environmentPackPath == seeded.environmentPackPath,
          "and the StratumsTab's environment pack path");
    Check(application.ExecutionSettings().bUseGpuTerrain == false
              && application.ExecutionSettings().bUseGpuFlow == false
              && application.ExecutionSettings().bWysiwygBaking == true,
          "and every ApplicationExecutionSettings toggle");
    Check(application.Assembler().Placement().ActiveDispatchPolicy().previewBackend
              == Sys::ComputeBackend::Gpu,
          "and bUseGpuMarkers reaches placementStage's own policy with no UI toggle involved");

    // Move every field away from what was just seeded, then flush the SAME call Run()'s normal
    // exit path makes — never Shutdown() itself, which also runs from the destructor and from a
    // caller that never ran a frame at all, and must NOT autosave.
    application.SetSanpackPath("C:/Fixtures/changed.sanpack");
    SetTruncated(application.TabState().system.assetCacheDirectory,
                sizeof(application.TabState().system.assetCacheDirectory), "C:/Fixtures/ChangedCache");
    application.TabState().stratums.environmentPackPath = "C:/Fixtures/changed-environment.sanpack";
    application.ExecutionSettings().bUseGpuTerrain = true;
    application.SaveAppSettingsAtShutdown();

    const Io::AppSettings reloaded = Io::LoadAppSettings(scratchDirectory);
    Check(reloaded.sanpackPath == "C:/Fixtures/changed.sanpack",
          "a clean shutdown flush writes the CURRENT sanpackPath back out");
    Check(reloaded.assetCacheDirectory == "C:/Fixtures/ChangedCache",
          "and the current asset cache directory");
    Check(reloaded.environmentPackPath == "C:/Fixtures/changed-environment.sanpack",
          "and the current environment pack path");
    Check(reloaded.bUseGpuTerrain == true, "and the current execution toggles");
    Check(reloaded.bWysiwygBaking == true, "including one that was left unchanged since the seed");
    Check(reloaded.bUseGpuMarkers == true, "bUseGpuMarkers round-trips even with no UI to edit it");
}

} // namespace Ui
} // namespace SanmapGen
