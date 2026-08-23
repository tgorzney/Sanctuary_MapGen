# STEP98 — Icon-layer atlas-page bucket chunking: fix the 16-bit `ImDrawIdx` wraparound in STEP53's shipped draw + cache paths

**Layer:** UI. **Domain:** `MapCanvas` screen-space overlay icon draw pass (`OverlayLayer_UI`,
ARCH §14.9), the exact production code STEP53 shipped and STEP59 benchmarks. **Corrects:** a
latent, unfixed correctness defect in `STEP53_OverlayIconDrawPass_UI.md`'s own shipped output —
this ticket does not reopen or re-litigate STEP53's design, only its one broken assumption about
`PrimReserve` batch size.

## Root problem

STEP53 §3 requires (ARCH §14.9, "Bulk vertex writes only") that an atlas-page bucket flush as
**one** `ImDrawList::PrimReserve` call followed by a raw per-quad `PrimWriteVtx`/`PrimWriteIdx`
loop, never per-instance `AddImage()`. That requirement is correct and stays correct. What STEP53
got wrong is silently assuming a bucket's quad count can never be large enough to matter for
16-bit index safety. It can, and at STEP53's own stated scale (`visibleInstanceBudget` default
450,000, `OverlayRenderingSettings`, `MapCanvas_IconLayer_UI.h:60-63`) it routinely will.

This project's vendored imgui uses the default 16-bit `ImDrawIdx`. Confirmed:
`build/_deps/imgui-src/imconfig.h:124`'s `#define ImDrawIdx unsigned int` override is commented
out. The production renderer backend does set the large-mesh support flags — confirmed
`ImGui_ImplOpenGL3_Init` (via `src/ui/Application_Window_UI.cpp`, GL>=3.2) sets
`ImGuiBackendFlags_RendererHasVtxOffset`/`ImDrawListFlags_AllowVtxOffset` — but that mechanism
cannot save a single oversized `PrimReserve` call, and both shipped call sites make exactly that
mistake.

### The mechanism, traced against the real vendored source
`ImDrawList::PrimReserve` (`build/_deps/imgui-src/imgui_draw.cpp:739-762`):
```cpp
void ImDrawList::PrimReserve(int idx_count, int vtx_count)
{
    if (sizeof(ImDrawIdx) == 2 && (_VtxCurrentIdx + vtx_count >= (1 << 16)) && (Flags & ImDrawListFlags_AllowVtxOffset))
    {
        _CmdHeader.VtxOffset = VtxBuffer.Size;
        _OnChangedVtxOffset();   // resets _VtxCurrentIdx = 0 (imgui_draw.cpp:647), starts a new ImDrawCmd
    }
    ...
}
```
The overflow check runs **once, at the top of the call**, comparing the *pre-call* `_VtxCurrentIdx`
against the requested `vtx_count` for that whole call. `PrimWriteVtx` then free-runs
`_VtxCurrentIdx++` (a plain `unsigned int`, confirmed `imgui.h:3473`) for every vertex written
inside that one reserved block, with no second check.

`FlushIconLayerBucket` (`src/ui/MapCanvas_IconLayer_Draw_UI.cpp:70-91`) reserves an **entire
bucket's** vertices in one call:
```cpp
void FlushIconLayerBucket(ImDrawList& drawList, const AtlasPageBucket& bucket) {
    if (bucket.quads.empty()) return;
    drawList.PushTextureID(static_cast<ImTextureID>(bucket.textureIdentifier));
    const int quadCount = static_cast<int>(bucket.quads.size());
    drawList.PrimReserve(quadCount * 6, quadCount * 4);          // <-- one call, whole bucket
    for (const OverlayVisibleInstance& instance : bucket.quads) {
        ...
        const ImDrawIdx base = static_cast<ImDrawIdx>(drawList._VtxCurrentIdx);   // <-- truncates/wraps
        ...
    }
    drawList.PopTextureID();
}
```
Worked example at 20,000 quads on one shared atlas page (well under `visibleInstanceBudget`'s own
450,000 default, and a plausible single-page count under heavy overlay use — many templates
legally share one bin-packed atlas page, ARCH §14.9's own "Atlas page bucketing is required" note):
`PrimReserve(120000, 80000)` is called with `_VtxCurrentIdx` starting at 0. The top-of-call check
is `0 + 80000 >= 65536` → **true**, so the bump fires immediately, once, resetting
`_VtxCurrentIdx = 0` and opening one new `ImDrawCmd`. The per-quad loop then free-runs
`_VtxCurrentIdx` from 0 up through 80,000 **inside that same already-reserved block**, crossing
65,536 again at quad index 16,384 with no further correction available (the bump already fired,
and can only fire at the top of a `PrimReserve` call). From quad 16,384 onward,
`static_cast<ImDrawIdx>(drawList._VtxCurrentIdx)` silently truncates: e.g. quad 16,384's `base`
computes as `65536`, which truncates to `0` — colliding with quad 0's indices. This corrupts
rendered geometry in Release and trips imgui's debug-only 16-bit-index assert
(`imgui_draw.cpp`, `AddDrawListToDrawDataEx`, "Too many vertices in ImDrawList using 16-bit
indices") in Debug.

**STEP59's own benchmark harness worked around this exact issue rather than fixing it** — its
synthetic scenes keep per-page bucket sizes under ~8,000 quads and/or set the VtxOffset backend
flag themselves. That is a test-only accommodation (see STEP59 §2 item 2,
`MapCanvas_IconLayer_Microbenchmark_UI_Test.cpp`'s scenario construction), explicitly not a
production fix — STEP59's own "Explicit out-of-scope" section states it makes **no** change to
STEP53's shipped files.

### Second, independent instance of the same defect — the C2 cache build/replay path
`src/ui/MapCanvas_IconLayer_DrawCache_UI.cpp` (§14.8's C2 interaction-scoped cache) has the
identical shape, confirmed by direct read of the current shipped file:

- `BuildLocalIndexPattern` (lines 17-26) casts `quad * 4` (a bucket-global, un-chunked quad index)
  to `ImDrawIdx` with **no boundary check at all** — this wraps unconditionally at `quad == 16384`
  regardless of whether `AllowVtxOffset` is even set, because this function only synthesizes raw
  index bytes for later replay; it never touches a live `ImDrawList`.
- `ReplayCachedBuckets` (lines 49-69) has the same single-`PrimReserve`-per-bucket,
  single-`base`-per-bucket shape as `FlushIconLayerBucket`: one `PrimReserve(indexCount,
  vertexCount)` for the whole cached bucket, one `base = static_cast<ImDrawIdx>(drawList._VtxCurrentIdx)`,
  then a rebase loop (`base + localIndices[index]`, cast to `ImDrawIdx`) over every cached index —
  the same free-running-past-65536-within-one-reserved-block bug, plus it inherits corrupted
  `localIndices` from `BuildLocalIndexPattern` regardless.

This needs the identical fix, not a variant of it — a cache-path bucket over the chunk cap
corrupts at cache-build time, independent of whether the live path (`FlushIconLayerBucket`) is
ever separately exercised at that size in the same session.

## ARCH ruling (obtained this session from the SanGen ARCH Expert — settled law, not
re-litigated by this ticket)
1. Chunking a bucket's quads into multiple `PrimReserve` calls, each kept safely under the 16-bit
   ceiling, is ARCH-conformant. `ARCH_14_09_RenderingPerformance.md`'s "flush one draw command per
   non-empty bucket" clause (lines 20-21) targets the scatter-across-pages pathology, not an
   internal size-driven split of one oversized single-page bucket; the aggregate draw-call bound
   stays O(pages touched this frame) — one extra `ImDrawCmd` per `ceil(bucketQuadCount /
   chunkCap)`, attributable entirely to that one page, never a regression toward O(instances) or
   O(visit order). §14.9's current wording is imprecise enough to read as "always exactly one
   command per bucket"; the ARCH Expert intends a separate clarifying amendment (§14.9 or a new
   §14.15) — that amendment is the ARCH Expert's own follow-up, explicitly out of this ticket's
   scope (see below).
2. The chunk-size cap is a fixed internal `constexpr`, **not** a Constitution §8 tweakable — it is
   dictated by the vendored imgui `ImDrawIdx` bit-width, not a creative/design dial. Exposing it as
   a settable parameter would let a designer reintroduce the exact corruption this ticket fixes,
   conflicting with Constitution §6 ("validate all input"). Name:
   `constexpr int kIconLayerBucketChunkQuadCap = 16000;` (16,000 quads = 64,000 vertices, safely
   under the 65,536 ceiling), with a comment citing the 16-bit `ImDrawIdx` ceiling directly.
3. A shared chunk-range helper lives in `MapCanvas_IconLayer_DrawInternal_UI.h`, next to the
   existing `FlushIconLayerBucket`/`BucketByAtlasPage` declarations, so both
   `MapCanvas_IconLayer_Draw_UI.cpp` and `MapCanvas_IconLayer_DrawCache_UI.cpp` consume identical
   chunking logic rather than each reimplementing it. `FlushIconLayerBucket` keeps
   `PushTextureID`/`PopTextureID` bracketing the whole bucket (texture doesn't change within a
   bucket) — only the `PrimReserve`/write-loop is chunked, once per chunk. Both consumer files'
   line counts must be checked against ARCH §1.5's soft-100/hard-150 ceiling before adding this
   machinery; split the shared helper into its own file if either would cross the ceiling.

## Solution

### 1. New shared machinery — `src/ui/MapCanvas_IconLayer_DrawInternal_UI.h` (currently 36 lines)
Add, next to the existing `BucketByAtlasPage`/`FlushIconLayerBucket`/`FlushBuckets` declarations:
```cpp
// The 16-bit ImDrawIdx ceiling (imconfig.h's 32-bit override is commented out in this project's
// vendored copy) means a single PrimReserve call spanning more than 65,536 vertices free-runs
// _VtxCurrentIdx past the point where imgui's own automatic VtxOffset bump can help — that bump
// only fires once, at the TOP of a PrimReserve call, checking the pre-call _VtxCurrentIdx
// (imgui_draw.cpp:739-762). kIconLayerBucketChunkQuadCap keeps every single PrimReserve call's
// own vertex span (chunkQuadCount * 4) safely under that ceiling. Fixed internal constant, NOT a
// Constitution §8 tweakable — it is dictated by the vendored index type, not a design dial
// (Constitution §6: exposing it would let a designer reintroduce 16-bit index wraparound).
constexpr int kIconLayerBucketChunkQuadCap = 16000;   // 16,000 quads = 64,000 vertices < 65,536

// One contiguous sub-range of a bucket's quad list, sized to stay under
// kIconLayerBucketChunkQuadCap. Pure/deterministic given totalQuadCount — build (FlushIconLayerBucket,
// CaptureAndCacheBuckets' BuildLocalIndexPattern) and replay (ReplayCachedBuckets) call
// ComputeIconLayerBucketChunks identically, so chunk boundaries never need to be separately cached.
struct IconLayerBucketChunkRange_UI {
    int quadStart = 0;
    int quadCount = 0;
};
std::vector<IconLayerBucketChunkRange_UI> ComputeIconLayerBucketChunks(int totalQuadCount);
```
Expected size after this addition: ~36 → ~54 lines. Comfortably under the soft-100 ceiling.

### 2. NEW `src/ui/MapCanvas_IconLayer_DrawChunk_UI.cpp` — the helper's implementation
A small, pure, imgui-free translation unit (mirrors the existing module's own precedent of
isolating pure/headless-testable logic, e.g. `MapCanvas_IconLayer_Cull_UI.cpp`,
`MapCanvas_IconLayer_Budget_UI.cpp`) rather than adding this to either consumer file, since both
consumers are already at or above the soft-100 ceiling (see §5 below):
```cpp
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
```
Needs `#include "MapCanvas_IconLayer_DrawInternal_UI.h"` and `<algorithm>` for `std::min`.
Estimated ~20-25 lines including the file header comment and namespace boilerplate.
(Implementation-call note, mirroring STEP53 §0's own "exact per-file line boundaries are an
implementation call, not locked here": if a coder finds this comfortably fits inline in the header
as a small `inline` function instead, that is an acceptable equivalent — the header already
includes imgui transitively via `MapCanvas_IconLayer_CullInternal_UI.h`/`<imgui.h>`, and this
function needs neither. The separate-file shape above is the recommended default given both
consumer files' existing size pressure, not a hard requirement.)

### 3. MODIFIED `src/ui/MapCanvas_IconLayer_Draw_UI.cpp` (currently 117 lines — already over the
soft-100 ceiling pre-existing this ticket; documented ARCH §1.5 exception carried forward, not
newly introduced here) — `FlushIconLayerBucket` only
Only this one function's body changes; its signature, and `BucketByAtlasPage`/`FlushBuckets`/
`DrawOverlayIconLayers`/`RebuildAndCache`/`ReplayAndRedrawSelection`/`SplitSelected`, are
untouched — none of the orchestration functions call `PrimReserve` directly, they only call the
now-fixed `FlushIconLayerBucket`. `PushTextureID`/`PopTextureID` stay bracketing the whole bucket,
per the ARCH ruling above; only the reserve+write body moves inside a chunk loop:
```cpp
void FlushIconLayerBucket(ImDrawList& drawList, const AtlasPageBucket& bucket) {
    if (bucket.quads.empty()) return;
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
```
Expected size after this change: 117 → ~126-128 lines. Stays under the hard-150 ceiling; no split
required. Every bucket at or under `kIconLayerBucketChunkQuadCap` produces exactly the same single
`PrimReserve` call and byte output as today (one chunk == today's behavior) — this is a pure
extension for the over-cap case, not a behavior change for any bucket size this codebase has
exercised until now.

### 4. MODIFIED `src/ui/MapCanvas_IconLayer_DrawCache_UI.cpp` (currently 73 lines) —
`BuildLocalIndexPattern` and `ReplayCachedBuckets`; `CaptureAndCacheBuckets` needs **no edit**
`CaptureAndCacheBuckets` (lines 30-47) calls `FlushIconLayerBucket` (now fixed, §3) to produce the
live vertex bytes it captures — vertex bytes are absolute screen-space values written contiguously
regardless of chunking, so capturing them needs no change — and calls `BuildLocalIndexPattern`
for the index bytes. It records `layout.quadCount = bucket.quads.size()` (whole-bucket, unchanged) —
this is deliberately **not** widened to store per-chunk boundaries, since `ReplayCachedBuckets`
recomputes identical chunk ranges from that same `quadCount` via `ComputeIconLayerBucketChunks`
(§1's own determinism guarantee), keeping `CachedIconLayerBucketLayout_UI` (`MapCanvas_IconLayer_UI.h`)
unmodified.

`BuildLocalIndexPattern` (lines 17-26) must produce **chunk-relative** local indices (each chunk's
own quads numbered from 0, not a bucket-global `quad * 4`) so they stay correct once
`ReplayCachedBuckets` gets a fresh per-chunk `base`:
```cpp
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
```
Chunks are visited in the same order the vertex bytes were written (contiguous, original quad
order), so the concatenated chunk-relative index stream still lines up byte-for-byte with the
concatenated vertex stream — no change to how `AppendCachedIndexBytes`/`AppendCachedVertexBytes`
are called.

`ReplayCachedBuckets` (lines 49-69) gets a fresh `PrimReserve` + fresh `base` per chunk instead of
once per bucket, mirroring `FlushIconLayerBucket`'s own restructuring — `PushTextureID`/
`PopTextureID` move outside the chunk loop (bracketing the whole bucket, unchanged in effect from
today, just now written around a loop instead of a single call):
```cpp
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
```
Expected size after this change: 73 → ~90-96 lines. Stays under the soft-100 ceiling; no split
required.

### 5. Line-count check against ARCH §1.5 (per the ARCH ruling's own instruction)
- `MapCanvas_IconLayer_DrawInternal_UI.h`: 36 → ~54. Under soft-100.
- NEW `MapCanvas_IconLayer_DrawChunk_UI.cpp`: ~20-25. Under soft-100.
- `MapCanvas_IconLayer_Draw_UI.cpp`: 117 → ~126-128. Already over soft-100 pre-existing (STEP53's
  own file); this ticket's addition must not push it past hard-150, and does not. Documented ARCH
  §1.5 exception carried forward, not newly introduced by this ticket.
- `MapCanvas_IconLayer_DrawCache_UI.cpp`: 73 → ~90-96. Stays under soft-100.
No file crosses the hard-150 ceiling; the "split into its own small file if either would cross the
ceiling" branch of the ARCH ruling is not triggered for the two consumer files, and the shared
helper is placed in its own new file anyway (§2) rather than growing either consumer further.

## Target files
- `src/ui/MapCanvas_IconLayer_DrawInternal_UI.h` (36 lines today) — §1.
- NEW `src/ui/MapCanvas_IconLayer_DrawChunk_UI.cpp` — §2.
- `src/ui/MapCanvas_IconLayer_Draw_UI.cpp` (117 lines today) — §3, `FlushIconLayerBucket` only.
- `src/ui/MapCanvas_IconLayer_DrawCache_UI.cpp` (73 lines today) — §4, `BuildLocalIndexPattern` and
  `ReplayCachedBuckets` only; `CaptureAndCacheBuckets` unedited.
- NEW `src/ui/MapCanvas_IconLayer_DrawChunk_UI_Test.cpp` — the new acceptance tests (see Acceptance
  test below). Kept out of `MapCanvas_IconLayer_Draw_UI_Test.cpp` (currently 111 lines, already
  near/over the soft-100 ceiling) rather than grown further; a new, separate TU stays inside
  Constitution §1.5, mirroring STEP53's own multi-file split precedent.
- `src/ui/MapCanvas_IconLayer_UI_Test.cpp` (28 lines today) — one new `extern`-style forward
  declaration (`void RunMapCanvasIconLayerDrawChunkChecks();`) and one new call in `main()`,
  alongside the existing four (mirrors the file's own documented pattern exactly).
- `CMakeLists.txt` — add `src/ui/MapCanvas_IconLayer_DrawChunk_UI.cpp` and
  `src/ui/MapCanvas_IconLayer_DrawChunk_UI_Test.cpp` to the existing
  `add_sangen_test(MapCanvas_IconLayer_UI_Test ...)` source list (lines 485-490, confirmed). No new
  `add_sangen_test(...)` block — this is additive to the existing correctness binary, not a new
  benchmark-style binary (unlike STEP59).

## Layer & accuracy class
UI. Accuracy class: **Visual**, inherited from STEP53's own classification of this whole draw pass
(screen-space presentation only, `OPTIMIZATION_PILLARS.md` pillar 15's exemption). Note: "Visual"
here means no CPU/GPU numeric-tolerance tradeoff applies to this pass — it does not mean rendering
a wrapped/corrupted index is an acceptable lossy approximation. Index-buffer integrity is a hard
structural-correctness bar (a wrapped `ImDrawIdx` produces genuinely wrong geometry, not a
softened one), independent of the accuracy-class label.

## Backend policy
CPU-only. `ImDrawList` lives entirely in the UI/imgui immediate-mode layer; there is no GPU compute
kernel here to dispatch, identical to STEP53's own Backend policy. This ticket adds no SIMD/
dispatch-backend decision — `ComputeIconLayerBucketChunks` is trivial scalar integer arithmetic
over at most a few dozen chunk entries per bucket (`quadCount / 16000`), not a hot inner loop
itself.

## ARCH rules invoked
- `ARCH_14_09_RenderingPerformance.md` lines 17-21 ("Atlas page bucketing is required... flush one
  draw command per non-empty bucket... bounds draw calls to O(pages touched this frame)") — this
  ticket's chunking keeps that O(pages) bound intact per the ARCH ruling above; it does not violate
  or reopen this clause, only clarifies (per the ARCH Expert's own pending, separate amendment,
  out-of-scope here) that an internal size-driven split of one oversized single-page bucket is
  distinct from the scatter-across-pages pathology the clause targets.
- Constitution §6 — "validate all input... to avoid crashes" — the direct basis for why
  `kIconLayerBucketChunkQuadCap` is a fixed `constexpr`, never a designer-reachable value.
- Constitution §7 — work-order schema (this document); the basis-tag law governing the performance
  estimate below.
- Constitution §8 — total tweakability — cited here specifically to record why this one constant is
  the deliberate exception: it is dictated by the vendored index type, not a creative/algorithmic
  dial, so §8 does not apply to it (ARCH ruling item 2, above).
- `ARCH_01_05_FileSizeCeilings.md` §1.5 — soft-100/hard-150 governs the file-split decisions in §5.

## Solution — performance estimate (basis)
**REASONED-PLACEHOLDER basis tag** (Constitution §7) — this is a correctness fix, not a
performance-motivated change, and no new benchmark run backs the estimate below; it is arithmetic
reasoning against the mechanism traced in "Root problem," not a measured number:
- **For every bucket at or under `kIconLayerBucketChunkQuadCap` (16,000 quads)** — the overwhelming
  common case at realistic per-page instance counts — this fix is a no-op: `ComputeIconLayerBucketChunks`
  returns exactly one chunk spanning the whole bucket, and the resulting `PrimReserve`/write-loop
  sequence is byte-identical to today's single-call path. Zero measurable cost.
- **For a bucket over the cap**, the added cost is `ceil(quadCount / 16000) - 1` extra
  `PrimReserve` calls (each O(1) beyond the vertex/index buffer `resize()` it was always going to
  do anyway, since the total reserved vertex/index count across all chunks equals what one call
  would have reserved) plus one extra `ImDrawCmd` entry per crossing of a 65,536-vertex boundary —
  the same O(pages-touched) draw-call bound `ARCH_14_09_RenderingPerformance.md` already accepts,
  now correctly realized instead of silently corrupting past it. `ComputeIconLayerBucketChunks`
  itself allocates one small `std::vector` of at most `ceil(450000 / 16000) ≈ 29` entries even at
  STEP53's entire cross-layer budget concentrated on one page (a pathological upper bound, not a
  realistic one) — negligible relative to the O(quadCount) vertex-write work the bucket was always
  going to do.
- This ticket does not claim a frame-time number and does not require STEP59's microbenchmark
  harness to be re-run — STEP59's own synthetic scenes already avoid the over-cap case entirely
  (their own stated per-page-size workaround), so this fix changes nothing STEP59 currently
  measures. If a future ticket wants an over-cap microbenchmark data point, that is a suggested
  follow-up extension to STEP59's harness, not required here.

## Acceptance test
1. **Chunk-range helper, pure/headless (`MapCanvas_IconLayer_DrawChunk_UI_Test.cpp`):**
   `ComputeIconLayerBucketChunks(0)` returns empty. For `totalQuadCount` in
   `{1, 16000, 16001, 32000, 20000}`: the returned range count equals exactly
   `ceil(totalQuadCount / 16000)`; every range's `quadCount <= kIconLayerBucketChunkQuadCap`; the
   ranges are contiguous and gapless (`ranges[i+1].quadStart == ranges[i].quadStart +
   ranges[i].quadCount`); the sum of all `quadCount` values equals `totalQuadCount` exactly.
2. **Live-path correctness (`FlushIconLayerBucket`, live headless imgui frame, mirroring
   `MapCanvas_Render_UI_Test.cpp`'s technique):** build one single-page synthetic bucket of exactly
   32,000 quads (an exact multiple of `kIconLayerBucketChunkQuadCap`, so the VtxOffset-bump
   crossing point is deterministic per the mechanism traced above — `_VtxCurrentIdx` sits at
   64,000 after the first chunk, and any second chunk's own vertex count reliably pushes
   `64,000 + vtxCount >= 65,536`), each instance given a distinct, identifiable
   `screenCenterX = static_cast<float>(quadIndex)` (`screenCenterY = 0`) so no two quads' geometry
   is indistinguishable. Flush through `BucketByAtlasPage` + `FlushIconLayerBucket`. Assert:
   - Exactly `ceil(32000 / 16000) = 2` distinct `ImDrawCmd` entries carry the bucket's texture id.
   - Total emitted `ImDrawVert` count equals exactly `32000 * 4`; total `ImDrawIdx` count equals
     exactly `32000 * 6` (mirrors STEP53's own existing acceptance-test wording, catches an
     accidental double-reserve or a collapse back to one unchunked call).
   - **The correctness assertion the bug demands:** for every one of the 32,000 quads, resolve its
     four vertices' absolute buffer positions as `cmd.VtxOffset + localIndexValue` for whichever
     `ImDrawCmd` covers that index's position in the index buffer, and confirm the four resolved
     vertices' `pos.x` values are exactly that quad's own unique `screenCenterX ± half` — i.e., no
     quad's geometry has collided with another quad's due to index wraparound. Before this fix,
     this assertion fails for quads at and beyond index 16,384 (verified above against the
     unchunked code's real behavior); this is the regression test that would have caught the
     original defect and must exist so it cannot silently regress again.
   - Repeat with 20,000 quads (`ceil(20000/16000) = 2`, an uneven 16,000+4,000 split) to confirm
     the ceiling-division boundary logic on a non-exact-multiple size, same position read-back
     check.
3. **Cache-path correctness (`CaptureAndCacheBuckets` + `ReplayCachedBuckets`, same test file):**
   build the identical 32,000-quad single-page synthetic bucket, run it through
   `CaptureAndCacheBuckets` into a fresh `IconLayerFrameCache`, then `ReplayCachedBuckets` into a
   second, independent `ImDrawList`. Assert the replayed draw list's per-quad position read-back
   (same technique as #2) matches for all 32,000 quads, proving `BuildLocalIndexPattern`'s
   chunk-relative local indices and `ReplayCachedBuckets`'s per-chunk `PrimReserve`+rebase stay in
   sync and neither wraps. Before this fix, `BuildLocalIndexPattern`'s `quad * 4` cast wraps
   unconditionally at `quad == 16384` regardless of whether the live path's own bug is separately
   exercised — this assertion catches that independently.
4. **No regression:** every existing `Check()` in `MapCanvas_IconLayer_Draw_UI_Test.cpp`,
   `MapCanvas_IconLayer_Cache_UI_Test.cpp`, `MapCanvas_IconLayer_Cull_UI_Test.cpp`,
   `MapCanvas_IconLayer_Budget_UI_Test.cpp` stay green unmodified — their existing bucket sizes are
   all well under `kIconLayerBucketChunkQuadCap`, so their observed output (draw-command counts,
   vertex/index totals) must be byte-identical to before this fix. `MapCanvas_IconLayer_Microbenchmark_UI_Test.cpp`
   and its two sibling scenario/frame-ops files stay green unmodified — untouched by this ticket
   (see Explicit out-of-scope).
5. Full `SanGenV2` build stays clean; `MapCanvas_IconLayer_UI_Test` (the correctness binary this
   ticket extends) passes with zero `Check()` failures, including the new
   `RunMapCanvasIconLayerDrawChunkChecks()`.

## Explicit out-of-scope
- **STEP59's test-only benchmark harness files** (`MapCanvas_IconLayer_Microbenchmark_UI_Test.cpp`,
  `..._MicrobenchmarkScenarios_UI_Test.cpp`, `..._MicrobenchmarkFrameOps_UI_Test.cpp`) — untouched.
  Their synthetic scenes already avoid the over-cap case via their own stated workarounds; this
  ticket does not rely on, remove, or extend those workarounds, and does not require re-running
  STEP59's measurements.
- **`OverlayRenderingSettings::visibleInstanceBudget`'s default (450,000)** — unrelated. That is a
  cross-layer, cross-page total; this ticket's fix operates strictly within a single already-bucketed
  page's own quad list, after bucketing has already occurred. Not touched, not re-tuned.
- **The `ARCH_14_09_RenderingPerformance.md` wording amendment** clarifying "one draw command per
  bucket" to explicitly permit an internal size-driven chunk split — the ARCH Expert's own separate
  follow-up (per the ruling obtained this session). This ticket edits no `ARCH_NN_*.md` file.
- **`BucketByAtlasPage`** — its per-page grouping logic is unaffected; chunking happens strictly
  inside one already-formed bucket's own flush/cache/replay path, never across bucket boundaries.
- **`DrawOverlayIconLayers`, `RebuildAndCache`, `ReplayAndRedrawSelection`, `SplitSelected`** (the
  orchestration functions in `MapCanvas_IconLayer_Draw_UI.cpp`) — none call `PrimReserve` directly;
  they only call the now-fixed `FlushIconLayerBucket`/`BucketByAtlasPage`, and need no edits.
- **`CaptureAndCacheBuckets`** — explicitly confirmed unedited (§4 above); it inherits correctness
  from its two now-fixed callees without needing its own change.
- **Making `kIconLayerBucketChunkQuadCap` a runtime-configurable/Constitution §8 tweakable** —
  explicitly forbidden by the ARCH ruling (item 2, above); it is a fixed constant tied to the
  vendored index type, not a design dial.
- **A codebase-wide audit for the same single-PrimReserve-per-oversized-batch pattern elsewhere**
  (e.g., any other future overlay layer type, or any other imgui bulk-vertex-write call site
  outside `MapCanvas_IconLayer_*`) — this ticket fixes the two confirmed instances named above only.
  A broader grep-audit for this defect class is a reasonable follow-up ticket, not performed here.
- **Any change to `imconfig.h` or switching to 32-bit `ImDrawIdx`** — out of scope; chunking under
  the existing 16-bit ceiling is the chosen fix per the ARCH ruling, not a vendored-library
  reconfiguration (which would also carry its own memory/bandwidth cost across every other imgui
  draw list in the application, an unrelated and much larger change).
