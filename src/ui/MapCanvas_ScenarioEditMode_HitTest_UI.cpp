// MapCanvas_ScenarioEditMode_HitTest_UI.cpp — the linear screen-rect hit test + the world<->screen
// projection helpers MapCanvas_ScenarioEditMode_Interaction_UI.cpp and this module's draw pass both
// need. Layer: UI. Pure/imgui-free/headless-testable. Reuses STEP47's own projection API exactly as
// MapCanvas::ApplyClick (MapCanvas_UI.cpp) and MapCanvas_IconLayer_CullEmit_UI.cpp's own
// AppendCandidate already compose it — mirrored, not reinvented.
#include "MapCanvas_ScenarioEditMode_InteractionInternal_UI.h"
#include "MapCanvasView_UI.h"
#include "PreviewComposite_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

RegionLocalPoint ProjectCandidateToRegionLocal(const ScenarioEditMarkerCandidate_UI& candidate,
                                               const PreviewComposite& composite, const MapCanvasView& view) {
    const PreviewComposite::PreviewPixelPoint previewPixel =
        composite.WorldToPreviewPixel(candidate.worldX, candidate.worldZ);
    return view.ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
}

} // namespace

float ScenarioEditModeDistanceSquared(float ax, float ay, float bx, float by) {
    const float deltaX = ax - bx, deltaY = ay - by;
    return deltaX * deltaX + deltaY * deltaY;
}

int HitTestScenarioEditModeCandidates(const std::vector<ScenarioEditMarkerCandidate_UI>& candidates,
                                      const PreviewComposite& composite, const MapCanvasView& view,
                                      float regionLocalX, float regionLocalY) {
    const float radiusSquared = kScenarioEditModeHitRadiusScreenPixels * kScenarioEditModeHitRadiusScreenPixels;
    int bestIndex = kScenarioEditModeNoIndex;
    float bestDistanceSquared = radiusSquared;
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        const RegionLocalPoint screenPoint = ProjectCandidateToRegionLocal(candidates[index], composite, view);
        const float distanceSquared = ScenarioEditModeDistanceSquared(
            screenPoint.regionLocalX, screenPoint.regionLocalY, regionLocalX, regionLocalY);
        if (distanceSquared <= bestDistanceSquared) { bestDistanceSquared = distanceSquared; bestIndex = static_cast<int>(index); }
    }
    return bestIndex;
}

float ResolveScenarioEditModeWorldXUnderCursor(const PreviewComposite& composite, const MapCanvasView& view,
                                               float regionLocalX, float regionLocalY, float& outWorldZ) {
    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite.PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));
    outWorldZ = worldPoint.worldZ;
    return worldPoint.worldX;
}

} // namespace Ui
} // namespace SanmapGen
