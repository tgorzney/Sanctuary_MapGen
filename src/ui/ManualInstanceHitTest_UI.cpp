// ManualInstanceHitTest_UI.cpp — see ManualInstanceHitTest_UI.h for the full rationale. Explicit
// instantiation for the three concrete manual-instance group types below is what lets this stay a
// normal, separately-compiled translation unit rather than header-only (ARCH §21.3's closed,
// already-known three-domain set — Units are out of scope).
#include "ManualInstanceHitTest_UI.h"
#include "PreviewComposite_UI.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/PropInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

template<typename GroupT>
bool HitTestManualInstances(const std::vector<GroupT>& instances, const PreviewComposite& composite,
                            const MapCanvasView& view, float regionLocalX, float regionLocalY,
                            float pickRadiusScreenPixels,
                            const std::function<bool(const typename GroupT::TransformType&)>& isInstanceLocked,
                            int& outGroupIndex, int& outTransformIndex, float* outDistanceSquared) {
    outGroupIndex = -1; outTransformIndex = -1;
    if (outDistanceSquared != nullptr) *outDistanceSquared = 0.0f;
    if (composite.PixelsPerPreviewCell() <= 0.0f) return false;
    const float radiusSquared = pickRadiusScreenPixels * pickRadiusScreenPixels;
    float bestDistanceSquared = radiusSquared;
    for (std::size_t groupIndex = 0; groupIndex < instances.size(); ++groupIndex) {
        const auto& transforms = instances[groupIndex].transforms;
        for (std::size_t transformIndex = 0; transformIndex < transforms.size(); ++transformIndex) {
            const auto& transform = transforms[transformIndex];
            if (isInstanceLocked && isInstanceLocked(transform)) continue;   // ARCH §21.5/§21.9
            const PreviewComposite::PreviewPixelPoint previewPixel =
                composite.WorldToPreviewPixel(transform.transform.positionX, transform.transform.positionZ);
            const RegionLocalPoint screenPoint =
                view.ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
            const float deltaX = screenPoint.regionLocalX - regionLocalX;
            const float deltaY = screenPoint.regionLocalY - regionLocalY;
            const float distanceSquared = deltaX * deltaX + deltaY * deltaY;
            // Strict `<` once a candidate is already held, mirroring Picking_UI::PickMarker's own
            // tie convention exactly: the FIRST (lowest group, then lowest transform) instance within
            // radius wins a tie, never a later one silently overwriting it. `distanceSquared <=
            // radiusSquared` (not `<`) still admits an instance sitting exactly on the pick radius.
            if (distanceSquared <= radiusSquared
                && (outGroupIndex < 0 || distanceSquared < bestDistanceSquared)) {
                bestDistanceSquared = distanceSquared;
                outGroupIndex = static_cast<int>(groupIndex);
                outTransformIndex = static_cast<int>(transformIndex);
                if (outDistanceSquared != nullptr) *outDistanceSquared = distanceSquared;
            }
        }
    }
    return outGroupIndex >= 0;
}

template<typename GroupT>
void CollectManualInstancesInWorldRegion(const std::vector<GroupT>& instances,
                                         float worldMinX, float worldMinZ, float worldMaxX, float worldMaxZ,
                                         const std::function<bool(const typename GroupT::TransformType&)>& isInstanceLocked,
                                         std::vector<std::pair<int, int>>& outGroupTransformPairs) {
    if (!(worldMaxX >= worldMinX) || !(worldMaxZ >= worldMinZ)) return;   // degenerate/NaN box: nothing
    for (std::size_t groupIndex = 0; groupIndex < instances.size(); ++groupIndex) {
        const auto& transforms = instances[groupIndex].transforms;
        for (std::size_t transformIndex = 0; transformIndex < transforms.size(); ++transformIndex) {
            const auto& transform = transforms[transformIndex];
            if (isInstanceLocked && isInstanceLocked(transform)) continue;   // ARCH §21.5/§21.9
            const float positionX = transform.transform.positionX;
            const float positionZ = transform.transform.positionZ;
            if (positionX < worldMinX || positionX > worldMaxX
                || positionZ < worldMinZ || positionZ > worldMaxZ)
                continue;
            outGroupTransformPairs.emplace_back(static_cast<int>(groupIndex), static_cast<int>(transformIndex));
        }
    }
}

template bool HitTestManualInstances<Params::MarkerInstanceGroup>(
    const std::vector<Params::MarkerInstanceGroup>&, const PreviewComposite&, const MapCanvasView&,
    float, float, float, const std::function<bool(const Params::MarkerTransform&)>&, int&, int&, float*);
template bool HitTestManualInstances<Params::PropInstanceGroup>(
    const std::vector<Params::PropInstanceGroup>&, const PreviewComposite&, const MapCanvasView&,
    float, float, float, const std::function<bool(const Params::PropTransform&)>&, int&, int&, float*);
template bool HitTestManualInstances<Params::DecalInstanceGroup>(
    const std::vector<Params::DecalInstanceGroup>&, const PreviewComposite&, const MapCanvasView&,
    float, float, float, const std::function<bool(const Params::DecalTransform&)>&, int&, int&, float*);

template void CollectManualInstancesInWorldRegion<Params::MarkerInstanceGroup>(
    const std::vector<Params::MarkerInstanceGroup>&, float, float, float, float,
    const std::function<bool(const Params::MarkerTransform&)>&, std::vector<std::pair<int, int>>&);
template void CollectManualInstancesInWorldRegion<Params::PropInstanceGroup>(
    const std::vector<Params::PropInstanceGroup>&, float, float, float, float,
    const std::function<bool(const Params::PropTransform&)>&, std::vector<std::pair<int, int>>&);
template void CollectManualInstancesInWorldRegion<Params::DecalInstanceGroup>(
    const std::vector<Params::DecalInstanceGroup>&, float, float, float, float,
    const std::function<bool(const Params::DecalTransform&)>&, std::vector<std::pair<int, int>>&);

} // namespace Ui
} // namespace SanmapGen
