// MapCanvas_MarkerHitTest_UI.cpp — HitTestManualMarkers, now a one-line wrapper over the generic
// HitTestManualInstances<Params::MarkerInstanceGroup> (ARCH §21.3). Binds an always-false-locked
// predicate, preserving this function's own pre-§21.5 bare behavior for its one remaining call site
// (TryBeginManualMarkerDrag's press-time hit-test, MapCanvas_MarkerDrag_UI.cpp) — the real,
// lock-gated predicate is bound at §21.2's new click/marquee call sites instead, both legal
// instantiations of the one template, never two copies of the algorithm.
#include "MapCanvas_MarkerDrag_UI.h"
#include "ManualInstanceHitTest_UI.h"

namespace SanmapGen {
namespace Ui {

bool HitTestManualMarkers(const std::vector<Params::MarkerInstanceGroup>& markers,
                          const PreviewComposite& composite, const MapCanvasView& view,
                          float regionLocalX, float regionLocalY, float pickRadiusScreenPixels,
                          int& outGroupIndex, int& outTransformIndex) {
    static const std::function<bool(int)> kAlwaysUnlocked = [](int) { return false; };
    return HitTestManualInstances<Params::MarkerInstanceGroup>(
        markers, composite, view, regionLocalX, regionLocalY, pickRadiusScreenPixels,
        kAlwaysUnlocked, outGroupIndex, outTransformIndex);
}

} // namespace Ui
} // namespace SanmapGen
