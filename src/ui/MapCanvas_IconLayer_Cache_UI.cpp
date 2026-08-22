// MapCanvas_IconLayer_Cache_UI.cpp — §4's C2 "interaction-scoped redraw" cache primitive: the
// invalidation decision and raw-byte accumulation only. Layer: UI. Pure, imgui-free,
// headless-testable — MapCanvas_IconLayer_Draw_UI.cpp is what actually knows ImDrawVert's layout
// and drives Begin/Append/replay around a live ImDrawList; this file never touches imgui.
//
// This ticket builds the CACHING PRIMITIVE only, not a marker drag/edit UX (`BRIEF_MarkersTabUI_R2.md`
// territory, explicit out-of-scope) — no gesture-start/gesture-end hook exists yet to call Begin
// from; MapCanvas_IconLayer_Draw_UI.cpp calls it every time ShouldInvalidateIconLayerCache says so,
// which today only fires on pan/zoom/selection/layer-setting change (§14.8), never mid-gesture.
#include "MapCanvas_IconLayer_Ops_UI.h"
#include <cstring>

namespace SanmapGen {
namespace Ui {

namespace {
bool NearlyEqual(float a, float b) { const float d = a - b; return d < 0.0001f && d > -0.0001f; }
} // namespace

// §4's four invalidation triggers, restated: pan (view center), zoom (zoomScale), selection
// change, or any overlay layer-setting change (the monotonic revision counter). LOD
// threshold-crossing needs no separate rule — it only ever happens as zoom changes, which already
// invalidates unconditionally.
bool ShouldInvalidateIconLayerCache(const IconLayerFrameCache& cache, float viewCenterPixelX,
                                    float viewCenterPixelY, float zoomScale,
                                    const OverlayInstanceKey_UI& selection, std::uint64_t layerSettingsRevision) {
    if (!cache.bValid) return true;
    if (!NearlyEqual(cache.cachedViewCenterPixelX, viewCenterPixelX)
        || !NearlyEqual(cache.cachedViewCenterPixelY, viewCenterPixelY)
        || !NearlyEqual(cache.cachedZoomScale, zoomScale))
        return true;
    if (!OverlayInstanceKeysEqual(cache.cachedSelectionKey, selection)) return true;
    return cache.cachedLayerSettingsRevision != layerSettingsRevision;
}

void BeginIconLayerCacheBuild(IconLayerFrameCache& cache, float viewCenterPixelX, float viewCenterPixelY,
                              float zoomScale, const OverlayInstanceKey_UI& selection,
                              std::uint64_t layerSettingsRevision) {
    cache.cachedVertexBytes.clear();
    cache.cachedIndexBytes.clear();
    cache.cachedBucketLayout.clear();
    cache.cachedViewCenterPixelX = viewCenterPixelX;
    cache.cachedViewCenterPixelY = viewCenterPixelY;
    cache.cachedZoomScale = zoomScale;
    cache.cachedSelectionKey = selection;
    cache.cachedLayerSettingsRevision = layerSettingsRevision;
    cache.bValid = true;
}

void AppendCachedVertexBytes(IconLayerFrameCache& cache, const void* data, std::size_t byteCount) {
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    cache.cachedVertexBytes.insert(cache.cachedVertexBytes.end(), bytes, bytes + byteCount);
}

void AppendCachedIndexBytes(IconLayerFrameCache& cache, const void* data, std::size_t byteCount) {
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    cache.cachedIndexBytes.insert(cache.cachedIndexBytes.end(), bytes, bytes + byteCount);
}

} // namespace Ui
} // namespace SanmapGen
