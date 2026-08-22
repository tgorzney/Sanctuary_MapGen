// MapCanvas_UI.cpp — the canvas's state and its three gestures, with no imgui in sight (the
// imgui frame is MapCanvas_Draw_UI.cpp, behind the same header). Layer: UI.
// Every gesture is a pure transition on `MapCanvasView` plus, for a click, STEP47's inverse
// projection (region-local -> preview pixel -> world) composed with one `Picking_UI::PickMarker`
// lookup against `Data::SpatialGrid` (STEP48). The canvas re-implements no picking of its own and
// tests no placement rule — a pick walks exactly one chunk's bucket in O(1)
// (UI_FRAMEWORK_SPEC §4), never a scan of 100k instances.
#include "MapCanvas_UI.h"
#include "Picking_UI.h"
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
        return selectedEntityIdentifier;
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
    SetSelection(static_cast<std::uint32_t>(pickedIndex));   // kNoMarkerPicked(-1) == emptySentinel
    return selectedEntityIdentifier;
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

void MapCanvas::SetSelection(std::uint32_t entityIdentifier) {
    if (entityIdentifier == selectedEntityIdentifier) return;
    selectedEntityIdentifier = entityIdentifier;
    if (selectionChangedCallback) selectionChangedCallback(entityIdentifier);
}

} // namespace Ui
} // namespace SanmapGen
