// Application_UI.cpp — the shell's construction, its callback wiring, and the ONE step that turns
// a pending dirty flag into a visible image. Layer: UI. No imgui and no GLFW in this file: the
// assembly of the parts is separable from the toolkit that draws them, so the acceptance test can
// drive exactly this wiring with no window and no GL context.
#include "Application_UI.h"
#include "../io/UnknownImportBag_IO.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Applies the stage-owned defaults and hands the assembler straight back. It has to happen in the
// init list, BEFORE `PreviewDriver` is constructed: the driver caches every stage's parameter hash
// on construction, so a default applied afterwards would leave the cache disagreeing with the
// stages and the next edit would be attributed to the wrong stage.
Pipeline::GenerationAssembler& AssemblerWithDefaultStages(Pipeline::GenerationAssembler& assembler) {
    ConfigureDefaultStages(assembler);
    return assembler;
}

} // namespace

// Member init order follows the DECLARATION order in the header: the assembler binds the recipe,
// the composite binds the assembler's baked fields, its resolved markers and the id buffer, and
// the driver binds the assembler. Every one of those is a reference to a member declared earlier.
Application::Application(ApplicationSettings applicationSettings)
    : settings(std::move(applicationSettings)),
      recipe(MakeDefaultMapRecipe()),
      assembler(recipe),
      composite(recipe.geometry, recipe.water, recipe.strata, assembler.Fields(),
                assembler.Placements().markers, entityIdentifiers),
      previewDriver(AssemblerWithDefaultStages(assembler)),
      threadPool(settings.workerThreadCount) {
    // STEP24_ImportNeverRefuses_IO ruling 4/6: one-time wiring — `tabState.files` (FilesTabState)
    // only ever holds a nullable, caller-owned pointer (see that header's own comment), never the
    // JSON-bearing value itself.
    assetBridge.unknownImportData = std::make_unique<Io::UnknownImportBag>();
    tabState.files.unknownImportData = assetBridge.unknownImportData.get();
    // STEP77 §5: the SAME one-time wiring posture — `gameInstallRoot`/`scenarioRuntimeOverridePath`
    // are plain Application members (STEP64), stable addresses for the whole process lifetime, so
    // both tabs that need them (Files, Scenarios) are pointed at them exactly once here.
    // `scenarioRuntimeResourceDirectory` is COPIED, never pointed — it never changes after launch.
    tabState.files.gameInstallRoot                    = &gameInstallRoot;
    tabState.files.scenarioRuntimeOverridePath        = &scenarioRuntimeOverridePath;
    // STEP96_FootprintBakeAndStalenessCheck_IO.md §3.1 call site 1 — same one-time wiring posture as
    // the pair above: `assetBridge.templateIngestReport` is stable for the whole process lifetime.
    tabState.files.templateIngestReport               = &assetBridge.templateIngestReport;
    tabState.files.scenarioRuntimeResourceDirectory   = settings.scenarioRuntimeResourceDirectory;
    tabState.scenarios.scenarioRuntimeOverridePath      = &scenarioRuntimeOverridePath;
    tabState.scenarios.scenarioRuntimeResourceDirectory = settings.scenarioRuntimeResourceDirectory;
    // STEP78 — same one-time wiring posture as the pair above.
    tabState.scenarios.scenarioEditModeState = &scenarioEditMode;
    ConfigureDefaultPreview(composite.Settings(), settings.previewResolution,
                            assembler.WorldUnitsPerCell());
    ConfigureDefaultOverlayLayers(overlaySettings, recipe);
    // The left column's `[O]`/`[ ]` rows are the composite's layer flags from the first frame on,
    // so the column and the image agree before anything is clicked (Application_Visibility_UI.h).
    ApplyPanelVisibility(tabState.visibility, composite.Settings());
    LoadExecutionSettings(assembler, executionSettings);
    assembler.SetThreadPool(&threadPool);
    WireCallbacks();
    // Runs LAST: it may overwrite the mirrors LoadExecutionSettings just read off the stages'
    // ARCH §4.2 defaults, and ApplyExecutionSettings (which it calls) only WRITES DispatchPolicy —
    // never a parameter hash — so it cannot disagree with the hashes PreviewDriver already cached
    // above (Application_AppSettings_UI.cpp).
    LoadAppSettingsAtStartup();
}

Application::~Application() { Shutdown(); }

// The two injected seams that keep the layer graph downward-only (ARCH §3.1). PIPELINE may not
// know a composite exists, and the canvas may not know a pipeline exists, so the shell — which
// legally sees both — hands each of them a closure over the other.
void Application::WireCallbacks() {
    previewDriver.SetPreviewCompositeCallback([this] { composite.Compose(/*bNeedsTexelReadback=*/false); });
    canvas.SetSelectionChangedCallback([this](std::uint32_t entityIdentifier) {
        lastSelectedEntityIdentifier = entityIdentifier;
    });
    // STEP48: picking reads the resolved markers and PIPELINE's spatial index over them, in world
    // space, instead of the baked entity-id buffer — see MapCanvas_UI.h's header comment.
    canvas.SetPreviewComposite(&composite);
    canvas.SetMarkerPickingSource(&assembler.Placements().markers, &assembler.MarkerSpatialGrid());
    canvas.SetMarkerPickRadiusScreenPixels(settings.markerIconRadiusPixels);
    // STEP53 — the screen-space overlay icon draw pass's sources, every one a push-in pointer
    // (§0's correction: never an Application reach-back from inside MapCanvas).
    canvas.SetOverlayLayerSettings(&overlaySettings);
    canvas.SetOverlayRenderingSettings(&overlayIconRenderingSettings);
    canvas.SetOverlayPlacementSource(&assembler.Placements(), &assembler.RuleBucketIndex());
    canvas.SetOverlayRecipe(&recipe);
    canvas.SetIconAtlasSource(&IconPairingLookup(), &IconManifest());
    canvas.SetWorldFootprintSizeTable(&WorldFootprintSizeTable());
    // STEP78 — Scenario Edit Mode's own state; see MapCanvas_UI.h's SetScenarioEditModeState.
    canvas.SetScenarioEditModeState(&scenarioEditMode);
    // STEP94 — the manual-marker drag-and-follow source; see MapCanvas_UI.h's
    // SetManualMarkerDragSource. `markers`/`markerLayers` are the SAME vectors the Markers tab
    // edits (recipe.markers/recipe.markerLayers) — one source of truth, never a second copy.
    canvas.SetManualMarkerDragSource(&recipe.markers, &recipe.markerLayers, &recipe.geometry, &recipe);
}

// The whole of the shell's generation duty. WHICH tier this is was derived by the driver from the
// stages' own parameter hashes; the shell neither decides it nor knows what ran.
Pipeline::RefreshTier Application::ServiceDirtyTier() {
    const Pipeline::RefreshTier servicedTier = previewDriver.Refresh();
    if (servicedTier != Pipeline::RefreshTier::Nothing) BindCompositeToCanvas();
    return servicedTier;
}

// The canvas draws a SYS-owned texture. The Gpu composite writes one itself; the Cpu twin (no
// context, or a kernel that would not compile — PreviewComposite_Gpu_UI.cpp falls back rather than
// producing nothing) has only texels, so the shell uploads them through the same SYS seam instead
// of leaving the viewport blank. Either way no GL handle enters the UI layer (ARCH §3.2).
void Application::BindCompositeToCanvas() {
    Sys::GpuTextureHandle previewTexture = composite.CompositeTexture();
    if (!composite.LastRunUsedGpu() && gpuResourceManager != nullptr)
        previewTexture = UploadCompositeTexels();
    canvas.SetPreviewTexture(gpuResourceManager.get(), previewTexture, composite.Resolution());
}

Sys::GpuTextureHandle Application::UploadCompositeTexels() {
    const int resolution = composite.Resolution();
    const std::vector<unsigned int>& texels = composite.CompositeTexels();
    const Sys::GpuTextureHandle mirrorTexture =
        gpuResourceManager->EnsureTexture("previewCompositeCpuMirror", resolution, resolution);
    if (mirrorTexture.IsValid() && !texels.empty())
        gpuResourceManager->UploadTexture(mirrorTexture, texels.data(),
                                          texels.size() * sizeof(unsigned int));
    return mirrorTexture;
}

// The Performance panel's fan-out, run every frame rather than only while that panel is drawn: a
// backend or determinism choice has to keep holding once the user moves to another tab. An
// execution change is invisible to every stage's parameter hash — the recipe did not move — so the
// driver's door for it is RequestMapUpdate (SystemTab_UI.cpp states the same reasoning).
bool Application::ApplyExecutionPolicy() {
    if (!ApplyExecutionSettings(executionSettings, tabState.system.bDeterministic, bUseGpuMarkers,
                                assembler))
        return false;
    previewDriver.RequestMapUpdate();
    return true;
}

const IconAtlasManifest* Application::ActiveIconManifest() const {
    return assetBridge.iconManifest.EntryCount() > 0 ? &assetBridge.iconManifest : nullptr;
}

} // namespace Ui
} // namespace SanmapGen
