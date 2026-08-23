// MapCanvas_IconLayer_DrawCache_UI.cpp — the imgui-typed half of §4's C2 cache: capturing a
// bucket's just-written vertex bytes plus a synthesized local index pattern into
// IconLayerFrameCache, and replaying those bytes back into a live ImDrawList. Layer: UI. The
// invalidation decision itself and the raw-byte accumulation are pure/imgui-free
// (MapCanvas_IconLayer_Cache_UI.cpp); this file is the bridge that knows ImDrawVert's layout.
#include "MapCanvas_IconLayer_DrawInternal_UI.h"
#include <cstring>

namespace SanmapGen {
namespace Ui {
namespace {

// The fixed 2-triangle quad topology, LOCAL to each quad's own 4 vertices (0,1,2,0,2,3) — a
// self-contained numbering the cache owns, decoupled from whatever absolute base imgui's own
// VtxOffset bookkeeping used at build time (imgui.h: "_VtxCurrentIdx ... resets to 0" past 64K
// vertices) so replay's rebase below is correct regardless of that internal detail.
// STEP98: chunk-relative — each chunk's own quads numbered from 0 (not a bucket-global quad * 4),
// so these stay correct once ReplayCachedBuckets gets a fresh per-chunk base (the same 16-bit
// ImDrawIdx ceiling that forces FlushIconLayerBucket's own chunking applies here identically).
std::vector<ImDrawIdx> BuildLocalIndexPattern(int quadCount) {
    std::vector<ImDrawIdx> indices;
    indices.reserve(static_cast<std::size_t>(quadCount) * 6);
    for (const IconLayerBucketChunkRange_UI& chunk : ComputeIconLayerBucketChunks(quadCount)) {
        for (int quadInChunk = 0; quadInChunk < chunk.quadCount; ++quadInChunk) {
            const ImDrawIdx base = static_cast<ImDrawIdx>(quadInChunk * 4);
            indices.push_back(base);     indices.push_back(base + 1); indices.push_back(base + 2);
            indices.push_back(base);     indices.push_back(base + 2); indices.push_back(base + 3);
        }
    }
    return indices;
}

} // namespace

void CaptureAndCacheBuckets(ImDrawList& drawList, IconLayerFrameCache& frameCache,
                            const std::vector<AtlasPageBucket>& buckets) {
    for (const AtlasPageBucket& bucket : buckets) {
        if (bucket.quads.empty()) continue;
        const int vertexCountBefore = drawList.VtxBuffer.Size;
        FlushIconLayerBucket(drawList, bucket);
        const int vertexCountAfter = drawList.VtxBuffer.Size;
        AppendCachedVertexBytes(frameCache, &drawList.VtxBuffer[vertexCountBefore],
                                static_cast<std::size_t>(vertexCountAfter - vertexCountBefore) * sizeof(ImDrawVert));
        const std::vector<ImDrawIdx> localIndices = BuildLocalIndexPattern(static_cast<int>(bucket.quads.size()));
        AppendCachedIndexBytes(frameCache, localIndices.data(), localIndices.size() * sizeof(ImDrawIdx));
        CachedIconLayerBucketLayout_UI layout;
        layout.atlasPage = bucket.atlasPage;
        layout.textureIdentifier = bucket.textureIdentifier;
        layout.quadCount = static_cast<int>(bucket.quads.size());
        frameCache.cachedBucketLayout.push_back(layout);
    }
}

void ReplayCachedBuckets(ImDrawList& drawList, const IconLayerFrameCache& frameCache) {
    std::size_t vertexByteOffset = 0, indexByteOffset = 0;
    for (const CachedIconLayerBucketLayout_UI& bucketLayout : frameCache.cachedBucketLayout) {
        if (bucketLayout.quadCount <= 0) continue;
        drawList.PushTextureID(static_cast<ImTextureID>(bucketLayout.textureIdentifier));
        for (const IconLayerBucketChunkRange_UI& chunk : ComputeIconLayerBucketChunks(bucketLayout.quadCount)) {
            const int vertexCount = chunk.quadCount * 4, indexCount = chunk.quadCount * 6;
            drawList.PrimReserve(indexCount, vertexCount);
            const ImDrawIdx base = static_cast<ImDrawIdx>(drawList._VtxCurrentIdx);
            std::memcpy(drawList._VtxWritePtr, frameCache.cachedVertexBytes.data() + vertexByteOffset,
                       static_cast<std::size_t>(vertexCount) * sizeof(ImDrawVert));
            drawList._VtxWritePtr += vertexCount;
            drawList._VtxCurrentIdx += static_cast<unsigned int>(vertexCount);
            const ImDrawIdx* localIndices =
                reinterpret_cast<const ImDrawIdx*>(frameCache.cachedIndexBytes.data() + indexByteOffset);
            for (int index = 0; index < indexCount; ++index)
                drawList.PrimWriteIdx(static_cast<ImDrawIdx>(base + localIndices[index]));
            vertexByteOffset += static_cast<std::size_t>(vertexCount) * sizeof(ImDrawVert);
            indexByteOffset += static_cast<std::size_t>(indexCount) * sizeof(ImDrawIdx);
        }
        drawList.PopTextureID();
    }
}

} // namespace Ui
} // namespace SanmapGen
