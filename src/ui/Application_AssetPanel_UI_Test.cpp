// Application_AssetPanel_UI_Test.cpp — STEP91 acceptance: the System tab's "Ingest game templates"
// button and its durable status display, driven through a REAL (hidden) `Ui::Application` window,
// the same "RunOneFrame()-equivalent call" posture ApplicationShell_Window_UI_Test.cpp already
// established (Initialize() creates a real GL 4.3 context but never shows it; a machine with no
// window/driver SKIPs rather than failing the suite). New file (verified 2026-08-22: no existing
// test exercises ServiceAssetLoadRequest/DrawAssetPanel/bAssetLoadRequested/LoadAssetAtlas beyond
// ApplicationShell_Window_UI_Test.cpp's own open/close smoke test — see the ticket's own correction).
//
// Real Io::IngestTemplates calls against a scratch install fixture (never a real game install),
// mirroring TemplateIngest_IO_Test.cpp's own scratch-folder conventions at a much smaller scale —
// this ticket owns the UI wiring, not ticket 89's own ingestion-correctness matrix.
#include "Application_UI.h"
#include "../io/AppSettings_IO.h"
#include "../io/FilesystemPrimitives_IO.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL: %s\n", label);
    ++failureCount;
}

std::string ScratchFolderPath(const char* name) {
    std::error_code pathError;
    const std::filesystem::path folder = std::filesystem::temp_directory_path(pathError) / name;
    std::filesystem::remove_all(folder, pathError);
    return folder.string();
}

void WriteTextFile(const std::string& filePath, const std::string& content) {
    std::error_code makeDirectoryError;
    std::filesystem::create_directories(std::filesystem::path(filePath).parent_path(), makeDirectoryError);
    std::ofstream outputStream(filePath, std::ios::binary | std::ios::trunc);
    outputStream << content;
}

// A minimal, real scratch install: one UnitTemplate and one propTemplate -- enough to exercise
// Io::IngestTemplates's real cold-ingest + cache-hit path through this button. The full dialect/
// collision/cache matrix is TemplateIngest_IO_Test.cpp's own job (ticket 89), not repeated here.
void WriteTwoFixtureInstall(const std::string& root) {
    const std::string luaCommon = Io::JoinExportPath(root, "engine/LJ/lua/common");
    WriteTextFile(Io::JoinExportPath(luaCommon, "units/unitsTemplates/unit1.santp"),
        R"(UnitTemplate = { general = { tpId = "uca1001" }, footprint = { x = 1.2, y = 1.2 } })");
    WriteTextFile(Io::JoinExportPath(luaCommon, "props/propsTemplates/propA1.sanprop"),
        R"(propTemplate = { general = { tpId = "epx1001" }, footprint = { x = 1.0, y = 1.0 } })");
}

// Never the real `%APPDATA%\SanGen\` (STEP19_AppSettings_IO) -- this binary flushes settings
// itself (scenario 5), so unlike ApplicationShell_Window_UI_Test.cpp it genuinely writes here, just
// never to a developer's real saved preferences.
std::string ScratchAppSettingsDirectory() {
    return ScratchFolderPath("SanGenAssetPanelUiTest_AppSettings");
}

ApplicationSettings MakeHiddenShellSettings(const char* shaderDirectory,
                                            const std::string& appSettingsDirectory) {
    ApplicationSettings settings;
    settings.bWindowVisible       = false;
    settings.bVerticalSyncEnabled = false;
    settings.idleWaitSeconds      = 0.0f;
    settings.windowWidth          = 640;
    settings.windowHeight         = 480;
    settings.appSettingsDirectoryOverride = appSettingsDirectory;
    if (shaderDirectory != nullptr && shaderDirectory[0] != '\0')
        settings.shaderSearchDirectories.push_back(shaderDirectory);
    return settings;
}

} // namespace

// argv[1] is the staged shader directory (add_sangen_test hands it to every acceptance binary).
int main(int argumentCount, char** arguments) {
    const char* const shaderDirectory = argumentCount > 1 ? arguments[1] : nullptr;
    const std::string appSettingsDirectory = ScratchAppSettingsDirectory();
    const std::string installRoot     = ScratchFolderPath("SanGenAssetPanelUiTest_Install");
    const std::string cacheDirectory  = ScratchFolderPath("SanGenAssetPanelUiTest_Cache");
    WriteTwoFixtureInstall(installRoot);

    Application application(MakeHiddenShellSettings(shaderDirectory, appSettingsDirectory));
    if (!application.Initialize()) {
        std::printf("SKIP: no window/GL context available on this machine.\n");
        return 0;
    }
    // Reaches Application::DrawAssetPanel() every frame below (Application_PanelSystem_UI.cpp's
    // DrawSystemGroupPanel -> DrawPerformancePanel -> DrawAssetPanel chain).
    application.TabState().activePanel = ApplicationPanel::Performance;
    std::snprintf(application.TabState().system.assetCacheDirectory,
                  sizeof(application.TabState().system.assetCacheDirectory), "%s", cacheDirectory.c_str());

    // --- acceptance 1: empty gameInstallRoot -- the "not configured" message, no functional button.
    application.gameInstallRoot.clear();
    Check(application.RunOneFrame(),
          "empty gameInstallRoot: the panel draws its \"not configured\" message without crashing");
    Check(!application.AssetBridge().bTemplateIngestRequested,
          "empty gameInstallRoot: nothing sets a request on its own -- the button is unreachable");
    Check(application.AssetBridge().templateIngestReport.totalSourceFileCount == 0,
          "empty gameInstallRoot: no ingestion happened");

    // --- acceptance 2: bTemplateIngestEnabled == false shows the opt-out message regardless of
    // gameInstallRoot (a real, non-empty root is set here on purpose).
    application.gameInstallRoot        = installRoot;
    application.bTemplateIngestEnabled = false;
    Check(application.RunOneFrame(), "opt-out: the panel draws its opt-out message without crashing");
    Check(!application.AssetBridge().bTemplateIngestRequested,
          "opt-out: nothing sets a request on its own regardless of gameInstallRoot");
    Check(application.AssetBridge().templateIngestReport.totalSourceFileCount == 0,
          "opt-out: no ingestion happened");

    // --- acceptance 3: both configured -- the two-frame announce-then-perform sequence.
    application.bTemplateIngestEnabled = true;
    application.AssetBridge().bTemplateIngestRequested = true;   // the click the real button performs

    Check(application.RunOneFrame(), "the announce frame presents and the window stays open");
    Check(application.AssetBridge().bTemplateIngestRequested && application.AssetBridge().bTemplateIngestAnnounced,
          "first call: the request is still pending, now announced (two-phase contract)");
    Check(application.AssetBridge().templateIngestReport.totalSourceFileCount == 0,
          "first call: the ingest itself has not run yet");
    Check(application.lastTemplateIngestTimestamp.empty(), "first call: lastTemplateIngestTimestamp still unset");

    Check(application.RunOneFrame(), "the perform frame runs and the window stays open");
    Check(!application.AssetBridge().bTemplateIngestRequested && !application.AssetBridge().bTemplateIngestAnnounced,
          "second call: both two-phase flags reset");
    {
        const Io::TemplateIngestReport& coldReport = application.AssetBridge().templateIngestReport;
        Check(coldReport.totalSourceFileCount == 2, "second call: the real fixture install was ingested");
        Check(coldReport.ingestedFootprintRecordCount == 2, "second call: both footprint records landed");
        Check(!coldReport.bLoadedFromDiskCache, "second call: the first ingest is a cold miss");
    }
    Check(!application.lastTemplateIngestTimestamp.empty(), "lastTemplateIngestTimestamp is now set");
    Check(application.lastTemplateIngestTimestamp.size() == 20 &&
          application.lastTemplateIngestTimestamp[10] == 'T' &&
          application.lastTemplateIngestTimestamp.back() == 'Z',
          "lastTemplateIngestTimestamp is ISO-8601 UTC shaped (YYYY-MM-DDTHH:MM:SSZ)");
    Check(application.lastTemplateIngestEntryCount == application.AssetBridge().templateIngestReport.ingestedFootprintRecordCount,
          "lastTemplateIngestEntryCount mirrors the report's own ingestedFootprintRecordCount");

    // --- acceptance 4: a second ingest against the unchanged install reuses ticket 88's disk cache.
    application.AssetBridge().bTemplateIngestRequested = true;
    Check(application.RunOneFrame(), "the second ingest's announce frame runs");
    Check(application.RunOneFrame(), "the second ingest's perform frame runs");
    Check(application.AssetBridge().templateIngestReport.bLoadedFromDiskCache,
          "an unchanged install's second ingest hits the disk cache (ticket 88 reused, not rebuilt)");

    // --- acceptance 5: SaveAppSettingsAtShutdown() persists the post-ingest values end to end.
    const std::string persistedTimestamp = application.lastTemplateIngestTimestamp;
    const int          persistedEntryCount = application.lastTemplateIngestEntryCount;
    application.SaveAppSettingsAtShutdown();
    const Io::AppSettings reloaded = Io::LoadAppSettings(appSettingsDirectory);
    Check(reloaded.lastTemplateIngestTimestamp == persistedTimestamp,
          "the settings flush persists lastTemplateIngestTimestamp (STEP90's wiring, exercised end to end)");
    Check(reloaded.lastTemplateIngestEntryCount == persistedEntryCount,
          "and lastTemplateIngestEntryCount");

    application.RequestClose();
    application.RunOneFrame();
    application.Shutdown();

    std::error_code removeError;
    std::filesystem::remove_all(installRoot, removeError);
    std::filesystem::remove_all(cacheDirectory, removeError);
    std::filesystem::remove_all(appSettingsDirectory, removeError);

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
