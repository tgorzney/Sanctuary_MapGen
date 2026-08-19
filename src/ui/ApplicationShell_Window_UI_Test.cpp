// ApplicationShell_Window_UI_Test.cpp — M5-7 acceptance, the launch check: the shell really does
// open a window with a GL 4.3 context, bring up imgui, draw its panels and canvas for a run of
// frames, generate through the Gpu path, and close cleanly. This is the "app runs without crashing
// on open / generate / close" half of the milestone, automated.
//
// Non-interactive by construction: the window is created HIDDEN (ApplicationSettings::
// bWindowVisible, the same tweakable a user never touches), vsync is off so the frames run at
// full speed, and the loop asks the shell to close itself after a fixed count. Nothing here waits
// for a human.
//
// A machine with no display or no GL driver cannot run this, and must not fail the suite for it:
// Initialize() reports that honestly and the test SKIPS, exactly as the GPU parity tests do.
#include "Application_UI.h"
#include <cstdio>
#include <filesystem>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

// A directory that never exists on disk (STEP19_AppSettings_IO). Deliberately local rather than
// ApplicationShell_TestSupport_UI.h's shared NullAppSettingsDirectoryForTests(): that header also
// pulls in ParameterTabs_TestSupport_UI.h's own Check(), which this file already declares its own
// (identically named and signed) local twin of.
std::string NullAppSettingsDirectory() {
    std::error_code pathError;
    return (std::filesystem::temp_directory_path(pathError) / "SanGenWindowTestNeverWritesHere").string();
}

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL: %s\n", label);
    ++failureCount;
}

constexpr int smokeFrameCount        = 12;
constexpr int smokeMapSize           = 64;
constexpr int smokePreviewResolution = 128;
constexpr int smokeErosionDropletCount = 2000;

ApplicationSettings MakeHiddenShellSettings(const char* shaderDirectory) {
    ApplicationSettings settings;
    settings.bWindowVisible       = false;
    settings.bVerticalSyncEnabled = false;
    settings.idleWaitSeconds      = 0.0f;
    settings.previewResolution    = smokePreviewResolution;
    settings.windowWidth          = 640;
    settings.windowHeight         = 480;
    // Never the real `%APPDATA%\SanGen\` — this binary calls RequestClose()+RunOneFrame()+
    // Shutdown() directly rather than Run(), so it never flushes, but LoadAppSettingsAtStartup()
    // still runs at construction and must not read a stranger's real saved preferences.
    settings.appSettingsDirectoryOverride = NullAppSettingsDirectory();
    if (shaderDirectory != nullptr && shaderDirectory[0] != '\0')
        settings.shaderSearchDirectories.push_back(shaderDirectory);
    return settings;
}

} // namespace

// argv[1] is the staged shader directory (add_sangen_test hands it to every acceptance binary).
int main(int argumentCount, char** arguments) {
    Application application(MakeHiddenShellSettings(argumentCount > 1 ? arguments[1] : nullptr));
    application.Recipe().geometry.mapSize = smokeMapSize;
    application.Assembler().Erosion().LayerSettings(0).dropletCount = smokeErosionDropletCount;

    if (!application.Initialize()) {
        std::printf("SKIP: no window/GL context available on this machine.\n");
        return 0;
    }
    Check(application.IsWindowOpen(), "the shell opened a window");

    for (int frame = 0; frame < smokeFrameCount; ++frame) {
        Check(application.RunOneFrame(), "the shell drew a frame without asking to close");
        if (frame == 0)
            Check(application.Driver().PipelineRunCount() == 1,
                  "the first frame generated from the default recipe");
    }
    Check(application.FrameCount() == smokeFrameCount, "every frame ran");
    Check(application.Driver().PreviewCompositeCount() >= 1, "the preview composited");
    Check(application.Composite().CompositeTexels().size()
              == static_cast<std::size_t>(smokePreviewResolution) * smokePreviewResolution,
          "the composite produced a full image");
    // Either backend leaves the viewport drawable: the Gpu composite writes the texture itself,
    // and the Cpu twin's texels are uploaded by the shell (Application_UI.cpp). Which one ran is
    // reported rather than asserted, because a machine without compute support is still valid.
    std::printf("composite backend: %s\n",
                application.Composite().LastRunUsedGpu() ? "Gpu" : "Cpu (shell-mirrored)");
    Check(application.Canvas().PresentationIdentifier() != 0ull,
          "the canvas has a real texture to draw, so the viewport is not blank");

    application.RequestClose();
    Check(!application.RunOneFrame(), "the shell stops once the window wants to close");
    application.Shutdown();
    Check(!application.IsWindowOpen(), "and the window is gone");
    application.Shutdown();   // idempotent: a second teardown must not double-free anything

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
