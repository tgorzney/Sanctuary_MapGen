// MapCanvas_IconLayer_DrawChunk_UI_Test.cpp — STEP98 acceptance test, part 1: ComputeIconLayerBucketChunks
// itself (pure/headless), and FlushIconLayerBucket's live-path chunking at 32,000 and 20,000 quads
// (live headless imgui frame, mirroring MapCanvas_Render_UI_Test.cpp's technique). The cache-path
// round trip is its own sibling TU (MapCanvas_IconLayer_DrawChunkCache_UI_Test.cpp) to stay inside
// Constitution §1.5. One translation unit of the MapCanvas_IconLayer_UI_Test binary.
#include "MapCanvas_IconLayer_DrawChunkTestSupport_UI.h"

namespace SanmapGen {
namespace Ui {

void RunMapCanvasIconLayerDrawChunkCacheChecks();   // MapCanvas_IconLayer_DrawChunkCache_UI_Test.cpp

namespace {

// §1: pure/headless helper correctness, no imgui frame needed.
void CheckComputeIconLayerBucketChunks() {
    check(ComputeIconLayerBucketChunks(0).empty(), "zero quads produce zero chunks");
    for (const int totalQuadCount : {1, 16000, 16001, 32000, 20000}) {
        const std::vector<IconLayerBucketChunkRange_UI> chunks = ComputeIconLayerBucketChunks(totalQuadCount);
        const int expectedChunkCount =
            (totalQuadCount + kIconLayerBucketChunkQuadCap - 1) / kIconLayerBucketChunkQuadCap;
        check(static_cast<int>(chunks.size()) == expectedChunkCount, "chunk count equals ceil(total/chunkCap)");
        bool bAllUnderCap = true, bGapless = true; int summedQuadCount = 0;
        for (std::size_t i = 0; i < chunks.size(); ++i) {
            if (chunks[i].quadCount > kIconLayerBucketChunkQuadCap) bAllUnderCap = false;
            if (i > 0 && chunks[i].quadStart != chunks[i - 1].quadStart + chunks[i - 1].quadCount) bGapless = false;
            summedQuadCount += chunks[i].quadCount;
        }
        check(bAllUnderCap, "every range's quadCount stays at or under kIconLayerBucketChunkQuadCap");
        check(bGapless, "ranges are contiguous and gapless");
        check(summedQuadCount == totalQuadCount, "the sum of all quadCount values equals totalQuadCount exactly");
    }
}

// §2: live-path correctness (FlushIconLayerBucket via BucketByAtlasPage), one single-page bucket.
void CheckLivePathNoIndexCollisionForQuadCount(int quadCount) {
    ImGui::CreateContext();
    BeginIconLayerChunkTestHeadlessFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(256.0f, 256.0f));
    ImGui::Begin("IconLayerDrawChunkLiveTestWindow");
    ImDrawList& drawList = *ImGui::GetWindowDrawList();
    const std::vector<AtlasPageBucket> buckets =
        BucketByAtlasPage(BuildIconLayerChunkTestQuads(quadCount, kIconLayerChunkTestTextureIdentifier));
    check(buckets.size() == 1, "the synthetic scene forms a single atlas-page bucket");
    FlushBuckets(drawList, buckets);
    ImGui::End();
    ImGui::Render();
    VerifyIconLayerChunkTestBucketDrawOutput(*ImGui::GetDrawData(), kIconLayerChunkTestTextureIdentifier,
                                             quadCount, nullptr);
    ImGui::DestroyContext();
}

} // namespace

void RunMapCanvasIconLayerDrawChunkChecks() {
    CheckComputeIconLayerBucketChunks();
    CheckLivePathNoIndexCollisionForQuadCount(32000);
    CheckLivePathNoIndexCollisionForQuadCount(20000);
    RunMapCanvasIconLayerDrawChunkCacheChecks();
}

} // namespace Ui
} // namespace SanmapGen
