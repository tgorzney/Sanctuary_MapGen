// Application_UI.h — the SanGen v2 application shell: the window, the GL context, the imgui
// runtime and the frame loop that mount the M5-6 tabs and the M5-5 canvas over the M4-5 pipeline.
// Layer: UI. Accuracy class: Visual. It OWNS NO SIM LOGIC (ARCH §3.2): each frame it only sets
// params, lets `Pipeline::PreviewDriver` DERIVE and service the two-tier dirty flags, and draws
// what the composite baked. It knows no stage order and holds no rival backend toggle.
//
// It is the one unit that legally sees IO and UI at once, which is why the sanpack -> atlas ->
// residency -> `Ui::IconAtlasManifest` bridge lives here and in no tab. GL objects reach it only
// through `Sys::GpuResourceManager` (ARCH §3.2).
//
// Aspect translation units behind this one header, each tagged at its own top (ARCH §1.5):
// _UI / _Window_UI / _Frame_UI (assembly, bring-up, the frame loop) · _Draw_UI / _LeftColumn_UI
// (the two panes; the v1 column and its `[O]`/`[ ]`) · _Panel{Terrain,Environment,System}_UI (the
// three groups' bodies) · _Execution_UI (the Performance toggles -> per-stage DispatchPolicy) ·
// _Assets_UI / _AssetPanel_UI (the sanpack -> atlas -> `tpId` bridge) · _Recipe_UI /
// _PreviewSetup_UI / _PreviewRamps_UI (the launch defaults).
// Member headers (the ARCH §7.1 composition rule, split for the §1.5 ceiling — none is a type any
// other unit reaches): _Settings_UI.h (every value the shell runs on, Constitution §8) ·
// _Panels_UI.h (the panel catalogue: groups, order, which rows toggle) · _Visibility_UI.h (the
// `[O]`/`[ ]` state and its mapping onto the composite) · _Execution_UI.h · _HostedSettings_UI.h
// (the tab settings with no `Params::MapRecipe` home yet) · _TabState_UI.h · _Defaults_UI.h (the
// launch defaults and the atlas bridge, as free functions).
// ApplicationMain_UI.cpp is the thin entry point, and is NOT part of the library.
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "Application_Defaults_UI.h"
#include "Application_Execution_UI.h"
#include "Application_HostedSettings_UI.h"
#include "Application_Settings_UI.h"
#include "Application_TabState_UI.h"
#include "MapCanvas_UI.h"
#include "PreviewComposite_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/GenerationAssembler_PIPELINE.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "../sys/AtlasResidency_SYS.h"
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

    // --- assets (Application_Assets_UI.cpp) ---
    void SetSanpackPath(const std::string& path);
    const char* SanpackPath() const { return sanpackPath; }
    bool LoadAssetAtlas();                             // sanpack -> atlas -> pages -> manifest
    const IconAtlasManifest& IconManifest() const { return iconManifest; }
    // The template identifier an icon id names; empty when it names nothing.
    std::string TemplateIdentifierOfIcon(int iconId) const;
    const std::string& AssetStatusMessage() const { return assetStatusMessage; }

    // --- the assembled parts, exposed so the acceptance test drives the SHELL's own wiring ---
    ApplicationSettings&           Settings()  { return settings; }
    Params::MapRecipe&             Recipe()    { return recipe; }
    Pipeline::GenerationAssembler& Assembler() { return assembler; }
    Pipeline::PreviewDriver&       Driver()    { return previewDriver; }
    PreviewComposite&              Composite() { return composite; }
    MapCanvas&                     Canvas()    { return canvas; }
    ApplicationTabState&           TabState()  { return tabState; }
    ApplicationExecutionSettings&  ExecutionSettings() { return executionSettings; }
    Sys::DispatchPolicy&           ActiveDispatchPolicy() { return dispatchPolicy; }
    Data::EntityIdBuffer&          EntityIdentifiers() { return entityIdentifiers; }
    std::uint32_t LastSelectedEntityIdentifier() const { return lastSelectedEntityIdentifier; }
    int FrameCount() const { return frameCount; }

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

    ApplicationSettings           settings;
    Params::MapRecipe             recipe;
    Pipeline::GenerationAssembler assembler;
    Data::EntityIdBuffer          entityIdentifiers;
    PreviewComposite              composite;
    Pipeline::PreviewDriver       previewDriver;
    MapCanvas                     canvas;
    ApplicationTabState           tabState;
    ApplicationHostedSettings     hostedSettings;   // the tab settings with no recipe home yet
    ApplicationExecutionSettings  executionSettings;
    Sys::DispatchPolicy           dispatchPolicy;   // the SystemTab's determinism/backend home
    Sys::ThreadPool               threadPool;
    std::unique_ptr<Sys::GpuResourceManager> gpuResourceManager;   // created with the context
    Sys::AtlasResidency           atlasResidency;
    Io::AssetAtlasCache           assetAtlasCache;
    IconAtlasManifest             iconManifest;
    std::vector<std::string>      iconTemplateIdentifiers;   // iconId -> `tpId` side table
    std::string                   assetStatusMessage = "No sanpack loaded.";
    char                          sanpackPath[260] = { 0 };
    void*                         windowHandle = nullptr;   // GLFWwindow*, kept opaque here
    std::uint32_t lastSelectedEntityIdentifier = Data::EntityIdBuffer::emptySentinel;
    int  frameCount           = 0;
    bool bAssetLoadRequested  = false;
    bool bAssetLoadAnnounced  = false;
    bool bImguiReady          = false;
};

} // namespace Ui
} // namespace SanmapGen
