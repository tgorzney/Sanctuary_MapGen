# Design Output — Marker Preview Layering, Round 1 (UI Expert + UI Optimization Expert)

Consult round 1 of `BRIEF_OptimizedPreviewPipeline.md`. UI Expert + UI Optimization Expert only,
per scope. Not yet ratified — this is input for the ARCH Expert next round. No code written.

Both experts independently converged on the same design (cross-checked, no contradictions).

## Ground truth corrections (both experts, reading code not spec)
- There are no icons at all today — every entity (marker/prop/army) bakes as an undifferentiated
  flat circular "mark" into the shared composite texture (`PreviewComposite_Cpu_UI.cpp:117-145`,
  GLSL twin `PreviewComposite_UI.glsl:68-97`), in **texel** space via `BuildEntityPoints()`
  (`PreviewComposite_Prepare_UI.cpp:80-98`). That's why marks scale with zoom.
- `Data::PlacementInstances` (`PlacementInstances_DATA.h`) is markers-only today, carries
  `category` already — no new DATA field needed to discriminate by type.
- Two picking paths exist; only one is wired. `MapCanvas::ApplyClick` (`MapCanvas_UI.cpp:31-39`)
  calls `PickEntity` (raster `EntityIdBuffer` read). `Picking_UI::PickMarker` — O(1)
  `Data::SpatialGrid` chunk hit-test, fully implemented and tested (`Picking_UI_Test.cpp`) — is
  **built but never called**. It is the ready-made backend for screen-space picking.
- `IconGridWidget_Draw_UI.cpp` already establishes the exact bypass technique needed:
  `ImDrawList::AddImage` at a fixed screen-pixel cell, atlas UV lookup, no imgui widget overhead.
  Reuse this, don't invent a new pattern.
- `PreviewDriver_PIPELINE.h`'s two-tier `RefreshTier` is already DAG-derived correctly — the gap
  is that it only has two tiers, not that today's tier assignment is wrong.

## Decision: screen-space draw-list, not per-layer GPU framebuffers
Both experts independently rejected per-marker-type FBOs. Reasoning (UI Optimization Expert):
a baked-texture layer is sampled through the same pan/zoom UV window as terrain, so it inherits
the exact "zoom never recomposites" property that causes today's bug — getting constant
screen-pixel icons out of an FBO would require re-rasterizing it every pan/zoom tick, which is
the cost the terrain path exists to avoid. A screen-space `ImDrawList` pass has no such problem:
positioned in screen space every frame by construction, same redraw-every-frame cost class the
rest of imgui already pays.

**Design: one screen-space `ImDrawList` pass per marker category, drawn after the existing
`ImGui::Image` composite call, in the same window draw list.** New file
`MapCanvas_IconLayer_UI.cpp` (keeps `MapCanvas_Draw_UI.cpp` as "only TU that draws the base
image," per its existing concern-split character).

## Layer model (bottom to top)
1. **Base field composite** (terrain/water/stratum) — unchanged, GPU-resident texture, UV-window
   pan/zoom exactly as today.
2. **Baked-dot fallback** (today's `OverlayPassCpu`/GLSL `entityPass`) — demoted from "the only
   entity rendering" to an optional far-zoom/high-density LOD style the icon layer can choose,
   not deleted outright. ❓ Exact LOD threshold is a UX call, see Open Questions.
3. **Icon layers, one per category** (marker / prop / army) — independent screen-space passes,
   each with its own visibility toggle, tint, z-order slot. Toggling one category = a state
   check, zero GPU work, zero recompose of the others.
4. **Selection/hover overlay** — topmost, screen-space, drawn last.

Click-through: icon hit-test runs before any raster/terrain interpretation. Drag-vs-click
disambiguation is already travel-distance-based in `ApplyPointerInput` — unaffected.

## Coordinate contract
World→texel math lives in `BuildEntityPoints()` (`PreviewComposite_Prepare_UI.cpp`). Texel→screen
lives in `MapCanvasView::ResolvePreviewPixel` (`MapCanvasView_UI.h`) — but only the forward
(screen→preview-pixel) direction exists today, no inverse. Icon placement needs the
**composition** of both directions (world→screen), and no single file owns that chain today.

**Recommendation (unanimous): add `MapCanvasView::ProjectPreviewPixelToScreen(pixelX, pixelY) ->
ImVec2`**, pure/imgui-free like its siblings, same header. Compose with `BuildEntityPoints`'s
world→texel math from `MapCanvas`/the new icon-layer file. ⚠️ `SpatialGrid_DATA.h`'s own header
already warns against a second copy of this kind of transform drifting — **route the exact home
of the composed world→screen function to ARCH**, don't let either UI expert's file choice stand
as the ruling.

## Picking
**Retire the `EntityIdBuffer`/`PickEntity` marker path in `MapCanvas::ApplyClick`. Route clicks
through `Picking_UI::PickMarker` against `Data::SpatialGrid` instead** — already built, already
tested, net line-count reduction, not new machinery. Pick radius: fixed icon screen-pixel radius
converted to world units at current zoom (`pickRadiusWorldUnits = iconScreenRadiusPixels /
view.ZoomScale() * worldUnitsPerPixel`).

❓ Open (UI Optimization Expert, unresolved between the two experts): does `MapCanvas` call
`PickMarker` directly with world coordinates it computes itself, or does `Picking_UI` gain a
"screen click → nearest marker" convenience wrapper so `MapCanvas` stays "does exactly three
things" per its own header comment? Route to ARCH or resolve directly with UI Expert next
touchpoint — small, non-blocking.

## Dirty-tier model (extends `Pipeline::PreviewDriver`, `PreviewDriver_PIPELINE.h`)
Both experts converge on four tiers, with the key insight that "single-layer recomposite" mostly
**evaporates as a GPU concept** once markers are immediate-mode — there's no bake to invalidate,
only a CPU-side style table the draw call reads at zero marginal cost.

1. **FullRegen** (existing `bNeedsMapUpdate`) — unchanged.
2. **FullRecomposite** (existing `bNeedsPreviewRender`, **scope-narrowed**) — terrain/water/
   stratum visual param changed → rerun clear + field-layer passes only. No longer includes
   markers.
3. **MarkerStyleChange** (new, GPU-free) — per-category color/visibility/icon toggle → mutate a
   small CPU style table the draw-list reads next frame. Not a `PrepareRun()`-triggering flag;
   closer to the RT-toggle pattern (`UI_FRAMEWORK_SPEC` item 7). Cost is **bounded by category
   count, not marker count** (Compute Optimization Expert, round 2: if per-category z-order is
   also mutable, draw-call merging across same-atlas-page categories can regress from one draw
   call per page to one per (category, page) pair — still O(category count), cheap, but not
   literally free; state this precisely in the work-order's acceptance test rather than "zero
   marginal cost"). ⚠️ **Both UI experts flag the same open question**: does this live as a
   `PreviewDriver`/PIPELINE-layer tier, or is it purely UI-local state with no PIPELINE
   involvement at all, since it never triggers PROC or GPU recompute? `PreviewDriver_PIPELINE.h`
   is explicitly PIPELINE-layer — adding a tier there needs ARCH's ruling, not a default. Compute
   Optimization Expert (round 2) leans UI-local: `MarkerStyleChange`/`ScreenSpaceOnly` never
   invoke a `DispatchPolicy`-selected stage, so they sit outside pillar #14's DAG/dirty-hash
   concern entirely — there's no stage to skip — but this is ARCH's ownership call to make.
4. **ScreenSpaceOnly** (pan/zoom/click) — zero GPU recompute, zero CPU regen. Base texture
   resampled via existing `TextureWindow()`; icon draw-list resubmitted at new screen positions.
   Effectively how terrain already behaves; icons just need to ride along.

## Round 2 — Compute Optimization Expert validation
**Verdict: CONFIRM**, with two phrasing corrections applied inline above (GPU-residency scope,
`MarkerStyleChange` cost bound) and no threshold at which the FBO approach would win instead —
quad-vertex generation is strictly cheaper per element than a full-texel gradient-sample-and-blend
at any on-screen marker density; the only real risk at extreme density is visual overdraw, already
correctly scoped as a UX/LOD question, not a compute-cost one.
- World→screen projection: composing `BuildEntityPoints`'s world→texel map with
  `MapCanvasView`'s texel→screen map is two chained affine transforms, collapsible to **one
  scale+offset pair computed once per frame, applied per marker as a single FMA** — the
  work-order should specify this collapsed single-pass form (pillar #9 conformant), not two
  chained per-marker function calls.
- SIMD: `Sanmath_SIMD.h` has no FMA/blend/load-store/gather primitives to reuse yet (a
  preexisting `MATH_SIMD_SPEC` gap, not introduced by this design). Scalar-per-marker projection
  is the same proven-fine cost class `Picking_UI` already uses at 100k scale — not a blocker;
  flag vectorized batch projection as a future `MATH_SIMD_SPEC` v2 target, not a requirement here.
- Determinism: no gate implication. `DETERMINISM_SPEC`'s cross-machine bit-exact gate governs
  generation/simulation data, not rendered pixels — marker *placement* stays PROC-computed and
  untouched; only *drawing* moves from CPU-baked-texture to immediate-mode, same class of
  operation as today's `OverlayPassCpu`, just a different destination.

## Hardware-cost summary (UI Optimization Expert, code-derived complexity — not yet benchmarked;
flag as the eventual coder work-order's acceptance-test deliverable)
- N-layer FBO blend model: `O(resolution²)` full-surface touches per layer change (confirmed
  field-layer passes iterate every texel, `PreviewComposite_Cpu_UI.cpp:63-82`) — e.g. ~1M texel
  R/W at 1024² to move a handful of markers, regardless of how few are on screen.
- Screen-space batched icon draw: `O(visible markers)`, using the same `SpatialGrid` chunk-bucket
  walk `PickMarker` already proves out — turns draw submission into `O(visible chunks)`, not
  `O(total instances)` and not `O(resolution²)`. This is a **strict improvement** over even
  today's baked path, which does an unconditional `O(N)` scan over every instance on every
  `PrepareRun()`.
- Draw-call cost: consecutive `AddImage` calls against the same atlas texture merge into one
  `ImDrawCmd` via imgui's own batching (same mechanism `IconGridWidget_Draw_UI.cpp` relies on) —
  one draw call per atlas page, independent of marker count.
- GPU-resident vs re-uploaded: terrain composite texture unchanged (stays resident, UV-resampled,
  no per-frame upload — the one part of today's design that already works right). Icon atlas
  already GPU-resident per `ASSET_LOADING_SPEC` (1-2 pages, disk-cached, built once). Marker
  positions **never go to GPU as a position buffer/SSBO** — stay CPU-resident SoA, draw-list
  emits screen-space quads CPU-side per frame from a `SpatialGrid` chunk query, same as picking.
  (Compute Optimization Expert, round 2: the derived screen-space quads still ride imgui's own
  existing per-frame vertex upload, same as every other on-screen widget — not a new transfer
  class, and outside `DISPATCH_INTERFACE_SPEC` §4's compute-stage SSBO contract. "Never go to
  GPU" was imprecise; corrected here.)

## Simplification unlocked (net removal, not just addition)
Once markers move off the baked composite: `EntityIdBuffer` + its SSBO
(`CompositeBufferName::kEntityIdentifiers`, `PreviewComposite_GpuBuffers_UI.cpp:68-70`) and the
whole entity-id GPU pass have **no remaining consumer** and become removable. ⚠️ Route to ARCH:
`PREVIEW_COMPOSITING_SPEC.md` currently calls the ID-buffer pick "keep — it's good," so retiring
it is a re-ratification, not a quiet drop. Neither expert found a non-entity consumer (e.g. a
terrain-cell pick) while reading, but confirm before removal is scoped into a work-order.

## WYSIWYG fidelity (UI Expert)
- Constant-screen-size icons are the *correct* fidelity model for editor glyphs — ARCH §3.2
  already establishes the preview never re-simulates in-game rendering, so "matches the bake's
  placement" is the right WYSIWYG bar, not "matches a hypothetical in-game pixel footprint."
- No new occlusion regression — the preview is flat top-down 2D with no depth concept today;
  icons always-on-top is equivalent to today's baked dot.
- Icon-vs-icon overlap: today is unordered GPU last-writer-wins (`PreviewComposite_UI.glsl:72`).
  Deterministic category-ordered draw-list submission is a WYSIWYG *improvement*.
- Per-instance tint (e.g. faction color): `ImDrawList::AddImage`'s tint overload covers this,
  zero shader work.
- ⚠️ Dense-cluster overlap at low zoom/high marker density needs a clustering/count-badge or LOD
  fallback. Both experts agree the perf side is cheap either way (same `SpatialGrid` query drives
  both a full-icon draw and a proxy-dot draw) — the *visual* rule (threshold, badge design) is a
  UX call neither expert made unilaterally. Scope as a follow-on question, not a blocker for this
  round's core design.

## Open items to carry into the ARCH round
1. ❓ Does `EntityIdBuffer`/its GPU pass retire entirely, or does something non-entity still need
   it? (Both experts found nothing; ARCH must confirm before it's scoped into a work-order as a
   removal.)
2. ⚠️ Where does the composed world→screen pure function live — extension of
   `MapCanvasView_UI.h`, or a new shared seam? (`SpatialGrid_DATA.h` already warns against a
   second copy of this class of transform.)
3. ⚠️ Does `MarkerStyleChange` live as a `PreviewDriver`/PIPELINE tier, or purely UI-local state
   with zero PIPELINE involvement? PIPELINE-layer ownership question, not UI's to decide.
4. ❓ Where does per-category marker style data (color/visibility/icon choice) live —
   `Params::GlobalMarkerSettings` (already ratified) or a new UI-only settings struct parallel to
   `PreviewCompositeSettings`? PARAMS/UI boundary call.
5. Note for later work-order scope, not this round's blocker: `PREVIEW_COMPOSITING_SPEC.md`
   already flags decals as "never composited." When decal compositing is designed, it should NOT
   default back to the baked-texture-overlay pattern this round is retiring for markers — flag
   now so a later, less-informed work-order doesn't reintroduce it.
6. Small/non-blocking: `Picking_UI` convenience-wrapper question (§Picking above) — can resolve
   directly between UI Expert and UI Optimization Expert without ARCH, if ARCH prefers not to
   rule on it.
7. UX call, not architecture: dense-marker LOD/cluster threshold and badge visual design
   (§WYSIWYG above).

## What's explicitly NOT re-litigated
- Window-fit preview scaling (`STEP44`) — shipped, closed, out of scope here.
- The PIPELINE two-tier DAG-derived hash model itself (`PreviewDriver_PIPELINE.h`'s existing
  `FullRegen`/`FullRecomposite` split) — correct as-is, this design only adds tiers, doesn't
  change how the existing two are derived.
