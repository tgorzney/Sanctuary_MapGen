// MapCanvas_IconLayer_Cache_UI_Test.cpp — acceptance test, part 3: §4's C2 cache invalidation
// decision, headless. One translation unit of the MapCanvas_IconLayer_UI_Test binary. Only the
// pure invalidation/accumulation half (MapCanvas_IconLayer_Cache_UI.cpp) is exercised here — the
// imgui-typed replay bridge is MapCanvas_IconLayer_Draw_UI_Test.cpp's job (a live frame).
#include "MapCanvas_IconLayer_TestFixture_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// A brand-new cache (bValid == false) always invalidates.
void CheckFreshCacheAlwaysInvalidates() {
    IconLayerFrameCache cache;
    check(ShouldInvalidateIconLayerCache(cache, 0.0f, 0.0f, 1.0f, OverlayInstanceKeySet_UI{}, 0),
          "a fresh, never-built cache always invalidates");
}

// Once built, replaying the identical keys never invalidates.
void CheckUnchangedKeysDoNotInvalidate() {
    IconLayerFrameCache cache;
    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, OverlayInstanceKeySet_UI{}, 5);
    check(!ShouldInvalidateIconLayerCache(cache, 10.0f, 20.0f, 2.0f, OverlayInstanceKeySet_UI{}, 5),
          "identical view/selection/revision keys never force a rebuild");
}

// Each of pan / zoom / selection-change / layer-setting-change independently forces a rebuild.
void CheckEachTriggerIndependentlyInvalidates() {
    IconLayerFrameCache cache;
    const OverlayInstanceKeySet_UI noSelection;
    OverlayInstanceKeySet_UI markerSelection;
    markerSelection.keys.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 3, true});

    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, noSelection, 5);
    check(ShouldInvalidateIconLayerCache(cache, 99.0f, 20.0f, 2.0f, noSelection, 5), "pan invalidates");

    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, noSelection, 5);
    check(ShouldInvalidateIconLayerCache(cache, 10.0f, 20.0f, 3.0f, noSelection, 5), "zoom invalidates");

    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, noSelection, 5);
    check(ShouldInvalidateIconLayerCache(cache, 10.0f, 20.0f, 2.0f, markerSelection, 5),
          "a selection change invalidates");

    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, noSelection, 5);
    check(ShouldInvalidateIconLayerCache(cache, 10.0f, 20.0f, 2.0f, noSelection, 6),
          "a layer-settings revision change invalidates");
}

// Begin clears and re-stamps every key; the accumulated bytes are exactly what was appended.
void CheckBuildAccumulatesRawBytes() {
    IconLayerFrameCache cache;
    BeginIconLayerCacheBuild(cache, 1.0f, 2.0f, 3.0f, OverlayInstanceKeySet_UI{}, 7);
    check(cache.bValid && cache.cachedVertexBytes.empty() && cache.cachedIndexBytes.empty(),
          "Begin leaves a valid, empty cache ready to accumulate");
    const unsigned char vertexBytes[4] = {1, 2, 3, 4};
    const unsigned char indexBytes[2]  = {5, 6};
    AppendCachedVertexBytes(cache, vertexBytes, sizeof(vertexBytes));
    AppendCachedIndexBytes(cache, indexBytes, sizeof(indexBytes));
    check(cache.cachedVertexBytes.size() == 4 && cache.cachedIndexBytes.size() == 2,
          "appended byte counts match exactly");
    check(cache.cachedVertexBytes[2] == 3 && cache.cachedIndexBytes[1] == 6,
          "appended bytes are copied verbatim");
}

// STEP229 — a set-changing edit that doesn't touch the primary (e.g. Ctrl-click removing a
// non-primary member) must still invalidate; the old single-key OverlayInstanceKeysEqual comparison
// this replaced could not see this kind of change at all.
void CheckNonPrimarySelectionMemberRemovalInvalidates() {
    IconLayerFrameCache cache;
    OverlayInstanceKeySet_UI twoKeys;
    twoKeys.keys.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 1, true});
    twoKeys.keys.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 2, true});   // primary
    OverlayInstanceKeySet_UI primaryOnly;
    primaryOnly.keys.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 2, true});

    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, twoKeys, 5);
    check(ShouldInvalidateIconLayerCache(cache, 10.0f, 20.0f, 2.0f, primaryOnly, 5),
          "STEP229 - removing a NON-PRIMARY selected member still invalidates the cache, even though "
          "the primary (last element) is unchanged");
}

} // namespace

void RunMapCanvasIconLayerCacheChecks() {
    CheckFreshCacheAlwaysInvalidates();
    CheckUnchangedKeysDoNotInvalidate();
    CheckEachTriggerIndependentlyInvalidates();
    CheckBuildAccumulatesRawBytes();
    CheckNonPrimarySelectionMemberRemovalInvalidates();
}

} // namespace Ui
} // namespace SanmapGen
