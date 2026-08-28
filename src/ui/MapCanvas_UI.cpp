// MapCanvas_UI.cpp — the canvas's state and its three gestures, with no imgui in sight (the
// imgui frame is MapCanvas_Draw_UI.cpp, behind the same header). Layer: UI.
// Every gesture is a pure transition on `MapCanvasView` plus, for a click, STEP47's inverse
// projection (region-local -> preview pixel -> world) composed with one `Picking_UI::PickMarker`
// lookup against `Data::SpatialGrid` (STEP48). The canvas re-implements no picking of its own and
// tests no placement rule — a pick walks exactly one chunk's bucket in O(1)
// (UI_FRAMEWORK_SPEC §4), never a scan of 100k instances.
#include "MapCanvas_UI.h"
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
// ARCH §21.2 — now a thin wrapper over the modifier-aware ApplyClickGesture
// (MapCanvas_SelectionGesture_UI.cpp), bCtrlHeld=bShiftHeld=false: an unconditional Replace, the
// exact behavior this public entry point always had, preserved byte-identically for every existing
// caller/test that never threaded modifier state through it.
std::uint32_t MapCanvas::ApplyClick(float regionLocalX, float regionLocalY) {
    ApplyClickGesture(regionLocalX, regionLocalY, /*bCtrlHeld=*/false, /*bShiftHeld=*/false);
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

// ARCH §21.1 — the canonical entry point every selection-setting path resolves through (ApplyClick's
// procedural/manual branches above, SelectManualMarkerByInstanceIdentifier, and the old §19.25
// `SetSelection`, now the thin single-key/bCtrl=false/bShift=false wrapper declared inline in
// MapCanvas_UI.h). Fires the widened callback only when the set actually changed — mirroring
// §19.25's own SetSelection equal-key short-circuit, extended to the whole ordered set.
void MapCanvas::ApplySelectionGesture(const OverlayInstanceKey_UI& touchedKey, bool bCtrlHeld, bool bShiftHeld) {
    const OverlayInstanceKeySet_UI previous = selectedInstanceKeys;
    if (bCtrlHeld) {
        ToggleInSelectionSet(selectedInstanceKeys, touchedKey);
    } else if (bShiftHeld) {
        UnionIntoSelectionSet(selectedInstanceKeys, {touchedKey});
    } else {
        ReplaceSelectionSet(selectedInstanceKeys, {touchedKey});
    }
    if (SelectionSetsEqual(previous, selectedInstanceKeys)) return;
    if (selectionChangedCallback)
        selectionChangedCallback(PrimaryOfSelectionSet(selectedInstanceKeys), selectedInstanceKeys);
}

// The marquee/list-batch counterpart. `ToggleInSelectionSet` takes a single key only (never a legal
// operation for a batch, per its own header comment), so a batch gesture only ever resolves to
// Replace (no modifier) or Union (Ctrl OR Shift held) — the same "no per-batch toggle mechanism
// exists" reading `ManualInstanceHitTest_UI.h`'s own release-time consumer (ARCH §21.2) depends on.
void MapCanvas::ApplySelectionGesture(const std::vector<OverlayInstanceKey_UI>& touchedKeys, bool bCtrlHeld,
                                      bool bShiftHeld) {
    const OverlayInstanceKeySet_UI previous = selectedInstanceKeys;
    if (bCtrlHeld || bShiftHeld) {
        UnionIntoSelectionSet(selectedInstanceKeys, touchedKeys);
    } else {
        ReplaceSelectionSet(selectedInstanceKeys, touchedKeys);
    }
    if (SelectionSetsEqual(previous, selectedInstanceKeys)) return;
    if (selectionChangedCallback)
        selectionChangedCallback(PrimaryOfSelectionSet(selectedInstanceKeys), selectedInstanceKeys);
}

} // namespace Ui
} // namespace SanmapGen
