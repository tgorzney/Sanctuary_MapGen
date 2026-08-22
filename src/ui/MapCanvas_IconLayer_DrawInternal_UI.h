// MapCanvas_IconLayer_DrawInternal_UI.h — declarations shared by MapCanvas_IconLayer_Draw_UI.cpp
// and MapCanvas_IconLayer_DrawCache_UI.cpp ONLY (§3's bucketing/flush split further than the
// ticket's 5-file minimum to stay inside Constitution §1.5's ceilings). Not part of this module's
// public surface — nothing outside this pair includes it. Both TUs include imgui (this header
// itself does), consistent with "the only translation unit here that includes imgui" meaning this
// PAIR, not a single file, since §3's bucketing/flush and §4's cache bridge are inseparably
// imgui-typed (ImDrawList/ImDrawVert/ImDrawIdx).
#pragma once
#include "MapCanvas_IconLayer_CullInternal_UI.h"
#include "MapCanvasView_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

std::vector<AtlasPageBucket> BucketByAtlasPage(const std::vector<OverlayVisibleInstance>& instances);

// §3, confirmed against this project's vendored imgui (PrimReserve/PrimWriteVtx/PrimWriteIdx,
// imgui.h ~3585-3595): ONE PrimReserve per bucket — PrimRectUV/PrimQuadUV each reserve internally,
// so calling them per-quad here would double-reserve and reintroduce per-instance call cost.
void FlushIconLayerBucket(ImDrawList& drawList, const AtlasPageBucket& bucket);
void FlushBuckets(ImDrawList& drawList, const std::vector<AtlasPageBucket>& buckets);

// Flushes normally (so the live ImDrawList is correct THIS frame too) and, per bucket, copies the
// same vertex bytes it just wrote plus a freshly-synthesized local index pattern into the cache
// (MapCanvas_IconLayer_DrawCache_UI.cpp).
void CaptureAndCacheBuckets(ImDrawList& drawList, IconLayerFrameCache& frameCache,
                            const std::vector<AtlasPageBucket>& buckets);
// One bulk memcpy of vertex bytes per bucket (position/uv/color are absolute screen-space values,
// independent of any index base — safe to copy verbatim), then a cheap per-index rebase pass —
// zero regeneration of the expensive part, exactly §14.8's contract.
void ReplayCachedBuckets(ImDrawList& drawList, const IconLayerFrameCache& frameCache);

} // namespace Ui
} // namespace SanmapGen
