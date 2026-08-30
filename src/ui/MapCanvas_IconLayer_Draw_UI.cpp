// MapCanvas_IconLayer_Draw_UI.cpp — §3's atlas-page bucketing + bulk vertex write, and the
// top-level DrawOverlayIconLayers orchestration (cull -> budget -> bucket -> flush, wired to the
// §4 C2 cache). Layer: UI. This TU and its sibling MapCanvas_IconLayer_DrawCache_UI.cpp are the
// only ones in this module that include imgui, mirroring MapCanvas_Draw_UI.cpp's own precedent of
// isolating imgui to a small, named set of translation units.
#include "MapCanvas_IconLayer_DrawInternal_UI.h"
#include <unordered_map>

namespace SanmapGen {
namespace Ui {
namespace {

// STEP133 — the spread multiplier mixing `markerTypeVisibilityRevision` into the C2 cache's own
// invalidation key below, so the Hide/Unhide toggle alone (nothing else changed) forces a rebuild —
// see MarkerTypeVisibility_UI.h's own header comment for why this must never write directly into
// `OverlayLayerSettings::layerSettingsRevision`. A large prime spreads the two counters' low values
// apart so an unrelated one-count bump in either counter never collides with the other's.
constexpr std::uint64_t kMarkerTypeVisibilityRevisionSpreadMultiplier = 1000003ull;

void SplitSelected(const std::vector<OverlayVisibleInstance>& budgeted,
                   std::vector<OverlayVisibleInstance>& outNonSelected,
                   std::vector<OverlayVisibleInstance>& outSelected) {
    outNonSelected.reserve(budgeted.size());
    for (const OverlayVisibleInstance& instance : budgeted)
        (instance.bSelected ? outSelected : outNonSelected).push_back(instance);
}

// STEP229 — selectedInstanceKeys is a push-in pointer (DrawOverlayIconLayersInput's established
// convention: null = no selection source wired). Every consumer of "the selection" in this file wants
// a plain reference to iterate/compare against, so this collapses null to a static empty set exactly
// once, here, rather than each call site re-deriving its own null-check.
const OverlayInstanceKeySet_UI& ResolveSelectionSetOrEmpty(const DrawOverlayIconLayersInput& input) {
    static const OverlayInstanceKeySet_UI kEmptySelection;
    return input.selectedInstanceKeys != nullptr ? *input.selectedInstanceKeys : kEmptySelection;
}

// Cache-invalid frame: run steps 1-3 excluding the selected instance(s), cache those bytes, and
// flush the (typically tiny) selected set live and uncached (§4's "Build" step).
void RebuildAndCache(const DrawOverlayIconLayersInput& input, IconLayerAabbCache_UI& aabbCache,
                     IconLayerFrameCache& frameCache, ImDrawList& drawList,
                     IconLayerCullDiagnostics_UI* cullDiagnostics, IconLayerBudgetDiagnostics_UI* budgetDiagnostics,
                     IconLayerGenerationDiagnostics_UI* generationDiagnostics,
                     float viewCenterX, float viewCenterY, float zoomScale, std::uint64_t revision) {
    if (generationDiagnostics != nullptr) ++generationDiagnostics->fullGenerationCount;
    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(input, aabbCache, cullDiagnostics, candidates);
    std::vector<OverlayVisibleInstance> budgeted =
        ApplyVisibleInstanceBudget(std::move(candidates), *input.renderingSettings, budgetDiagnostics);
    std::vector<OverlayVisibleInstance> nonSelected, selected;
    SplitSelected(budgeted, nonSelected, selected);
    BeginIconLayerCacheBuild(frameCache, viewCenterX, viewCenterY, zoomScale,
                             ResolveSelectionSetOrEmpty(input), revision);
    CaptureAndCacheBuckets(drawList, frameCache, BucketByAtlasPage(nonSelected));
    FlushBuckets(drawList, BucketByAtlasPage(selected));
}

// Cache-valid frame: replay the cached bytes (zero regeneration) then regenerate live only the
// selected instance (§4's "Replay" step).
void ReplayAndRedrawSelection(const DrawOverlayIconLayersInput& input, IconLayerFrameCache& frameCache,
                              ImDrawList& drawList) {
    ReplayCachedBuckets(drawList, frameCache);
    std::vector<OverlayVisibleInstance> selectedOnly;
    ResolveSelectedInstanceCandidate(input, selectedOnly);
    FlushBuckets(drawList, BucketByAtlasPage(selectedOnly));
}

} // namespace

std::vector<AtlasPageBucket> BucketByAtlasPage(const std::vector<OverlayVisibleInstance>& instances) {
    std::vector<AtlasPageBucket> buckets;
    std::unordered_map<int, std::size_t> bucketIndexByPage;
    for (const OverlayVisibleInstance& instance : instances) {
        const auto found = bucketIndexByPage.find(instance.atlasPage);
        std::size_t bucketIndex;
        if (found == bucketIndexByPage.end()) {
            bucketIndex = buckets.size();
            bucketIndexByPage.emplace(instance.atlasPage, bucketIndex);
            buckets.push_back(AtlasPageBucket{instance.atlasPage, instance.textureIdentifier, {}});
        } else {
            bucketIndex = found->second;
        }
        buckets[bucketIndex].quads.push_back(instance);
    }
    return buckets;
}

void FlushIconLayerBucket(ImDrawList& drawList, const AtlasPageBucket& bucket) {
    if (bucket.quads.empty()) return;   // never a zero-quad draw command
    drawList.PushTextureID(static_cast<ImTextureID>(bucket.textureIdentifier));
    const int quadCount = static_cast<int>(bucket.quads.size());
    for (const IconLayerBucketChunkRange_UI& chunk : ComputeIconLayerBucketChunks(quadCount)) {
        drawList.PrimReserve(chunk.quadCount * 6, chunk.quadCount * 4);
        for (int i = 0; i < chunk.quadCount; ++i) {
            const OverlayVisibleInstance& instance = bucket.quads[chunk.quadStart + i];
            const float half = instance.screenSize * 0.5f;
            // STEP231 — the actual fix: bSelected previously had ZERO visual effect anywhere in this
            // pass (it only ever routed an instance into the C2 cache's selected/non-selected bucket
            // split, MapCanvas_IconLayer_Draw_UI.cpp's own SplitSelected). ARCH §19.18's own "selected
            // replaces fill, full opacity" visual language (ratified for the roster/dot pass) is
            // applied here too — kIconLayerSelectedTint is already full alpha (255), so a selected
            // instance in a low-opacity layer is not left faint the way multiplying instance.tintAlpha
            // in would leave it.
            const ImU32 tint = instance.bSelected
                ? kIconLayerSelectedTint
                : ImGui::ColorConvertFloat4ToU32(
                      ImVec4(instance.tintColorRed, instance.tintColorGreen, instance.tintColorBlue, instance.tintAlpha));
            const ImDrawIdx base = static_cast<ImDrawIdx>(drawList._VtxCurrentIdx);
            drawList.PrimWriteIdx(base);     drawList.PrimWriteIdx(base + 1); drawList.PrimWriteIdx(base + 2);
            drawList.PrimWriteIdx(base);     drawList.PrimWriteIdx(base + 2); drawList.PrimWriteIdx(base + 3);
            drawList.PrimWriteVtx(ImVec2(instance.screenCenterX - half, instance.screenCenterY - half),
                                  ImVec2(instance.uvMinimumX, instance.uvMinimumY), tint);
            drawList.PrimWriteVtx(ImVec2(instance.screenCenterX + half, instance.screenCenterY - half),
                                  ImVec2(instance.uvMaximumX, instance.uvMinimumY), tint);
            drawList.PrimWriteVtx(ImVec2(instance.screenCenterX + half, instance.screenCenterY + half),
                                  ImVec2(instance.uvMaximumX, instance.uvMaximumY), tint);
            drawList.PrimWriteVtx(ImVec2(instance.screenCenterX - half, instance.screenCenterY + half),
                                  ImVec2(instance.uvMinimumX, instance.uvMaximumY), tint);
        }
    }
    drawList.PopTextureID();
}

void FlushBuckets(ImDrawList& drawList, const std::vector<AtlasPageBucket>& buckets) {
    for (const AtlasPageBucket& bucket : buckets) FlushIconLayerBucket(drawList, bucket);
}

void DrawOverlayIconLayers(const DrawOverlayIconLayersInput& input, IconLayerAabbCache_UI& aabbCache,
                           IconLayerFrameCache& frameCache, ImDrawList& drawList,
                           IconLayerCullDiagnostics_UI* cullDiagnostics,
                           IconLayerBudgetDiagnostics_UI* budgetDiagnostics,
                           IconLayerGenerationDiagnostics_UI* generationDiagnostics) {
    if (input.overlayLayerSettings == nullptr || input.renderingSettings == nullptr || input.view == nullptr) return;
    const float viewCenterX = input.view->ViewCenterPixelX();
    const float viewCenterY = input.view->ViewCenterPixelY();
    const float zoomScale = input.view->ZoomScale();
    // STEP133 — combines the Markers tab's own per-Type Hide/Unhide revision into the C2 cache's
    // invalidation key, so toggling it alone (view/selection unchanged) still forces a rebuild.
    const std::uint64_t revision = input.overlayLayerSettings->layerSettingsRevision
        + input.markerTypeVisibilityRevision * kMarkerTypeVisibilityRevisionSpreadMultiplier;
    if (ShouldInvalidateIconLayerCache(frameCache, viewCenterX, viewCenterY, zoomScale,
                                       ResolveSelectionSetOrEmpty(input), revision)) {
        RebuildAndCache(input, aabbCache, frameCache, drawList, cullDiagnostics, budgetDiagnostics,
                        generationDiagnostics, viewCenterX, viewCenterY, zoomScale, revision);
        return;
    }
    ReplayAndRedrawSelection(input, frameCache, drawList);
}

} // namespace Ui
} // namespace SanmapGen
