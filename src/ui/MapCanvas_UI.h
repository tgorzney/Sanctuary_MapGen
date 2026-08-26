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
#include "MapCanvasView_UI.h"
#include "MapCanvas_IconLayer_Ops_UI.h"
#include "MapCanvas_ScenarioEditMode_Ops_UI.h"
#include "MarkerDragGesture_UI.h"
#include "OverlayLayer_Settings_UI.h"
#include "PreviewComposite_UI.h"
#include "../data/EntityIdBuffer_DATA.h"
#include "../data/PlacementInstances_DATA.h"
#include "../data/SpatialGrid_DATA.h"
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
    // The constant on-screen radius a click must land within to hit a marker icon (Constitution
    // §8 — a named setting, not a literal). Matches Phase 3's icon draw radius; the two must agree
    // or a click can miss a visibly-hit icon.
    void SetMarkerPickRadiusScreenPixels(float radius) { pickRadiusScreenPixels = radius; }
    void SetSelectionChangedCallback(std::function<void(std::uint32_t)> selectionChanged) {
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

    // One imgui frame; `regionSidePixels` is the square viewport side in screen pixels.
    void Draw(const char* canvasIdentifier, float regionSidePixels);   // MapCanvas_Draw_UI.cpp

    // The three gestures, as pure state transitions — Draw() calls exactly these.
    std::uint32_t ApplyClick(float regionLocalX, float regionLocalY);
    void ApplyDrag(float deltaRegionPixelsX, float deltaRegionPixelsY);
    void ApplyScroll(float regionLocalX, float regionLocalY, float wheelSteps);

    MapCanvasView& View() { return view; }
    const MapCanvasView& View() const { return view; }

    // Presentation state of a viewport: what the user last selected. `emptySentinel` = nothing.
    std::uint32_t SelectedEntityIdentifier() const { return selectedEntityIdentifier; }
    bool HasSelection() const {
        return selectedEntityIdentifier != Data::EntityIdBuffer::emptySentinel;
    }
    const PreviewPixelCoordinate& LastPickedPixel() const { return lastPickedPixel; }
    // The toolkit identifier the image draw uses; zero when nothing has been composited yet.
    unsigned long long PresentationIdentifier() const;

private:
    void SetSelection(std::uint32_t entityIdentifier);
    // Translates the imgui pointer state over the region into the gestures (MapCanvas_Draw_UI.cpp).
    void ApplyPointerInput(float regionOriginX, float regionOriginY);
    // STEP53 — assembles this frame's DrawOverlayIconLayersInput from the injected sources above
    // and calls DrawOverlayIconLayers (MapCanvas_Draw_UI.cpp).
    void DrawOverlayIconLayerPass(float regionOriginX, float regionOriginY, float regionSidePixels);
    // STEP78 — draws Scenario Edit Mode's overlay when active, from the SAME overlay* sources
    // above (MapCanvas_Draw_UI.cpp).
    void DrawScenarioEditModeOverlayPass(float regionOriginX, float regionOriginY);
    // STEP94 — Gap 6's minimal stopgap draw + the live gesture's ghost/refused-tint dots
    // (MapCanvas_MarkerDrag_UI.cpp).
    void DrawManualMarkerDragPass(float regionOriginX, float regionOriginY);
    // STEP94 — the drag gesture's three lifecycle calls (MapCanvas_MarkerDrag_UI.cpp), tried at
    // press-time before the existing pan-vs-click disambiguation (MapCanvas_Draw_UI.cpp).
    bool TryBeginManualMarkerDrag(float regionLocalX, float regionLocalY);
    void ContinueManualMarkerDrag(float regionLocalX, float regionLocalY);
    void EndManualMarkerDrag();

    MapCanvasView view;
    Sys::GpuResourceManager*        gpuResourceManager = nullptr;
    Sys::GpuTextureHandle           previewTexture;
    const PreviewComposite*         composite = nullptr;
    const Data::PlacementInstances* pickMarkerInstances = nullptr;
    const Data::SpatialGrid*        pickMarkerSpatialGrid = nullptr;
    float                           pickRadiusScreenPixels = 8.0f;   // Constitution §8; wired from
                                                                       // ApplicationSettings::markerIconRadiusPixels
    std::function<void(std::uint32_t)>    selectionChangedCallback;
    PreviewPixelCoordinate lastPickedPixel;
    std::uint32_t selectedEntityIdentifier = Data::EntityIdBuffer::emptySentinel;

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
    MarkerDragGestureState manualMarkerDragState;
    bool                   bManualMarkerDragActive = false;   // this press started on a manual marker
};

} // namespace Ui
} // namespace SanmapGen
