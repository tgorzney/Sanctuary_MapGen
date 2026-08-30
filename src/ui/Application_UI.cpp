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
      composite(recipe.geometry, recipe.water, recipe.strata, recipe.areas, assembler.Fields(),
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
    // ARCH §14.18 item 6 — the production hot path never needs the texel readback (the canvas
    // samples CompositeTexture() directly), and now also gates the baked-input uploads: a
    // PreviewRender-tier refresh provably left the bake untouched (PreviewDriver_PIPELINE.h's own
    // invariant), so it skips re-packing/re-uploading heightfield/flow/accumulation/slope/
    // surface-stratum-weights. A MapUpdate-tier refresh always re-uploads (the bake just changed).
    previewDriver.SetPreviewCompositeCallback([this](Pipeline::RefreshTier tier) {
        composite.Compose({ /*bNeedsTexelReadback=*/false,
                            /*bBakedInputsChanged=*/tier == Pipeline::RefreshTier::MapUpdate });
    });
    // ARCH §19.25, item 4 — widened to the full key: a procedural change (bManual == false) updates
    // lastSelectedEntityIdentifier exactly as before; a manual change updates the Markers tab's own
    // selectedManualInstanceIdentifier instead, keeping the list's highlight in sync when the CANVAS
    // is what changed the selection (the two-way sync's other half is item 5's
    // selectManualMarkerInstanceCallback, below).
    // ARCH §21.1 — the closure now generalizes by PARTITIONING THE FULL SET, not just reading the
    // primary: `selectedManualInstanceIdentifiers` (STEP141's already-shipped plural field) now
    // sources from the real multi-select set instead of a synthesized single-element list — nothing
    // else about it changes. Props/Decals: no-op, unchanged (no tab-level plural field exists yet).
    // STEP232/STEP233 — this closure's third parameter is Application's own consumer of
    // MapCanvas_UI.h's `bSuppressTabStateResync` (renamed/generalized from STEP232's narrower
    // `bWasShiftGesture` — see that setter's own header comment for the full "why"). Gating BOTH writes
    // below on it fixes THREE independent same-click clobbers total, across STEP232 and STEP233 together
    // (all three share the identical mechanism: a synchronous same-click echo from the canvas
    // overwriting a value the LIST side had already correctly computed one call earlier):
    //  1. (STEP232) selectedManualInstanceIdentifiers (plural) used to resync UNCONDITIONALLY from the
    //     canvas's own key set on every change — clobbering a Shift-click's own richer list-computed
    //     range.
    //  2. (STEP232) manualInstanceSelectionAnchorIdentifier — a Shift-click's own PRESERVED anchor
    //     getting overwritten.
    //  3. (STEP233) manualInstanceSelectionAnchorIdentifier AGAIN, for a Ctrl-DESELECT specifically:
    //     ApplyManualInstanceSelectionClick's own Ctrl branch sets the anchor to the clicked id
    //     UNCONDITIONALLY (even when deselecting it), but this closure's old `!bWasShiftGesture` gate
    //     only protected Shift, so a Ctrl-deselect's own correct anchor write still got clobbered by
    //     whatever the canvas's own (differently-computed, necessarily-different-since-the-clicked-id-
    //     is-no-longer-selected) fallback primary happened to be. `bSuppressTabStateResync` generalizes
    //     the same protection to EVERY list-driven change, not just a Shift one: MapCanvas::
    //     SyncManualMarkerSelection (the new production landing point for a list click, MapCanvas_UI.h)
    //     always fires this callback with it `true`, since the list has ALREADY correctly written both
    //     fields for EVERY modifier kind (ApplyManualInstanceSelectionClick), one call earlier in the
    //     SAME click — there is no modifier kind for which THIS closure's own re-derivation from the
    //     canvas's echo is ever the right thing to do on that path.
    // A canvas-NATIVE gesture (ApplySelectionGesture, no list involved) still passes its own literal
    // bShiftHeld here — STEP232's own documented trade-off (a Shift-held canvas MARQUEE no longer syncs
    // into selectedManualInstanceIdentifiers either) is UNCHANGED by STEP233: SyncManualMarkerSelection
    // is reached ONLY from a list click, never from ApplyMarqueeGesture/ApplyClickGesture, so it neither
    // fixes nor worsens that documented gap — see STEP233's own Explicit out-of-scope for the explicit
    // confirmation.
    canvas.SetSelectionChangedCallback([this](const OverlayInstanceKey_UI& primary,
                                              const OverlayInstanceKeySet_UI& selectedKeys,
                                              bool bSuppressTabStateResync) {
        if (!bSuppressTabStateResync) {
            tabState.markers.selectedManualInstanceIdentifiers.clear();
            for (const OverlayInstanceKey_UI& key : selectedKeys.keys)
                if (key.bValid && key.collection == PlacementCollectionKind_UI::Markers && key.bManual)
                    tabState.markers.selectedManualInstanceIdentifiers.push_back(key.instanceIndex);
        }

        // STEP143 (human's own bug report) — an empty-space click's own synthetic miss-key always
        // constructs with bManual == false, so gating purely on bManual could never route a miss back
        // to the Markers tab's own manual selection — the row stayed highlighted after clicking empty
        // space. Re-derived from the SET's primary here (not the lone callback argument §19.25 read),
        // for exactly the same reason: an invalid primary (miss or explicit clear) always resolves
        // both fields to "nothing selected" together. UNCHANGED by STEP232/STEP233 — always runs,
        // regardless of bSuppressTabStateResync, since it is not the field either ticket's bug lives in.
        const bool bPrimaryIsManualMarker = primary.bValid
            && primary.collection == PlacementCollectionKind_UI::Markers && primary.bManual;
        tabState.markers.selectedManualInstanceIdentifier = bPrimaryIsManualMarker ? primary.instanceIndex : -1;
        // STEP232 fix #2 / STEP233 fix #3, the anchor — see this closure's own header comment above for
        // the full mechanism (both are the SAME clobber class, now both closed by the SAME general gate).
        if (!bSuppressTabStateResync)
            tabState.markers.manualInstanceSelectionAnchorIdentifier = bPrimaryIsManualMarker ? primary.instanceIndex : -1;

        if (!primary.bValid) {
            lastSelectedEntityIdentifier = Data::EntityIdBuffer::emptySentinel;
        } else if (!primary.bManual) {
            lastSelectedEntityIdentifier = static_cast<std::uint32_t>(primary.instanceIndex);
        }
        // A valid MANUAL primary touches neither `lastSelectedEntityIdentifier` — it stays whatever
        // the last procedural selection left it, exactly §19.25's own original behavior.
    });
    // STEP48: picking reads the resolved markers and PIPELINE's spatial index over them, in world
    // space, instead of the baked entity-id buffer — see MapCanvas_UI.h's header comment.
    canvas.SetPreviewComposite(&composite);
    canvas.SetMarkerPickingSource(&assembler.Placements().markers, &assembler.MarkerSpatialGrid());
    canvas.SetMarkerPickRadiusScreenPixels(settings.markerIconRadiusPixels);
    // ARCH §21.2/§21.6 — the marquee's procedural region-query source: the SAME resolved
    // Data::PlacementResults SetOverlayPlacementSource below already injects (via overlayPlacements),
    // paired with PIPELINE's own SpatialGridSet (STEP166) instead of just the Markers-only grid above.
    canvas.SetSpatialGridSetSource(&assembler.SpatialGridSet());
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
    // STEP113 — the active-panel gate; see MapCanvas_UI.h's SetActivePanelSource. Points at the
    // shell's own live tabState.activePanel — one source of truth, never a second copy (same
    // posture as every other canvas.Set*Source call in this function).
    canvas.SetActivePanelSource(&tabState.activePanel);
    // STEP94 — the manual-marker drag-and-follow source; see MapCanvas_UI.h's
    // SetManualMarkerDragSource. `markers`/`markerLayers` are the SAME vectors the Markers tab
    // edits (recipe.markers/recipe.markerLayers) — one source of truth, never a second copy.
    canvas.SetManualMarkerDragSource(&recipe.markers, &recipe.markerLayers, &recipe.geometry, &recipe);
    // ARCH §21.2/§21.7 — mirrors SetManualMarkerDragSource exactly, one tier over: Props/Decals'
    // first drag-and-follow source of any kind. `props`/`propLayers`/`decals`/`decalLayers` are the
    // SAME vectors the Props/Decals tabs edit — one source of truth, never a second copy.
    canvas.SetManualPropDragSource(&recipe.props, &recipe.propLayers, &recipe.geometry, &recipe);
    canvas.SetManualDecalDragSource(&recipe.decals, &recipe.decalLayers, &recipe.geometry, &recipe);
    // ARCH §21.8 / §14.17 item 9 / STEP212 — the Area canvas gesture's drag source: `recipe.areas`,
    // `composite.Settings().areaColors` and `tabState.areas.areaLocks` are the SAME storage the
    // tab/composite already own — one source of truth, never a second copy. `areaLocks` replaces
    // the retired `&tabState.areas.bAreasLocked` (STEP212 — per-area lock); it stays TAB-owned,
    // unlike `areaColors`, because lock has no composite-side reader (AreaLockTable_UI.h's own
    // ruling). ARCH §14.18 item 23-D/G — the drag-suppression mechanism's old fifth argument (a
    // composite-side transient index) is retired along with the composite-side field and setter
    // parameter it fed.
    canvas.SetManualAreaDragSource(&recipe.areas, &composite.Settings().areaColors,
                                   &tabState.areas.areaLocks, &tabState.areas.selectedAreaIndex);
    // STEP231 — SetManualMarkerSelectionSource is retired; MapCanvas now reads its own
    // selectedInstanceKeys directly for the roster/dot pass's highlight (see MapCanvas_MarkerDrag_UI.cpp
    // and MapCanvas_UI.h's own retirement comments). tabState.markers.selectedManualInstanceIdentifier
    // itself is UNCHANGED and still live — it still drives the Markers-tab LIST's own row highlight
    // (DrawManualInstanceRow), which is a separate concern this ticket does not touch.
    // STEP133 — the per-Type Hide/Unhide preview filter source; see MapCanvas_UI.h's
    // SetMarkerTypeVisibilitySource. Points at the SAME MarkersTabState field the Markers tab's own
    // Type-section Hide/Unhide buttons write (tabState.markers.markerTypeVisibility) — one source of
    // truth, never a second copy.
    canvas.SetMarkerTypeVisibilitySource(&tabState.markers.markerTypeVisibility);
    // ARCH §19.25, item 5 — the OTHER half of the two-way sync: a Markers-tab instance-list click
    // resolves through this closure into the canvas's own real selection (item 3's SetSelection),
    // mirroring SetManualMarkerSelectionSource's own injection pattern exactly, just in the opposite
    // direction (tab -> canvas instead of canvas -> tab).
    // STEP205 — forwards the row click's real Ctrl/Shift state into MapCanvas's own modifier-aware
    // overload instead of always defaulting to Replace.
    // STEP233 — retargeted from SelectManualMarkerByInstanceIdentifier (which re-derived Toggle/Union/
    // Replace against the canvas's OWN, independently-touched copy of the set — the root cause) to
    // SyncManualMarkerSelection (which instead REPLACES the canvas's own manual-marker subset with the
    // list's own already-resolved full selection, verbatim). See MapCanvas_UI.h's own header comment on
    // SyncManualMarkerSelection for the full contract.
    selectManualMarkerInstanceCallback = [this](int clickedInstanceIdentifier,
                                                const std::vector<int>& selectedInstanceIdentifiers) {
        canvas.SyncManualMarkerSelection(selectedInstanceIdentifiers, clickedInstanceIdentifier);
    };
    // STEP132 (ARCH §19.27) — the procedural sibling, same shell-mediated pattern, opposite direction
    // of nothing new: the Rule row's own instance-list click resolves through this into the canvas's
    // own real selection, exactly like the manual closure above. STEP205 — same modifier forwarding.
    selectProceduralMarkerInstanceCallback = [this](int arrayPosition, bool bCtrlHeld, bool bShiftHeld) {
        canvas.SelectProceduralMarkerInstanceByArrayPosition(arrayPosition, bCtrlHeld, bShiftHeld);
    };
    // ARCH §14.17 item 11 — the drag-suppression recomposite request: the canvas asks, PIPELINE
    // decides the tier. Mirrors the left column's own "mutate PreviewCompositeSettings then
    // NotifyParametersChanged()" precedent (Application_LeftColumn_UI.cpp) for a presentation-only
    // edit — exactly the derive-the-tier call a presentation-only mutation already makes elsewhere.
    canvas.SetAreaCompositeRefreshCallback([this] { previewDriver.NotifyParametersChanged(); });
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
