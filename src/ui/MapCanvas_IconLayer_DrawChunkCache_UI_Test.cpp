// MapCanvas_IconLayer_DrawChunkCache_UI_Test.cpp — STEP98 acceptance test, part 2: the
// CaptureAndCacheBuckets/ReplayCachedBuckets cache-path round trip at 32,000 quads. Split out of
// MapCanvas_IconLayer_DrawChunk_UI_Test.cpp (its sibling TU) to stay inside Constitution §1.5; called
// from that TU's RunMapCanvasIconLayerDrawChunkChecks(), the single symbol MapCanvas_IconLayer_UI_Test.cpp's
// main() invokes.
#include "MapCanvas_IconLayer_DrawChunkTestSupport_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Cache-path correctness (CaptureAndCacheBuckets -> ReplayCachedBuckets), same 32,000-quad bucket,
// replayed into a second, independent ImDrawList (a second window's own draw list, same frame/context).
void CheckCachePathRoundTrip() {
    constexpr int quadCount = 32000;
    ImGui::CreateContext();
    BeginIconLayerChunkTestHeadlessFrame();
    const std::vector<AtlasPageBucket> buckets =
        BucketByAtlasPage(BuildIconLayerChunkTestQuads(quadCount, kIconLayerChunkTestTextureIdentifier));
    check(buckets.size() == 1, "the synthetic scene forms a single atlas-page bucket");

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(256.0f, 256.0f));
    ImGui::Begin("IconLayerDrawChunkCacheBuildWindow");
    IconLayerFrameCache frameCache;
    BeginIconLayerCacheBuild(frameCache, 0.0f, 0.0f, 1.0f, OverlayInstanceKey_UI{}, 1);
    CaptureAndCacheBuckets(*ImGui::GetWindowDrawList(), frameCache, buckets);
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(0.0f, 300.0f));
    ImGui::SetNextWindowSize(ImVec2(256.0f, 256.0f));
    ImGui::Begin("IconLayerDrawChunkCacheReplayWindow");
    ImDrawList& replayDrawList = *ImGui::GetWindowDrawList();
    ReplayCachedBuckets(replayDrawList, frameCache);
    ImGui::End();
    ImGui::Render();

    // CaptureAndCacheBuckets also flushes live (the build window carries this texture id too);
    // restrict to the replay window's own draw list so this exercises ONLY ReplayCachedBuckets.
    VerifyIconLayerChunkTestBucketDrawOutput(*ImGui::GetDrawData(), kIconLayerChunkTestTextureIdentifier,
                                             quadCount, &replayDrawList);
    ImGui::DestroyContext();
}

} // namespace

void RunMapCanvasIconLayerDrawChunkCacheChecks() {
    CheckCachePathRoundTrip();
}

} // namespace Ui
} // namespace SanmapGen
