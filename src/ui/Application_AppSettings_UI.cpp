// Application_AppSettings_UI.cpp — the shell's durable-preferences bridge (STEP19_AppSettings_IO):
// load `AppSettings` once at construction, seed the caller-owned state that has no other durable
// home (`sanpackPath`, the SystemTab's asset cache directory, the StratumsTab's environment pack
// path, `ApplicationExecutionSettings`, and `bUseGpuMarkers`), then write the current values back
// out on a clean `Run()` exit. Layer: UI — this is the one unit that legally touches both IO and
// SYS at once (ARCH §3.1), the same standing as the sanpack/atlas bridge in Application_Assets_UI.cpp.
#include "Application_UI.h"
#include "../io/AppSettings_IO.h"
#include "../io/AppSettingsLocation_IO.h"
#include <cstring>

namespace SanmapGen {
namespace Ui {
namespace {

// Copies into a fixed caller-owned buffer, TRUNCATING rather than overflowing (Constitution §6) —
// the same rule Application_AssetPanel_UI.cpp's StoreTemplateIdentifier and
// Application_Assets_UI.cpp's SetSanpackPath already apply to their own fixed buffers.
void CopyTruncated(const std::string& source, char* destination, std::size_t destinationSize) {
    const std::size_t copyCount = source.size() < destinationSize - 1 ? source.size() : destinationSize - 1;
    std::memcpy(destination, source.c_str(), copyCount);
    destination[copyCount] = '\0';
}

} // namespace

// Constructor-time only (never ApplicationMain_UI.cpp — ARCH §5.5 keeps that file entry-point-only).
// A bad/missing file degrades to compiled defaults inside Io::LoadAppSettings itself, so nothing
// here needs its own fallback branch (Constitution §6).
void Application::LoadAppSettingsAtStartup() {
    appSettingsDirectory = settings.appSettingsDirectoryOverride.empty()
                               ? Io::DefaultAppSettingsDirectory()
                               : settings.appSettingsDirectoryOverride;
    const Io::AppSettings loaded = Io::LoadAppSettings(appSettingsDirectory);

    SetSanpackPath(loaded.sanpackPath);
    CopyTruncated(loaded.assetCacheDirectory, tabState.system.assetCacheDirectory,
                 sizeof(tabState.system.assetCacheDirectory));
    tabState.stratums.environmentPackPath = loaded.environmentPackPath;
    gameInstallRoot             = loaded.gameInstallRoot;
    scenarioRuntimeOverridePath = loaded.scenarioRuntimeOverridePath;

    executionSettings.bUseGpuTerrain = loaded.bUseGpuTerrain;
    executionSettings.bUseGpuFlow    = loaded.bUseGpuFlow;
    executionSettings.bWysiwygBaking = loaded.bWysiwygBaking;
    bUseGpuMarkers = loaded.bUseGpuMarkers;
    ApplyExecutionSettings(executionSettings, tabState.system.bDeterministic, bUseGpuMarkers, assembler);
}

// The mirror image, gathered from the exact fields LoadAppSettingsAtStartup seeded — every one of
// them is either a caller-owned buffer the tabs edit directly or a mirror ApplyExecutionPolicy keeps
// current every frame, so reading them back here needs no fresh mirror pass of its own.
void Application::SaveAppSettingsAtShutdown() {
    Io::AppSettings current;
    current.sanpackPath          = SanpackPath();
    current.assetCacheDirectory  = tabState.system.assetCacheDirectory;
    current.environmentPackPath  = tabState.stratums.environmentPackPath;
    current.gameInstallRoot             = gameInstallRoot;
    current.scenarioRuntimeOverridePath = scenarioRuntimeOverridePath;
    current.bUseGpuTerrain       = executionSettings.bUseGpuTerrain;
    current.bUseGpuFlow          = executionSettings.bUseGpuFlow;
    current.bWysiwygBaking       = executionSettings.bWysiwygBaking;
    current.bUseGpuMarkers       = bUseGpuMarkers;
    Io::SaveAppSettings(appSettingsDirectory, current);
}

} // namespace Ui
} // namespace SanmapGen
