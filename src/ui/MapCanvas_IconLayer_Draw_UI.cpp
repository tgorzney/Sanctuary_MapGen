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

void SplitSelected(const std::vector<OverlayVisibleInstance>& budgeted,
                   std::vector<OverlayVisibleInstance>& outNonSelected,
                   std::vector<OverlayVisibleInstance>& outSelected) {
    outNonSelected.reserve(budgeted.size());
    for (const OverlayVisibleInstance& instance : budgeted)
        (instance.bSelected ? outSelected : outNonSelected).push_back(instance);
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
    BeginIconLayerCacheBuild(frameCache, viewCenterX, viewCenterY, zoomScale, input.selectedInstanceKey, revision);
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
            const ImU32 tint = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, instance.tintAlpha));
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
    const std::uint64_t revision = input.overlayLayerSettings->layerSettingsRevision;
    if (ShouldInvalidateIconLayerCache(frameCache, viewCenterX, viewCenterY, zoomScale,
                                       input.selectedInstanceKey, revision)) {
        RebuildAndCache(input, aabbCache, frameCache, drawList, cullDiagnostics, budgetDiagnostics,
                        generationDiagnostics, viewCenterX, viewCenterY, zoomScale, revision);
        return;
    }
    ReplayAndRedrawSelection(input, frameCache, drawList);
}

} // namespace Ui
} // namespace SanmapGen
