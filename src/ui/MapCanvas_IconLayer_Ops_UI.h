// MapCanvas_IconLayer_Ops_UI.h — the screen-space overlay icon draw pass's public entry point,
// its per-frame orchestration input, and the test-observable diagnostics hooks. Split out of
// MapCanvas_IconLayer_UI.h (Constitution §1.5 ceiling); include both together, this one pulls the
// other in. Layer: UI. `struct ImDrawList` is only NAMED here (forward-declared) — this header
// stays imgui-free so MapCanvas_UI.h, which includes it for the setters, never sees imgui.h.
#pragma once
#include "MapCanvas_IconLayer_UI.h"

struct ImDrawList;

namespace SanmapGen {
namespace Data { struct PlacementResults; struct RuleBucketIndexSet; }
namespace Io { class WorldFootprintSizeTable; }
namespace Params { struct MapRecipe; }
namespace Ui {

class PreviewComposite;
class MapCanvasView;
struct OverlayLayerSettings;
class IconAtlasPairingLookup;
struct IconAtlasManifest;
struct MarkerTypeVisibility_UI;   // STEP133 — MarkerTypeVisibility_UI.h; pointer-only here

// Test-observable counters (never read by production code) — how the acceptance tests assert
// "the grid/bucket was never touched" / "logged once, not per instance" / "generated exactly once".
struct IconLayerCullDiagnostics_UI {
    int subLayerWalksIssued = 0;    // bumped once per sub-layer bucket/manual-array actually walked
    // STEP83 §8 — bumped once per Params::PropInstanceGroup visited by the Props/Reclaim Manual
    // walk's group-level bReclaimable predicate; proves the predicate sits at group granularity,
    // never per-transform (a per-instance regression would instead tally the transform count).
    int reclaimGroupPredicateEvaluations = 0;
    std::vector<std::string> loggedMissingTemplateIdentifiers;
    bool HasLoggedMissing(const std::string& templateIdentifier) const {
        for (const std::string& logged : loggedMissingTemplateIdentifiers)
            if (logged == templateIdentifier) return true;
        return false;
    }
    void LogMissingOnce(const std::string& templateIdentifier) {
        if (!HasLoggedMissing(templateIdentifier)) loggedMissingTemplateIdentifiers.push_back(templateIdentifier);
    }
};
struct IconLayerBudgetDiagnostics_UI { int fallbackInvocationCount = 0; };
struct IconLayerGenerationDiagnostics_UI { int fullGenerationCount = 0; };

// Everything one frame's draw needs; MapCanvas assembles this from its own injected pointers
// (push-in setters, STEP48's pattern — never an Application reach-back).
struct DrawOverlayIconLayersInput {
    const OverlayLayerSettings*         overlayLayerSettings = nullptr;
    const OverlayRenderingSettings*     renderingSettings     = nullptr;
    const Data::PlacementResults*       placements            = nullptr;
    const Data::RuleBucketIndexSet*     ruleBucketIndex        = nullptr;
    const Params::MapRecipe*            recipe                 = nullptr;
    const IconAtlasPairingLookup*       pairingLookup          = nullptr;
    const IconAtlasManifest*            atlasManifest          = nullptr;
    const Io::WorldFootprintSizeTable*  footprintSizeTable     = nullptr;
    const PreviewComposite*             composite              = nullptr;
    const MapCanvasView*                view                   = nullptr;
    float regionOriginX = 0.0f, regionOriginY = 0.0f, regionSidePixels = 0.0f;
    OverlayInstanceKey_UI selectedInstanceKey;
    // STEP133 — the Markers tab's per-Type Hide/Unhide preview filter (null = no shell has wired the
    // source, i.e. today's exact unfiltered behavior). `markerTypeVisibilityRevision` is threaded
    // separately (0 when the pointer is null) so the C2 cache's own invalidation key
    // (MapCanvas_IconLayer_Draw_UI.cpp) can combine it without dereferencing the pointer itself.
    const MarkerTypeVisibility_UI*      markerTypeVisibility          = nullptr;
    std::uint64_t                       markerTypeVisibilityRevision  = 0;
};

// §2 — the cross-layer visible-vertex budget + decimation (MapCanvas_IconLayer_Budget_UI.cpp).
// `candidates` is consumed by value (moved from); never mutates Data::PlacementInstances/
// Data::SpatialGrid/any CSR bucket (§14.11 — the binding guardrail).
std::vector<OverlayVisibleInstance> ApplyVisibleInstanceBudget(
    std::vector<OverlayVisibleInstance> candidates, const OverlayRenderingSettings& settings,
    IconLayerBudgetDiagnostics_UI* diagnostics = nullptr);

// §4 — the C2 cache's invalidation decision and its raw-byte accumulation
// (MapCanvas_IconLayer_Cache_UI.cpp). Pure/imgui-free: `cachedVertexBytes`/`cachedIndexBytes` are
// plain `unsigned char` vectors here — MapCanvas_IconLayer_Draw_UI.cpp (the only TU that knows
// ImDrawVert's layout) is what interprets/produces the bytes these two Append* calls accumulate.
bool ShouldInvalidateIconLayerCache(const IconLayerFrameCache& cache, float viewCenterPixelX,
                                    float viewCenterPixelY, float zoomScale,
                                    const OverlayInstanceKey_UI& selection, std::uint64_t layerSettingsRevision);
void BeginIconLayerCacheBuild(IconLayerFrameCache& cache, float viewCenterPixelX, float viewCenterPixelY,
                              float zoomScale, const OverlayInstanceKey_UI& selection,
                              std::uint64_t layerSettingsRevision);
void AppendCachedVertexBytes(IconLayerFrameCache& cache, const void* data, std::size_t byteCount);
void AppendCachedIndexBytes(IconLayerFrameCache& cache, const void* data, std::size_t byteCount);

// The one entry point MapCanvas_Draw_UI.cpp calls, right after ImGui::Image(). Defined in
// MapCanvas_IconLayer_Draw_UI.cpp (orchestrates cull -> budget -> bucket -> flush, and the C2
// cache). Diagnostics pointers are test-only hooks; production always passes nullptr.
void DrawOverlayIconLayers(const DrawOverlayIconLayersInput& input, IconLayerAabbCache_UI& aabbCache,
                           IconLayerFrameCache& frameCache, ImDrawList& drawList,
                           IconLayerCullDiagnostics_UI* cullDiagnostics = nullptr,
                           IconLayerBudgetDiagnostics_UI* budgetDiagnostics = nullptr,
                           IconLayerGenerationDiagnostics_UI* generationDiagnostics = nullptr);

} // namespace Ui
} // namespace SanmapGen
