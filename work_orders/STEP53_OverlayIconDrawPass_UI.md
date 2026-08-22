# STEP53 — Screen-space overlay icon draw pass: bulk vertex writes, atlas bucketing, cross-layer budget, two-mode LOD

**Layer:** UI. **Domain:** `MapCanvas`, overlay rendering (`OverlayLayer_UI`). **Sequence:**
Phase 3.1 + 3.2 + 3.4, `work_orders/SEQUENCE_PreviewOverlayLayering.md` — bundled into one
work-order because the sequence document itself invites it (3.2's own note: "Bundle into 3.1 or
keep separate — coder's call"; 3.4's own note: "wired into 3.1's draw pass"). This is the core
deliverable of ARCH_14_PreviewOverlayLayering.md §14's redesign: the first code that actually draws Alloy/Spawns-Armies/Units/
Props/Decals overlay icons on the canvas at all.

**Depends on five prerequisites, none implemented yet — this ticket is not dispatchable until
they land, and only consumes their public surface, never invents a substitute for any of them:**
- **STEP47** (Phase 1.1, **DRAFTED**, `STEP47_WorldScreenProjection_UI.md`) — world<->screen
  projection: `PreviewComposite::WorldToPreviewPixel()`/`PixelsPerPreviewCell()`,
  `MapCanvasView::ProjectPreviewPixelToRegionLocal()`.
- **STEP50** (Phase 1.3) — the per-layer CSR bucket index over `Data::PlacementInstances`
  (`ruleIndex`/`category`-keyed flat index arrays, built once after Placement, rebuilt only when a
  layer's own sub-layer membership changes) — §14.9's own name for it.
- **STEP51** (Phase 2.1) — the `OverlayLayer_UI`/`OverlayDomainKind_UI`/`OverlaySubLayerRef_UI`
  data model and the `overlayLayers` stack itself (§14.2).
- **STEP52** (Phase 2.2) — the `templateIdentifier -> {thumbnailIconId, strategicIconId}` atlas
  pairing lookup (§14.3).
- **STEP58** (Phase 2.3) — the placeholder `templateIdentifier -> baseFootprintWidth/Depth` table
  (`Io::WorldFootprintSizeTable`) §14.3's thumbnail formula needs.

  **⚠️ Correction 2026-08-22 — the bundling this ticket originally assumed never happened.** This
  ticket's text used to say STEP58's table wiring was "bundled under this same umbrella per this
  ticket's own brief" as part of STEP52. It is not — verified directly: `STEP52_IconAtlasPairingLookup_UI.md`'s
  own "Explicit out-of-scope" section names "the `baseFootprintWidth/Depth` table (Phase 2.3, a
  separate READY sequence item)" as something it deliberately does NOT wire. `STEP58_WorldFootprintSizeTable_IO.md`
  likewise explicitly declines to wire itself in, calling that "STEP51's or STEP52's job at their
  own dispatch time." No ticket anywhere actually does it — **this ticket now does, in §0 below**,
  since it is the one ticket that actually needs the wired accessor to exist.

## 0. Wiring STEP58's footprint table — this ticket's own job (correction, see above)

Mirrors STEP52's own `IconAtlasPairingLookup` wiring pattern exactly (`Application_AssetBridge_UI.h`
field → populate in `LoadAssetAtlas()` → public accessor on `Application_UI.h`), for
`Io::WorldFootprintSizeTable` instead:

```cpp
// Application_AssetBridge_UI.h — new include + field, next to iconPairingLookup
#include "../io/WorldFootprintSizeTable_IO.h"
...
Io::WorldFootprintSizeTable   worldFootprintSizeTable;
```

```cpp
// Application_Assets_UI.cpp, LoadAssetAtlas() — populate alongside iconPairingLookup.
// Unlike iconPairingLookup, this table's content isn't derived from the loaded atlas at all
// (it's STEP58's hand-seeded placeholder, Io::BuildPlaceholderWorldFootprintSizeTable()) — call
// it once, unconditionally, not gated on atlas-load success:
assetBridge.worldFootprintSizeTable = Io::BuildPlaceholderWorldFootprintSizeTable();
```

```cpp
// Application_UI.h — new accessor, next to IconPairingLookup()
const Io::WorldFootprintSizeTable& WorldFootprintSizeTable() const { return assetBridge.worldFootprintSizeTable; }
```

**⚠️ Correction 2026-08-22 (second pass) — the accessor above is NOT what the draw pass calls.**
`MapCanvas` has no `Application` reference anywhere (confirmed: `src/ui/MapCanvas_UI.h` holds no
such pointer/member) and the real call site, `Application_Draw_UI.cpp:54`'s bare
`canvas.Draw("mapCanvas", regionSide);`, has no path back to `Application`. This codebase already
ratified how `MapCanvas` receives exactly this kind of cross-cutting state — STEP48's own
"RESOLVED — ARCH ruling" section: a push-in setter/pointer, the same mechanism
`SetPreviewComposite`/`SetMarkerPickingSource`/`SetMarkerPickRadiusScreenPixels` already use, not
a pull-based reach-back into `Application`. Follow that pattern instead:

```cpp
// MapCanvas_UI.h — new member + setter, alongside SetPreviewComposite/SetMarkerPickingSource
const Io::WorldFootprintSizeTable* worldFootprintSizeTable = nullptr;
void SetWorldFootprintSizeTable(const Io::WorldFootprintSizeTable* table) { worldFootprintSizeTable = table; }
```

```cpp
// Application_UI.cpp, WireCallbacks() — wire it once, same place SetPreviewComposite is wired
canvas.SetWorldFootprintSizeTable(&WorldFootprintSizeTable());
```

This ticket's own `baseFootprint` lookup (§1 item 8 below, the LOD formula) reads
`worldFootprintSizeTable->Resolve(templateIdentifier)` off `MapCanvas`'s own injected pointer, not
off `Application` directly. The `Application_UI.h` accessor above is still needed (it's what
`WireCallbacks()` calls to get the pointer to inject) — just not called from inside the draw pass
itself. Add `src/ui/Application_AssetBridge_UI.h`, `src/ui/Application_Assets_UI.cpp`,
`src/ui/Application_UI.h`, and `src/ui/Application_UI.cpp` (the `WireCallbacks()` edit) to this
ticket's own "Files touched" list (below) — they were missing from it.

## Problem
Today, **nothing draws overlay icons on the canvas at all.** `Props`/`Units`/`Decals`, all
resolved in `Data::PlacementResults`, never reach the canvas (§14's own opening problem
statement); `Alloy`/`Spawns-Armies` markers get only a flat, texel-space baked mark
(`PreviewComposite_Prepare_UI.cpp`'s `BuildEntityPoints`, `entityMarkRadiusPixels`/
`entityMarkColor` in `PreviewComposite_Settings_UI.h:76-77`) — the exact texel-space coupling
(marker size drifting with zoom) §14's whole redesign exists to kill. `src/ui/` has zero files
named `MapCanvas_IconLayer*` (confirmed by glob) — this is new code, not a modification of an
existing draw path.

§14.9 states hard, non-deferrable performance requirements for **whichever work-order first draws
overlay icons** — that is this one. Unlike most tickets, this Fix section is establishing
compliance with mandatory law on day one, not describing a change against an existing correct
baseline:
- **Individual `ImDrawList::AddImage()` calls per marker/prop/unit/decal instance are an explicit
  non-starter and MUST NOT be written anywhere in this module.** §14.9's own text: per-call
  overhead at 600k markers could plausibly cost 30-60ms — larger than the entire 16ms frame
  budget, independent of the transform math itself. This is a stated work-order requirement, not
  left to whatever imgui idiom is easiest to type.
- Atlas page bucketing is mandatory (not an optimization to consider later): thumbnails for many
  distinct templates legally scatter across many atlas pages (general bin-packed atlas, no
  same-page guarantee), so drawing in raw visit order risks draw-call count regressing toward
  O(pages touched) instead of O(layers).
- A cross-layer visible-vertex budget with automatic decimation is mandatory in this first
  work-order, not a follow-up.

## Fix

### 0. Module shape — one named entry point, split behind it per Constitution §1.5
The task names `MapCanvas_IconLayer_UI.cpp` as the core file. Given the size ceilings that file
alone cannot legally hold everything below (soft 100 / hard 150 lines, functions ≤40 lines, one
primary type per file) — this ticket ships a small public header plus the existing
`Type_Aspect_*.cpp` split this codebase already uses (`PreviewComposite_Prepare_UI.cpp`,
`PreviewComposite_Gpu_UI.cpp`, `PreviewComposite_Settings_UI.h` precedent):
- `MapCanvas_IconLayer_UI.h` — the public entry point (`DrawOverlayIconLayers(...)`), the pure
  value types shared across the split (`OverlayVisibleInstance`, `AtlasPageBucket`,
  `OverlayRenderingSettings`, `IconLayerFrameCache`).
- `MapCanvas_IconLayer_Cull_UI.cpp` — per-layer AABB + `Data::SpatialGrid` view-window culling,
  LOD-mode resolution, world->screen projection. Pure, imgui-free, headless-testable.
- `MapCanvas_IconLayer_Budget_UI.cpp` — the cross-layer visible-vertex budget: screen-cell
  clustering, then priority-cap fallback. Pure, imgui-free, headless-testable.
- `MapCanvas_IconLayer_Draw_UI.cpp` — the actual `ImDrawList` bulk vertex-write + atlas-page-
  bucketed flush. The only translation unit here that includes imgui, mirroring
  `MapCanvas_Draw_UI.cpp`'s own stated precedent of isolating imgui to one TU per concern.
- `MapCanvas_IconLayer_Cache_UI.cpp` — the C2 interaction-scoped cache (build/replay/invalidate).

No `.glsl` sibling: naming law's CPU/GPU pairing (`Erosion_PROC.cpp` + `Erosion_PROC.glsl`)
applies where a GPU compute counterpart exists; this pass has none — it is inherently CPU-side
immediate-mode vertex generation, consumed by imgui's own renderer backend. Do not create one.

Exact per-file line boundaries are an implementation call, not locked here, as long as every file
and function stays inside the ceilings above.

### 1. Per-layer culling + LOD resolution -> the candidate instance list
Once per frame (not per layer), compute the current view's world-space visible rectangle by
composing STEP47's own functions in their documented direction (region-local -> preview pixel ->
world), the same composition `ApplyClick` (STEP48) already performs in reverse:
```cpp
// two opposite screen corners are enough: the mapping is an axis-aligned affine transform
const PreviewPixelCoordinate lowPixel  = view.ResolvePreviewPixel(0.0f, 0.0f);
const PreviewPixelCoordinate highPixel = view.ResolvePreviewPixel(regionSidePixels, regionSidePixels);
const PreviewComposite::PreviewWorldPoint lowWorld  =
    composite.PreviewPixelToWorld(static_cast<float>(lowPixel.pixelX), static_cast<float>(lowPixel.pixelY));
const PreviewComposite::PreviewWorldPoint highWorld =
    composite.PreviewPixelToWorld(static_cast<float>(highPixel.pixelX), static_cast<float>(highPixel.pixelY));
```
Then, for every `OverlayLayer_UI` in `overlayLayers`, **processed in vector order** (§14.2:
"vector order = Z order"):
1. Skip if `!layer.bEnabled`.
2. **AABB early-out**: skip the whole layer if its cached world AABB does not intersect
   `[lowWorld, highWorld]`. The AABB is STEP50's to maintain (rebuilt only when the layer's own
   sub-layer membership changes, never per frame) — flagged below as an open boundary question.
3. For each enabled `ProceduralRule` sub-layer, resolve candidate global `Data::PlacementInstances`
   indices via STEP50's per-layer index. **Open boundary, flag to ARCH/STEP50 before dispatch if
   not already settled** (mirrors STEP48's own precedent of flagging an inter-ticket boundary
   rather than silently picking one): §14.9 names both "a per-layer flat index array" (the CSR
   bucket) AND "per-layer `Data::SpatialGrid`... for view-window culling" as separate mandatory
   pieces. This ticket is written against the minimum viable contract — a per-layer
   `Query(worldMinX, worldMinY, worldMaxX, worldMaxY, visitInstance)`-shaped call, same bucket-walk
   shape `Data::SpatialGrid` (`SpatialGrid_DATA.h:44-53`) already exposes — but whether that grid is
   built once per layer over the CSR-selected subset, or is the existing shared per-domain grid
   queried and then filtered by CSR membership, is STEP50's design to fix, not this ticket's to
   silently assume.
4. For each enabled `Manual` sub-layer, walk its (typically small, authored) array directly — no
   grid needed at authoring-list scale — applying the same per-instance AABB test (manual layers
   have no locality guarantee a grid could exploit anyway).
5. ⚠️ §14.6 asymmetry law applies here directly: `domainKind` does not equal DATA-bucket identity.
   Alloy/SpawnsArmies both re-slice the shared `markers` collection by `category`; Props/Units/
   Decals map 1:1 to their own `Data::PlacementResults` collection. Resolve the correct DATA
   collection per §14.2's own binding sub-layer table — do not hardcode a single collection for
   every domain.
6. For each surviving candidate, resolve `templateIdentifier -> {thumbnailIconId, strategicIconId}`
   via STEP52's pairing lookup. **A miss draws nothing for that instance** — §14.3 rules out a
   generic fallback icon ("bespoke per blueprint," no generic glyph). Log the unresolved
   `templateIdentifier` once per unique id per session, never per-instance or per-frame (a 600k-
   instance frame with one bad id must not become 600k log lines).
7. Project world -> screen, composing STEP47's forward half with its own new inverse:
   `previewPixel = composite.WorldToPreviewPixel(worldX, worldZ);`
   `regionLocal  = view.ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);`
   `screenX = regionOriginX + regionLocal.regionLocalX;` (same region-origin-add convention
   `MapCanvas_Draw_UI.cpp` already uses in reverse — STEP47 §2's own note not to invert it).
8. **Two-mode LOD (§14.3, formula verbatim, not re-derived)**:
   ```cpp
   const float thumbnailScreenSize = (baseFootprint * instance.scale) / composite.Settings().worldUnitsPerCell
                                    * composite.PixelsPerPreviewCell() * view.ZoomScale();
   if (thumbnailScreenSize >= layer.thumbnailLodThresholdPixels) {
       mode = LodMode::Thumbnail; iconId = pairing.thumbnailIconId; screenSize = thumbnailScreenSize;
   } else {
       mode = LodMode::Strategic; iconId = pairing.strategicIconId;
       screenSize = layer.strategicIconScreenSizePixels;   // NEW field, see below
   }
   ```
   `baseFootprint` comes from `worldFootprintSizeTable->Resolve(templateIdentifier)` — the pointer
   §0 above injects into `MapCanvas` via `SetWorldFootprintSizeTable()`, sourced from STEP58's
   placeholder table — **never a placeholder invented in this draw-pass file itself** (§14.13
   item 1, the real mesh-derived table, stays open; this ticket must not paper over that with its
   own guess, only consume §0's plumbing).
   ⚠️ **New field needed on `OverlayLayer_UI`:** `float strategicIconScreenSizePixels` — the §14.2
   struct as ratified only carries `thumbnailLodThresholdPixels`, not a strategic-mode fixed size.
   Check whether STEP51 has already landed it by dispatch time; if not, add it there (same
   per-layer-tweakable pattern as `thumbnailLodThresholdPixels`), not as a shadow field here.
9. Emit an `OverlayVisibleInstance` (screen center, `screenSize`, resolved `iconId`'s atlas page +
   UV rect from `Ui::IconAtlasManifest`, `tintAlpha = layer.opacity` — §14.2's opacity-folded-into-
   tint rule, never a second blend path) into **both** that layer's per-frame candidate list and
   the cross-layer atlas-page bucket keyed by `atlasPage`, accumulated during this same walk
   (§14.9: "accumulate ... during vertex-gen," not a second pass).

⚠️ Fully zoomed-out worst case: §14.9 states the grid gives zero help here (every cell is queried,
genuinely O(N)) — step 2 below is what bounds that case. Do not add a second, ad-hoc cap in this
step; one budget, one place.

### 2. Cross-layer visible-vertex budget + decimation
New named tweakables (Constitution §8 — never literals), added alongside wherever STEP51 places
`overlayLayers`' session settings (a small sibling struct if STEP51 hasn't already created a home):
```cpp
struct OverlayRenderingSettings {
    // §14.9: reasoned PLACEHOLDER pending Phase 3.3's real microbenchmark — not a ratified
    // constant (Constitution §7/§12 basis-tag law). Do not treat this default as final.
    int   visibleInstanceBudget      = 450000;    // 400k-500k range per §14.9
    int   screenCellClusterSizePixels = 8;         // primary decimation mechanism
};
```
Budget is **cross-layer** (§14.9's own heading) — summed over every layer's candidate list this
frame, evaluated once after step 1 completes for all layers, not per layer:
- If total candidate count <= `visibleInstanceBudget`: draw everything accumulated, no decimation.
- Else, decimate down to budget:
  1. **Primary — screen-cell clustering.** Bucket every candidate (across all layers, screen
     space) into a uniform grid of `screenCellClusterSizePixels`-sized screen cells. Any cell
     holding more than one candidate draws exactly **one representative** instead of dropping the
     rest silently — icons landing on the same few screen pixels are visually indistinguishable
     anyway, so this is not a WYSIWYG violation the way an arbitrary drop would be. (A member-count
     badge on a cluster representative is a real, later UX nicety — **explicitly out of scope
     here**, not implied by "clustering.")
  2. **Fallback — priority cap.** If clustering alone still exceeds budget, rank the surviving
     cluster representatives and truncate: (a) a selected/hovered instance always wins — it must
     never be clustered or capped away, or the C2 cache's "regenerate live only the selection"
     contract (§14.8) becomes visibly wrong; (b) later-Z-order layer wins over earlier (matches
     §14.7's "vector order = Z order = visual precedence"); (c) stable index tie-break, so the
     visible set doesn't flicker frame-to-frame at a fixed view.
- **Binding guardrail, restated explicitly because this is exactly the risk it forbids (§14.11):**
  both mechanisms operate only on the in-memory candidate list built in step 1. Neither may ever
  mutate or discard `Data::PlacementInstances`, `Data::SpatialGrid`, or any CSR bucket, and neither
  may feed back into export/bake. A "helpful" decimation silently becoming a second placement
  decision is the exact failure mode this sentence forbids.

### 3. Atlas page bucketing + the bulk vertex write itself
This is the part §14.9 requires and forbids `AddImage()` for. Confirmed against this project's
vendored imgui (`build/_deps/imgui-src/imgui.h:3588-3595`, "Advanced: Primitives allocations" —
`PrimReserve`, `PrimWriteVtx`, `PrimWriteIdx`, `_VtxCurrentIdx` are public, documented surface, not
a private hack):
```cpp
// MapCanvas_IconLayer_Draw_UI.cpp
void FlushIconLayerBucket(ImDrawList& drawList, const AtlasPageBucket& bucket) {
    if (bucket.quads.empty()) return;                          // never a zero-quad draw command
    drawList.PushTextureID(static_cast<ImTextureID>(bucket.textureIdentifier));
    const int quadCount = static_cast<int>(bucket.quads.size());
    // ONE reserve for the whole bucket. PrimRectUV/PrimQuadUV each call PrimReserve internally —
    // calling them per-quad here would double-reserve and reintroduce a per-instance call cost,
    // defeating the point as surely as AddImage would.
    drawList.PrimReserve(quadCount * 6, quadCount * 4);
    for (const OverlayVisibleInstance& instance : bucket.quads) {
        const float half = instance.screenSize * 0.5f;
        const ImU32 tint = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, instance.tintAlpha));
        const ImDrawIdx base = static_cast<ImDrawIdx>(drawList._VtxCurrentIdx);
        drawList.PrimWriteIdx(base);     drawList.PrimWriteIdx(base + 1); drawList.PrimWriteIdx(base + 2);
        drawList.PrimWriteIdx(base);     drawList.PrimWriteIdx(base + 2); drawList.PrimWriteIdx(base + 3);
        drawList.PrimWriteVtx({instance.screenCenterX - half, instance.screenCenterY - half},
                              {instance.uvMinimumX, instance.uvMinimumY}, tint);
        drawList.PrimWriteVtx({instance.screenCenterX + half, instance.screenCenterY - half},
                              {instance.uvMaximumX, instance.uvMinimumY}, tint);
        drawList.PrimWriteVtx({instance.screenCenterX + half, instance.screenCenterY + half},
                              {instance.uvMaximumX, instance.uvMaximumY}, tint);
        drawList.PrimWriteVtx({instance.screenCenterX - half, instance.screenCenterY + half},
                              {instance.uvMinimumX, instance.uvMaximumY}, tint);
    }
    drawList.PopTextureID();
}
```
Flush one non-empty bucket per resolved atlas page — bounds draw calls to O(pages touched this
frame) regardless of visit order (§14.9's own phrase). Strategic-icon mode's small fixed
low-cardinality icon set goes on one dedicated always-resident page, per §14.9.

⚠️ **Accepted, ARCH-sanctioned ordering tradeoff, documented so a future reader doesn't mistake it
for a defect:** bucketing is cross-layer (a shared atlas page pulls candidates from every layer
that touches it into one flushed draw command), so two icons from *different* layers that happen
to visually overlap on screen and share a page are not guaranteed strict per-instance Z order
against each other — only against same-page candidates from the same layer (which stay in
layer-walk append order within the bucket). §14.9's own text ("bounds draw calls... regardless of
visit order") accepts this; it is not this ticket's job to add a secondary sort to restore strict
cross-layer Z at the cost of the O(pages) bound.

### 4. The C2 "interaction-scoped redraw" cache (§14.8)
This ticket builds the **caching primitive only** — not a marker drag/edit UX, which is a
different, unscheduled ticket's job (`BRIEF_MarkersTabUI_R2.md` territory). The mechanism must
exist now so that future ticket has something to call into instead of inventing its own bespoke
redraw-avoidance scheme.
```cpp
// MapCanvas_IconLayer_UI.h
struct IconLayerFrameCache {
    std::vector<unsigned char> cachedVertexBytes;      // raw ImDrawVert bytes, non-selected only
    std::vector<unsigned char> cachedIndexBytes;        // raw ImDrawIdx bytes, matching topology
    std::vector<AtlasPageBucket> cachedBucketLayout;    // page id + quad count, for replay
    bool bValid = false;
    float cachedViewCenterPixelX = 0.0f, cachedViewCenterPixelY = 0.0f, cachedZoomScale = 0.0f;
    std::uint32_t cachedSelectionIdentifier = 0;
    std::uint64_t cachedLayerSettingsRevision = 0;      // bumped by any overlayLayers mutation
};
```
- **Build** (gesture-start only): run steps 1-3 excluding the selected instance(s); write the
  resulting bytes into `cachedVertexBytes`/`cachedIndexBytes` (CPU bytes, not a GPU texture/FBO —
  §14.8's own distinction) instead of a live `ImDrawList`; record the bucket layout needed to
  replay draw commands; snapshot the invalidation keys.
- **Replay** (every frame during the gesture): if the live view center/zoom/selection/layer-
  settings-revision still match the cached keys, `memcpy` the cached bytes into the live
  `ImDrawList`'s buffers (grow via the same `PrimReserve`-then-raw-write path, but as one bulk copy
  instead of a per-vertex loop) and re-issue the cached bucket's draw commands — zero regeneration.
  Then run steps 1-3 fresh for **only** the selected instance(s), appended after.
- **Invalidate** (drop `bValid`, rebuild next frame) on: pan (view center changed), zoom (`zoomScale`
  changed), selection change, or any overlay layer-setting change — a monotonic revision counter on
  `overlayLayers`, bumped by every mutation (reorder/opacity/enable-toggle/threshold change),
  mirroring `PreviewDriver::NotifyParametersChanged()`'s own hash-bump pattern (§14.7). `OverlayLayer_UI`
  (STEP51) does not have one yet; add it there if still missing at dispatch time, not a duplicate
  counter here.
- LOD threshold-crossing needs **no separate invalidation rule** — §14.8 states zoom already
  invalidates C2 unconditionally, and a threshold-cross only ever happens as zoom changes.
- Today only markers have a working picker (STEP48's `PickMarker`), so cache keys should be
  `(domainKind, instanceIndex)` generically — not a bare `uint32_t` assumed to always mean
  "marker" — so Props/Units/Decals don't need cache rework once they get their own pickers later.
  Until then, non-marker layers simply never have a selection and always take the full-cache path,
  which is correct, just never exercises the live-regenerate-selection branch.

### 5. Wiring into `MapCanvas::Draw`
One new call in `MapCanvas_Draw_UI.cpp`, right after the existing `ImGui::Image()` call (so icons
composite visually on top of the terrain texture) and before `ApplyPointerInput` — ordering here is
for readability only, not correctness, since click-picking is already off the draw list (STEP48
onward uses `Data::SpatialGrid`/`PickMarker`, never reads what got drawn).

## Files touched
- NEW `src/ui/MapCanvas_IconLayer_UI.h` — public entry point + shared value types.
- NEW `src/ui/MapCanvas_IconLayer_Cull_UI.cpp` — §1.
- NEW `src/ui/MapCanvas_IconLayer_Budget_UI.cpp` — §2.
- NEW `src/ui/MapCanvas_IconLayer_Draw_UI.cpp` — §3.
- NEW `src/ui/MapCanvas_IconLayer_Cache_UI.cpp` — §4.
- NEW `src/ui/MapCanvas_IconLayer_UI_Test.cpp` (or split per aspect file, coder's call, mirroring
  `MapCanvas_Render_UI_Test.cpp`'s existing headless-imgui-frame technique).
- MODIFIED `src/ui/MapCanvas_Draw_UI.cpp` — one new call, §5.
- MODIFIED `src/ui/MapCanvas_UI.h` — setters to receive `overlayLayers` (or a pointer to wherever
  STEP51 owns it), the atlas manifest + pairing lookup (STEP52), the footprint table, and the
  per-layer CSR/grid source (STEP50). Exact setter shape depends on STEP50/51/52's landed
  signatures — sketch only, not locked here.
- MODIFIED (possibly) `OverlayLayer_UI`'s home (STEP51's file) — the new
  `strategicIconScreenSizePixels` field and the layer-settings revision counter, **only if** they
  are not already there by dispatch time.
- MODIFIED `src/ui/Application_AssetBridge_UI.h` — new include + `worldFootprintSizeTable` field (§0).
- MODIFIED `src/ui/Application_Assets_UI.cpp` — `LoadAssetAtlas()` populate call (§0).
- MODIFIED `src/ui/Application_UI.h` — new `WorldFootprintSizeTable()` accessor (§0). (The
  `SetWorldFootprintSizeTable()` pointer setter itself lives on `MapCanvas_UI.h`, already covered
  by this list's pre-existing `MapCanvas_UI.h` entry above — "the footprint table" in that entry
  now means this setter specifically.)
- MODIFIED `src/ui/Application_UI.cpp` — one new `WireCallbacks()` line injecting the pointer (§0).

## Layer & accuracy class
UI. Accuracy class: Visual — screen-space presentation only, the same Visual-class exemption
`OPTIMIZATION_PILLARS.md` pillar 15 already grants GPU-resident preview compositing (§14.11).

## Backend policy
CPU-only vertex generation. `ImDrawList` lives entirely in the UI/imgui immediate-mode layer;
there is no GPU compute kernel here to dispatch. The eventual GPU work is imgui's own renderer
backend consuming the CPU-built vertex/index buffers; atlas page textures stay routed through the
existing `Sys::GpuResourceManager`/`Ui::IconAtlasManifest` (§14.1 — a UI-owned GL pipeline is a
named v1 defect class and must not reappear here). No SIMD/dispatch-backend decision is in this
ticket's scope: Phase 3.3's microbenchmark decides whether the transform step later gets a SIMD
backend; this ticket ships the scalar baseline that benchmark will compare against.

## ARCH rules invoked
- §14.1 — GPU-resident draw state routes through `GpuResource_SYS`; no UI-owned GL pipeline.
- §14.2 — opacity folded into per-vertex tint alpha, not a per-layer blend-mode switch; vector
  order = Z order.
- §14.3 — two-mode LOD formula (verbatim), bespoke-per-blueprint strategic icons (no generic
  fallback), the still-open footprint-size gap (§14.13 item 1).
- §14.6 — `domainKind` is not DATA-bucket identity; resolve per domain, not by assumption.
- §14.8 — the C2 cache mechanism and its exact invalidation triggers.
- §14.9 — every bullet: bulk vertex writes only, cross-layer budget + decimation, mandatory atlas
  page bucketing, resident-atlas reuse, per-layer AABB + `Data::SpatialGrid` culling, CSR bucket
  index (no physical resort).
- §14.11 — decimation may only affect what is drawn, never `Data::PlacementInstances` itself.
- §14.12 — naming (`_UI` suffix throughout).
- Constitution §7 — basis-tag law: the visible-instance budget default is explicitly a reasoned
  placeholder, not a benchmarked constant, until Phase 3.3 lands.
- Constitution §8 — every new numeric knob (`visibleInstanceBudget`, `screenCellClusterSizePixels`,
  `strategicIconScreenSizePixels`) is a named, exposed setting, never a literal.
- Constitution §1.5 — size ceilings drive the multi-file split in §0.

## Explicit out-of-scope
- Implementing STEP47/STEP50/STEP51/STEP52/STEP58 themselves — this ticket only consumes their
  surface (STEP58's placeholder table itself; §0 above is this ticket's own wiring of it, not a
  reimplementation of STEP58's table).
- The real, mesh-derived footprint-size table (§14.13 item 1) — placeholder only, consumed not
  invented.
- The real, benchmarked cross-layer budget constant (§14.13 item 2) — Phase 3.3's job.
- **Phase 3.3's microbenchmark itself** (SIMD-transform / bulk-write / naive-`AddImage` timed at
  N ∈ {100k, 300k, 600k}, 0%-culled and ~5%-visible, real dev hardware) — a separate follow-up
  ticket, sequenced after this one specifically because it needs this ticket's built binary to
  exist first (`SEQUENCE_PreviewOverlayLayering.md`'s own note on Phase 3.3). Not folded into this
  ticket's acceptance bar.
- The actual marker drag/edit interaction UX that would trigger a C2 gesture — this ticket ships
  the cache primitive only; the UX is `BRIEF_MarkersTabUI_R2.md` territory.
- The View toolbar (Phase 4) — reading/writing `overlayLayers` order/opacity/enable state is
  STEP51's + Phase 4's job; this ticket only draws whatever state already exists.
- Restoring strict cross-layer Z order for same-page icons from different layers that visually
  overlap — §14.9 itself accepts bucketing "regardless of visit order" (see §3's callout above);
  not a defect this ticket must close.
- Manual sub-layer stable-id/PROC resolution (Phase 5, §14.13 item 3) — this ticket's CSR-bucket
  consumption is written generically enough not to care whether a sub-layer's flat index array
  points at procedural or manual instances, but does not implement Phase 5 itself.
- Reclaim domain — no data or rule type exists yet (§14.2's table); zero cost if/when it lands, not
  exercised by this ticket's tests.
- GPU atlas texture creation/eviction, font-atlas handling — unmodified, routes through existing
  `Sys::GpuResourceManager`/`Ui::IconAtlasManifest`.

## Solution — performance estimate (basis)
**Reasoned, not yet benchmarked** — Constitution §7 basis tag: REASONED-PLACEHOLDER, explicitly
superseded by Phase 3.3's measured numbers, never to be read as a ratified figure. Bulk
`PrimReserve` + raw `PrimWriteVtx`/`PrimWriteIdx` writes eliminates `AddImage()`'s per-call
overhead entirely (the 30-60ms/600k figure §14.9 cites is that per-call cost, not an intrinsic
transform cost). At the placeholder 400k-500k visible-instance budget, per-instance work is a
small, branch-light constant (one LOD compare, a handful of multiplies for the screen projection,
4 vertex writes + 6 index writes, no trigonometry) — a few-millisecond CPU cost is a defensible
reasoned target on typical dev hardware, but this is explicitly not a ratified number.

## Verify
Functional correctness is this ticket's acceptance bar — proving the 600k-instance performance
number is Phase 3.3's job, sequenced after this ticket specifically because it needs this ticket's
binary to exist. Tests use small synthetic instance counts (hundreds to low thousands) sufficient
to exercise every code path below, headless (no live imgui frame required except where noted):

- **Culling (`MapCanvas_IconLayer_Cull_UI.cpp`):** a layer whose cached AABB does not intersect the
  view rect produces an empty candidate list and never touches its grid (assert via a query-call
  counter). A candidate inside the view rect but whose `templateIdentifier` has no atlas pairing
  entry is silently dropped, logged at most once per unique id across the whole test run — not once
  per instance.
- **LOD switch:** one instance whose computed `thumbnailScreenSize` sits above
  `thumbnailLodThresholdPixels` emits the thumbnail icon id at the scaled size; one just below
  emits the strategic icon id at the fixed `strategicIconScreenSizePixels`, independent of zoom.
- **Budget/decimation (`MapCanvas_IconLayer_Budget_UI.cpp`):** the drawn count never exceeds
  `visibleInstanceBudget` regardless of candidate count fed in. When candidates exceed budget but
  screen-cell clustering alone brings the count at/under budget, the priority-cap fallback never
  engages (assert via a call counter on the fallback function). When clustering alone still
  exceeds budget, the fallback truncates to exactly the budget, always keeping a synthetic
  "selected" instance and always preferring the later-Z-order layer on ties.
- **Determinism guardrail (§14.11):** byte-compare `Data::PlacementInstances`/`Data::SpatialGrid`/
  the CSR bucket before and after a decimated draw call — bit-identical; decimation touches only
  the in-memory candidate list.
- **Bucketing + bulk write (`MapCanvas_IconLayer_Draw_UI.cpp`, live headless imgui frame, mirroring
  `MapCanvas_Render_UI_Test.cpp`'s technique):** the number of distinct `ImDrawCmd` entries the
  icon layer produces equals the number of distinct atlas pages actually touched — not the
  instance count, not the layer count. Total generated `ImDrawVert`/`ImDrawIdx` counts equal
  exactly `quadCount * 4` / `quadCount * 6` (catches an accidental double-reserve or a regression
  back toward per-instance `AddImage`).
- **C2 cache (`MapCanvas_IconLayer_Cache_UI.cpp`):** build once, redraw N frames with nothing
  changed -> byte-identical cached bytes every frame, cull/budget pipeline not re-invoked (assert
  via a generation-function call counter, expect exactly 1 call for N replays). Move only the
  selected instance -> only its quad bytes change frame-to-frame; the cached bytes for every other
  instance are untouched (memcpy'd, not regenerated). Each of pan / zoom / selection-change /
  layer-setting-change independently forces exactly one rebuild, not one per frame.
- **No regression:** `MapCanvas_Render_UI_Test.cpp`, `MapCanvas_Picking_UI_Test.cpp`,
  `MapCanvas_UI_Test.cpp`, `MapCanvas_View_UI_Test.cpp` stay green with no edits beyond whatever
  STEP47/48/50/51/52 already required — the new icon draw commands are additional `ImDrawCmd`
  entries in the same `ImDrawData`, and `FindDrawnImage`'s existing composite-texture search must
  still find exactly the composite rectangle, unperturbed.
