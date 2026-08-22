// MapCanvas_IconLayer_UI.h — the pure per-instance/per-bucket value types the screen-space overlay
// icon draw pass (ARCH_14_09_RenderingPerformance.md §14.9, STEP53) shares across its whole split
// (Constitution §1.5): MapCanvas_IconLayer_Cull_UI.cpp (culling + LOD), _Budget_UI.cpp (cross-layer
// decimation), _Draw_UI.cpp (atlas-page bucketing + the bulk ImDrawList write), _Cache_UI.cpp (the
// C2 interaction-scoped redraw cache). The module's entry point / orchestration input / diagnostics
// hooks live in the sibling MapCanvas_IconLayer_Ops_UI.h — split out to stay inside Constitution
// §1.5's file-size ceiling. Layer: UI. Accuracy class: Visual. No imgui, no GL.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Ui {

// Which Data::PlacementResults collection an overlay candidate's world position/template came
// from — NOT the same axis as Ui::OverlayDomainKind_UI (§14.6: domainKind is not DATA-bucket
// identity; Alloy AND SpawnsArmies both resolve to Markers here).
enum class PlacementCollectionKind_UI { Markers, Props, Units, Decals };

// Today only markers have a working picker (STEP48) — see MapCanvas_IconLayer_Cache_UI.cpp's
// header comment. Generic on `collection` so Props/Units/Decals need no cache rework once they
// get their own pickers later.
struct OverlayInstanceKey_UI {
    PlacementCollectionKind_UI collection = PlacementCollectionKind_UI::Markers;
    std::int32_t instanceIndex = -1;
    bool bValid = false;
};
inline bool OverlayInstanceKeysEqual(const OverlayInstanceKey_UI& a, const OverlayInstanceKey_UI& b) {
    return a.bValid == b.bValid
        && (!a.bValid || (a.collection == b.collection && a.instanceIndex == b.instanceIndex));
}

enum class IconLayerLodMode_UI { Thumbnail, Strategic };

// One candidate icon quad, already fully resolved (screen position, size, atlas UV, tint) — the
// single record both the budget pass and the atlas-bucketed flush consume.
struct OverlayVisibleInstance {
    float screenCenterX = 0.0f, screenCenterY = 0.0f;
    float screenSize     = 0.0f;
    float uvMinimumX = 0.0f, uvMinimumY = 0.0f, uvMaximumX = 1.0f, uvMaximumY = 1.0f;
    int   atlasPage         = 0;
    std::uint64_t textureIdentifier = 0;
    float tintAlpha = 1.0f;           // layer.opacity, folded in once here (§14.2)
    int   layerIndex = 0;             // vector order = Z order = decimation priority (§14.7/§14.9)
    int   stableOrder = 0;            // monotonic append order this frame — decimation tie-break
    OverlayInstanceKey_UI instanceKey;
    bool  bSelected = false;          // never clustered/capped away (§14.9's C2 contract)
};

// One resolved atlas page's worth of quads, flushed as exactly one ImDrawList command.
struct AtlasPageBucket {
    int atlasPage = 0;
    std::uint64_t textureIdentifier = 0;
    std::vector<OverlayVisibleInstance> quads;
};

// Constitution §8 — named tweakables, never literals. REASONED-PLACEHOLDER (Constitution §7):
// visibleInstanceBudget is not yet benchmarked; superseded by Phase 3.3's measured figure.
struct OverlayRenderingSettings {
    int visibleInstanceBudget       = 450000;   // 400k-500k range, §14.9
    int screenCellClusterSizePixels = 8;        // primary decimation mechanism
};

// A per-layer cached world AABB (UI-owned; see MapCanvas_IconLayer_Cull_UI.cpp's header comment
// for why this lives here rather than in STEP50's DATA-layer index).
struct LayerWorldAabb_UI {
    float lowWorldX = 0.0f, lowWorldZ = 0.0f, highWorldX = 0.0f, highWorldZ = 0.0f;
    bool  bValid = false;
};
struct IconLayerAabbCache_UI {
    std::vector<LayerWorldAabb_UI> perLayerAabb;
    std::uint64_t cachedForRevision = ~static_cast<std::uint64_t>(0);   // never matches revision 0 unset
};

// page id + quad count only (not full OverlayVisibleInstance data — that already lives in the raw
// vertex/index bytes below); enough for the replay loop to know where each ImDrawCmd's boundary is.
struct CachedIconLayerBucketLayout_UI {
    int atlasPage = 0;
    std::uint64_t textureIdentifier = 0;
    int quadCount = 0;
};

// The C2 "interaction-scoped redraw" cache primitive (§14.8) — CPU bytes, not a GPU texture/FBO.
struct IconLayerFrameCache {
    std::vector<unsigned char>   cachedVertexBytes;    // raw ImDrawVert bytes, non-selected only
    std::vector<unsigned char>   cachedIndexBytes;      // ImDrawIdx values, LOCAL to each bucket
    std::vector<CachedIconLayerBucketLayout_UI> cachedBucketLayout;   // for replay
    bool bValid = false;
    float cachedViewCenterPixelX = 0.0f, cachedViewCenterPixelY = 0.0f, cachedZoomScale = 0.0f;
    OverlayInstanceKey_UI cachedSelectionKey;
    std::uint64_t cachedLayerSettingsRevision = 0;
};

} // namespace Ui
} // namespace SanmapGen
