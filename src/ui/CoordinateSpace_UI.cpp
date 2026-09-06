// CoordinateSpace_UI.cpp — the two conversions that need PreviewComposite's full type
// (CoordinateSpace_UI.h's own header comment explains why these are out-of-line rather than
// inline), plus the two screen<->grid compositions built on top of them.
#include "CoordinateSpace_UI.h"
#include "PreviewComposite_UI.h"

namespace SanmapGen {
namespace Ui {

// Recomputes MapCanvasView::ResolvePreviewPixel's own region-local -> image-pixel math from that
// class's already-public accessors (ViewCenterPixelX/Y, VisibleSpanPixels, RegionSidePixels) but
// WITHOUT the final integer floor — the one change that fixes the "large increment" drag bug (see
// CoordinateSpace_UI.h's header comment). Degenerate view state (regionSidePixels not yet set)
// answers (0, 0), mirroring ResolvePreviewPixel's own early-return-zeroed contract.
WorldPoint ScreenToWorld(const MapCanvasView& view, const PreviewComposite& composite, ScreenPoint screen) {
    const float regionSidePixels = view.RegionSidePixels();
    if (regionSidePixels <= 0.0f) return WorldPoint{};
    const float span = view.VisibleSpanPixels();
    const float regionReciprocal = 1.0f / regionSidePixels;
    const float imagePointX = view.ViewCenterPixelX() + (screen.screenX * regionReciprocal - 0.5f) * span;
    const float imagePointY = view.ViewCenterPixelY() + (screen.screenY * regionReciprocal - 0.5f) * span;
    const PreviewComposite::PreviewWorldPoint worldPoint = composite.PreviewPixelToWorld(imagePointX, imagePointY);
    return WorldPoint{worldPoint.worldX, worldPoint.worldZ};
}

ScreenPoint WorldToScreen(const MapCanvasView& view, const PreviewComposite& composite, WorldPoint world) {
    const PreviewComposite::PreviewPixelPoint pixel = composite.WorldToPreviewPixel(world.worldX, world.worldZ);
    const RegionLocalPoint regionLocal = view.ProjectPreviewPixelToRegionLocal(pixel.pixelX, pixel.pixelY);
    return ScreenPoint{regionLocal.regionLocalX, regionLocal.regionLocalY};
}

GridPoint ScreenToGrid(const MapCanvasView& view, const PreviewComposite& composite,
                       float cellSizeWorldUnits, ScreenPoint screen) {
    return WorldToGrid(cellSizeWorldUnits, ScreenToWorld(view, composite, screen));
}

ScreenPoint GridToScreen(const MapCanvasView& view, const PreviewComposite& composite,
                         float cellSizeWorldUnits, GridPoint grid) {
    return WorldToScreen(view, composite, GridToWorld(cellSizeWorldUnits, grid));
}

} // namespace Ui
} // namespace SanmapGen
