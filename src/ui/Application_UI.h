// Application_UI.h — SanGen v2's application shell: window, GL context, imgui runtime, and the
// frame loop mounting the tabs/canvas over the pipeline. Layer: UI. Accuracy class: Visual. Owns
// NO SIM LOGIC (ARCH §3.2): each frame it only sets params, lets `Pipeline::PreviewDriver` derive
// and service the dirty flags, draws what the composite baked — no stage order, no rival backend
// toggle. The one unit that legally sees IO and UI at once (sanpack -> atlas -> residency ->
// `Ui::IconAtlasManifest` bridge lives here, in no tab); GL reaches it only through
// `Sys::GpuResourceManager` (ARCH §3.2).
// Aspect .cpp units behind this header are each self-documented at their own top (ARCH §1.5): _UI
// / _Window_UI / _Frame_UI / _Draw_UI / _LeftColumn_UI / _Panel{Terrain,Environment,System}_UI /
// _Execution_UI / _Assets_UI / _AssetPanel_UI / _Recipe_UI / _Preview{Setup,Ramps}_UI /
// _AppSettings_UI / _ViewLayersPopup_UI (STEP54, the "View" toolbar popup). Member headers (§7.1
// composition, §1.5 split; none reached by any other unit):
// _Settings_UI.h / _Panels_UI.h / _Visibility_UI.h / _Execution_UI.h / _HostedSettings_UI.h (no
// `Params::MapRecipe` home yet) / _TabState_UI.h / _AssetBridge_UI.h (sanpack -> atlas ->
// residency -> `Ui::IconAtlasManifest` bridge) / _Defaults_UI.h (launch defaults, free functions) /
// _ViewLayersPopup_UI.h (STEP54's pure ApplyViewLayerSignal, testable with no imgui frame).
// ApplicationMain_UI.cpp is the thin entry point, not part of the library.
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "Application_AssetBridge_UI.h"
#include "Application_Defaults_UI.h"
#include "Application_Execution_UI.h"
#include "Application_HostedSettings_UI.h"
#include "Application_Settings_UI.h"
#include "Application_TabState_UI.h"
#include "MapCanvas_ScenarioEditMode_State_UI.h"
#include "MapCanvas_UI.h"
#include "OverlayLayer_Settings_UI.h"
#include "PreviewComposite_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/GenerationAssembler_PIPELINE.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "../sys/ThreadPool_SYS.h"

namespace SanmapGen {
namespace Ui {

class Application {
public:
    explicit Application(ApplicationSettings applicationSettings = ApplicationSettings());
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // --- lifecycle (Application_Window_UI.cpp / Application_Frame_UI.cpp) ---
    bool Initialize();       // window + context + imgui + the GPU seam. False = no window.
    void Shutdown();         // idempotent; GPU objects die while the context is still current.
    int  Run();              // frames until the window closes; returns a process exit code.
    bool RunOneFrame();      // one frame; false once the window wants to close.
    bool IsWindowOpen() const;
    void RequestClose();

    // --- the three per-frame steps the acceptance test drives directly ---
    // Services whichever dirty tier is pending and re-points the canvas at the result.
    Pipeline::RefreshTier ServiceDirtyTier();          // Application_UI.cpp
    // Turns a NEW icon-grid selection into the selected rule's template id (`tpId`).
    void ResolveIconSelections();                      // Application_Assets_UI.cpp
    // Pushes the Performance toggles onto every stage's policy; true = one moved and a regeneration
    // was requested. Runs every frame, not only while that panel is on screen.
    bool ApplyExecutionPolicy();                       // Application_UI.cpp

    // The ONE step Run()'s clean-exit path performs after the frame loop ends (STEP19_AppSettings_IO)
    // — never folded into Shutdown(), which also runs from the destructor and must not autosave.
    void SaveAppSettingsAtShutdown();          // Application_AppSettings_UI.cpp

    // --- assets (Application_Assets_UI.cpp) ---
    void SetSanpackPath(const std::string& path);
    const char* SanpackPath() const { return assetBridge.sanpackPath; }
    bool LoadAssetAtlas();                             // sanpack -> atlas -> pages -> manifest
    const IconAtlasManifest& IconManifest() const { return assetBridge.iconManifest; }
    // The template identifier an icon id names; empty when it names nothing.
    std::string TemplateIdentifierOfIcon(int iconId) const;
    // The atlas-pairing lookup STEP53's overlay renderer consumes: templateIdentifier ->
    // {thumbnailIconId, strategicIconId}. strategicIconId presently always resolves to
    // kInvalidIconId — no authored strategic-icon content exists yet (ARCH_14_03_IconRenderingLod.md §14.3, separate,
    // unscheduled ticket); this accessor's contract does not change when that content lands, only
    // the resolved value does.
    const IconAtlasPairingLookup& IconPairingLookup() const { return assetBridge.iconPairingLookup; }
    // STEP58's placeholder table (§0) — WireCallbacks() injects a pointer to this into MapCanvas;
    // the draw pass itself never calls this accessor directly (MapCanvas_UI.h's own header comment).
    const Io::WorldFootprintSizeTable& WorldFootprintSizeTable() const { return assetBridge.worldFootprintSizeTable; }
    const std::string& AssetStatusMessage() const { return assetBridge.assetStatusMessage; }

    // --- the assembled parts, exposed so the acceptance test drives the SHELL's own wiring ---
    ApplicationSettings&           Settings()  { return settings; }
    Params::MapRecipe&             Recipe()    { return recipe; }
    Pipeline::GenerationAssembler& Assembler() { return assembler; }
    Pipeline::PreviewDriver&       Driver()    { return previewDriver; }
    PreviewComposite&              Composite() { return composite; }
    MapCanvas&                     Canvas()    { return canvas; }
    OverlayLayerSettings&          OverlaySettings() { return overlaySettings; }
    // STEP78 — the interactive canvas overlay's own cross-frame state (default-off; the Scenarios
    // tab's detail panel toggles it, MapCanvas takes exclusive interaction ownership while active).
    ScenarioEditModeState&         ScenarioEditMode() { return scenarioEditMode; }
    // STEP53's cross-layer visible-vertex budget tunables (Constitution §8) — a push-in pointer
    // into MapCanvas, same pattern as OverlaySettings() above.
    OverlayRenderingSettings&      OverlayIconRenderingSettings() { return overlayIconRenderingSettings; }
    ApplicationTabState&           TabState()  { return tabState; }
    ApplicationExecutionSettings&  ExecutionSettings() { return executionSettings; }
    Sys::DispatchPolicy&           ActiveDispatchPolicy() { return dispatchPolicy; }
    Data::EntityIdBuffer&          EntityIdentifiers() { return entityIdentifiers; }
    std::uint32_t LastSelectedEntityIdentifier() const { return lastSelectedEntityIdentifier; }
    int FrameCount() const { return frameCount; }

    // Map Scenario export target root / runtime-resource override (STEP64_GameInstallLocation_IO) —
    // plain caller-owned members, same "no picker, no checkbox yet" posture bUseGpuMarkers shipped
    // with (STEP19_AppSettings_IO); public (unlike bUseGpuMarkers) because nothing else on Application
    // exposes an accessor for them yet, and the shell bridge / its own test read and write them directly.
    std::string gameInstallRoot;
    std::string scenarioRuntimeOverridePath;

private:
    void WireCallbacks();                    // Application_UI.cpp
    void BindCompositeToCanvas();            // Application_UI.cpp
    Sys::GpuTextureHandle UploadCompositeTexels();   // Application_UI.cpp
    void InitializeImgui();                  // Application_Window_UI.cpp
    void InitializeGpuResources();           // Application_Window_UI.cpp
    void PumpWindowEvents();                 // Application_Frame_UI.cpp
    void BeginImguiFrame();                  // Application_Frame_UI.cpp
    void EndImguiFrame();                    // Application_Frame_UI.cpp
    void DrawSettingsWindow();               // Application_Draw_UI.cpp
    void DrawActivePanel();                  // Application_Draw_UI.cpp
    void DrawCanvasWindow();                 // Application_Draw_UI.cpp
    void DrawViewLayersPopup();              // Application_ViewLayersPopup_UI.cpp
    void DrawPanelSwitcher();                // Application_LeftColumn_UI.cpp
    void DrawTerrainGroupPanel();            // Application_PanelTerrain_UI.cpp
    void DrawHeightRampSection();            // Application_PanelTerrain_UI.cpp
    void DrawEnvironmentGroupPanel();        // Application_PanelEnvironment_UI.cpp
    void DrawSystemGroupPanel();             // Application_PanelSystem_UI.cpp
    void DrawPerformancePanel();             // Application_PanelSystem_UI.cpp
    bool UploadAtlasPages();                 // Application_Assets_UI.cpp
    void DrawAssetPanel();                   // Application_AssetPanel_UI.cpp
    bool ServiceAssetLoadRequest();          // Application_AssetPanel_UI.cpp
    bool ApplyIconSelection(int selectedIconId, int& lastIconId,
                            char (&templateIdentifier)[8]);   // Application_AssetPanel_UI.cpp
    const IconAtlasManifest* ActiveIconManifest() const;   // null until an atlas is resident
    void LoadAppSettingsAtStartup();         // Application_AppSettings_UI.cpp

    ApplicationSettings           settings;
    Params::MapRecipe             recipe;
    Pipeline::GenerationAssembler assembler;
    Data::EntityIdBuffer          entityIdentifiers;
    PreviewComposite              composite;
    Pipeline::PreviewDriver       previewDriver;
    MapCanvas                     canvas;
    OverlayLayerSettings          overlaySettings;   // the six-domain screen-space overlay stack
    ScenarioEditModeState         scenarioEditMode;  // STEP78 — default off, one canvas-wide slot
    OverlayRenderingSettings      overlayIconRenderingSettings;   // STEP53's cross-layer budget knobs
    ApplicationTabState           tabState;
    ApplicationHostedSettings     hostedSettings;   // the tab settings with no recipe home yet
    ApplicationExecutionSettings  executionSettings;
    bool bUseGpuMarkers = false;   // placementStage's preview backend; no checkbox yet (STEP19)
    std::string appSettingsDirectory;   // resolved once at construction (STEP19_AppSettings_IO)
    Sys::DispatchPolicy           dispatchPolicy;   // the SystemTab's determinism/backend home
    Sys::ThreadPool               threadPool;
    std::unique_ptr<Sys::GpuResourceManager> gpuResourceManager;   // created with the context
    ApplicationAssetBridge        assetBridge;   // sanpack -> atlas -> residency -> manifest
    void*                         windowHandle = nullptr;   // GLFWwindow*, kept opaque here
    std::uint32_t lastSelectedEntityIdentifier = Data::EntityIdBuffer::emptySentinel;
    int  frameCount           = 0;
    bool bImguiReady          = false;
};

} // namespace Ui
} // namespace SanmapGen
