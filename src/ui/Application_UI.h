// Application_UI.h — the SanGen v2 application shell: the window, the GL context, the imgui
// runtime and the frame loop that mount the M5-6 tabs and the M5-5 canvas over the M4-5 pipeline.
// Layer: UI. Accuracy class: Visual. It OWNS NO SIM LOGIC (ARCH §3.2): each frame it only sets
// params, lets `Pipeline::PreviewDriver` DERIVE and service the two-tier dirty flags, and draws
// what the composite baked. It knows no stage order and holds no rival backend toggle.
//
// It is the one unit that legally sees IO and UI at once, which is why the sanpack -> atlas ->
// residency -> `Ui::IconAtlasManifest` bridge lives here and in no tab (Application_Assets_UI.cpp;
// the gap M5-6 flagged). GL objects reach it only through `Sys::GpuResourceManager` (ARCH §3.2).
//
// Aspect translation units behind this one header (ARCH §1.5):
//   Application_UI.cpp               construction, callback wiring, the dirty-tier service
//   Application_Window_UI.cpp        GLFW window + GL context + imgui bring-up and teardown
//   Application_Frame_UI.cpp         the frame loop and the imgui frame begin/end
//   Application_Draw_UI.cpp          the chrome: the panel switcher, the panels, the canvas
//   Application_Assets_UI.cpp        the sanpack/atlas load and the icon-id -> template-id bridge
//   Application_AssetPanel_UI.cpp    the asset panel and the per-frame icon-selection resolution
//   Application_Recipe_UI.cpp        the default MapRecipe and its stage constants
//   Application_PreviewSetup_UI.cpp  the default preview composition (ramps + field layers)
// Member headers (the ARCH §7.1 composition rule, split out for the §1.5 ceiling — neither is a
// type any other unit reaches):
//   Application_Settings_UI.h        every value the shell runs on (Constitution §8)
//   Application_TabState_UI.h        the caller-owned state of each hosted panel
//   ApplicationMain_UI.cpp           the thin entry point (NOT part of the library)
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
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

    // --- the two per-frame steps the acceptance test drives directly ---
    // Services whichever dirty tier is pending and re-points the canvas at the result.
    Pipeline::RefreshTier ServiceDirtyTier();          // Application_UI.cpp
    // Turns a NEW icon-grid selection into the selected rule's template id (`tpId`).
    void ResolveIconSelections();                      // Application_Assets_UI.cpp

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
    void DrawPanelSwitcher();                // Application_Draw_UI.cpp
    void DrawActivePanel();                  // Application_Draw_UI.cpp
    void DrawPreviewPanel();                 // Application_Draw_UI.cpp
    void DrawSystemPanel();                  // Application_Draw_UI.cpp
    void DrawCanvasWindow();                 // Application_Draw_UI.cpp
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

// The shell's defaults, in their own translation unit so the class file stays small.
Params::MapRecipe MakeDefaultMapRecipe();                          // Application_Recipe_UI.cpp
void ConfigureDefaultStages(Pipeline::GenerationAssembler& assembler);
void ConfigureDefaultPreview(PreviewCompositeSettings& previewSettings, int previewResolution,
                             float worldUnitsPerCell);

// The atlas-id bridge itself, as a free function so it is drivable without a window: assigns each
// `Io::AtlasEntry` its index in Entries() as the `iconId` the grid emits, carries the uv rect
// across, and fills the id -> template-identifier side table.  (Application_Assets_UI.cpp)
void BuildIconAtlasManifest(const Io::AssetAtlas& atlas, const Sys::AtlasResidency& atlasResidency,
                            Sys::GpuResourceManager* gpuResourceManager,
                            IconAtlasManifest& outManifest,
                            std::vector<std::string>& outTemplateIdentifiers);

} // namespace Ui
} // namespace SanmapGen
