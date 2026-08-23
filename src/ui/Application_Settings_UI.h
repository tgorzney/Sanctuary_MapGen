// Application_Settings_UI.h — every value the app shell runs on (Constitution §8: no literal at a
// use site, including the GL version, the window metrics, the idle wait and the atlas caps).
// Layer: UI. A member file of Application_UI.h, exactly as PreviewComposite_Settings_UI.h is of
// PreviewComposite_UI.h. These are PRESENTATION/EXECUTION settings: they are not map-recipe
// content and never serialize with it, so they do not live in `_PARAMS`.
#pragma once
#include <string>
#include <vector>
#include "../io/AssetAtlasCache_IO.h"

namespace SanmapGen {
namespace Ui {

struct ApplicationSettings {
    // An ORDERED search path, never a hardcoded path: the caller resolves it
    // (ApplicationMain_UI.cpp) and Sys::GpuResourceManager scans it, first match wins.
    std::vector<std::string> shaderSearchDirectories;

    std::string windowTitle          = "Sanctuary Map Generator - SanGen v3";
    std::string glslVersionDirective = "#version 430";
    int   windowWidth                = 1600;
    int   windowHeight               = 900;
    int   glContextMajorVersion      = 4;
    int   glContextMinorVersion      = 3;
    bool  bWindowVisible             = true;    // false = an offscreen shell (the smoke test)
    bool  bVerticalSyncEnabled       = true;
    float idleWaitSeconds            = 0.10f;   // event block while nothing is dirty

    float leftPaneWidth              = 190.0f;
    float settingsWindowWidth        = 700.0f;
    float settingsWindowHeight       = 860.0f;
    float canvasRegionSidePixels     = 760.0f;  // Minimum-size floor for the preview canvas square,
                                                 // not a fixed size: DrawCanvasWindow fits the real
                                                 // window content region, falling back to this floor
                                                 // for the first frame / a degenerate 0-sized region.
    float backgroundColor[4]         = { 0.10f, 0.10f, 0.12f, 1.0f };
    int   previewResolution          = 512;
    // The constant on-screen radius (screen pixels) a click must land within to hit a marker icon
    // (MapCanvas::SetMarkerPickRadiusScreenPixels, STEP48). Shared with Phase 3's icon draw
    // radius — the two must agree or a click can miss a visibly-hit icon.
    float markerIconRadiusPixels     = 8.0f;

    unsigned workerThreadCount       = 0;       // 0 = hardware concurrency
    Io::AtlasBuildSettings atlasBuildSettings;  // page size / validation caps / thumbnail size
    Io::SanpackEntryFilter assetEntryFilter;    // which archive entries the atlas ingests

    // Empty = resolve the real platform default (Io::DefaultAppSettingsDirectory(),
    // `%APPDATA%\SanGen\` on Windows) at construction. A test points this at a scratch folder
    // instead, so no automated run ever touches the real user's roaming profile
    // (STEP19_AppSettings_IO).
    std::string appSettingsDirectoryOverride;

    // STEP77 — mirrors shaderSearchDirectories' resolution posture exactly, but
    // Io::LoadScenarioRuntimeText (STEP72) takes ONE directory, never a search list: resolved by
    // ApplicationMain_UI.cpp's ResolveScenarioRuntimeResourceDirectory, staged beside the
    // executable at build time as `sangen_lua_resources` (CMakeLists.txt).
    std::string scenarioRuntimeResourceDirectory;
};

} // namespace Ui
} // namespace SanmapGen
