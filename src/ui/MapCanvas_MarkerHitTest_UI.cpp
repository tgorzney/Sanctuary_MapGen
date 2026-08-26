// MapCanvas_MarkerHitTest_UI.cpp — HitTestManualMarkers, split out of MapCanvas_MarkerDrag_UI.cpp
// (STEP126) once that file's own line count — already 207, over ARCH §1.5's 150-line hard ceiling
// before this ticket — would have crossed further with this ticket's own tint-priority/highlight
// additions. Moved verbatim; no logic change. Declared in MapCanvas_MarkerDrag_UI.h, unchanged.
#include "MapCanvas_MarkerDrag_UI.h"
#include "PreviewComposite_UI.h"

namespace SanmapGen {
namespace Ui {

bool HitTestManualMarkers(const std::vector<Params::MarkerInstanceGroup>& markers,
                          const PreviewComposite& composite, const MapCanvasView& view,
                          float regionLocalX, float regionLocalY, float pickRadiusScreenPixels,
                          int& outGroupIndex, int& outTransformIndex) {
    outGroupIndex = -1; outTransformIndex = -1;
    if (composite.PixelsPerPreviewCell() <= 0.0f) return false;
    const float radiusSquared = pickRadiusScreenPixels * pickRadiusScreenPixels;
    float bestDistanceSquared = radiusSquared;
    for (std::size_t groupIndex = 0; groupIndex < markers.size(); ++groupIndex) {
        const std::vector<Params::MarkerTransform>& transforms = markers[groupIndex].transforms;
        for (std::size_t transformIndex = 0; transformIndex < transforms.size(); ++transformIndex) {
            const Params::MarkerTransform& transform = transforms[transformIndex];
            const PreviewComposite::PreviewPixelPoint previewPixel =
                composite.WorldToPreviewPixel(transform.transform.positionX, transform.transform.positionZ);
            const RegionLocalPoint screenPoint =
                view.ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
            const float deltaX = screenPoint.regionLocalX - regionLocalX;
            const float deltaY = screenPoint.regionLocalY - regionLocalY;
            const float distanceSquared = deltaX * deltaX + deltaY * deltaY;
            // Strict `<` once a candidate is already held, mirroring Picking_UI::PickMarker's own
            // tie convention exactly: the FIRST (lowest group, then lowest transform) marker within
            // radius wins a tie, never a later one silently overwriting it. `distanceSquared <=
            // radiusSquared` (not `<`) still admits a marker sitting exactly on the pick radius.
            if (distanceSquared <= radiusSquared
                && (outGroupIndex < 0 || distanceSquared < bestDistanceSquared)) {
                bestDistanceSquared = distanceSquared;
                outGroupIndex = static_cast<int>(groupIndex);
                outTransformIndex = static_cast<int>(transformIndex);
            }
        }
    }
    return outGroupIndex >= 0;
}

} // namespace Ui
} // namespace SanmapGen
