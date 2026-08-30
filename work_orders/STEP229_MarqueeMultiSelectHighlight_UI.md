# STEP229 — Marquee multi-select: widen the canvas highlight from the MRU primary to the whole selected set

**Layer:** UI. **Domain:** the screen-space overlay icon draw pass's culling/emit/cache trio
(`MapCanvas_IconLayer_Cull_UI.cpp`/`_CullEmit_UI.cpp`/`_Cache_UI.cpp`/`_Draw_UI.cpp`) plus the one
`MapCanvas_Draw_UI.cpp` wiring line that feeds it. **Executor:** SanGen Coder. Authored by the SanGen
UI Expert. Implements a gap the code and ARCH already named and deliberately deferred, not new law —
`MapCanvas_Draw_UI.cpp:105-106`'s own comment ("the draw pass still highlights only the PRIMARY;
widening it to the whole multi-select set is a visual-language ticket of its own, not this one") and
`ARCH_21_01_MultiSelectRepresentation.md` line 66 ("multi-instance canvas highlighting" scoped out as
"a separate, larger change"). The dispatching agent confirmed with the ARCH Expert this needs no new
ARCH ruling. ARCH §21.1/§21.2 are binding, unmodified law for this ticket — no ARCH file is touched.

## Summary
`ApplyMarqueeGesture` (`MapCanvas_SelectionGesture_UI.cpp:111-175`) already builds the full ordered
`OverlayInstanceKeySet_UI selectedInstanceKeys` correctly — every marker/prop/decal hit by the box
lands in it, in order (confirmed by reading the file; unaffected by this ticket). The bug is entirely
downstream, in the icon-layer draw pass: `MapCanvas_Draw_UI.cpp:108` narrows that whole set down to
one `PrimaryOfSelectionSet(selectedInstanceKeys)` key before it ever reaches the cull/emit code, and
`MapCanvas_IconLayer_CullEmit_UI.cpp:69-70` sets `OverlayVisibleInstance::bSelected` by strict
equality against that single key. Net effect: box-select N markers, and N-1 of them never report
`bSelected=true`.

**Important finding, confirmed by reading every consumer of `bSelected` in the icon-layer module**
(`MapCanvas_IconLayer_Draw_UI.cpp`, `_DrawCache_UI.cpp`, `_Budget_UI.cpp` — no `AddCircle`/`AddRect`/
ring/outline draw call exists anywhere in `src/ui` keyed on selection, confirmed by grep): `bSelected`
today has **no distinct visual rendering effect at all** — `FlushIconLayerBucket`
(`MapCanvas_IconLayer_Draw_UI.cpp`) draws a selected and non-selected icon pixel-identically. Its only
two functional effects, both already implemented and both already correct at the *set* level once fed
a correct per-instance flag, are: (1) `MapCanvas_IconLayer_Budget_UI.cpp:19`'s "never clustered or
capped away" decimation exemption, and (2) the C2 cache split (`SplitSelected`,
`MapCanvas_IconLayer_Draw_UI.cpp:20-26`) that keeps every selected instance out of the cached bytes and
redraws it live every frame. This ticket is therefore a **data-correctness fix** to the shared
`bSelected` flag every present-and-future consumer reads — verified the same way every existing test in
this module already verifies it (`candidate.bSelected` inspected directly, e.g.
`CheckIndexSpaceCollisionRegressionClosed`), not by inspecting rendered pixels (there is nothing yet to
distinguish visually — a separate, later ticket if/when a selection ring or tint is designed). A real
side benefit: once this ships, budget decimation also correctly protects *every* selected instance
during a large marquee, not just the primary — `MapCanvas_IconLayer_Budget_UI.cpp` needs no change to
get this; it already reads `bSelected` per-instance.

### Design
`DrawOverlayIconLayersInput::selectedInstanceKey` (a single `OverlayInstanceKey_UI`) becomes
`selectedInstanceKeys` — a push-in **pointer** to the whole `OverlayInstanceKeySet_UI` (matching this
struct's own established "push-in pointer, null = no source wired" convention for every other field,
e.g. `markerTypeVisibility`). `MapCanvas_Draw_UI.cpp` points it at its own `selectedInstanceKeys` member
(`MapCanvas_UI.h:377`) unconditionally — no more `if (HasSelection())` guard, since an empty set is
already the correct "nothing selected" value every downstream consumer treats as a no-op.

`SelectionSetContains(const OverlayInstanceKeySet_UI&, const OverlayInstanceKey_UI&)` already exists
and is implemented (`MapCanvas_SelectionSet_UI.h:23`/`.cpp:12-16`, confirmed by reading both files) —
this ticket reuses it for `bSelected` computation rather than reinventing set membership, per the
dispatching agent's own instruction.

Per-call-site widening decision inside `MapCanvas_IconLayer_Cull_UI.cpp` (the dispatching agent's own
instruction — don't blanket-convert without checking what each consumer is actually for):
- `AppendCandidate`'s `bSelected` computation (`CullEmit_UI.cpp`) — **widens** to set membership. This
  is the actual bug.
- The C2 cache's invalidation key and replay-redraw path (`ShouldInvalidateIconLayerCache`,
  `BeginIconLayerCacheBuild`, `ResolveSelectedInstanceCandidate`) — **widen**. All three exist purely to
  serve the icon-layer draw pass's own C2 contract (§14.8), not any camera-follow/list-scroll-to concept.
- `MapCanvas::SelectedEntityIdentifier()`/`HasSelection()` (`MapCanvas_UI.h:242-245`) — confirmed by
  reading `MapCanvas_UI.h` fully — these already read `PrimaryOfSelectionSet(selectedInstanceKeys)`
  directly, **not** through `DrawOverlayIconLayersInput` at all. They are the genuine "jump to
  primary"/camera-follow/list-scroll-to-selected surface the dispatching agent flagged as a concern —
  confirmed **completely untouched** by this ticket; no other call site in `src/ui` reads
  `DrawOverlayIconLayersInput::selectedInstanceKey(s)` for anything but the icon-layer module's own
  internal cull/cache/draw pipeline (grepped exhaustively — see Key files read/cited).

`IconLayerFrameCache::cachedSelectionKey` (single key, `MapCanvas_IconLayer_UI.h:99`) becomes
`cachedSelectionKeys` — a raw `std::vector<OverlayInstanceKey_UI>`, **not** the `OverlayInstanceKeySet_UI`
wrapper type. This is a deliberate, documented choice (Interpretation call 1): `MapCanvas_IconLayer_UI.h`
is the icon-layer module's shared leaf value-types header (its own comment: "the pure per-instance/
per-bucket value types... shares across its whole split") and currently includes nothing but
`<cstdint>`/`<string>`/`<vector>`. `MapCanvas_SelectionSet_UI.h` itself `#include`s
`MapCanvas_IconLayer_UI.h` (for `OverlayInstanceKey_UI`) — the reverse include would be a genuine
circular structural dependency. Storing a raw vector avoids it entirely at zero functional cost:
`MapCanvas_IconLayer_Cache_UI.cpp` (a `.cpp`, free to include anything) wraps it in a temporary
`OverlayInstanceKeySet_UI{...}` at its two call sites and reuses the canonical, already-implemented
`SelectionSetsEqual` (order-sensitive, `MapCanvas_SelectionSet_UI.h:28`) for the invalidation compare.

## Required reading
`ARCH_21_01_MultiSelectRepresentation.md` (line 66's own deferral, and the ordered-set/"primary is the
last element" contract this ticket does not change), `ARCH_14_09_RenderingPerformance.md` §14.9 (the
C2 "never clustered/capped away" and interaction-scoped-redraw contracts `bSelected` already serves and
this ticket widens correctly), and `ARCH_19_25_SelectionRepresentationUnification.md` (the
`bManual`-tagged key shape this ticket's widened loop iterates unchanged).

## Session coordination (required before EVERY file edit, not just once at ticket start)
Multiple Claude Code sessions are active on this machine, editing this SAME working directory (no
per-ticket worktrees). A single check at the start of this ticket is NOT sufficient — this ticket
touches 11 files and may span a long working session; a peer can start editing any one of them at any
point after your initial check. Before EACH individual file edit in §1-11 below (not just once,
up front):
1. Call `ListAgents` to enumerate active/open peer sessions on this machine.
2. Message each one (`SendMessage`) naming the SPECIFIC file you are about to edit right now, asking
   if they are currently editing it or planning to.
3. Wait for replies before making that edit.
4. If a peer reports current or planned work in that exact file, do NOT edit concurrently — negotiate
   a sequential order (whichever session is further along lands and merges first; the other rebases
   onto that afterward) and record the agreed order in this ticket's own notes before proceeding.
5. If no peer claims that file, proceed with that one edit — then repeat steps 1-4 for the NEXT file
   before editing it. A "no conflict" answer for file A is not a "no conflict" answer for file B, and
   an answer from ten minutes ago is not an answer for right now — re-check per file, every time.

**Pre-check already completed once, at ticket-authoring time (a starting point only, not a substitute
for the per-file checks above):** the dispatching agent confirmed with `map-generator-12` — its only
in-flight edit is `src/ui/MapCanvas_AreaDraw_UI.cpp`, unrelated to this ticket's file list, no
conflict. This is now a stale, point-in-time, whole-ticket-level check — the coder must still re-run
steps 1-5 above before touching each individual file, not just once before starting the ticket.

**Sibling-ticket note:** `STEP230` (marquee Ctrl-toggle/Shift-union fix) touches
`src/ui/MapCanvas_SelectionSet_UI.h`/`.cpp` to add `ToggleEachInSelectionSet`. This ticket only *reads*
that file's existing `SelectionSetContains`/`SelectionSetsEqual` and does not edit it — the two tickets
are additive/disjoint on that file regardless of landing order.

## Files touched
`src/ui/MapCanvas_IconLayer_Ops_UI.h`, `src/ui/MapCanvas_IconLayer_UI.h`,
`src/ui/MapCanvas_IconLayer_CullInternal_UI.h`, `src/ui/MapCanvas_IconLayer_CullEmit_UI.cpp`,
`src/ui/MapCanvas_IconLayer_Cull_UI.cpp`, `src/ui/MapCanvas_IconLayer_Cache_UI.cpp`,
`src/ui/MapCanvas_IconLayer_Draw_UI.cpp`, `src/ui/MapCanvas_Draw_UI.cpp`,
`src/ui/MapCanvas_IconLayer_Cull_UI_Test.cpp`, `src/ui/MapCanvas_IconLayer_Cache_UI_Test.cpp`,
`src/ui/MapCanvas_IconLayer_DrawChunkCache_UI_Test.cpp`. No `CMakeLists.txt` change — no new
translation unit is added; every file above already exists and is already wired into its test target
(`MapCanvas_IconLayer_UI_Test`, confirmed via `CMakeLists.txt:609-621`).
`src/ui/MapCanvas_SelectionSet_UI.h`/`.cpp` were read and confirmed to need **no change** —
`SelectionSetContains`/`SelectionSetsEqual` already exist and are already correctly implemented.

---

## 1. Modified: `src/ui/MapCanvas_IconLayer_Ops_UI.h`

Add the include, right after the existing one (currently line 7):
```cpp
#pragma once
#include "MapCanvas_IconLayer_UI.h"
```
becomes
```cpp
#pragma once
#include "MapCanvas_IconLayer_UI.h"
// STEP229 — OverlayInstanceKeySet_UI, for the widened selectedInstanceKeys field below. No cycle:
// MapCanvas_UI.h already includes both this header and MapCanvas_SelectionSet_UI.h directly and
// side-by-side (MapCanvas_UI.h:34,37), and MapCanvas_SelectionSet_UI.h itself never includes this
// header's own module (MapCanvas_IconLayer_Ops_UI.h) — confirmed by reading both files fresh.
#include "MapCanvas_SelectionSet_UI.h"
```

Change the struct field (currently line 59, inside `DrawOverlayIconLayersInput`):
```cpp
    float regionOriginX = 0.0f, regionOriginY = 0.0f, regionSidePixels = 0.0f;
    OverlayInstanceKey_UI selectedInstanceKey;
```
to
```cpp
    float regionOriginX = 0.0f, regionOriginY = 0.0f, regionSidePixels = 0.0f;
    // STEP229 — ARCH §21.1's own deferred "multi-instance canvas highlighting" ticket, now
    // implemented: a push-in pointer to the WHOLE ordered multi-select set (mirrors this struct's own
    // every-other-source convention — null = no selection source wired, never a crash), not a single
    // primary key. MapCanvas_Draw_UI.cpp points this at its own `selectedInstanceKeys` member
    // (MapCanvas_UI.h:377) unconditionally, empty set included.
    const OverlayInstanceKeySet_UI*     selectedInstanceKeys   = nullptr;
```

Change the two cache-primitive declarations (currently lines 79-84):
```cpp
bool ShouldInvalidateIconLayerCache(const IconLayerFrameCache& cache, float viewCenterPixelX,
                                    float viewCenterPixelY, float zoomScale,
                                    const OverlayInstanceKey_UI& selection, std::uint64_t layerSettingsRevision);
void BeginIconLayerCacheBuild(IconLayerFrameCache& cache, float viewCenterPixelX, float viewCenterPixelY,
                              float zoomScale, const OverlayInstanceKey_UI& selection,
                              std::uint64_t layerSettingsRevision);
```
to
```cpp
bool ShouldInvalidateIconLayerCache(const IconLayerFrameCache& cache, float viewCenterPixelX,
                                    float viewCenterPixelY, float zoomScale,
                                    const OverlayInstanceKeySet_UI& selection, std::uint64_t layerSettingsRevision);
void BeginIconLayerCacheBuild(IconLayerFrameCache& cache, float viewCenterPixelX, float viewCenterPixelY,
                              float zoomScale, const OverlayInstanceKeySet_UI& selection,
                              std::uint64_t layerSettingsRevision);
```

---

## 2. Modified: `src/ui/MapCanvas_IconLayer_UI.h`

Change the cache struct's own field (currently line 99):
```cpp
struct IconLayerFrameCache {
    std::vector<unsigned char>   cachedVertexBytes;    // raw ImDrawVert bytes, non-selected only
    std::vector<unsigned char>   cachedIndexBytes;      // ImDrawIdx values, LOCAL to each bucket
    std::vector<CachedIconLayerBucketLayout_UI> cachedBucketLayout;   // for replay
    bool bValid = false;
    float cachedViewCenterPixelX = 0.0f, cachedViewCenterPixelY = 0.0f, cachedZoomScale = 0.0f;
    OverlayInstanceKey_UI cachedSelectionKey;
    std::uint64_t cachedLayerSettingsRevision = 0;
};
```
to
```cpp
struct IconLayerFrameCache {
    std::vector<unsigned char>   cachedVertexBytes;    // raw ImDrawVert bytes, non-selected only
    std::vector<unsigned char>   cachedIndexBytes;      // ImDrawIdx values, LOCAL to each bucket
    std::vector<CachedIconLayerBucketLayout_UI> cachedBucketLayout;   // for replay
    bool bValid = false;
    float cachedViewCenterPixelX = 0.0f, cachedViewCenterPixelY = 0.0f, cachedZoomScale = 0.0f;
    // STEP229 — order-sensitive snapshot of the WHOLE selected set, not just the primary. A raw
    // vector, deliberately NOT the OverlayInstanceKeySet_UI wrapper type: this header is the
    // icon-layer module's own shared leaf value-types header (see this file's own top comment) and
    // stays free of MapCanvas_SelectionSet_UI.h's own include, which itself includes THIS header —
    // pulling it in here would be a genuine circular structural dependency. MapCanvas_IconLayer_Cache_UI.cpp
    // (a .cpp, free to include anything) wraps this in a temporary OverlayInstanceKeySet_UI at its own
    // two call sites and compares it via the canonical SelectionSetsEqual.
    std::vector<OverlayInstanceKey_UI> cachedSelectionKeys;
    std::uint64_t cachedLayerSettingsRevision = 0;
};
```

---

## 3. Modified: `src/ui/MapCanvas_IconLayer_CullInternal_UI.h`

Doc-comment only — `ResolveSelectedInstanceCandidate`'s own signature is unchanged; its contract
widens. Currently (lines 33-38):
```cpp
// §4's "run steps 1-3 fresh for only the selected instance(s)" replay-frame path — a cheap,
// picker-scoped lookup (today: Markers only, STEP48), never an O(N) re-walk of every candidate.
// Appends at most one instance; returns false (appends nothing) if there is no valid selection, the
// selected instance's owning layer/sub-layer is disabled, or its collection has no picker yet.
bool ResolveSelectedInstanceCandidate(const DrawOverlayIconLayersInput& input,
                                       std::vector<OverlayVisibleInstance>& outCandidates);
```
Replace with:
```cpp
// §4's "run steps 1-3 fresh for only the selected instance(s)" replay-frame path — a cheap,
// picker-scoped lookup (today: Markers only, STEP48), never an O(N) re-walk of every candidate.
// STEP229 — widened from "at most one" (the old single-primary-key contract) to "one per
// Markers-collection key in the whole `selectedInstanceKeys` set" (ARCH §21.1): appends zero, one, or
// many instances. A Props/Decals key in the set is still skipped (no picker yet, STEP48, unchanged
// restriction) exactly as a Props/Decals PRIMARY already was, silently, before this ticket. Returns
// true iff at least one key resolved.
bool ResolveSelectedInstanceCandidate(const DrawOverlayIconLayersInput& input,
                                       std::vector<OverlayVisibleInstance>& outCandidates);
```

---

## 4. Modified: `src/ui/MapCanvas_IconLayer_CullEmit_UI.cpp`

Add the include (after the existing block, currently ending line 11):
```cpp
#include "MapCanvas_IconLayer_CullInternal_UI.h"
#include "IconAtlasPairing_UI.h"
#include "IconGridWidget_UI.h"
#include "MapCanvasView_UI.h"
#include "PreviewComposite_UI.h"
#include "../io/WorldFootprintSizeTable_IO.h"
```
becomes
```cpp
#include "MapCanvas_IconLayer_CullInternal_UI.h"
#include "IconAtlasPairing_UI.h"
#include "IconGridWidget_UI.h"
#include "MapCanvasView_UI.h"
#include "MapCanvas_SelectionSet_UI.h"   // STEP229 — SelectionSetContains
#include "PreviewComposite_UI.h"
#include "../io/WorldFootprintSizeTable_IO.h"
```

Change `AppendCandidate`'s `bSelected` computation (currently lines 68-71):
```cpp
    instance.instanceKey = OverlayInstanceKey_UI{collection, instanceIndex, true, bManual};   // ARCH §19.25
    instance.bSelected = input.selectedInstanceKey.bValid
                       && OverlayInstanceKeysEqual(instance.instanceKey, input.selectedInstanceKey);
    outCandidates.push_back(instance);
```
to
```cpp
    instance.instanceKey = OverlayInstanceKey_UI{collection, instanceIndex, true, bManual};   // ARCH §19.25
    // STEP229 — ARCH §21.1's deferred "multi-instance canvas highlighting" ticket: membership in the
    // WHOLE ordered multi-select set, not equality against one primary key. `selectedInstanceKeys` is
    // a push-in pointer (this struct's own established convention) — null means no selection source
    // was wired (mirrors every other null-safe pointer in DrawOverlayIconLayersInput), never a crash.
    instance.bSelected = input.selectedInstanceKeys != nullptr
                       && SelectionSetContains(*input.selectedInstanceKeys, instance.instanceKey);
    outCandidates.push_back(instance);
```

---

## 5. Modified: `src/ui/MapCanvas_IconLayer_Cull_UI.cpp`

Replace the whole block from the anonymous-namespace `ResolveSelectedManualMarkerCandidate` through
the end of the public `ResolveSelectedInstanceCandidate` (currently lines 110-186 — verbatim):
```cpp
namespace {

// ARCH §19.25 — the manual-marker half of ResolveSelectedInstanceCandidate: `input.selectedInstanceKey.
// instanceIndex` is a MarkerTransform::instanceIdentifier here, NOT a Data::PlacementInstances array
// position (the procedural branch below's own assumption), so it cannot walk `input.placements->
// markers` — it must re-run the same manual-marker resolution ResolveMarkersManual already performs
// for a full cull pass, scoped to the ONE selected instanceIdentifier. The viewRect passed is
// deliberately unrestricted (not `ComputeViewWorldRect`, which this replay-frame path never
// recomputes): the selected instance keeps rendering even if the view moved without invalidating the
// cache, mirroring the procedural branch's own "no view-membership re-test" behavior below.
bool ResolveSelectedManualMarkerCandidate(const DrawOverlayIconLayersInput& input,
                                          std::vector<OverlayVisibleInstance>& outCandidates) {
    if (input.overlayLayerSettings == nullptr || input.recipe == nullptr) return false;
    const int targetInstanceIdentifier = input.selectedInstanceKey.instanceIndex;
    const ViewWorldRect_UI unrestrictedRect{
        std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::max(),    std::numeric_limits<float>::max() };
    const std::vector<OverlayLayer_UI>& layers = input.overlayLayerSettings->overlayLayers;
    for (std::size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex) {
        const OverlayLayer_UI& layer = layers[layerIndex];
        if (!layer.bEnabled || (layer.domainKind != OverlayDomainKind_UI::Alloy
                              && layer.domainKind != OverlayDomainKind_UI::SpawnsArmies))
            continue;
        for (const OverlaySubLayerRef_UI& subLayerRef : layer.subLayers) {
            if (!subLayerRef.bEnabled || subLayerRef.kind == OverlaySubLayerKind_UI::ProceduralRule) continue;
            int stableOrderCounter = 0;
            ResolveMarkersManual(input, layer, static_cast<int>(layerIndex), subLayerRef.index,
                                 &stableOrderCounter, nullptr, &unrestrictedRect, nullptr, outCandidates,
                                 &targetInstanceIdentifier);
            if (!outCandidates.empty()) return true;
        }
    }
    return false;
}

} // namespace

bool ResolveSelectedInstanceCandidate(const DrawOverlayIconLayersInput& input,
                                      std::vector<OverlayVisibleInstance>& outCandidates) {
    if (!input.selectedInstanceKey.bValid || input.overlayLayerSettings == nullptr || input.placements == nullptr)
        return false;
    if (input.selectedInstanceKey.collection != PlacementCollectionKind_UI::Markers)
        return false;   // no other domain has a working picker yet (STEP48)
    if (input.selectedInstanceKey.bManual)
        return ResolveSelectedManualMarkerCandidate(input, outCandidates);   // ARCH §19.25
    const Data::PlacementInstances& markers = input.placements->markers;
    const std::int32_t instanceIndex = input.selectedInstanceKey.instanceIndex;
    if (instanceIndex < 0 || static_cast<std::size_t>(instanceIndex) >= markers.Count()) return false;
    const int ruleIndex = markers.ruleIndex[static_cast<std::size_t>(instanceIndex)];
    const std::vector<OverlayLayer_UI>& layers = input.overlayLayerSettings->overlayLayers;
    for (std::size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex) {
        const OverlayLayer_UI& layer = layers[layerIndex];
        if (!layer.bEnabled || (layer.domainKind != OverlayDomainKind_UI::Alloy
                              && layer.domainKind != OverlayDomainKind_UI::SpawnsArmies))
            continue;
        for (const OverlaySubLayerRef_UI& subLayerRef : layer.subLayers) {
            if (!subLayerRef.bEnabled || subLayerRef.kind != OverlaySubLayerKind_UI::ProceduralRule
                || subLayerRef.index != ruleIndex)
                continue;
            const std::size_t index = static_cast<std::size_t>(instanceIndex);
            const std::string templateIdentifier = TemplateIdentifierToString8(markers.templateIdentifier[index].characters);
            float tintRed = 1.0f, tintGreen = 1.0f, tintBlue = 1.0f;
            if (input.recipe != nullptr) {
                const Params::MarkerCategory category =
                    static_cast<Params::MarkerCategory>(markers.category[index]);
                ResolveMarkerCategoryTintColor(category, input.recipe->globalMarkerSettings, tintRed, tintGreen, tintBlue);
            }
            int stableOrderCounter = 0;
            EmitCandidateIfVisible(input, layer, static_cast<int>(layerIndex), templateIdentifier,
                                   markers.positionX[index], markers.positionZ[index], markers.scaleX[index],
                                   PlacementCollectionKind_UI::Markers, instanceIndex, tintRed, tintGreen, tintBlue,
                                   &stableOrderCounter, nullptr, outCandidates);
            return !outCandidates.empty();
        }
    }
    return false;
}
```
with:
```cpp
namespace {

// ARCH §19.25 — the manual-marker half of ResolveSelectedInstanceCandidate: `key.instanceIndex` is a
// MarkerTransform::instanceIdentifier here, NOT a Data::PlacementInstances array position (the
// procedural helper's own assumption below), so it cannot walk `input.placements->markers` — it must
// re-run the same manual-marker resolution ResolveMarkersManual already performs for a full cull
// pass, scoped to the ONE instanceIdentifier `key` names. The viewRect passed is deliberately
// unrestricted (not `ComputeViewWorldRect`, which this replay-frame path never recomputes): the
// selected instance keeps rendering even if the view moved without invalidating the cache, mirroring
// the procedural helper's own "no view-membership re-test" behavior below.
// STEP229 — scoped to an explicit `key` parameter (was `input.selectedInstanceKey` read directly) so
// the widened multi-select loop below can call this once per manual key in the selected set. The
// "did THIS call find a match" test switched from `!outCandidates.empty()` to a before/after size
// delta — with the loop now accumulating across multiple keys into the SAME outCandidates vector, the
// old empty-check would spuriously report "found" on the second and later keys regardless of whether
// this particular call actually appended anything.
bool ResolveSelectedManualMarkerCandidate(const DrawOverlayIconLayersInput& input,
                                          const OverlayInstanceKey_UI& key,
                                          std::vector<OverlayVisibleInstance>& outCandidates) {
    if (input.overlayLayerSettings == nullptr || input.recipe == nullptr) return false;
    const int targetInstanceIdentifier = key.instanceIndex;
    const ViewWorldRect_UI unrestrictedRect{
        std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::max(),    std::numeric_limits<float>::max() };
    const std::vector<OverlayLayer_UI>& layers = input.overlayLayerSettings->overlayLayers;
    const std::size_t candidateCountBefore = outCandidates.size();
    for (std::size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex) {
        const OverlayLayer_UI& layer = layers[layerIndex];
        if (!layer.bEnabled || (layer.domainKind != OverlayDomainKind_UI::Alloy
                              && layer.domainKind != OverlayDomainKind_UI::SpawnsArmies))
            continue;
        for (const OverlaySubLayerRef_UI& subLayerRef : layer.subLayers) {
            if (!subLayerRef.bEnabled || subLayerRef.kind == OverlaySubLayerKind_UI::ProceduralRule) continue;
            int stableOrderCounter = 0;
            ResolveMarkersManual(input, layer, static_cast<int>(layerIndex), subLayerRef.index,
                                 &stableOrderCounter, nullptr, &unrestrictedRect, nullptr, outCandidates,
                                 &targetInstanceIdentifier);
            if (outCandidates.size() > candidateCountBefore) return true;
        }
    }
    return false;
}

// The procedural half of ResolveSelectedInstanceCandidate, extracted unchanged in behavior (STEP229)
// so the widened multi-select loop below can call it once per procedural key in the selected set.
// `key` must already be known bValid/collection==Markers/bManual==false by the caller. Same
// before/after size-delta reasoning as the manual helper above for the multi-key accumulation case.
bool ResolveSelectedProceduralMarkerCandidate(const DrawOverlayIconLayersInput& input,
                                              const OverlayInstanceKey_UI& key,
                                              std::vector<OverlayVisibleInstance>& outCandidates) {
    if (input.placements == nullptr) return false;
    const Data::PlacementInstances& markers = input.placements->markers;
    const std::int32_t instanceIndex = key.instanceIndex;
    if (instanceIndex < 0 || static_cast<std::size_t>(instanceIndex) >= markers.Count()) return false;
    const int ruleIndex = markers.ruleIndex[static_cast<std::size_t>(instanceIndex)];
    const std::vector<OverlayLayer_UI>& layers = input.overlayLayerSettings->overlayLayers;
    for (std::size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex) {
        const OverlayLayer_UI& layer = layers[layerIndex];
        if (!layer.bEnabled || (layer.domainKind != OverlayDomainKind_UI::Alloy
                              && layer.domainKind != OverlayDomainKind_UI::SpawnsArmies))
            continue;
        for (const OverlaySubLayerRef_UI& subLayerRef : layer.subLayers) {
            if (!subLayerRef.bEnabled || subLayerRef.kind != OverlaySubLayerKind_UI::ProceduralRule
                || subLayerRef.index != ruleIndex)
                continue;
            const std::size_t index = static_cast<std::size_t>(instanceIndex);
            const std::string templateIdentifier = TemplateIdentifierToString8(markers.templateIdentifier[index].characters);
            float tintRed = 1.0f, tintGreen = 1.0f, tintBlue = 1.0f;
            if (input.recipe != nullptr) {
                const Params::MarkerCategory category =
                    static_cast<Params::MarkerCategory>(markers.category[index]);
                ResolveMarkerCategoryTintColor(category, input.recipe->globalMarkerSettings, tintRed, tintGreen, tintBlue);
            }
            int stableOrderCounter = 0;
            const std::size_t candidateCountBefore = outCandidates.size();
            EmitCandidateIfVisible(input, layer, static_cast<int>(layerIndex), templateIdentifier,
                                   markers.positionX[index], markers.positionZ[index], markers.scaleX[index],
                                   PlacementCollectionKind_UI::Markers, instanceIndex, tintRed, tintGreen, tintBlue,
                                   &stableOrderCounter, nullptr, outCandidates);
            return outCandidates.size() > candidateCountBefore;
        }
    }
    return false;
}

} // namespace

// STEP229 — widened from "resolve the one primary selected instance" to "resolve every selected
// instance in the whole ordered set" (ARCH §21.1's own deferred "multi-instance canvas highlighting"
// ticket). Still Markers-only (`no other domain has a working picker yet, STEP48` — unchanged,
// pre-existing restriction, not this ticket's job to lift): a Props/Decals key in the set is simply
// skipped here exactly as a Props/Decals PRIMARY was already silently skipped before this ticket —
// see this ticket's own "Explicit out-of-scope" for why that pre-existing, adjacent gap is not fixed.
bool ResolveSelectedInstanceCandidate(const DrawOverlayIconLayersInput& input,
                                      std::vector<OverlayVisibleInstance>& outCandidates) {
    if (input.selectedInstanceKeys == nullptr || input.overlayLayerSettings == nullptr
        || input.placements == nullptr)
        return false;
    bool bResolvedAny = false;
    for (const OverlayInstanceKey_UI& key : input.selectedInstanceKeys->keys) {
        if (!key.bValid || key.collection != PlacementCollectionKind_UI::Markers)
            continue;   // no other domain has a working picker yet (STEP48) — unchanged restriction
        const bool bResolved = key.bManual
            ? ResolveSelectedManualMarkerCandidate(input, key, outCandidates)
            : ResolveSelectedProceduralMarkerCandidate(input, key, outCandidates);
        bResolvedAny = bResolvedAny || bResolved;
    }
    return bResolvedAny;
}
```

---

## 6. Modified: `src/ui/MapCanvas_IconLayer_Cache_UI.cpp`

Add the include (currently lines 10-11):
```cpp
#include "MapCanvas_IconLayer_Ops_UI.h"
#include <cstring>
```
becomes
```cpp
#include "MapCanvas_IconLayer_Ops_UI.h"
#include "MapCanvas_SelectionSet_UI.h"   // STEP229 — SelectionSetsEqual
#include <cstring>
```

Replace the whole invalidation/build pair (currently lines 24-48):
```cpp
// §4's four invalidation triggers, restated: pan (view center), zoom (zoomScale), selection
// change, or any overlay layer-setting change (the monotonic revision counter). LOD
// threshold-crossing needs no separate rule — it only ever happens as zoom changes, which already
// invalidates unconditionally.
bool ShouldInvalidateIconLayerCache(const IconLayerFrameCache& cache, float viewCenterPixelX,
                                    float viewCenterPixelY, float zoomScale,
                                    const OverlayInstanceKey_UI& selection, std::uint64_t layerSettingsRevision) {
    if (!cache.bValid) return true;
    if (!NearlyEqual(cache.cachedViewCenterPixelX, viewCenterPixelX)
        || !NearlyEqual(cache.cachedViewCenterPixelY, viewCenterPixelY)
        || !NearlyEqual(cache.cachedZoomScale, zoomScale))
        return true;
    if (!OverlayInstanceKeysEqual(cache.cachedSelectionKey, selection)) return true;
    return cache.cachedLayerSettingsRevision != layerSettingsRevision;
}

void BeginIconLayerCacheBuild(IconLayerFrameCache& cache, float viewCenterPixelX, float viewCenterPixelY,
                              float zoomScale, const OverlayInstanceKey_UI& selection,
                              std::uint64_t layerSettingsRevision) {
    cache.cachedVertexBytes.clear();
    cache.cachedIndexBytes.clear();
    cache.cachedBucketLayout.clear();
    cache.cachedViewCenterPixelX = viewCenterPixelX;
    cache.cachedViewCenterPixelY = viewCenterPixelY;
    cache.cachedZoomScale = zoomScale;
    cache.cachedSelectionKey = selection;
    cache.cachedLayerSettingsRevision = layerSettingsRevision;
    cache.bValid = true;
}
```
with:
```cpp
// §4's four invalidation triggers, restated: pan (view center), zoom (zoomScale), selection
// change, or any overlay layer-setting change (the monotonic revision counter). LOD
// threshold-crossing needs no separate rule — it only ever happens as zoom changes, which already
// invalidates unconditionally.
bool ShouldInvalidateIconLayerCache(const IconLayerFrameCache& cache, float viewCenterPixelX,
                                    float viewCenterPixelY, float zoomScale,
                                    const OverlayInstanceKeySet_UI& selection, std::uint64_t layerSettingsRevision) {
    if (!cache.bValid) return true;
    if (!NearlyEqual(cache.cachedViewCenterPixelX, viewCenterPixelX)
        || !NearlyEqual(cache.cachedViewCenterPixelY, viewCenterPixelY)
        || !NearlyEqual(cache.cachedZoomScale, zoomScale))
        return true;
    // STEP229 — order-sensitive WHOLE-SET comparison (SelectionSetsEqual, MapCanvas_SelectionSet_UI.h)
    // replaces the old single-key OverlayInstanceKeysEqual check: a set that gained, lost, or
    // reordered ANY member (not just the primary) must invalidate — otherwise a Ctrl-click that
    // removes a non-primary member would leave that member's now-stale bytes sitting untouched in a
    // cache that still believes nothing changed.
    if (!SelectionSetsEqual(OverlayInstanceKeySet_UI{cache.cachedSelectionKeys}, selection)) return true;
    return cache.cachedLayerSettingsRevision != layerSettingsRevision;
}

void BeginIconLayerCacheBuild(IconLayerFrameCache& cache, float viewCenterPixelX, float viewCenterPixelY,
                              float zoomScale, const OverlayInstanceKeySet_UI& selection,
                              std::uint64_t layerSettingsRevision) {
    cache.cachedVertexBytes.clear();
    cache.cachedIndexBytes.clear();
    cache.cachedBucketLayout.clear();
    cache.cachedViewCenterPixelX = viewCenterPixelX;
    cache.cachedViewCenterPixelY = viewCenterPixelY;
    cache.cachedZoomScale = zoomScale;
    cache.cachedSelectionKeys = selection.keys;
    cache.cachedLayerSettingsRevision = layerSettingsRevision;
    cache.bValid = true;
}
```

---

## 7. Modified: `src/ui/MapCanvas_IconLayer_Draw_UI.cpp`

Insert a new helper right after `SplitSelected` (currently lines 20-26), before `RebuildAndCache`:
```cpp
void SplitSelected(const std::vector<OverlayVisibleInstance>& budgeted,
                   std::vector<OverlayVisibleInstance>& outNonSelected,
                   std::vector<OverlayVisibleInstance>& outSelected) {
    outNonSelected.reserve(budgeted.size());
    for (const OverlayVisibleInstance& instance : budgeted)
        (instance.bSelected ? outSelected : outNonSelected).push_back(instance);
}
```
becomes
```cpp
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
```

Change the one `BeginIconLayerCacheBuild` call inside `RebuildAndCache` (currently line 42):
```cpp
    BeginIconLayerCacheBuild(frameCache, viewCenterX, viewCenterY, zoomScale, input.selectedInstanceKey, revision);
```
to
```cpp
    BeginIconLayerCacheBuild(frameCache, viewCenterX, viewCenterY, zoomScale,
                             ResolveSelectionSetOrEmpty(input), revision);
```

Change the one `ShouldInvalidateIconLayerCache` call inside `DrawOverlayIconLayers` (currently lines
121-122):
```cpp
    if (ShouldInvalidateIconLayerCache(frameCache, viewCenterX, viewCenterY, zoomScale,
                                       input.selectedInstanceKey, revision)) {
```
to
```cpp
    if (ShouldInvalidateIconLayerCache(frameCache, viewCenterX, viewCenterY, zoomScale,
                                       ResolveSelectionSetOrEmpty(input), revision)) {
```

`ReplayAndRedrawSelection` (lines 49-55) is **unchanged** — it already just calls
`ResolveSelectedInstanceCandidate(input, selectedOnly);`, whose own widening (§5 above) is transparent
to this caller. `OverlayInstanceKeySet_UI` is already visible in this TU transitively
(`MapCanvas_IconLayer_DrawInternal_UI.h` → `MapCanvas_IconLayer_CullInternal_UI.h` →
`MapCanvas_IconLayer_Ops_UI.h` → `MapCanvas_SelectionSet_UI.h`, confirmed by reading the include chain)
— no new `#include` needed in this file.

---

## 8. Modified: `src/ui/MapCanvas_Draw_UI.cpp`

Currently (lines 103-108):
```cpp
    // ARCH §19.25 — `selectedInstanceKey` IS the canonical key now (procedural or manual, correctly
    // tagged `bManual`); no longer reconstructed from a bare entity id, which could only ever
    // represent the procedural case. ARCH §21.1 — the draw pass still highlights only the PRIMARY;
    // widening it to the whole multi-select set is a visual-language ticket of its own, not this one.
    if (HasSelection())
        iconLayerInput.selectedInstanceKey = PrimaryOfSelectionSet(selectedInstanceKeys);
```
Replace with:
```cpp
    // ARCH §19.25 — the canonical key shape (procedural or manual, correctly tagged `bManual`).
    // STEP229 — ARCH §21.1's "multi-instance canvas highlighting is a separate ticket" deferral is
    // now implemented: the WHOLE ordered multi-select set is threaded through (a pointer into
    // MapCanvas's own `selectedInstanceKeys`, MapCanvas_UI.h:377) — always, empty set included, so the
    // icon-layer module never needs its own null-vs-empty special case. Replaces the old
    // `if (HasSelection()) ... PrimaryOfSelectionSet(...)` guard, which narrowed the whole set down to
    // one key before it ever reached the cull/emit code — the actual bug this ticket fixes.
    iconLayerInput.selectedInstanceKeys = &selectedInstanceKeys;
```

---

## 9. Modified: `src/ui/MapCanvas_IconLayer_Cull_UI_Test.cpp`

Four existing call sites construct `input.selectedInstanceKey` directly and must convert to the
pointer-to-set shape. `OverlayInstanceKeySet_UI`/`ReplaceSelectionSet` are already visible in this file
transitively (`MapCanvas_IconLayer_TestFixture_UI.h` → `MapCanvas_IconLayer_CullInternal_UI.h` →
`MapCanvas_IconLayer_Ops_UI.h` → `MapCanvas_SelectionSet_UI.h`) — no new `#include` needed.

**9a.** `CheckSelectedInstanceCandidateResolvesMarkerColor` (currently lines 460-461):
```cpp
    DrawOverlayIconLayersInput input = fixture.Input();
    input.selectedInstanceKey = OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 0, true};
```
to
```cpp
    DrawOverlayIconLayersInput input = fixture.Input();
    OverlayInstanceKeySet_UI selectionSet;
    selectionSet.keys.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 0, true});
    input.selectedInstanceKeys = &selectionSet;
```

**9b.** `CheckIndexSpaceCollisionRegressionClosed` (currently lines 960-962 and 982-984):
```cpp
    DrawOverlayIconLayersInput input = fixture.Input();
    // Select the PROCEDURAL instance at array position 0.
    input.selectedInstanceKey = OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 0, true, /*bManual=*/false};
```
to
```cpp
    DrawOverlayIconLayersInput input = fixture.Input();
    // STEP229 — selectedInstanceKeys is now a pointer to the whole ordered multi-select set;
    // `selectionSet` is reassigned (reused) for the reverse-direction check further below.
    OverlayInstanceKeySet_UI selectionSet;
    // Select the PROCEDURAL instance at array position 0.
    selectionSet.keys = {OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 0, true, /*bManual=*/false}};
    input.selectedInstanceKeys = &selectionSet;
```
and
```cpp
    // The reverse direction: selecting the manual instanceIdentifier (55) selects ONLY the manual
    // candidate, never the procedural one — proves the fix holds both ways, not just one.
    input.selectedInstanceKey = OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 55, true, /*bManual=*/true};
```
to
```cpp
    // The reverse direction: selecting the manual instanceIdentifier (55) selects ONLY the manual
    // candidate, never the procedural one — proves the fix holds both ways, not just one.
    selectionSet.keys = {OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 55, true, /*bManual=*/true}};
```
(`input.selectedInstanceKeys` already points at `selectionSet` from the first assignment above — no
need to reassign the pointer itself.)

**9c.** `CheckSelectedInstanceCandidateResolvesManualMarker` (currently lines 1028-1029):
```cpp
    DrawOverlayIconLayersInput input = fixture.Input();
    input.selectedInstanceKey = OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 77, true, /*bManual=*/true};
```
to
```cpp
    DrawOverlayIconLayersInput input = fixture.Input();
    OverlayInstanceKeySet_UI selectionSet;
    selectionSet.keys.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 77, true, /*bManual=*/true});
    input.selectedInstanceKeys = &selectionSet;
```

**9d.** New acceptance check — the actual bug fix, verified against the exact call shape a real
marquee release makes. Add immediately before the closing `} // namespace` (currently line 1045):
```cpp
// STEP229 — the actual bug fix: bSelected must be true for EVERY member of the multi-select set, not
// just the primary (formerly the sole comparison target via OverlayInstanceKeysEqual). Builds the set
// via ReplaceSelectionSet — the SAME canonical mutator MapCanvas::ApplySelectionGesture calls on a
// plain (no-Ctrl/Shift) marquee release (MapCanvas_SelectionGesture_UI.cpp's ApplyMarqueeGesture ->
// ApplySelectionGesture -> ReplaceSelectionSet, ARCH §21.1) — not hand-rolled `.keys.push_back`, so
// this exercises the precise set shape a real box-select produces.
// MapCanvas_GestureOwnership_UI_Test.cpp already exhaustively proves the real imgui pointer-state
// machine correctly drives ApplyMarqueeGesture to build a set of this exact shape (unaffected by this
// ticket); ResolveVisibleCandidates/AppendCandidate cannot distinguish a key built this way from one
// built any other way, so this is the precise proof that the two halves compose correctly, without a
// second GL-backed harness that (per this ticket's own investigation) would have no legal way to
// observe bSelected from outside this module's own restricted CullInternal header anyway.
void CheckMarqueeStyleMultiSelectHighlightsAllMembers() {
    IconLayerTestFixture fixture;
    AppendMarkerInstance(fixture.placements, 1.0f, 1.0f, 0, Params::MarkerCategory::Alloys, "markerA");
    AppendMarkerInstance(fixture.placements, 2.0f, 1.0f, 0, Params::MarkerCategory::Alloys, "markerA");
    AppendMarkerInstance(fixture.placements, 3.0f, 3.0f, 0, Params::MarkerCategory::Alloys, "markerA");
    fixture.ruleBucketIndex.markers.Build(fixture.placements.markers.ruleIndex.data(), 3, 1);
    SeedAtlasEntry(fixture.pairingLookup, fixture.atlasManifest, "markerA", 0);
    OverlayLayer_UI layer; layer.domainKind = OverlayDomainKind_UI::Alloy;
    layer.thumbnailLodThresholdPixels = 1.0f;
    layer.subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, 0, true});
    fixture.overlaySettings.overlayLayers.push_back(layer);

    // Array positions 0 and 1 — mirroring a real drag-box covering the markers at world (1,1) and
    // (2,1) but not the one at (3,3).
    OverlayInstanceKeySet_UI selectionSet;
    ReplaceSelectionSet(selectionSet, {
        OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 0, true},
        OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 1, true}});

    DrawOverlayIconLayersInput input = fixture.Input();
    input.selectedInstanceKeys = &selectionSet;

    std::vector<OverlayVisibleInstance> candidates;
    ResolveVisibleCandidates(input, fixture.aabbCache, nullptr, candidates);
    check(candidates.size() == 3, "all three procedural markers resolve as candidates");
    for (const OverlayVisibleInstance& candidate : candidates) {
        const bool bExpectedSelected =
            candidate.instanceKey.instanceIndex == 0 || candidate.instanceKey.instanceIndex == 1;
        check(candidate.bSelected == bExpectedSelected,
              "STEP229 - the exact ReplaceSelectionSet call shape a real marquee release makes (ARCH "
              "Sec21.1) lights up bSelected for BOTH members, not just the MRU primary (array "
              "position 1)");
    }
}
```

Register it in `RunMapCanvasIconLayerCullChecks` (currently line 1076, the last call):
```cpp
    CheckIndexSpaceCollisionRegressionClosed();
    CheckSelectedInstanceCandidateResolvesManualMarker();
}
```
to
```cpp
    CheckIndexSpaceCollisionRegressionClosed();
    CheckSelectedInstanceCandidateResolvesManualMarker();
    CheckMarqueeStyleMultiSelectHighlightsAllMembers();
}
```

---

## 10. Modified: `src/ui/MapCanvas_IconLayer_Cache_UI_Test.cpp`

**10a.** `CheckFreshCacheAlwaysInvalidates` (currently lines 12-16):
```cpp
void CheckFreshCacheAlwaysInvalidates() {
    IconLayerFrameCache cache;
    check(ShouldInvalidateIconLayerCache(cache, 0.0f, 0.0f, 1.0f, OverlayInstanceKey_UI{}, 0),
          "a fresh, never-built cache always invalidates");
}
```
to
```cpp
void CheckFreshCacheAlwaysInvalidates() {
    IconLayerFrameCache cache;
    check(ShouldInvalidateIconLayerCache(cache, 0.0f, 0.0f, 1.0f, OverlayInstanceKeySet_UI{}, 0),
          "a fresh, never-built cache always invalidates");
}
```

**10b.** `CheckUnchangedKeysDoNotInvalidate` (currently lines 19-24):
```cpp
void CheckUnchangedKeysDoNotInvalidate() {
    IconLayerFrameCache cache;
    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, OverlayInstanceKey_UI{}, 5);
    check(!ShouldInvalidateIconLayerCache(cache, 10.0f, 20.0f, 2.0f, OverlayInstanceKey_UI{}, 5),
          "identical view/selection/revision keys never force a rebuild");
}
```
to
```cpp
void CheckUnchangedKeysDoNotInvalidate() {
    IconLayerFrameCache cache;
    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, OverlayInstanceKeySet_UI{}, 5);
    check(!ShouldInvalidateIconLayerCache(cache, 10.0f, 20.0f, 2.0f, OverlayInstanceKeySet_UI{}, 5),
          "identical view/selection/revision keys never force a rebuild");
}
```

**10c.** `CheckEachTriggerIndependentlyInvalidates` (currently lines 27-45):
```cpp
void CheckEachTriggerIndependentlyInvalidates() {
    IconLayerFrameCache cache;
    const OverlayInstanceKey_UI noSelection;
    const OverlayInstanceKey_UI markerSelection{PlacementCollectionKind_UI::Markers, 3, true};

    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, noSelection, 5);
    check(ShouldInvalidateIconLayerCache(cache, 99.0f, 20.0f, 2.0f, noSelection, 5), "pan invalidates");

    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, noSelection, 5);
    check(ShouldInvalidateIconLayerCache(cache, 10.0f, 20.0f, 3.0f, noSelection, 5), "zoom invalidates");

    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, noSelection, 5);
    check(ShouldInvalidateIconLayerCache(cache, 10.0f, 20.0f, 2.0f, markerSelection, 5),
          "a selection change invalidates");

    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, noSelection, 5);
    check(ShouldInvalidateIconLayerCache(cache, 10.0f, 20.0f, 2.0f, noSelection, 6),
          "a layer-settings revision change invalidates");
}
```
to
```cpp
void CheckEachTriggerIndependentlyInvalidates() {
    IconLayerFrameCache cache;
    const OverlayInstanceKeySet_UI noSelection;
    OverlayInstanceKeySet_UI markerSelection;
    markerSelection.keys.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 3, true});

    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, noSelection, 5);
    check(ShouldInvalidateIconLayerCache(cache, 99.0f, 20.0f, 2.0f, noSelection, 5), "pan invalidates");

    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, noSelection, 5);
    check(ShouldInvalidateIconLayerCache(cache, 10.0f, 20.0f, 3.0f, noSelection, 5), "zoom invalidates");

    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, noSelection, 5);
    check(ShouldInvalidateIconLayerCache(cache, 10.0f, 20.0f, 2.0f, markerSelection, 5),
          "a selection change invalidates");

    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, noSelection, 5);
    check(ShouldInvalidateIconLayerCache(cache, 10.0f, 20.0f, 2.0f, noSelection, 6),
          "a layer-settings revision change invalidates");
}
```

**10d.** `CheckBuildAccumulatesRawBytes` (currently line 50):
```cpp
    BeginIconLayerCacheBuild(cache, 1.0f, 2.0f, 3.0f, OverlayInstanceKey_UI{}, 7);
```
to
```cpp
    BeginIconLayerCacheBuild(cache, 1.0f, 2.0f, 3.0f, OverlayInstanceKeySet_UI{}, 7);
```

**10e.** New regression check — directly answers the "does removing a non-primary member still
invalidate the cache" concern. Add immediately before the closing `} // namespace` (currently line 63):
```cpp
// STEP229 — a set-changing edit that doesn't touch the primary (e.g. Ctrl-click removing a
// non-primary member) must still invalidate; the old single-key OverlayInstanceKeysEqual comparison
// this replaced could not see this kind of change at all.
void CheckNonPrimarySelectionMemberRemovalInvalidates() {
    IconLayerFrameCache cache;
    OverlayInstanceKeySet_UI twoKeys;
    twoKeys.keys.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 1, true});
    twoKeys.keys.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 2, true});   // primary
    OverlayInstanceKeySet_UI primaryOnly;
    primaryOnly.keys.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, 2, true});

    BeginIconLayerCacheBuild(cache, 10.0f, 20.0f, 2.0f, twoKeys, 5);
    check(ShouldInvalidateIconLayerCache(cache, 10.0f, 20.0f, 2.0f, primaryOnly, 5),
          "STEP229 - removing a NON-PRIMARY selected member still invalidates the cache, even though "
          "the primary (last element) is unchanged");
}
```

Register it in `RunMapCanvasIconLayerCacheChecks` (currently lines 65-70):
```cpp
void RunMapCanvasIconLayerCacheChecks() {
    CheckFreshCacheAlwaysInvalidates();
    CheckUnchangedKeysDoNotInvalidate();
    CheckEachTriggerIndependentlyInvalidates();
    CheckBuildAccumulatesRawBytes();
}
```
to
```cpp
void RunMapCanvasIconLayerCacheChecks() {
    CheckFreshCacheAlwaysInvalidates();
    CheckUnchangedKeysDoNotInvalidate();
    CheckEachTriggerIndependentlyInvalidates();
    CheckBuildAccumulatesRawBytes();
    CheckNonPrimarySelectionMemberRemovalInvalidates();
}
```

---

## 11. Modified: `src/ui/MapCanvas_IconLayer_DrawChunkCache_UI_Test.cpp`

Currently (line 26):
```cpp
    BeginIconLayerCacheBuild(frameCache, 0.0f, 0.0f, 1.0f, OverlayInstanceKey_UI{}, 1);
```
to
```cpp
    BeginIconLayerCacheBuild(frameCache, 0.0f, 0.0f, 1.0f, OverlayInstanceKeySet_UI{}, 1);
```

---

## ARCH rules invoked
- `ARCH_21_01_MultiSelectRepresentation.md` §21.1 — the ordered `OverlayInstanceKeySet_UI` / "primary
  is the last element" contract (`MapCanvas_SelectionSet_UI.h`, untouched by this ticket) and its own
  line 66 deferral ("multi-instance canvas highlighting... a separate, larger change"), which this
  ticket is that deferred change, implementing exactly what was always intended — no new ARCH ruling.
- `ARCH_14_09_RenderingPerformance.md` §14.9 — the C2 "interaction-scoped redraw" cache and its "never
  clustered/capped away" selected-instance exemption (`OverlayVisibleInstance::bSelected`'s own
  comment), both of which this ticket widens correctly to the whole set without changing their own
  contracts.
- `ARCH_19_25_SelectionRepresentationUnification.md` — the `bManual`-tagged key shape, unchanged; the
  widened loop in `ResolveSelectedInstanceCandidate` iterates keys of this exact shape.
- Constitution §1 — UI sets PARAMS/reads baked results, never simulates; this ticket writes no new sim
  logic, only widens an existing pure comparison from one key to set membership.

## Explicit out-of-scope
- **No visual highlight (ring/tint/border) is added.** Confirmed by reading every consumer of
  `bSelected` in the icon-layer module (grepped for `AddCircle`/`AddRect`/ring/outline/highlight across
  `src/ui` — the only hit, `MarkerSelectionHighlight_UI.cpp`, is the unrelated symmetry-orbit-sibling
  feature, ARCH §19.19) that no such visual exists today. This ticket is a data-correctness fix to the
  shared `bSelected` flag; a distinct visual treatment is a separate, later ticket if/when designed.
- **No fix to the pre-existing Props/Decals replay-path gap.** `ResolveSelectedInstanceCandidate` is
  restricted to `PlacementCollectionKind_UI::Markers` (STEP48's own scope, "no other domain has a
  working picker yet") — this restriction is unchanged and pre-dates this ticket. Consequence: on a
  cache-VALID (static, no pan/zoom/selection/revision-change) frame, a selected Prop or Decal is not
  re-resolved by the replay path and so is not redrawn on that frame — this was already true for a
  Props/Decals PRIMARY before this ticket (verified by reading the pre-ticket code) and remains true,
  unchanged, for any Props/Decals member of a wider set after this ticket. On every frame that DOES
  trigger a rebuild (which a selection change, e.g. the frame right after a marquee ends, always is —
  one of `ShouldInvalidateIconLayerCache`'s four triggers) every collection's `bSelected` is already
  correct, all collections, via `RebuildAndCache`'s own `SplitSelected`. Building a Props/Decals picker
  for the replay path is real, separate, follow-on work, not silently folded into this ticket.
- **No change to `MapCanvas::SelectedEntityIdentifier()`/`HasSelection()`** (`MapCanvas_UI.h:242-245`).
  Confirmed by reading `MapCanvas_UI.h` fully: these already read
  `PrimaryOfSelectionSet(selectedInstanceKeys)` directly, never through `DrawOverlayIconLayersInput` —
  the genuine "jump to primary"/camera-follow/list-scroll-to-selected surface stays exactly as-is,
  singular, by design (a "jump to" concept is inherently singular).
- **No ARCH file edit.** §21.1's own deferral text becomes stale prose once this ships ("a separate,
  larger change" is now done) — re-syncing ARCH prose to shipped code is the ARCH Expert's own call,
  not authored by this ticket or this agent.
- **No touch to `MapCanvas_SelectionSet_UI.h`/`.cpp`.** Read fresh; `SelectionSetContains` and
  `SelectionSetsEqual` already exist and are already correctly implemented — reused, not reinvented,
  per the dispatching agent's own instruction.
- **No new GL-backed end-to-end test file.** Investigated and deliberately rejected — see
  Interpretation call 4.
- **No change to `MapCanvas_IconLayer_Budget_UI.cpp`.** Its own `bSelected`-based decimation-exemption
  comparator (line 19) already operates per-instance and needs no change to correctly protect every
  member of a wider selected set once `bSelected` itself is set correctly upstream.
- **No change to `ResolveMarkersManual`'s own signature** (`MapCanvas_IconLayer_CullInternal_UI.h`).
  Its existing single optional `targetInstanceIdentifier` scoping filter is reused, called once per
  manual key in the widened loop — an authoring-scale (`tens, not tens of thousands`,
  `MapCanvas_SelectionSet_UI.h`'s own header comment) N-times re-walk, not a signature change to a
  function with other, unrelated call sites.

## Acceptance test
1. `CheckMarqueeStyleMultiSelectHighlightsAllMembers` — a two-key set built via the SAME
   `ReplaceSelectionSet` call shape a real marquee release makes lights up `bSelected=true` for BOTH
   members (array positions 0 and 1), not just the MRU primary (position 1) — the direct proof of the
   fixed bug.
2. `CheckIndexSpaceCollisionRegressionClosed` and `CheckSelectedInstanceCandidateResolvesManualMarker`
   continue to pass unmodified in behavior (only their `input` construction syntax changes, per §9) —
   proves the single-selection case (procedural and manual) is byte-identical to before.
3. `CheckNonPrimarySelectionMemberRemovalInvalidates` — removing a non-primary member from a
   previously-cached two-member set forces `ShouldInvalidateIconLayerCache` to return true.
4. `CheckFreshCacheAlwaysInvalidates`/`CheckUnchangedKeysDoNotInvalidate`/
   `CheckEachTriggerIndependentlyInvalidates`/`CheckBuildAccumulatesRawBytes` continue to pass with the
   widened `OverlayInstanceKeySet_UI` signature.
5. `MapCanvas_IconLayer_DrawChunkCache_UI_Test.cpp`'s `CheckCachePathRoundTrip` continues to pass
   unmodified in behavior.
6. Full `SanGenV2` build stays clean; every existing test continues to pass; the
   `MapCanvas_IconLayer_UI_Test` binary passes with `ALL PASS`, including the new
   `CheckMarqueeStyleMultiSelectHighlightsAllMembers` and `CheckNonPrimarySelectionMemberRemovalInvalidates`.
7. `MapCanvas_UI_Test`'s `RunMapCanvasGestureOwnershipChecks` continues to pass unmodified — the
   gesture → `OverlayInstanceKeySet_UI` wiring this ticket depends on (but does not itself touch) stays
   correct.

## Interpretation calls made
1. **`IconLayerFrameCache::cachedSelectionKeys` is a raw `std::vector<OverlayInstanceKey_UI>`, not the
   `OverlayInstanceKeySet_UI` wrapper type**, specifically to avoid a genuine circular structural header
   dependency (`MapCanvas_SelectionSet_UI.h` already includes `MapCanvas_IconLayer_UI.h`; the reverse
   would cycle). `MapCanvas_IconLayer_Ops_UI.h` and `MapCanvas_IconLayer_Cache_UI.cpp`, both `.cpp`-
   reachable or already-a-.cpp, freely include `MapCanvas_SelectionSet_UI.h` and wrap the raw vector in
   a temporary `OverlayInstanceKeySet_UI{...}` at the two points that need `SelectionSetsEqual`. This
   keeps the icon-layer module's own shared leaf value-types header (`MapCanvas_IconLayer_UI.h`) exactly
   as narrowly-included as it was before this ticket.
2. **`DrawOverlayIconLayersInput::selectedInstanceKeys` is a pointer, not a value.** Matches every other
   field in this struct (push-in pointers, null-safe), avoids a per-frame copy of an
   authoring-scale-but-still-non-trivial ordered vector, and lets `MapCanvas_Draw_UI.cpp` point it
   directly at its own live `selectedInstanceKeys` member with zero synchronization risk.
3. **`ResolveSelectedInstanceCandidate`'s Markers-only restriction is preserved, unchanged, not lifted**
   — see Explicit out-of-scope. Building Props/Decals pickers is real, separate, unscoped work.
4. **No new GL-backed end-to-end test file was written**, after investigating the option in depth.
   `MapCanvas::ApplyMarqueeGesture` is private, reachable only through a real `MapCanvas::Draw()` imgui
   cycle, which itself requires a genuinely non-zero `PresentationIdentifier()` — i.e. a real GPU-
   composed texture (`Sys::GpuResourceManager`, confirmed by reading `MapCanvas_UI.cpp:24-27`), ruling
   out the headless `MapCanvas_IconLayer_UI_Test` binary entirely. But the ONLY function that can
   observe `bSelected` per-candidate (`ResolveVisibleCandidates`) is declared in
   `MapCanvas_IconLayer_CullInternal_UI.h`, whose own header comment restricts it to "only
   `MapCanvas_IconLayer_Draw_UI.cpp`... and this module's own tests" — a `MapCanvas_UI_Test`-target file
   is outside that boundary, and the module's PUBLIC entry point (`DrawOverlayIconLayers`) exposes no
   per-candidate `bSelected` read-back at all (only vertex bytes and counter diagnostics). Rather than
   widen that public surface or violate the internal header's own documented scope (both larger,
   separately-scopable changes), this ticket instead composes the two already-adequate proofs: (a)
   `MapCanvas_GestureOwnership_UI_Test.cpp`, unmodified, already exhaustively proves the real imgui
   pointer-state-machine correctly drives a marquee to build a multi-key ordered set; (b) the new
   `CheckMarqueeStyleMultiSelectHighlightsAllMembers` (§9d), built via the SAME production
   `ReplaceSelectionSet` mutator a real marquee release calls, proves `bSelected` widens correctly for a
   set of that exact shape. `ResolveVisibleCandidates`/`AppendCandidate` are pure functions that cannot
   distinguish a mutator-built key from a gesture-derived one, so (a)+(b) together are the precise,
   legal, and sufficient proof.
5. **No distinct visual highlight is designed or added** — see Explicit out-of-scope. Confirmed by
   reading every consumer of `bSelected` that none exists today; this ticket's acceptance criterion is
   therefore `bSelected` correctness itself, verified the same direct way every pre-existing test in
   this module already verifies it.
6. **The `ResolveSelectedManualMarkerCandidate`/`ResolveSelectedProceduralMarkerCandidate` "did this
   call find a match" test switched from `!outCandidates.empty()` to a before/after size-delta** — a
   genuine, necessary correctness fix for the multi-key accumulation case (§5's own code comment),
   not a stylistic change; the single-key case is byte-identical in outcome.
7. **`ResolveMarkersManual`'s own signature is untouched**; the widened manual-marker loop calls it once
   per manual key (an authoring-scale N-times re-walk) rather than widening its single optional
   `targetInstanceIdentifier` filter to accept a list — the smaller blast-radius option, since that
   function has other call sites this ticket does not need to touch.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\ARCH_21_01_MultiSelectRepresentation.md` (referenced by section
number; not directly re-read line-by-line this session — cited from the dispatching agent's own
verified quote, "line 66," and from `MapCanvas_SelectionSet_UI.h`'s own header comment which restates
its contract),
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_Draw_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_Ops_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_CullInternal_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_CullEmit_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_Cull_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_Cache_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_Draw_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_DrawCache_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_Budget_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_SelectionSet_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_SelectionSet_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_SelectionGesture_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_GestureOwnership_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_Picking_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_TestFixture_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_Cull_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_Cache_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_DrawChunkCache_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_DrawChunkTestSupport_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkerSelectionHighlight_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkerSelectionHighlight_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\CMakeLists.txt`,
and `work_orders\STEP214_AreaAltCenterResizeModifier_UI.md` (this document's own structure/rigor
template, per the dispatching instruction).
