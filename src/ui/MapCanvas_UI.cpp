// MapCanvas_UI.cpp — the canvas's state and its three gestures, with no imgui in sight (the
// imgui frame is MapCanvas_Draw_UI.cpp, behind the same header). Layer: UI.
// Every gesture is a pure transition on `MapCanvasView` plus, for a click, STEP47's inverse
// projection (region-local -> preview pixel -> world) composed with one `Picking_UI::PickMarker`
// lookup against `Data::SpatialGrid` (STEP48). The canvas re-implements no picking of its own and
// tests no placement rule — a pick walks exactly one chunk's bucket in O(1)
// (UI_FRAMEWORK_SPEC §4), never a scan of 100k instances.
#include "MapCanvas_UI.h"
#include "MapCanvas_MarkerDrag_UI.h"
#include "Picking_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cmath>

namespace SanmapGen {
namespace Ui {

void MapCanvas::SetPreviewTexture(Sys::GpuResourceManager* manager, Sys::GpuTextureHandle texture,
                                  int previewResolution) {
    gpuResourceManager = manager;
    previewTexture = texture;
    if (view.PreviewResolution() != previewResolution) view.SetPreviewResolution(previewResolution);
}

// The manager owns the texture; the canvas only carries the opaque value the toolkit draws with,
// so no GL handle lives in the UI layer (ARCH §3.2).
unsigned long long MapCanvas::PresentationIdentifier() const {
    if (gpuResourceManager == nullptr || !previewTexture.IsValid()) return 0ull;
    return gpuResourceManager->TexturePresentationIdentifier(previewTexture);
}

// A click resolves in WORLD space: the region-local cursor becomes a preview pixel (MapCanvasView,
// unchanged), the preview pixel becomes a world point (STEP47's PreviewComposite::PreviewPixelToWorld,
// the exact inverse of the mapping BuildEntityPoints bakes marker marks through), and PickMarker
// hit-tests the ONE SpatialGrid chunk that world point falls in. Off the image, no source wired, or
// no composite baked yet — selects nothing.
std::uint32_t MapCanvas::ApplyClick(float regionLocalX, float regionLocalY) {
    lastPickedPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    if (!lastPickedPixel.bInsideImage || pickMarkerInstances == nullptr
        || pickMarkerSpatialGrid == nullptr || composite == nullptr
        || composite->PixelsPerPreviewCell() <= 0.0f) {
        SetSelection(Data::EntityIdBuffer::emptySentinel);
        return SelectedEntityIdentifier();
    }
    const PreviewComposite::PreviewWorldPoint worldPoint =
        composite->PreviewPixelToWorld(static_cast<float>(lastPickedPixel.pixelX),
                                       static_cast<float>(lastPickedPixel.pixelY));
    // Screen pixels -> preview pixels (view.PreviewPixelsPerRegionPixel()) -> world units, so the
    // pick radius stays a constant ON-SCREEN size at every zoom level (the texel-space coupling
    // this migration exists to remove).
    const float pickRadiusWorldUnits = pickRadiusScreenPixels
        * view.PreviewPixelsPerRegionPixel()
        * composite->Settings().worldUnitsPerCell / composite->PixelsPerPreviewCell();
    const std::int32_t pickedIndex = PickMarker(*pickMarkerSpatialGrid, *pickMarkerInstances,
                                                worldPoint.worldX, worldPoint.worldZ,
                                                pickRadiusWorldUnits);
    if (pickedIndex != kNoMarkerPicked) {
        SetSelection(static_cast<std::uint32_t>(pickedIndex));
        return SelectedEntityIdentifier();
    }
    // ARCH §19.25, item 3 — a procedural miss tries a manual hit next: the SAME linear,
    // authoring-scale hit-test TryBeginManualMarkerDrag already uses (MapCanvas_MarkerHitTest_UI.cpp
    // — "no grid needed" for a manual roster at authoring scale), reused rather than re-derived, so a
    // canvas click can select a manual marker for the first time.
    if (manualMarkerDragMarkers != nullptr) {
        int hitGroupIndex = -1, hitTransformIndex = -1;
        if (HitTestManualMarkers(*manualMarkerDragMarkers, *composite, view, regionLocalX, regionLocalY,
                                 pickRadiusScreenPixels, hitGroupIndex, hitTransformIndex)) {
            const Params::MarkerTransform& hitTransform =
                (*manualMarkerDragMarkers)[static_cast<std::size_t>(hitGroupIndex)]
                    .transforms[static_cast<std::size_t>(hitTransformIndex)];
            SetSelection(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers,
                                               hitTransform.instanceIdentifier, true, /*bManual=*/true});
            return SelectedEntityIdentifier();
        }
    }
    SetSelection(Data::EntityIdBuffer::emptySentinel);   // both pickers missed: nothing selected
    return SelectedEntityIdentifier();
}

// ARCH §19.25, item 5 — the shell-mediated list-click-to-canvas path's landing point. A negative
// `instanceIdentifier` (the tab's own "-1 = nothing selected" sentinel) clears the selection instead
// of claiming a nonsensical manual key, mirroring the binding edge case §19.25 states for every
// manual-marker key: `instanceIdentifier < 0` is never a legal selection target.
void MapCanvas::SelectManualMarkerByInstanceIdentifier(int instanceIdentifier) {
    SetSelection(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, instanceIdentifier,
                                       instanceIdentifier >= 0, /*bManual=*/true});
}

// STEP132 (ARCH §19.27) — the procedural sibling: routes through the SAME canonical SetSelection
// above, `bManual=false` (a procedural array position is never a manual instanceIdentifier).
void MapCanvas::SelectProceduralMarkerInstanceByArrayPosition(int arrayPosition) {
    SetSelection(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, arrayPosition,
                                       arrayPosition >= 0, /*bManual=*/false});
}

void MapCanvas::ApplyDrag(float deltaRegionPixelsX, float deltaRegionPixelsY) {
    view.PanByRegionPixels(deltaRegionPixelsX, deltaRegionPixelsY);
}

// The wheel is multiplicative so a step feels the same at every zoom level; the step factor is a
// setting, never a literal (Constitution §8).
void MapCanvas::ApplyScroll(float regionLocalX, float regionLocalY, float wheelSteps) {
    if (wheelSteps == 0.0f) return;
    const float zoomStepScale = std::pow(view.settings.zoomStepFactor, wheelSteps);
    view.ZoomAtRegionPoint(regionLocalX, regionLocalY, zoomStepScale);
}

// ARCH §19.25 — the canonical full-key setter every selection-setting path resolves through
// (ApplyClick's procedural/manual branches above, SelectManualMarkerByInstanceIdentifier). The old
// `std::uint32_t` overload is now the thin wrapper declared inline in MapCanvas_UI.h.
void MapCanvas::SetSelection(const OverlayInstanceKey_UI& key) {
    if (OverlayInstanceKeysEqual(key, selectedInstanceKey)) return;
    selectedInstanceKey = key;
    if (selectionChangedCallback) selectionChangedCallback(selectedInstanceKey);
}

} // namespace Ui
} // namespace SanmapGen
