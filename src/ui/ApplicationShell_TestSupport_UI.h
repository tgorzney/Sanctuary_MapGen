// ApplicationShell_TestSupport_UI.h — shared scaffolding for the M5-7 acceptance binary: the
// small, fast shell every check drives and the probes they read. Test-only; nothing outside a
// *_Test.cpp includes it (the same standing as PreviewIntegration_TestScene_UI.h).
//
// The point of this harness is that the scene is NOT hand-built: it is a real `Ui::Application`,
// so what the checks exercise is the SHELL's own assembly — its default recipe, its composite
// callback, its canvas binding, its icon bridge — not a replica of it. The only things adjusted
// are the sizes, so the whole binary runs in a second on the Cpu twin with no window and no GL.
#pragma once
#include "Application_UI.h"
#include "ParameterTabs_TestSupport_UI.h"   // Check(), the synthetic pointer sequences
#include <filesystem>

namespace SanmapGen {
namespace Ui {

constexpr int shellTestMapSize          = 64;
constexpr int shellTestPreviewResolution = 64;
constexpr int shellTestErosionDropletCount = 2000;
constexpr float shellTestCanvasSidePixels  = 256.0f;

// A directory that never exists on disk. EVERY shell test's Application must be constructed with
// this override (ApplicationSettings::appSettingsDirectoryOverride) rather than the bare default,
// so an automated run never reads — and, since none of these binaries calls Run() or
// SaveAppSettingsAtShutdown(), never could write — the real `%APPDATA%\SanGen\AppSettings.json` a
// developer's real, previously-run app may have created (STEP19_AppSettings_IO). Without this, a
// machine that HAS such a file would seed these shells from a stranger's saved preferences instead
// of the compiled defaults every other assertion here assumes.
inline std::string NullAppSettingsDirectoryForTests() {
    std::error_code pathError;
    return (std::filesystem::temp_directory_path(pathError) / "SanGenShellTestsNeverWriteHere").string();
}

inline ApplicationSettings TestApplicationSettings() {
    ApplicationSettings settings;
    settings.appSettingsDirectoryOverride = NullAppSettingsDirectoryForTests();
    return settings;
}

// A shell shrunk to test size, generated once. Nothing here replaces a shell member: every value
// below is set through the same public surface a user's edit would move.
inline void PrepareShellForTest(Application& application) {
    application.Recipe().geometry.mapSize = shellTestMapSize;
    application.Composite().Settings().previewResolution = shellTestPreviewResolution;
    application.Assembler().Erosion().LayerSettings(0).dropletCount = shellTestErosionDropletCount;
    application.Canvas().View().SetRegionSide(shellTestCanvasSidePixels);
}

inline unsigned long long CompositeImageChecksum(const Application& application) {
    unsigned long long checksum = 1469598103934665603ull;
    for (unsigned int texel : const_cast<Application&>(application).Composite().CompositeTexels())
        checksum = (checksum ^ texel) * 1099511628211ull;
    return checksum;
}

// Preview pixels per heightfield cell — the factor PreviewComposite::BuildEntityPoints uses.
inline float ShellPreviewPixelsPerCell(Application& application) {
    return static_cast<float>(application.Composite().Resolution())
         / static_cast<float>(application.Assembler().Fields().VertexSize() - 1);
}

// The region-local cursor position that lands on one resolved marker's mark, at the default
// (unzoomed) view — the click a user makes on that marker.
inline void MarkerCursorPosition(Application& application, std::size_t markerIndex,
                                 float& cursorX, float& cursorY) {
    const Data::PlacementInstances& markers = application.Assembler().Placements().markers;
    const float cellsPerWorldUnit =
        ReciprocalOrZero(application.Composite().Settings().worldUnitsPerCell);
    const float pixelsPerCell = ShellPreviewPixelsPerCell(application);
    const float regionPerPixel = application.Canvas().View().RegionSidePixels()
                               / static_cast<float>(application.Composite().Resolution());
    const int pixelX = static_cast<int>(markers.positionX[markerIndex] * cellsPerWorldUnit * pixelsPerCell);
    const int pixelY = static_cast<int>(markers.positionZ[markerIndex] * cellsPerWorldUnit * pixelsPerCell);
    cursorX = (static_cast<float>(pixelX) + 0.5f) * regionPerPixel;
    cursorY = (static_cast<float>(pixelY) + 0.5f) * regionPerPixel;
}

// Defined in the sibling test translation units. Each binary below links only the ones its own
// main() calls — a declaration pulls nothing in.
void RunShellDirtyTierChecks(Application& application);   // ApplicationShell_DirtyTier_UI_Test.cpp
void RunShellIconBridgeChecks();                          // ApplicationShell_IconBridge_UI_Test.cpp
void RunShellVisibilityChecks();                          // ApplicationShell_Visibility_UI_Test.cpp
void RunShellExecutionChecks();                           // ApplicationShell_Execution_UI_Test.cpp
void RunShellAppSettingsChecks();                          // ApplicationShell_AppSettings_UI_Test.cpp

} // namespace Ui
} // namespace SanmapGen
