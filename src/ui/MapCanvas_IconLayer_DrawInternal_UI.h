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

// The 16-bit ImDrawIdx ceiling (imconfig.h's 32-bit override is commented out in this project's
// vendored copy) means a single PrimReserve call spanning more than 65,536 vertices free-runs
// _VtxCurrentIdx past the point where imgui's own automatic VtxOffset bump can help — that bump
// only fires once, at the TOP of a PrimReserve call, checking the pre-call _VtxCurrentIdx
// (imgui_draw.cpp:739-762). kIconLayerBucketChunkQuadCap keeps every single PrimReserve call's
// own vertex span (chunkQuadCount * 4) safely under that ceiling. Fixed internal constant, NOT a
// Constitution §8 tweakable — it is dictated by the vendored index type, not a design dial
// (Constitution §6: exposing it would let a designer reintroduce 16-bit index wraparound).
constexpr int kIconLayerBucketChunkQuadCap = 16000;   // 16,000 quads = 64,000 vertices < 65,536

// STEP231 — the icon-atlas pass's own "selected" indicator. A LITERAL, matching
// GlobalMarkerSettings::selectColorAlloy's own new lime-green default (GlobalMarkerSettings_PARAMS.h)
// for a consistent "selected = green" visual language against the roster/dot pass's own select tint
// (Params::ResolveMarkerGroupSelectTintColor) — but NOT read from Params here: FlushIconLayerBucket
// is this module's one deliberately domain-agnostic choke point (serves Markers/Props/Decals/Units
// uniformly, confirmed by direct read of every call site in MapCanvas_IconLayer_Cull*_UI.cpp), and
// threading a per-marker-category selectColor* resolution all the way through
// EmitCandidateIfVisible/AppendCandidate would mean giving this function marker-domain knowledge it
// does not otherwise have. This mirrors the SAME file-family's own established precedent for a fixed,
// non-parameter-driven state-indicator color: MapCanvas_MarkerRosterDraw_UI.cpp's own
// refusedTint/ghostTint (two IM_COL32 literals for drag-refused-red/drag-ghost-grey, neither threaded
// through GlobalMarkerSettings either). A category-correct selectColor* threaded all the way through
// the cull/emit pipeline is a real, larger, separate follow-up (see this ticket's own Interpretation
// calls) — not invented here.
constexpr ImU32 kIconLayerSelectedTint = IM_COL32(51, 255, 51, 255);   // matches {0.2, 1.0, 0.2, 1.0}

// One contiguous sub-range of a bucket's quad list, sized to stay under
// kIconLayerBucketChunkQuadCap. Pure/deterministic given totalQuadCount — build (FlushIconLayerBucket,
// CaptureAndCacheBuckets' BuildLocalIndexPattern) and replay (ReplayCachedBuckets) call
// ComputeIconLayerBucketChunks identically, so chunk boundaries never need to be separately cached.
struct IconLayerBucketChunkRange_UI {
    int quadStart = 0;
    int quadCount = 0;
};
std::vector<IconLayerBucketChunkRange_UI> ComputeIconLayerBucketChunks(int totalQuadCount);

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
