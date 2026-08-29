// MapCanvas_UI.h — the map viewport. Layer: UI. Accuracy class: Visual.
// It does exactly three things: DRAW the preview composite's GL texture (M5-5's repoint of
// PreviewComposite_UI) in an imgui image region, PAN/ZOOM that view, and route a click through
// STEP47's inverse projection + `Picking_UI::PickMarker` (M4-4) to the marker under the cursor,
// resolved in WORLD space against `Data::SpatialGrid` — never against a baked, texel-space id
// buffer (STEP48; the texel-space coupling was the whole reason ARCH_14_PreviewOverlayLayering.md
// §14 exists). `Data::EntityIdBuffer`/`Picking_UI::PickEntity` still exist (a separate, larger
// retirement, out of STEP48's scope) but this canvas no longer reads them.
//
// It never simulates and never spawns (ARCH §3.2, §5.3). The ~720-line `Widget_MapCanvas` it
// replaces owned unit-grid spawning, symmetry spawn, army creation and triangle height
// interpolation; all of that is Placement/PROC reached through PIPELINE, and this widget causes
// no work to happen at all — it is draw + pick + pan/zoom only. Nothing here
// includes a `_PROC` header, and it holds no GL handle: the texture stays behind
// `Sys::GpuResourceManager` and the canvas only carries its opaque presentation identifier.
//
// The pan/zoom math is `MapCanvasView` — pure, imgui-free, so the cursor -> preview-pixel
// contract a click depends on is testable without a UI frame. `Draw` is a thin translator from
// imgui input to the three gesture methods below (MapCanvas_Draw_UI.cpp). MapCanvas also depends
// on `PreviewComposite_UI` (STEP48; ratified) for the composite's world<->preview-pixel mapping —
// both are UI layer, so this is an intra-layer edge, not a layer-graph violation (ARCH §3.1). The
// alternative (Application re-deriving and re-pushing that mapping) would keep a second copy of
// state that must stay in sync with the composite's own live baked state, exactly the drift
// `Data::SpatialGrid`'s header warns a second copy of world->cell arithmetic invites.
#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include "Application_Panels_UI.h"
#include "DecalDragGesture_UI.h"
#include "ManualInstanceHitTest_UI.h"
#include "MapCanvasView_UI.h"
#include "MapCanvas_IconLayer_Ops_UI.h"
#include "MapCanvas_ManualDragSources_UI.h"
#include "MapCanvas_ScenarioEditMode_Ops_UI.h"
#include "MapCanvas_SelectionSet_UI.h"
#include "MarkerDragGesture_UI.h"
#include "OverlayLayer_Settings_UI.h"
#include "PreviewComposite_UI.h"
#include "PropDragGesture_UI.h"
#include "../data/EntityIdBuffer_DATA.h"
#include "../data/PlacementInstances_DATA.h"
#include "../data/SpatialGrid_DATA.h"
#include "../data/SpatialGridSet_DATA.h"
#include "../io/WorldFootprintSizeTable_IO.h"
#include "../sys/GpuResource_SYS.h"

namespace SanmapGen {
namespace Ui {

class MapCanvas {
public:
    // What is displayed: the composited image texture, owned by SYS, sized `previewResolution`
    // squared. Re-pointing it at a different resolution resets the view.
    void SetPreviewTexture(Sys::GpuResourceManager* manager, Sys::GpuTextureHandle texture,
                           int previewResolution);
    // What is displayed, and the single source of the world<->preview-pixel mapping a click's
    // inverse projection reuses (STEP47's `PreviewPixelToWorld`) — read-only, the canvas never
    // composites. First MapCanvas_UI dependency on PreviewComposite_UI; see the header comment.
    void SetPreviewComposite(const PreviewComposite* previewComposite) { composite = previewComposite; }
    // What a click is resolved against: the resolved markers and the spatial index over them,
    // both read-only, both owned by PIPELINE (Generation_PIPELINE, M4-5). Replaces
    // SetEntityIdentifierBuffer — see STEP48.
    void SetMarkerPickingSource(const Data::PlacementInstances* markerInstances,
                                const Data::SpatialGrid* markerSpatialGrid) {
        pickMarkerInstances = markerInstances;
        pickMarkerSpatialGrid = markerSpatialGrid;
    }
    // ARCH §21.2/§21.6 — the marquee release's PROCEDURAL half: one region query per collection
    // (Markers/Props/Decals; Units out of scope, §21's own closing note) against the SAME
    // `Data::SpatialGridSet` PIPELINE already builds (STEP166). The instances themselves are the
    // SAME `overlayPlacements` this canvas already reads for the icon draw pass (SetOverlayPlacementSource,
    // below) — never a second copy of that pointer.
    void SetSpatialGridSetSource(const Data::SpatialGridSet* grids) { pickSpatialGridSet = grids; }
    // The constant on-screen radius a click must land within to hit a marker icon (Constitution
    // §8 — a named setting, not a literal). Matches Phase 3's icon draw radius; the two must agree
    // or a click can miss a visibly-hit icon.
    void SetMarkerPickRadiusScreenPixels(float radius) { pickRadiusScreenPixels = radius; }
    // ARCH §19.25 — widened from `void(std::uint32_t)` to carry the full key: a manual selection's
    // `instanceIndex` is a MarkerTransform::instanceIdentifier, not an entity id, and only the full
    // key (with `bManual`) lets Application::WireCallbacks() tell the two cases apart.
    // ARCH §21.1 — widened again to also carry the full multi-select set: `primary` is
    // `PrimaryOfSelectionSet(selectedKeys)`, restated as its own argument so every existing
    // primary-only caller (every one before this ticket) keeps compiling by simply ignoring the
    // second parameter, never re-deriving the primary itself.
    void SetSelectionChangedCallback(
        std::function<void(const OverlayInstanceKey_UI& primary, const OverlayInstanceKeySet_UI& selectedKeys)>
            selectionChanged) {
        selectionChangedCallback = std::move(selectionChanged);
    }

    // STEP53 — the screen-space overlay icon draw pass's read-only sources, every one a push-in
    // pointer (STEP48's ARCH-ruled pattern, ARCH_14_09_RenderingPerformance.md §14.9; never an
    // Application reach-back — see this work-order's §0 correction).
    void SetOverlayLayerSettings(const OverlayLayerSettings* settings) { overlayLayerSettings = settings; }
    void SetOverlayRenderingSettings(const OverlayRenderingSettings* settings) { overlayRenderingSettings = settings; }
    void SetOverlayPlacementSource(const Data::PlacementResults* placements,
                                   const Data::RuleBucketIndexSet* ruleBucketIndex) {
        overlayPlacements = placements;
        overlayRuleBucketIndex = ruleBucketIndex;
    }
    void SetOverlayRecipe(const Params::MapRecipe* recipe) { overlayRecipe = recipe; }
    void SetIconAtlasSource(const IconAtlasPairingLookup* pairingLookup, const IconAtlasManifest* atlasManifest) {
        overlayPairingLookup = pairingLookup;
        overlayAtlasManifest = atlasManifest;
    }
    // Mirrors SetPreviewComposite/SetMarkerPickingSource exactly (§0 above) — STEP58's placeholder
    // table, sourced from Application::WorldFootprintSizeTable(), never reached directly.
    void SetWorldFootprintSizeTable(const Io::WorldFootprintSizeTable* table) { worldFootprintSizeTable = table; }

    // STEP78 — while IsActive(), ApplyPointerInput (MapCanvas_Draw_UI.cpp) takes EXCLUSIVE
    // interaction ownership: drag/click route to Scenario Edit Mode, not the normal pan/pick path.
    void SetScenarioEditModeState(ScenarioEditModeState* state) { scenarioEditModeState = state; }

    // STEP113 — the active-panel gate: a manual-marker drag may only BEGIN while the Markers panel
    // is the shell's active tab. Mirrors SetScenarioEditModeState exactly (same class of injected,
    // caller-owned, read-every-frame pointer; see this header's own comment on why a second copy of
    // this state is never made).
    void SetActivePanelSource(const ApplicationPanel* panel) { activePanelSource = panel; }

    // STEP94 — the manual-marker drag-and-follow source: `markers` is the ONLY mutable pointer
    // here (a drag writes straight into `recipe.markers`, Constitution §1 — UI sets PARAMS, never
    // simulates); everything else is read-only. Mirrors SetPreviewComposite/SetMarkerPickingSource's
    // own injected-pointer shape exactly (MapCanvas_UI.h header comment's "second copy" ruling).
    void SetManualMarkerDragSource(std::vector<Params::MarkerInstanceGroup>* markers,
                                   const std::vector<Params::MarkerInstanceLayer>* markerLayers,
                                   const Params::Geometry* geometry,
                                   const Params::MapRecipe* recipeForGlobalSymmetry) {
        manualMarkerDragMarkers  = markers;
        manualMarkerDragLayers   = markerLayers;
        manualMarkerDragGeometry = geometry;
        manualMarkerDragRecipe   = recipeForGlobalSymmetry;
    }
    // ARCH §21.7 — mirrors SetManualMarkerDragSource's exact shape, one tier over: Props/Decals'
    // first drag-and-follow source of any kind (ARCH §21.2). `props`/`decals` are the only mutable
    // pointers, same "UI sets PARAMS, never simulates" posture as the Marker sibling.
    void SetManualPropDragSource(std::vector<Params::PropInstanceGroup>* props,
                                 const std::vector<Params::PropInstanceLayer>* propLayers,
                                 const Params::Geometry* geometry,
                                 const Params::MapRecipe* recipeForGlobalSymmetry) {
        manualPropDrag.props    = props;
        manualPropDrag.layers   = propLayers;
        manualPropDrag.geometry = geometry;
        manualPropDrag.recipe   = recipeForGlobalSymmetry;
    }
    void SetManualDecalDragSource(std::vector<Params::DecalInstanceGroup>* decals,
                                  const std::vector<Params::DecalInstanceLayer>* decalLayers,
                                  const Params::Geometry* geometry,
                                  const Params::MapRecipe* recipeForGlobalSymmetry) {
        manualDecalDrag.decals   = decals;
        manualDecalDrag.layers   = decalLayers;
        manualDecalDrag.geometry = geometry;
        manualDecalDrag.recipe   = recipeForGlobalSymmetry;
    }

    // STEP126 — the static selection-highlight source: `selectedInstanceIdentifier` is the SAME
    // address as MarkersTabState::selectedManualInstanceIdentifier (Application_UI.cpp) — one source
    // of truth, never a second copy. A single scalar pointer, the simplest form of this file's own
    // established null-safe-injection shape (ARCH §19.19 — closer to SetActivePanelSource's
    // one-pointer form than SetManualMarkerDragSource's bundle). Null (no shell has wired a selection
    // source) refuses — the highlight computation treats null identically to "-1: nothing selected,"
    // never defaulting to "everything selected."
    void SetManualMarkerSelectionSource(const int* selectedInstanceIdentifier) {
        manualMarkerSelectedInstanceIdentifier = selectedInstanceIdentifier;
    }

    // STEP133 — the Markers tab's per-Type Hide/Unhide preview filter source. Mirrors
    // SetManualMarkerSelectionSource's exact injected-pointer shape (this header's own established
    // pattern): a single, caller-owned, read-every-frame pointer, null-safe (null = no filtering,
    // today's exact behavior). Points at the SAME MarkersTabState field the Markers tab's own
    // Hide/Unhide buttons write (tabState.markers.markerTypeVisibility) — one source of truth, never
    // a second copy.
    void SetMarkerTypeVisibilitySource(const MarkerTypeVisibility_UI* visibility) {
        markerTypeVisibilitySource = visibility;
    }

    // One imgui frame; `regionSidePixels` is the square viewport side in screen pixels.
    void Draw(const char* canvasIdentifier, float regionSidePixels);   // MapCanvas_Draw_UI.cpp

    // The three gestures, as pure state transitions — Draw() calls exactly these.
    std::uint32_t ApplyClick(float regionLocalX, float regionLocalY);
    void ApplyDrag(float deltaRegionPixelsX, float deltaRegionPixelsY);
    void ApplyScroll(float regionLocalX, float regionLocalY, float wheelSteps);

    // ARCH §19.25, item 5 — the shell-mediated list-click-to-canvas path: a Markers-tab instance-list
    // Selectable click resolves through Application's own `selectManualMarkerInstanceCallback` (bound
    // to this method in WireCallbacks(), mirroring SetManualMarkerSelectionSource's existing
    // injection pattern) so the SAME real icon-sprite render path a canvas click drives
    // (MapCanvas_IconLayer_CullEmit_UI.cpp's `instance.bSelected`) also lights up for a list click —
    // never a second, parallel highlight mechanism. A negative `instanceIdentifier` clears the
    // selection (mirrors MarkersTabState::selectedManualInstanceIdentifier's own `-1` sentinel).
    // STEP205 — gains `bCtrlHeld`/`bShiftHeld` (default false, byte-identical Replace for every
    // existing caller): resolves through `ApplySelectionGesture` directly, not the modifier-blind
    // `SetSelection` wrapper, so a Ctrl/Shift-held list click joins/ranges into the canvas's real
    // multi-select set instead of unconditionally replacing it (the "clobber" bug §21.1 deferred).
    void SelectManualMarkerByInstanceIdentifier(int instanceIdentifier, bool bCtrlHeld = false,
                                                bool bShiftHeld = false);   // MapCanvas_UI.cpp

    // STEP132 (ARCH §19.27) — the procedural sibling of SelectManualMarkerByInstanceIdentifier above:
    // a Markers-tab PROCEDURAL instance-list click resolves through the SAME canonical SetSelection,
    // `bManual=false` and `arrayPosition` (a raw Data::PlacementInstances SoA index) as the key — the
    // exact representation §19.25 already establishes for canvas click-pick, because it IS the same
    // array. A negative `arrayPosition` clears the selection, mirroring the manual sibling's own
    // sentinel handling. Built once, not a second divergent selection path (§19.27's own instruction).
    // STEP205 — gains `bCtrlHeld`/`bShiftHeld`, same shape/defaults as the manual sibling above.
    void SelectProceduralMarkerInstanceByArrayPosition(int arrayPosition, bool bCtrlHeld = false,
                                                       bool bShiftHeld = false);   // MapCanvas_UI.cpp

    MapCanvasView& View() { return view; }
    const MapCanvasView& View() const { return view; }

    // Presentation state of a viewport: what the user last selected. `emptySentinel` = nothing.
    // ARCH §19.25 — both stay thin reads of the widened `selectedInstanceKey` for existing
    // procedural-only callers (Application_Draw_UI.cpp): `.instanceIndex`/`.bValid` respectively.
    // ARCH §21.1 — now thin reads of the multi-select set's own PRIMARY (computed on demand,
    // authoring-scale set, no caching needed — same posture PrimaryOfSelectionSet's own header
    // comment states).
    std::uint32_t SelectedEntityIdentifier() const {
        return static_cast<std::uint32_t>(PrimaryOfSelectionSet(selectedInstanceKeys).instanceIndex);
    }
    bool HasSelection() const { return PrimaryOfSelectionSet(selectedInstanceKeys).bValid; }
    // ARCH §21.1 — the full ordered multi-select set, for any caller that needs more than the
    // primary (e.g. Application::WireCallbacks()'s own partition into tabState.markers.
    // selectedManualInstanceIdentifiers).
    const OverlayInstanceKeySet_UI& SelectedInstanceKeys() const { return selectedInstanceKeys; }
    const PreviewPixelCoordinate& LastPickedPixel() const { return lastPickedPixel; }
    // The toolkit identifier the image draw uses; zero when nothing has been composited yet.
    unsigned long long PresentationIdentifier() const;

private:
    // ARCH §21.1 — the canonical entry point, widened from §19.25's `SetSelection`: every
    // selection-setting path (canvas click-pick, a Markers-tab list click for a manual instance)
    // resolves through one of these two overloads, never a second divergent one. Both resolve to
    // exactly one of Replace/Toggle/Union (Ctrl wins if both are somehow held), update
    // `selectedInstanceKeys`, and fire the widened callback ONLY when the set actually changed.
    // Defined in MapCanvas_UI.cpp.
    void ApplySelectionGesture(const OverlayInstanceKey_UI& touchedKey, bool bCtrlHeld, bool bShiftHeld);
    void ApplySelectionGesture(const std::vector<OverlayInstanceKey_UI>& touchedKeys, bool bCtrlHeld,
                               bool bShiftHeld);
    // The pre-§21.1 canonical full-key setter, now a thin wrapper: every existing single-target call
    // site (ApplyClick's procedural/manual branches, SelectManualMarkerByInstanceIdentifier,
    // SelectProceduralMarkerInstanceByArrayPosition) keeps calling this unqualified name with
    // `bCtrlHeld=false, bShiftHeld=false` — an unconditional Replace, byte-identical to the old
    // SetSelection's own behavior.
    void SetSelection(const OverlayInstanceKey_UI& key) { ApplySelectionGesture(key, false, false); }
    // The pre-existing procedural overload, now a thin wrapper over the canonical one above — every
    // existing procedural call site (ApplyClick's PickMarker branch) compiles unchanged.
    // `entityIdentifier != emptySentinel` is `bValid`; `bManual` is always false here (a procedural
    // array-position key never claims to be a manual instanceIdentifier key).
    void SetSelection(std::uint32_t entityIdentifier) {
        SetSelection(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers,
                                           static_cast<std::int32_t>(entityIdentifier),
                                           entityIdentifier != Data::EntityIdBuffer::emptySentinel,
                                           /*bManual=*/false});
    }
    // Translates the imgui pointer state over the region into the gestures (MapCanvas_Draw_UI.cpp).
    void ApplyPointerInput(float regionOriginX, float regionOriginY);
    // STEP53 — assembles this frame's DrawOverlayIconLayersInput from the injected sources above
    // and calls DrawOverlayIconLayers (MapCanvas_Draw_UI.cpp).
    void DrawOverlayIconLayerPass(float regionOriginX, float regionOriginY, float regionSidePixels);
    // STEP78 — draws Scenario Edit Mode's overlay when active, from the SAME overlay* sources
    // above (MapCanvas_Draw_UI.cpp).
    void DrawScenarioEditModeOverlayPass(float regionOriginX, float regionOriginY);
    // STEP94 — Gap 6's minimal stopgap draw + the live gesture's ghost/refused-tint dots
    // (MapCanvas_MarkerDrag_UI.cpp). Props/Decals need no equivalent — their manual instances
    // already render every frame through the real icon-atlas overlay pass (DrawOverlayIconLayerPass,
    // ARCH §20's ResolvePropsManual/ResolveDecalsManual), which re-reads the SAME live `recipe.props`/
    // `recipe.decals` a drag writes into, so a Prop/Decal drag needs no stopgap draw of its own.
    void DrawManualMarkerDragPass(float regionOriginX, float regionOriginY);
    // STEP207 — the marquee's own visual feedback: MapCanvas::Draw never drew a rubber-band
    // rectangle for a left-drag box-select, so the (already-correct, ARCH §21.2/§21.6) selection
    // applied silently at mouse-up. Guard: `bPressActive` true AND none of the three
    // `b*ManualDragActive` flags (a drag gesture, once active, owns the whole press exclusively —
    // never show a marquee box mid-drag). Collection-agnostic by construction — keyed only on the
    // press state, never per Markers/Props/Decals (MapCanvas_Draw_UI.cpp).
    void DrawMarqueeRectanglePass(float regionOriginX, float regionOriginY);
    // ARCH §21.2/§21.3 — the drag gesture's three lifecycle calls, generalized across Markers/Props/
    // Decals (renamed from TryBeginManualMarkerDrag; MapCanvas_ManualDragDispatch_UI.cpp), tried at
    // press-time before the click/marquee disambiguation (MapCanvas_Draw_UI.cpp). `TryBeginManualInstanceDrag`
    // is the shared 3-way nearest-hit-wins dispatcher (§21.3's own "hand-written, NOT templated" ruling
    // — it touches three concrete `Params::` group types by name in one function body); `Continue`/`End`
    // dispatch to whichever ONE domain's own `b*ManualDragActive` flag is set.
    bool TryBeginManualInstanceDrag(float regionLocalX, float regionLocalY);
    void ContinueManualInstanceDrag(float regionLocalX, float regionLocalY);
    void EndManualInstanceDrag();
    // The shared cross-domain hit-test both `TryBeginManualInstanceDrag` (press-time) and
    // `ApplyClickGesture` (release-time click resolution — "re-runs the hit-test from scratch," §21.2's
    // own explicitly-granted implementation freedom) call. NOT active-panel-gated itself (a click may
    // select any domain's manual instance regardless of which tab is open, preserving §19.25's existing
    // ungated manual-marker click-select behavior) — `TryBeginManualInstanceDrag` applies the
    // per-domain active-panel gate itself, AFTER this returns its nearest hit, refusing only the DRAG.
    bool HitTestManualInstanceAcrossDomains(float regionLocalX, float regionLocalY,
                                            PlacementCollectionKind_UI& outCollection,
                                            int& outGroupIndex, int& outTransformIndex) const;
    // Resolves a (collection, groupIndex, transformIndex) triple — from either
    // HitTestManualInstanceAcrossDomains or a CollectManualInstancesInWorldRegion pair — into the
    // real selection key, reading that transform's own `instanceIdentifier` (ARCH §21.4) from the
    // matching domain's own vector. Shared by ApplyClickGesture and ApplyMarqueeGesture so the key
    // construction rule lives in exactly one place (MapCanvas_SelectionGesture_UI.cpp).
    OverlayInstanceKey_UI ResolveManualHitKey(PlacementCollectionKind_UI collection, int groupIndex,
                                              int transformIndex) const;
    // ARCH §21.2 — the modifier-aware click resolution `ApplyClick` now wraps (MapCanvas_SelectionGesture_UI.cpp):
    // a manual hit (any of the three domains, via the shared cross-domain hit-test above) wins first;
    // a miss falls through to the existing procedural Markers-only PickMarker path (§21's own "today
    // only markers have a working picker" note — Props/Decals gain no procedural single-click picker
    // here, only §21.6's region query below, for marquee).
    void ApplyClickGesture(float regionLocalX, float regionLocalY, bool bCtrlHeld, bool bShiftHeld);
    // ARCH §21.2/§21.6 — the release-time marquee resolver: one world-space AABB (from the press-start
    // and release-time region-local points) feeds both `Picking_UI::PickInstancesInRegion` (procedural,
    // Markers/Props/Decals, against `pickSpatialGridSet`) and `CollectManualInstancesInWorldRegion`
    // (manual, Markers/Props/Decals, each lock-gated) — every resulting key concatenated into one
    // ordered list and resolved through `ApplySelectionGesture`'s batch overload.
    void ApplyMarqueeGesture(float pressRegionLocalX, float pressRegionLocalY, float releaseRegionLocalX,
                             float releaseRegionLocalY, bool bCtrlHeld, bool bShiftHeld);

    MapCanvasView view;
    Sys::GpuResourceManager*        gpuResourceManager = nullptr;
    Sys::GpuTextureHandle           previewTexture;
    const PreviewComposite*         composite = nullptr;
    const Data::PlacementInstances* pickMarkerInstances = nullptr;
    const Data::SpatialGrid*        pickMarkerSpatialGrid = nullptr;
    // ARCH §21.2/§21.6 — the marquee's procedural region-query source (SetSpatialGridSetSource);
    // `overlayPlacements` below already carries the matching instances, one source of truth.
    const Data::SpatialGridSet*     pickSpatialGridSet = nullptr;
    float                           pickRadiusScreenPixels = 8.0f;   // Constitution §8; wired from
                                                                       // ApplicationSettings::markerIconRadiusPixels
    std::function<void(const OverlayInstanceKey_UI& primary, const OverlayInstanceKeySet_UI& selectedKeys)>
        selectionChangedCallback;
    PreviewPixelCoordinate lastPickedPixel;
    // ARCH §19.25 — replaces the old bare `std::uint32_t selectedEntityIdentifier`; default-
    // constructed (`instanceIndex = -1, bValid = false, bManual = false`) is exactly "nothing
    // selected", the same state `emptySentinel` represented before.
    // ARCH §21.1 — widened from a single key to the ordered multi-select set; an empty `keys` vector
    // is exactly "nothing selected" (PrimaryOfSelectionSet answers the same default invalid key).
    OverlayInstanceKeySet_UI selectedInstanceKeys;

    // STEP53 — overlay icon draw pass sources (read-only, injected) and its own per-canvas state.
    const OverlayLayerSettings*         overlayLayerSettings    = nullptr;
    const OverlayRenderingSettings*     overlayRenderingSettings = nullptr;
    const Data::PlacementResults*       overlayPlacements        = nullptr;
    const Data::RuleBucketIndexSet*     overlayRuleBucketIndex   = nullptr;
    const Params::MapRecipe*            overlayRecipe            = nullptr;
    const IconAtlasPairingLookup*       overlayPairingLookup     = nullptr;
    const IconAtlasManifest*            overlayAtlasManifest     = nullptr;
    const Io::WorldFootprintSizeTable*  worldFootprintSizeTable  = nullptr;
    IconLayerAabbCache_UI overlayLayerAabbCache;
    IconLayerFrameCache   overlayIconLayerFrameCache;
    float         pressTravelPixels        = 0.0f;    // how far the current press has dragged
    bool          bPressActive             = false;
    // ARCH §21.2 — the press-start region-local point, captured the SAME frame `pressTravelPixels`
    // resets to 0 (IsItemActivated()); paired with the release-time point to form the marquee's one
    // world-space AABB. Mirrors `pressTravelPixels`'s own naming convention (§21.2's own instruction).
    float         pressStartRegionLocalX   = 0.0f;
    float         pressStartRegionLocalY   = 0.0f;
    // ARCH §21.2 — the independent right-button pan tracker, replacing today's left-drag-pans
    // entirely. Keyed off raw `io.MouseDown[ImGuiMouseButton_Right]` (imgui's item-activation is
    // left-button-only for a default-flags InvisibleButton) — begins on a hovered right-click,
    // persists across every frame the button stays down independent of hover (mirroring how
    // IsItemActive() persists for the left button once a press began), ends on release.
    bool          bRightPressActive        = false;

    // STEP78 — Scenario Edit Mode's own state (Application-owned, injected).
    ScenarioEditModeState* scenarioEditModeState = nullptr;

    // STEP113 — mirrors scenarioEditModeState exactly: injected, caller-owned, read every frame.
    const ApplicationPanel* activePanelSource = nullptr;

    // STEP94 — the manual-marker drag-and-follow source (injected, see SetManualMarkerDragSource)
    // and this canvas's own live gesture state.
    std::vector<Params::MarkerInstanceGroup>*       manualMarkerDragMarkers  = nullptr;
    const std::vector<Params::MarkerInstanceLayer>* manualMarkerDragLayers   = nullptr;
    const Params::Geometry*                         manualMarkerDragGeometry = nullptr;
    const Params::MapRecipe*                        manualMarkerDragRecipe   = nullptr;
    // STEP126 — the static selection-highlight source (injected, see SetManualMarkerSelectionSource).
    const int*                                      manualMarkerSelectedInstanceIdentifier = nullptr;
    // STEP133 — the per-Type Hide/Unhide preview filter source (injected, see
    // SetMarkerTypeVisibilitySource).
    const MarkerTypeVisibility_UI*                  markerTypeVisibilitySource = nullptr;
    MarkerDragGestureState manualMarkerDragState;
    bool                   bManualMarkerDragActive = false;   // this press started on a manual marker
    // ARCH §21.2/§21.7 — Props/Decals' own drag sources + live gesture state, grouped into two small
    // structs (MapCanvas_ManualDragSources_UI.h) rather than eight more scattered fields here (this
    // file's own flagged file-size ceiling, §21.7). Exactly one of the three `b*ManualDragActive`
    // flags is ever true at once — `TryBeginManualInstanceDrag`'s own nearest-hit-wins dispatch
    // never begins more than one domain's gesture for the same press.
    ManualPropDragSources_UI  manualPropDrag;
    ManualDecalDragSources_UI manualDecalDrag;
    bool                       bManualPropDragActive  = false;
    bool                       bManualDecalDragActive = false;
};

} // namespace Ui
} // namespace SanmapGen
