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
this (Decals is one of the six overlay domains, §14.2), but §14.13 item 4 records
that whether Decals actually route through `Data::PlacementInstances` the way
Props/Units do was **not** independently re-verified in that ratification pass.
Treat "decals reach the canvas" as designed, not yet shipped, and the data-source
question as still open.

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
  question still open, §14.13 item 4); **accumulation has no own ramp** (v2's
  `PreviewCompositeSettings::gradientRamps` is one-ramp-per-field, ARCH §8.2, so this
  is already fixed in the real v2 tree); **global mutable statics** (single-context,
  resize-keyed reallocation misses format changes) — v1 only.
- **`loc_stratumRemaps` uniform is dropped, not ported** — it was a global,
  stratum-0-only contrast knob (see "Coloring" above), not a real per-stratum
  mechanism; v2's stratum splat is `surfaceStratumWeights × tint` only
  (`MASKING_SPEC` §1.2).

## v2 guidance (terrain/field compositing — unchanged by ARCH §14)
Preview = composite + colorize + pick over the **single baked result**; never
re-simulate. Two-tier dirty flags (A/B, above) derived from the DAG for the
terrain/field layers; preview-resolution/accuracy toggle (shipped); per-map
independent ramps (shipped); all constants tweakable (Constitution §8); GPU via the
shared resource manager; one source of truth with the bake (`DISPATCH_INTERFACE_SPEC`,
`MASKING_SPEC`, `PLACEMENT_SCATTER_SPEC`). This paragraph governs
`PreviewCompositeSettings::fieldLayers` — the GPU-recomposited terrain/water/stratum
stack. It does **not** govern the six screen-space overlay domains (markers, armies,
props, decals, reclaim) — those are a structurally different draw path, specified in
full below.

---

## Overlay layering (v2, ARCH §14) — markers/armies/props/decals/reclaim are screen-space, not baked-texture

**Ratifies `work_orders/DESIGN_MarkerPreviewLayering_R2.md`. Binding law; see
`ARCH.md` §14 for the full ruling text and reasoning — this section is the
implementation-facing summary a coder actually reaches for.** Supersedes the
markers-only framing of the historical, superseded `DESIGN_MarkerPreviewLayering_R1.md`
— do not consult R1 as current.

### The core distinction this section exists to enforce
`PreviewCompositeSettings::fieldLayers` (above) bakes terrain/water/stratum into the
shared `GL_RGBA8` composite texture — a Tier B recomposite is real GPU recompute.
**Overlay domains never do this.** Alloy/Spawns-Armies/Units/Props/Reclaim/Decals are
drawn **on top**, in screen space, every frame, from the already-resolved
`Data::PlacementInstances` the bake accepted — panning, zooming, toggling a layer, or
dragging one marker never re-touches the shared composite texture. Baking a marker
into that texture is the exact v1 bug this design exists to kill (ARCH §3.2, "the
shadow-sim problem" above) — a coder must not "simplify" by routing an overlay domain
back through `PreviewCompositeSettings::fieldLayers`.

### Data model
```cpp
enum class OverlayDomainKind_UI   { Alloy, SpawnsArmies, Units, Props, Reclaim, Decals };
enum class OverlaySubLayerKind_UI { Manual, ProceduralRule };

struct OverlaySubLayerRef_UI { OverlaySubLayerKind_UI kind; int index; bool bEnabled = true; };

struct OverlayLayer_UI {
    std::string name;
    OverlayDomainKind_UI domainKind;
    bool bEnabled = true;
    Ui::PreviewBlendMode blendMode;                 // reuse leaning, ARCH §14.13 item 5 (open)
    std::vector<OverlaySubLayerRef_UI> subLayers;    // any mix/count of Manual + ProceduralRule
    float thumbnailLodThresholdPixels = 5.0f;
};
std::vector<OverlayLayer_UI> overlayLayers;          // vector order = Z order
```
Sub-layer → data mapping, LOD icon rendering, the View-toolbar's two-section popup,
the four dirty-flag tiers (A/B/C/C2), the mandatory perf requirements (bulk vertex
writes, cross-layer budget + decimation, atlas page bucketing), the "Regenerate"
retirement, the GPU-readback bug, and the determinism guardrail are all specified in
full in `ARCH.md` §14.1–§14.11 — not duplicated here to avoid a second copy drifting
out of sync. Read `ARCH.md` §14 alongside this section before implementing any part
of the overlay pipeline.

### Open items — not settled by this ratification (`ARCH.md` §14.13)
1. Real world-footprint-size data source (placeholder-per-domain only, today).
2. Cross-layer visible-vertex budget default and Tier B per-resolution costs — both
   are rough-estimate placeholders pending a real microbenchmark.
3. Whether a stable id column exists yet for manual sub-layers (needed for the CSR
   bucket scheme, ARCH §14.9).
4. Whether Decals actually route through `Data::PlacementInstances` at all today —
   confirm before assuming one CSR scheme covers all six domains.
5. `OverlayLayer_UI::blendMode` — reuse `Ui::PreviewBlendMode` verbatim vs. a new
   enum — UI Expert's call, not made here.

A coder or future ARCH pass must treat all five as genuinely open, not resolved by
this document's existence.
