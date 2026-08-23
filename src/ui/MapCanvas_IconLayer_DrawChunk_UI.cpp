// MapCanvas_IconLayer_DrawChunk_UI.cpp — ComputeIconLayerBucketChunks, the shared chunk-range
// helper both MapCanvas_IconLayer_Draw_UI.cpp (live flush) and MapCanvas_IconLayer_DrawCache_UI.cpp
// (cache build/replay) consume identically (STEP98). Small, pure, imgui-free translation unit,
// mirroring this module's own precedent of isolating pure/headless-testable logic
// (MapCanvas_IconLayer_Cull_UI.cpp, MapCanvas_IconLayer_Budget_UI.cpp).
#include "MapCanvas_IconLayer_DrawInternal_UI.h"
#include <algorithm>

namespace SanmapGen {
namespace Ui {

std::vector<IconLayerBucketChunkRange_UI> ComputeIconLayerBucketChunks(int totalQuadCount) {
    std::vector<IconLayerBucketChunkRange_UI> chunks;
    if (totalQuadCount <= 0) return chunks;
    chunks.reserve(static_cast<std::size_t>(
        (totalQuadCount + kIconLayerBucketChunkQuadCap - 1) / kIconLayerBucketChunkQuadCap));
    for (int quadStart = 0; quadStart < totalQuadCount; quadStart += kIconLayerBucketChunkQuadCap) {
        const int quadCount = std::min(kIconLayerBucketChunkQuadCap, totalQuadCount - quadStart);
        chunks.push_back(IconLayerBucketChunkRange_UI{quadStart, quadCount});
    }
    return chunks;
}

} // namespace Ui
} // namespace SanmapGen
