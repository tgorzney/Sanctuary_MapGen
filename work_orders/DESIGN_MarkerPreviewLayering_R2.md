# Design Output — Preview Overlay Layering, Round 2 (supersedes/extends R1)

Consult round 2-3 of `BRIEF_OptimizedPreviewPipeline.md`. Consulted: UI Expert, UI Optimization
Expert, Format Expert (one factual question), Compute Optimization Expert, ARCH Expert
(ratification pass). Not yet authored into `sangen_arch_pack/` — ARCH Expert's own dedicated
authoring session does that. No code written. This round widened scope significantly past R1's
markers-only framing, per human direction — read this file, not R1, as current.

## Scope correction from R1
R1 scoped "markers only." This round's human direction: the design covers **six dynamic overlay
domains** — Alloy, Spawns/Armies, Units, Props, Reclaim (not in-game yet, plan for it), Decals —
and must stay open to adding more without a code-shape change. `Props`/`Units`/`Decals` are
resolved in `Data::PlacementResults` (`PlacementResults_DATA.h:11-16`) but **never reach the
canvas today at all** — bigger gap than R1's "markers scale wrong" framing.

## Data model (ratified shape, pending naming pass)
```
enum OverlayDomainKind_UI { Alloy, SpawnsArmies, Units, Props, Reclaim, Decals };  // open/additive
enum OverlaySubLayerKind_UI { Manual, ProceduralRule };

struct OverlaySubLayerRef_UI { OverlaySubLayerKind_UI kind; int index; bool bEnabled = true; };

struct OverlayLayer_UI {
    std::string name;
    OverlayDomainKind_UI domainKind;
    bool bEnabled = true;
    PreviewBlendMode blendMode;              // reuse Ui::PreviewBlendMode — confirm w/ UI Expert
    std::vector<OverlaySubLayerRef_UI> subLayers;   // ANY mix/count of Manual + Procedural
    float thumbnailLodThresholdPixels = 5.0f;       // see LOD section
    // color[4]/iconScale: NOT always here — see ARCH ruling §5 below, per-domain source-of-truth
};
std::vector<OverlayLayer_UI> overlayLayers;   // vector order = Z order, View-toolbar stack
```
A layer's drawn set = union of every `bEnabled` sub-layer's resolved instances, one blend mode
for the whole layer. Reorder/add/remove never touch a fixed enum or switch statement.

### Sub-layer → data mapping
| Domain | Manual sub-layers | Procedural sub-layers |
|---|---|---|
| Props | `recipe.propLayers[i]` (`PropInstanceLayer`, real vector today) | `recipe.propRules[i]` |
| Decals | `recipe.decalLayers[i]` (`DecalInstanceLayer`, real vector today) | `recipe.decalRules[i]` |
| Units | One sub-layer per top-level `Army.groups[name]` (`UnitGroup`) — **flat addressing, ARCH-ratified** (§ARCH-3). Nested `UnitGroup.groups` draw as part of their top-level parent, not separately addressable. Confirmed via Format Expert: this mirrors the *official* `.sanmap` format's own `Army.groups`/`UnitGroup.units`/`.groups` tree 1:1 — not a SanGen invention. | `recipe.unitRules[i]` |
| Alloy / Spawns-Armies | ⚠️ Blocked — no `MarkerInstanceLayer` PARAMS type exists yet (same gap `BRIEF_MarkersTabUI.md` already named). Single undifferentiated Manual bucket until that lands; struct already shaped to split to N once it does. | `recipe.markerRules[i]`, filtered by `category` (Spawn vs. rest) |
| Reclaim | n/a — no data yet | n/a — no rule type yet. Slot reserved, zero cost until it ships |

Sub-layer authoring (add/remove/toggle) lives in each domain's own tab (Props/Decals/Armies/
Markers), not the View toolbar. Toolbar only orders/blends/hides whole `OverlayLayer_UI`s.

## Icon rendering — two-mode LOD (screen-constant icon was WRONG as the only mode)
Icons are not always constant-screen-size. Two draw modes, switched by a per-layer threshold:
1. **Thumbnail mode** (zoomed in enough): draw the entity's raster thumbnail at its true
   world-footprint size — `screenSize = (baseFootprint * instance.scale) / worldUnitsPerCell *
   pixelsPerCell * view.ZoomScale()`. Scales WITH zoom, by design.
2. **Strategic icon mode** (thumbnail would render below `layer.thumbnailLodThresholdPixels`,
   default 5px, tunable): switch to a fixed-size symbolic icon. Constant screen pixels below
   threshold — this is R1's "never multiply by zoom" rule, now scoped to this mode only.

⚠️ **Real gaps found, not yet solvable, don't block structure on them**:
- No world-footprint-size data exists anywhere in the codebase today (`InstancedTransform` only
  carries a scale *multiplier*, not an absolute size). Needs a new `tpId -> baseFootprintWidth/
  Depth` table, IO-layer (asset-derived, not PARAMS-authored). Buildable now with a placeholder
  default per domain; real mesh-derived bounds are later, separately-scoped work (same posture
  already used for prop thumbnails themselves).
- Today's "prop thumbnail" (`AssetAtlasCache_PropThumbnail_IO.cpp`) is a **placeholder
  flat-shaded stand-in derived from a digest of the model bytes, not a real rendered view** — its
  own header says so. Real thumbnail rendering is out of scope here too (same fence the asset
  pipeline already drew around itself).
- A "strategic icon" per entity type is **new visual content someone authors**, not a second
  render of existing data. **Decided: bespoke per blueprint**, not a generic one-glyph-per-domain
  fallback — every `tpId` needs its own authored strategic icon, same content-authoring scope as
  the existing per-blueprint thumbnail pipeline. This is real authoring/asset-pipeline work
  (out of this design's scope), not a rendering detail.
- `IconAtlasEntry`/`IconAtlasManifest` (`IconGridWidget_UI.h:25-46`) is one `iconId` → one UV
  rect today. Don't widen the struct (its only consumer, the icon-picker grid, only wants one
  slot). Add a **separate pairing lookup** the new overlay renderer consumes:
  `tpId -> {thumbnailIconId, strategicIconId}`, each id still resolving through the existing
  single-slot manifest unchanged. The widget's own header already names this exact seam as
  anticipated ("when M5-4 lands it either publishes this shape or an adapter fills it").

## Rendering / performance (Compute Optimization Expert + UI Optimization Expert)
- Reuse the existing resident icon atlas (`Ui::IconAtlasManifest`) — already shared by Markers/
  Armies/Props pickers, already proven at 10k+ scale via `ImGuiListClipper`-style virtualization.
- ⚠️ **Correctness-critical clarification for the work-order**: "batched icon quads" must mean
  **one bulk `ImDrawList::PrimReserve` + raw vertex/index writes per layer**, NOT N individual
  `ImDrawList::AddImage()` calls. Per-call overhead at 600k markers could plausibly cost
  30–60ms — bigger than the entire frame budget — independent of the transform math. This must
  be a stated requirement, not left to a coder's default imgui usage.
- Per-instance vertex-gen cost is small and vectorizable (SoA `positionX/Y` already exists,
  textbook SIMD transform+cull+compact shape) — reasoned low-single-digit ms for the transform
  stage; bulk vertex-buffer write reasoned ~3–5.5ms at 600k instances (92 bytes/instance,
  bandwidth-bound). **Not yet measured** — needs a real microbenchmark (plan specified by Compute
  Optimization Expert: time SIMD-transform, bulk-write, and naive-AddImage separately at
  N ∈ {100k, 300k, 600k}, both 0%-culled and ~5%-visible cases, on real dev hardware, before any
  number here becomes a ratified constant per Constitution §7/§12 basis-tag law).
- Per-layer AABB early-out + per-layer `Data::SpatialGrid` for view-window culling. ⚠️ The grid
  gives **zero help** in the fully-zoomed-out worst case (everything visible, every bucket
  queried) — that case is genuinely O(N), the cross-layer budget/decimation mechanism below is
  what bounds it, not the grid.
- **Mandatory in the first work-order, not deferrable**: a cross-layer visible-vertex budget with
  automatic decimation (screen-cell clustering, then priority-cap fallback). Rough-estimate
  placeholder default ~400–500k instances before decimation kicks in (derived from a 3ms-of-16ms
  frame-budget target) — **explicitly a placeholder pending the real benchmark**, must ship as a
  named tweakable setting, never a literal.
- ⚠️ **Atlas page bucketing required**: thumbnails for many distinct prop templates can legally
  scatter across many atlas pages (general bin-packed atlas, no same-page guarantee). Drawing in
  raw visit order risks draw-call count regressing toward O(pages touched) instead of O(layers).
  Fix: accumulate each visible instance's quad into a per-page bucket during vertex-gen, flush one
  draw command per non-empty bucket — bounds draw calls to O(pages touched this frame) regardless
  of visit order. State this as a work-order requirement. (Strategic-icon mode is naturally safe
  here — small fixed low-cardinality icon set, put them on one dedicated always-resident page.)
- LOD threshold crossing during zoom needs no new invalidation rule — zoom already invalidates
  Tier C2's cache (below) unconditionally. If the threshold itself becomes a live-tunable slider,
  editing it mid-gesture folds into the existing "layer setting changed" invalidation bucket.
- Layer-id column: **do not physically resort `PlacementInstances` by layer**. Reuse existing
  `ruleIndex`/`category` columns (`PlacementInstance_DATA.h:46-47`) via a CSR bucket index built
  once (same lifecycle point as `Data::SpatialGrid`'s build, right after Placement) — per-layer
  flat index arrays, cached, rebuilt only when that layer's own sub-layer membership changes, not
  every frame. ⚠️ Manual sub-layers need a stable id column to key the same scheme — not
  confirmed to exist yet, may be a small new-column ask. ⚠️ Decals currently have no SSBO/pass at
  all (`PREVIEW_COMPOSITING_SPEC.md:54-56`) — confirm with Generator/Format Expert whether Decals
  sources from `PlacementInstances` at all before assuming one CSR scheme covers all 6 domains.

## Dirty-flag tiers (4)
| Tier | Trigger | Cost |
|---|---|---|
| A — Full regen | Sim/recipe param changed | Unchanged (PROC) |
| B — Full recomposite | Terrain/water/stratum layer setting changed | Unchanged pass sequence. ⚠️ Cost claim must be split by resolution tier, not one number: sub-ms-to-low-ms credible at the 512² default, plausibly several-to-10ms+ at the 8192² cap (256× the pixel work) — benchmark both, don't ship one range. |
| C — Screen-space redraw | Every overlay layer, every frame: pan/zoom/hover/visibility/blend/reorder | Zero GPU recompute, per-layer culled, bounded by the cross-layer budget above |
| C2 — Interaction-scoped redraw (new) | Active drag/edit on a marker or group | Cache non-selected instances' generated vertex+draw-command bytes once at gesture-start (CPU bytes, not a GPU texture/FBO), replay via memcpy each frame, regenerate live only the selection. Invalidates on pan/zoom/selection-change/layer-setting-change mid-gesture. |

Reorder and blend-mode changes in the View stack are O(layerCount), never O(instances) for
standard blend modes. ⚠️ Confirm chosen blend modes don't need divergent per-vertex color
encoding (e.g. premultiplied vs straight alpha) — if one does, that layer's C2 cache must
invalidate on mode change, and thumbnail-vs-strategic swap needs the same check.

## View toolbar (replaces "Regenerate")
- "View" button → popup listing `overlayLayers` (click-to-open, not literal hover — hover-close
  would fight a drag-reorder gesture reaching into the list; confirm this reading is correct).
- Rows via the existing `DraggableList` widget (same one `LayersTab` already uses for GeoLayers)
  + a `Combo_UI` blend-mode picker per row. No new widget, straight reuse.
- **ARCH-ratified: retire "Regenerate" from the primary toolbar.** `Pipeline::PreviewDriver`
  already auto-derives refresh tier from parameter hashes (`NotifyParametersChanged()`) — a
  manual full-regen button is the exact anti-pattern that system exists to replace.
  ⚠️ **`MapCanvas::RequestRegeneration()` is currently a second, rival trigger path** alongside
  `PreviewDriver::RequestMapUpdate()` — must collapse to one call path, not leave both live. Keep
  exactly one debug/System-panel affordance calling `RequestMapUpdate()` directly, for the one
  legitimate manual case `PreviewDriver`'s own docstring already names ("a change no parameter
  hash can see: a resize, a recipe reload, new stratum art") — not on the View toolbar.
- **Resolved by UI Expert's dedicated pass**: `fieldLayers` and `overlayLayers` stay two separate
  backing vectors (different row shapes — `PreviewLayerKind`+ramp vs. `OverlayDomainKind_UI`+
  heterogeneous `subLayers` — a merged type would need this codebase's first discriminated-union
  settings struct, rejected). **One popup, two sections**: `ImGui::BeginPopup("ViewLayersPopup")`
  renders both as two independent `DraggableList` calls, separated by a static section label
  ("Terrain (composited)" / "Overlays (screen-space)"), each row still with its own blend-mode
  `Combo_UI`. Reorder is real *within* each section (drag Flow above Water → visibly changes the
  texture blend; drag Props above Decals → visibly changes screen-space draw order) but a row
  **cannot cross sections** — true interleaving (a marker rendering "under" a terrain layer) is
  rejected outright: not renderable without either re-baking markers into the texture (the exact
  bug this whole redesign kills) or rebuilding `PreviewComposite` into an interleaved multi-target
  compositor, and a control that *looks* interleaved but isn't would violate the WYSIWYG law
  (`PREVIEW_COMPOSITING_SPEC.md`) by showing an order that isn't the real render order. Mechanism:
  give the two `DraggableList` renders different drag-payload identifiers so cross-section drops
  structurally fail to match, no new validation code. `overlayLayers` recommended session-only
  presentation (not recipe-serialized), matching `fieldLayers`' already-ratified policy — same
  toolbar, same policy, ARCH to confirm formally.
  **Confirmed by human**: two sections, no crossing — matches the recommendation above.

## ARCH rulings (this round)
1. **Module boundary**: `OverlayLayer_UI`/`overlayLayers` is UI, same precedent as
   `PreviewCompositeSettings::fieldLayers`. Procedural sub-layers reuse existing DATA columns
   (`ruleIndex`/`category`) — no new DATA field. Manual sub-layers never touch DATA at all — they
   read `Params::MapRecipe` pass-through arrays directly, filtered by existing `layerIndex`/group
   identity. **This DATA-vs-PARAMS split must be named explicitly in the spec**, not assumed
   homogeneous. ⚠️ GPU-resident draw state must route through the existing SYS
   `GpuResourceManager` (`DISPATCH_INTERFACE_SPEC` §3) — do not reintroduce a UI-owned GL
   pipeline, already a named v1 defect class.
2. **`OverlayDomainKind_UI` vs. `MarkerCategory`/`PlacementResults`**: sits alongside, changes
   neither. `Alloy`/`SpawnsArmies` re-slice the existing `markers` buffer by its existing
   `category` column — a UI enum may re-slice an existing DATA collection by its own field
   without the DATA shape changing. Zero blast radius on `MarkersTab_Rules_UI.h` or marker
   import/export. ⚠️ Domain-kind is asymmetric vs. DATA buckets (splits markers 2 ways, maps
   Props/Units/Decals 1:1) — name this explicitly so a coder doesn't assume domain == DATA-bucket
   identity.
3. **Nested `UnitGroup` addressing: flat.** Top-level `Army.groups[name]` = one sub-layer; nested
   subgroups draw as part of their parent, not separately addressable. Keeps `OverlaySubLayerRef`
   uniform across every domain (no domain-specific recursive-index special case) and avoids the
   "flattened pre-order index into a mutable recursive tree" corruption class
   `ENTITY_AUTHORING_PARAMS_SPEC` already ruled against elsewhere. Recursive addressing is a
   legitimate future ask, needs its own ratification if actually requested later.
4. **fieldLayers/overlayLayers unification: not ratifiable as scoped** — see View toolbar section
   above. Route back to UI Expert with the two named open questions before ARCH will rule.
5. **View-stack state — split by field, not one blanket policy**:
   - Order / `bEnabled` / blend mode: session-only UI presentation (same policy already governing
     `PreviewCompositeSettings`; V1's serialized `PreviewLayers` was already a named defect to
     replace, not evidence v2 must re-serialize).
   - `color`/`iconScale`: ⚠️ **cannot be a blanket UI-only field.** For Props/Decals these are
     **already recipe-serialized PARAMS** (`PropInstanceLayer`/`DecalInstanceLayer`). Where a
     domain already owns a recipe-serialized layer-metadata record, `OverlayLayer_UI` reads/
     writes that record directly — no shadow copy, no second source of truth. Where no such
     PARAMS record exists yet (Alloy/SpawnsArmies, Units), these stay UI-session defaults until a
     future ratification gives them a real home — mirror the shipped Props/Decals pattern, don't
     invent a new one now.
6. **Naming**: `OverlayLayer`/`OverlayDomainKind`/`OverlaySubLayerRef` need the `_UI` suffix when
   they land in a real file (Constitution §2) — reflected above.

## GPU-readback bug (found independently, real, separate fix)
`ComposeOnGpu()` (`PreviewComposite_Gpu_UI.cpp:78-81`) unconditionally reads back the full
**color texture** even on the GPU-resident hot path where nothing downstream consumes it
(confirmed: `Application_UI.cpp:82-83` only consumes it `if (!composite.LastRunUsedGpu())`) — up
to 256MB wasted PCIe transfer + blocking wait at the 8192² cap, every recompose. ⚠️ **Correction**:
the **entity-id buffer** readback on the same lines is *not* dead — `MapCanvas_UI.cpp:33-37`
click-picking reads it unconditionally on both backends. Fix scope: gate only the color-texture
readback on `!bLastRunUsedGpu`, leave entity-id readback as-is. Land independent of, and before,
the overlay redesign — narrow, already-diagnosed, compounds with every future Tier B trigger.

## Determinism
No concern — presentation-only, same reasoning `OPTIMIZATION_PILLARS.md` pillar 15 already
applies to GPU-resident preview compositing (Visual-class, excluded from bit-exact determinism
gate). ⚠️ One guardrail worth one sentence in the ratified spec: any future screen-space
decimation/clustering must only affect what's *drawn*, never mutate/discard `PlacementInstances`
or feed back into export/bake — otherwise a "helpful" LOD optimization could silently become a
second, non-deterministic placement decision.

## Consolidated ❓ open items for next touchpoint
1. Terrain fieldLayers + marker overlayLayers: one unified View list or two backing arrays under
   one UI? → sent back to UI Expert for a dedicated pass (in progress).
2. Real footprint-size source: placeholder-per-domain now; who/when derives real mesh bounds
   later?
3. Cross-layer visible-vertex budget default and Tier B per-resolution costs: need the actual
   benchmark (plan specified above), not the reasoned placeholders in this doc.
4. Manual sub-layer stable-id column: confirm exists or needs adding, for the CSR bucket scheme.
5. Decals: confirm data source (do they route through `PlacementInstances` at all today?) before
   assuming one CSR scheme covers all 6 domains.
6. `OverlayLayer_UI::blendMode` — reuse `Ui::PreviewBlendMode` verbatim, or new enum? (ARCH leans
   reuse, wants UI Expert to confirm at authoring time.)

## Out of scope, noted for a future separate conversation
Icons/thumbnails/unit blueprints ultimately come from a texture importer reading sanpack (zip)
files out of a user-selected Gamedata folder, multiple formats including `.dds`. A "Browse
Gamedata folder" file-select affordance and an icon/thumbnail/unit browser UI are both needed to
support this, but neither is in scope for the preview-compositing design — separate conversation.

## ⚠️ Consolidated risks / must-not-skip items for the work-order
- Bulk vertex-buffer writes required, not per-marker `AddImage()` calls — real frame-budget risk
  if skipped.
- Cross-layer visible-vertex budget + decimation must ship in the first work-order.
- Atlas page bucketing (one draw call per page touched) must ship in the first work-order.
- `MapCanvas::RequestRegeneration()` / `PreviewDriver::RequestMapUpdate()` rival paths must
  collapse to one.
- GPU color-texture readback waste — separate, narrow, should land first.
- `color`/`iconScale` duplicate-source-of-truth risk for Props/Decals if `OverlayLayer_UI` invents
  its own copy instead of reading the existing PARAMS record.

## What's explicitly NOT re-litigated
- Window-fit preview scaling (`STEP44`) — shipped, closed.
- The PIPELINE two-tier DAG-derived hash model itself — correct as-is, this design adds tiers, not
  changes to how the existing two are derived.
- Heightmap/erosion dependency-aware recompute — real, deferred to a separate round with Generator
  Expert + Compute Optimization Expert, not this document's scope.
