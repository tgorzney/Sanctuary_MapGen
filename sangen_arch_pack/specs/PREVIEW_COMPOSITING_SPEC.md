# PREVIEW_COMPOSITING_SPEC — building the preview image (and why it must equal the bake)

Source: `gui/PreviewRenderer.cpp/.h`, `shaders/PreviewCompute.glsl` (referenced),
`core/gen/Gen_Tex_Albedo.h`, `core/params/Params_Geometry.h` (`StratumSettings`,
`SlopeSettings`), `core/params/Params_Gradients.h`, `core/Parameters.h` (dirty
flags, `EntityIDBuffer`). This is the WYSIWYG surface: it composites height, shading,
stratum splats, water, and entities into one image and drives O(1) picking. It reads
the outputs of every other spec and must show **bake truth**, not a second
implementation of it.

**This spec mixes two things, kept in separate halves below.** Everything down to
"v2 guidance" is **v1 legacy analysis** (`gui/PreviewRenderer.cpp`, ground truth for
what v1 got wrong) — read it as history, not current law. Everything from
"Overlay layering (v2, ARCH §14)" onward is **ratified v2 law**, binding on the real
v2 tree (`src/ui/PreviewComposite_*_UI.*`, `PreviewComposite_Settings_UI.h`) that
already exists there today.

## The composite pipeline (one GPU compute shader, permutation stack)
`PreviewRenderer::UpdatePreviewTexture` dispatches `PreviewCompute.glsl` as a
sequence of permutations into a single `GL_RGBA8` image:
1. **Clear** (`PASS_CLEAR`).
2. **Layer passes** — iterate `params.PreviewLayers` in UI order; each dispatches
   perm `layerType+1` (13 layer types) and blends per `layer.Blend`
   (`GenerationParams::LayerBlendMode` — the **preview-only** Z-order enum, distinct
   from geometry `BlendMode`; see `NOISE_BLEND_SPEC`). Height shading, slope, flow,
   accumulation, stratum/albedo, water, markers/props are all selected here.
3. **Overlay** (`PASS_OVERLAY`) — debug focus-gradient, conditional.
4. **EntityID readback** from SSBO 0.

SSBO layout: `0=EntityIDBuffer` · `1=heightmap` · `2=FlowMap` · `3=AccumulationMap`
· `4=9 MaterialMasks` · `5=area bounds/colors` · `6=marker/prop rule bounds` ·
`7=baked gradient LUTs`.

**Note on SSBO 5 (`area bounds/colors`) — this is the direct v1 precedent ARCH §14.17
restores, not a legacy oddity to be avoided.** v1 already shipped map areas to the
compositor as an analytic bounds+color buffer and resolved them per pixel in the
shader; that is exactly the shape §14.17 ratifies for v2 (`CompositeBinding::
kMapAreaRectangles`, `PreviewMapAreaRectangle`). What v1 got wrong here was NOT the
buffer — it was SSBO **6**, where raw placement-rule bounds were shipped and the
shader re-decided placement (see "The shadow-sim problem"). Areas carry no placement
rule, so SSBO 5 never had that defect. See "Map areas are a field layer, not an
overlay domain" below.

## Coloring
- **Height ramp / auto-level**: `AutoLevelPreview` scans min/max CPU-side; absolute
  range from `TerrainMaxHeight`.
- **Gradient ramps**: `buildGradientCache` bakes each `GradientSettings` (`.Stops`,
  `.SmoothInterpolation`; `GradientStop.Location/.Color`) into a 256×4 LUT (SSBO 7) —
  slope / flow / accumulation / water. (Accumulation currently **reuses the flow
  gradient** — "legacy"; give it its own ramp in v2.)
- **Stratum splat**: `stratColors` from `Stratums[i].previewColor`, multiplied by
  the stratum's `MaterialMasks` weight — splat weight × preview tint. A single
  global `stratumRemaps` uniform (`loc_stratumRemaps`) is also uploaded, but it is
  **not** a per-stratum remap: the code builds one `[maskRemapMin[0], maskRemapMax[0]]`
  pair per stratum and then only ever reads stratum **0**'s pair, applying it
  identically to the whole splat blend. It is a legacy, ad-hoc, global contrast knob
  — not evidence of a real per-stratum remap mechanism (that reading was
  investigated and withdrawn, ARCH §7.2 item 5 / `MASKING_SPEC` §1.6, Part 2
  "Consumption (legacy)"). v2 does not carry this uniform forward. Stratum cap
  **9** is hardcoded everywhere.
- **Water**: `Water.WaterLevelMax/DeepWaterDepthMin/DeepWaterDepthMax`,
  `FlowMapColor`.
- The real albedo/mask blend is baked upstream (`Gen_Tex_Albedo::ApplyAlbedoMask`,
  impl absent from the snapshot); the preview receives finished weights.

## Stratum & decals (v1 state — superseded for decals by ARCH §14, see below)
`StratumSettings` supplies the tile/normal/physics fields (see `MASKING_SPEC`,
`GAMEDATA_LAYOUT_SPEC`); its `maskRemapMin`/`maskRemapMax` fields are per-stratum
material/appearance pass-through data consumed only by the game's own renderer
against the stratum's composite texture — no SanGen stage, preview included,
computes with them (`MASKING_SPEC` §1.6). **Decals were never composited in v1** —
`DecalRule` exists and is imported (`ImportedDecalsJSON`) but the v1 preview has no
decal SSBO or pass. As of this spec's last direct check, `src/ui`'s real v2 preview
still has **no decal draw path either** — ARCH §14 ratifies the design that closes
this (Decals is one of the six overlay domains, §14.2). **Update (§14.13 item 4,
closed):** confirmed procedural Decals already resolve into
`Data::PlacementResults::decals`, the identical `Data::PlacementInstances` SoA type
with identical `ruleIndex`/`category` columns Props/Units use — the §14.9 CSR bucket
scheme applies to procedural Decals unchanged, no special-case needed. The remaining
gap is purely a **missing draw-pass consumer** (no compositor reads them yet), not a
data-shape mismatch — "decals reach the canvas" is designed, not yet shipped, but the
data-source question that was open is now closed. Separately, and unresolved
(§14.13 item 3): manual/authored decals (`recipe.decals`) are not yet live-wired into
`BuildSanmapJsonText`/`ParseSanmapJsonText` and do not resolve into `results.decals`
at all today — a distinct gap from the procedural path above.

## The shadow-sim problem (the central hit-list item)
The v1 preview **re-derives** results instead of sampling the bake:
- **Slope** is recomputed in-shader from the heightmap using `cellSize` +
  `bUseEngineParityMath` — two code paths that must match the CPU bake but can
  silently diverge.
- **Marker/prop filtering** is re-done in-shader: raw `rule.MinSlope/MaxSlope/
  MinHeight/MaxHeight` are shipped to SSBO 6 and the shader decides which cells pass
  — so a marker the **bake rejected can still paint in preview** (and vice-versa).
- **Flow/accumulation** colorized live from raw maps, independent of baked shading.
So slope derivation, flow colorization, and rule filtering each exist **twice**
(CPU bake + GLSL preview) → "preview truth ≠ bake truth." v2 rule: the preview
**samples the single baked result** (one source of truth per Constitution §4 and
`DISPATCH_INTERFACE_SPEC`); it colorizes and composites, it does not re-simulate.
This rule is unaffected by, and is a precondition for, the overlay-layering design
below: overlays draw the resolved `PlacementInstances` the bake already accepted,
never a second in-shader re-test of a placement rule.

**Read the scope of this rule precisely (ARCH §14.17 item 1).** The defect is
**re-deciding a placement rule the bake already resolved** — not "using anything that
is not a baked `Data::FloatField`." A per-pixel color source flattened from PARAMS
with no placement rule behind it (the stratum splat's tints, water's level
thresholds, map-area rectangles) is legal and always was; SSBO 5 was never part of
this problem, only SSBO 6 was.

## Picking (keep — it's good)
CPU seeds `EntityIDBuffer` (`0xFFFFFFFF` = empty); the compute passes write a
per-pixel entity ID into SSBO 0 while shading; after all passes it's read back so a
click is `EntityIDBuffer[y*w+x]` — O(1), no 100k test. Formalize the sentinel and
buffer as named constants.

## Dirty flags (v1: two-tier — see ARCH §14.8 for the ratified v2 four-tier model)
- **`bNeedsMapUpdate`** → full async CPU regen (`TerrainGenerator::GenerateMap`:
  height/erosion/flow/masks/placement); on completion sets `bNeedsPreviewRender` +
  `bGeometryChanged`.
- **`bNeedsPreviewRender`** → GPU composite only.
- **`bGeometryChanged`** → gates the expensive big-map SSBO re-uploads (height/flow/
  accum/masks); rule/area/gradient SSBOs + dispatch run every render.
Assignment is inconsistent per widget today (some visual-only controls trip regen,
and vice-versa). v2 derives which flag each parameter sets from the dependency DAG
(`PARAMS_PIPELINE_SPEC`), not by hand (`UI_FRAMEWORK_SPEC`) — and ARCH §14.8 adds two
further screen-space-only tiers (C, C2) on top of this pair for the overlay layers,
which never touch the GPU recompute this two-tier model gates.

## CPU vs GPU
GPU: clear, all shading/colorization, splat composite, marker/prop raster, entity-ID
write. CPU: full generation, auto-level scan, gradient LUT bake, SSBO packing,
entity-ID readback. **No resolution/quality toggle** exists in v1 — the v1 preview
always runs at full resolution. The real v2 tree already has one
(`PreviewCompositeSettings::previewResolution`, `PreviewComposite_Settings_UI.h`;
Constitution §4 Visual class for scrubbing, escalate on idle per ARCH §4.4).

## Known issues (v1) fixed or being fixed in v2
- **Shadow reimplementation** (above) — the WYSIWYG-breaking bug; unify on sampling
  the bake. Fixed in the real v2 tree's composite/dirty-hash design.
- **God-object / layer violation**: `UpdatePreviewTexture` (~300 lines) owns GL
  loading, shader compile, all SSBO packing, gradient baking, sim-parameter
  interpretation, dispatch, picking — and **mutates a `const` params** (`EntityIDBuffer`).
  Split per Constitution §1 (UI composites; SYS owns GL; PROC owns sim) and route GPU
  through the shared resource manager (`DISPATCH_INTERFACE_SPEC`), not a private
  UI-layer pipeline. This split is why the real v2 tree is already several
  `PreviewComposite_*_UI.*` files, not one.
- **Hardcoded**: absolute shader path (blank preview if missing), stratum cap 9,
  entity sentinel, workgroup 16, slope default colors, 15-slot program array kept in
  sync with `LayerType` by hand.
- **Duplicate/empty types**: two `StratumSettings`, empty `TerrainType_Decal.h`.
- **Decals never previewed** — being closed by ARCH §14 (design ratified; data-source
  confirmed and closed, §14.13 item 4 — see "Stratum & decals" above; draw-pass
  consumer still unbuilt); **accumulation has no own ramp** (v2's
  `PreviewCompositeSettings::gradientRamps` is one-ramp-per-field, ARCH §8.2, so this
  is already fixed in the real v2 tree); **global mutable statics** (single-context,
  resize-keyed reallocation misses format changes) — v1 only.
- **`loc_stratumRemaps` uniform is dropped, not ported** — it was a global,
  stratum-0-only contrast knob (see "Coloring" above), not a real per-stratum
  mechanism; v2's stratum splat is `surfaceStratumWeights × tint` only
  (`MASKING_SPEC` §1.2).
- **Map areas were dropped from the v2 composite and are being restored** — v1's
  SSBO 5 had no v2 counterpart until ARCH §14.17; the v2 tree drew areas only as an
  imgui immediate-mode canvas pass (ARCH §21.8). Restored as a real composited field
  layer; see the §14.17 section below.

## v2 guidance (terrain/field compositing — unchanged by ARCH §14)
Preview = composite + colorize + pick over the **single baked result**; never
re-simulate. Two-tier dirty flags (A/B, above) derived from the DAG for the
terrain/field layers; preview-resolution/accuracy toggle (shipped); per-map
independent ramps (shipped); all constants tweakable (Constitution §8); GPU via the
shared resource manager; one source of truth with the bake (`DISPATCH_INTERFACE_SPEC`,
`MASKING_SPEC`, `PLACEMENT_SCATTER_SPEC`). This paragraph governs
`PreviewCompositeSettings::fieldLayers` — the GPU-recomposited terrain/water/stratum/
map-area stack (**not** every entry of which is a baked raster field: `StratumSplat`,
`Water` and `MapAreas` are PARAMS-flattened analytic sources, ARCH §14.17 item 1). It
does **not** govern the six screen-space overlay domains (markers, armies, props,
decals, reclaim) — those are a structurally different draw path, specified in full
below.

---

## Overlay layering (v2, ARCH §14) — markers/armies/props/decals/reclaim are screen-space, not baked-texture

**Ratifies `work_orders/DESIGN_MarkerPreviewLayering_R2.md`. Binding law; see
`ARCH_14_PreviewOverlayLayering.md` §14 for the full ruling text and reasoning — this section is the
implementation-facing summary a coder actually reaches for.** Supersedes the
markers-only framing of the historical, superseded `DESIGN_MarkerPreviewLayering_R1.md`
— do not consult R1 as current.

### The core distinction this section exists to enforce
`PreviewCompositeSettings::fieldLayers` (above) composites terrain/water/stratum/
map-areas into the shared `GL_RGBA8` composite texture — a Tier B recomposite is real
GPU recompute. **Overlay domains never do this.** Alloy/Spawns-Armies/Units/Props/
Reclaim/Decals are drawn **on top**, in screen space, every frame, from the
already-resolved `Data::PlacementInstances` the bake accepted — panning, zooming,
toggling a layer, or dragging one marker never re-touches the shared composite
texture. Baking a marker into that texture is the exact v1 bug this design exists to
kill (ARCH §3.2, "the shadow-sim problem" above) — a coder must not "simplify" by
routing an overlay domain back through `PreviewCompositeSettings::fieldLayers`.

**The gate is `Data::PlacementInstances`, not "is it a baked field" (ARCH §14.17
item 1).** Ask: *does this layer re-decide something a PROC stage already resolved?*
If yes, it is an overlay domain. If it is a per-pixel color source flattened from
PARAMS with no placement rule behind it, it is a legal `PreviewFieldLayer` kind — as
`StratumSplat` and `Water` already are, and as `MapAreas` now is.

### Data model
```cpp
enum class OverlayDomainKind_UI   { Alloy, SpawnsArmies, Units, Props, Reclaim, Decals };
enum class OverlaySubLayerKind_UI { Manual, ProceduralRule };

struct OverlaySubLayerRef_UI { OverlaySubLayerKind_UI kind; int index; bool bEnabled = true; };

struct OverlayLayer_UI {
    std::string name;
    OverlayDomainKind_UI domainKind;
    bool bEnabled = true;
    float opacity = 1.0f;                                // layer-wide alpha multiplier, folded
                                                           // into each instance's tint alpha at
                                                           // draw time — NOT Ui::PreviewBlendMode,
                                                           // ARCH §14.13 item 5 (closed)
    std::vector<OverlaySubLayerRef_UI> subLayers;    // any mix/count of Manual + ProceduralRule
    float thumbnailLodThresholdPixels = 5.0f;
};
std::vector<OverlayLayer_UI> overlayLayers;          // vector order = Z order
```
Sub-layer → data mapping, LOD icon rendering, the View-toolbar's two-section popup,
the four dirty-flag tiers (A/B/C/C2), the mandatory perf requirements (bulk vertex
writes, cross-layer budget + decimation, atlas page bucketing), the "Regenerate"
retirement, the GPU-readback bug, and the determinism guardrail are all specified in
full in `ARCH_14_PreviewOverlayLayering.md` §14.1–§14.11 — not duplicated here to avoid a second copy drifting
out of sync. Read `ARCH_14_PreviewOverlayLayering.md` §14 alongside this section before implementing any part
of the overlay pipeline.

### Open items — not settled by this ratification (`ARCH_14_13_OpenItems.md` §14.13)
1. Real world-footprint-size data source (placeholder-per-domain only, today).
2. Cross-layer visible-vertex budget default and Tier B per-resolution costs — both
   are rough-estimate placeholders pending a real microbenchmark.
3. **Manual sub-layer stable-id — a real DATA-shape work item (open, sharpened).**
   Two-part gap: (a) `PropInstanceLayer`/`DecalInstanceLayer` carry no id field that
   survives reorder/delete — the only backward reference, `layerIndex`, is a plain
   vector position that gets renumbered on reorder and clamped on delete; (b)
   `Data::PlacementInstances` has no `layerIndex`-equivalent column at all, so no
   resolved instance can be correlated back to the manual layer that authored it.
   Needs both a stable id on the layer-metadata types and a new correlation column
   (or side table) on `Data::PlacementInstances`. Manual/authored decals additionally
   don't resolve into `results.decals` at all yet (separate from procedural Decals,
   item 4). Full statement: `ARCH_14_13_OpenItems.md` §14.13 item 3.
4. ✅ **CLOSED.** Confirmed: procedural Decals route through `Data::PlacementInstances`
   exactly like Props/Units/Markers (`Data::PlacementResults::decals`, same SoA type,
   same `ruleIndex`/`category` columns) — the §14.9 CSR bucket scheme covers Decals
   with no special-case. Only the draw-pass consumer is still unbuilt. Full statement:
   `ARCH_14_13_OpenItems.md` §14.13 item 4.
5. ✅ **CLOSED.** `OverlayLayer_UI::blendMode` is retired; `opacity: float` replaces it
   (struct above). `Ui::PreviewBlendMode` is a GPU raster-compositing enum with no
   meaning for an icon-quad draw under ImGui's single global blend equation. Full
   statement: `ARCH_14_13_OpenItems.md` §14.13 item 5.

Items 1 and 2 remain genuinely open, not resolved by this document's existence. Items
3-5 above reflect their current (mixed open/closed) status; do not re-open 4 or 5
without a new ratification.

---

## Map areas are a field layer, not an overlay domain (v2, ARCH §14.17)

**Binding law. Full ruling: [`ARCH_14_17_MapAreaFieldLayer.md`](../../ARCH_14_17_MapAreaFieldLayer.md)
§14.17; this is the implementation-facing summary.** Human-approved 2026-08-29.

`Params::MapArea` rectangles composite through `PreviewCompositeSettings::fieldLayers`
as a new `PreviewFieldLayer` of kind `PreviewLayerKind::MapAreas` — **not** a seventh
`OverlayDomainKind_UI` (that enum stays closed at six), **not** a baked raster mask,
**not** a PROC stage. This restores v1's SSBO-5 `area bounds/colors` shape (see the
note under "The composite pipeline" above), which was the one analytic buffer v1 got
right; it does not restore SSBO 6, which is the shadow-sim defect and stays deleted.

Shape, at a glance:
- `PreviewLayerKind` gains `MapAreas`, **appended last** after `Slope` (the enum's
  integer values are load-bearing on both sides of the CPU/GLSL seam), with a matching
  generated `PREVIEW_LAYER_MAP_AREAS` `#define`.
- `CompositeBinding::kMapAreaRectangles = 12` (binding **7 stays vacant**, per
  `PreviewComposite_Kernel_UI.h`'s own documented hole); buffer name
  `"previewCompositeMapAreaRectangles"`.
- `struct PreviewMapAreaRectangle` — 8 scalars / 32 bytes, no padding:
  `minimumX, minimumZ, maximumX, maximumZ, colorRed, colorGreen, colorBlue, colorAlpha`.
  Coordinates in **cell space** (`world × 1/worldUnitsPerCell`), flattened CPU-side at
  `PrepareRun()`, which is exactly the space `layerColorAtPixel` already receives.
- **No count field is added to `PreviewCompositeConfiguration`** — that would break its
  80-byte/16-byte-multiple std430 stride, which is mirrored by hand in *two* GLSL units.
  Read the buffer's own `.length()`/`.size()`; push one degenerate sentinel rectangle
  (`minimumX > maximumX`) when the list is empty, preserving the "never a 0-byte buffer"
  idiom.
- Overlap: forward iteration, **last containing match wins** — one Z rule shared by the
  visual and §21.8's body hit-test, so click-to-select and what-you-see cannot disagree.
- Blend needs **no new machinery**: `layerColorAtPixel` already returns
  `vec4(rgb, coverageAlpha)` and the pass blends with `layerOpacity × layerColor.a`;
  outside every area the layer contributes nothing.
- Picking is **untouched** — areas never enter `Data::EntityIdBuffer`; §21.8's CPU-side
  hit test remains the entire area-picking story.
- `LayerSourceField` gets an explicit `case MapAreas: return nullptr;` (the
  `StratumSplat` posture), never the `default:` heightfield fall-through.
- Ownership: `AreaColorEntry`'s single owner becomes `PreviewCompositeSettings::areaColors`
  (moved out of `AreasTabState`), with the type itself relocated to a new minimal
  `src/ui/AreaColorTable_UI.h` so the settings header does not depend on a tab header.
- Defaults: new areas Green, blend **Overlay**; `"PlayableArea"` forced Green and
  non-editable; the layer seeded topmost and enabled (via the Areas panel-catalogue row
  becoming a real `PreviewVisibilityTarget::FieldLayer`).
- **`Params::MapArea` and the `.sanmap` schema are completely unchanged** — this is
  presentation state, same category as `PreviewCompositeSettings` itself.
- **Drag cost:** a transient `PreviewCompositeSettings::mapAreaSuppressedIndex` (never
  serialized) omits the dragged area from the composite input for the duration of a
  gesture, so a drag costs exactly **two** recomposites (begin + end), never one per
  frame — and the immediate-mode canvas pass draws that one area meanwhile. Amends
  ARCH §21.8's original "fill+border every area every frame" draw-pass ruling; the
  border is now edit-time-only, and never draws at all while the MapAreas layer is
  disabled.

**Documentation defect corrected by this ratification.**
`PreviewComposite_Settings_UI.h`'s comment above `PreviewLayerKind` ("Which BAKED field
a layer colorizes. Every entry names a field `Data::MapFields` actually carries") was
**already false** for `StratumSplat` before this ticket — `LayerSourceField` answers
`nullptr` for it — and is corrected in place to state what §14.17 item 1 rules: a layer
names a per-pixel color source (a baked field, a PARAMS-flattened analytic source, or a
combination); what it may never be is a re-decision of a placement rule. This spec's own
"terrain/water/stratum stack" phrasings above have been corrected to match.
