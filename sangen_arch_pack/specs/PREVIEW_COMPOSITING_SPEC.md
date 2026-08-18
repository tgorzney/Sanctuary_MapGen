# PREVIEW_COMPOSITING_SPEC — building the preview image (and why it must equal the bake)

Source: `gui/PreviewRenderer.cpp/.h`, `shaders/PreviewCompute.glsl` (referenced),
`core/gen/Gen_Tex_Albedo.h`, `core/params/Params_Geometry.h` (`StratumSettings`,
`SlopeSettings`), `core/params/Params_Gradients.h`, `core/Parameters.h` (dirty
flags, `EntityIDBuffer`). This is the WYSIWYG surface: it composites height, shading,
stratum splats, water, and entities into one image and drives O(1) picking. It reads
the outputs of every other spec and must show **bake truth**, not a second
implementation of it.

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

## Stratum & decals
`StratumSettings` supplies the tile/normal/physics fields (see `MASKING_SPEC`,
`GAMEDATA_LAYOUT_SPEC`); its `maskRemapMin`/`maskRemapMax` fields are per-stratum
material/appearance pass-through data consumed only by the game's own renderer
against the stratum's composite texture — no SanGen stage, preview included,
computes with them (`MASKING_SPEC` §1.6). **Decals are never composited** —
`DecalRule` exists and is imported (`ImportedDecalsJSON`) but the preview has no
decal SSBO or pass. A silent feature gap to close in v2.

## The shadow-sim problem (the central hit-list item)
The preview **re-derives** results instead of sampling the bake:
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

## Picking (keep — it's good)
CPU seeds `EntityIDBuffer` (`0xFFFFFFFF` = empty); the compute passes write a
per-pixel entity ID into SSBO 0 while shading; after all passes it's read back so a
click is `EntityIDBuffer[y*w+x]` — O(1), no 100k test. Formalize the sentinel and
buffer as named constants.

## Dirty flags (two-tier — keep, formalize)
- **`bNeedsMapUpdate`** → full async CPU regen (`TerrainGenerator::GenerateMap`:
  height/erosion/flow/masks/placement); on completion sets `bNeedsPreviewRender` +
  `bGeometryChanged`.
- **`bNeedsPreviewRender`** → GPU composite only.
- **`bGeometryChanged`** → gates the expensive big-map SSBO re-uploads (height/flow/
  accum/masks); rule/area/gradient SSBOs + dispatch run every render.
Assignment is inconsistent per widget today (some visual-only controls trip regen,
and vice-versa). v2 derives which flag each parameter sets from the dependency DAG
(`PARAMS_PIPELINE_SPEC`), not by hand (`UI_FRAMEWORK_SPEC`).

## CPU vs GPU
GPU: clear, all shading/colorization, splat composite, marker/prop raster, entity-ID
write. CPU: full generation, auto-level scan, gradient LUT bake, SSBO packing,
entity-ID readback. **No resolution/quality toggle** exists — preview always runs at
full resolution. v2 should add a preview-resolution/accuracy control (Constitution §4
Visual class for scrubbing, escalate on idle).

## Known issues to fix in v2
- **Shadow reimplementation** (above) — the WYSIWYG-breaking bug; unify on sampling
  the bake.
- **God-object / layer violation**: `UpdatePreviewTexture` (~300 lines) owns GL
  loading, shader compile, all SSBO packing, gradient baking, sim-parameter
  interpretation, dispatch, picking — and **mutates a `const` params**
  (`EntityIDBuffer`). Split per Constitution §1 (UI composites; SYS owns GL; PROC
  owns sim) and route GPU through the shared resource manager
  (`DISPATCH_INTERFACE_SPEC`), not a private UI-layer pipeline.
- **Hardcoded**: absolute shader path (blank preview if missing), stratum cap 9,
  entity sentinel, workgroup 16, slope default colors, 15-slot program array kept in
  sync with `LayerType` by hand.
- **Duplicate/empty types**: two `StratumSettings`, empty `TerrainType_Decal.h`.
- **Decals never previewed**; **accumulation has no own ramp**; **global mutable
  statics** (single-context, resize-keyed reallocation misses format changes).
- **`loc_stratumRemaps` uniform is dropped, not ported** — it was a global,
  stratum-0-only contrast knob (see "Coloring" above), not a real per-stratum
  mechanism; v2's stratum splat is `surfaceStratumWeights × tint` only
  (`MASKING_SPEC` §1.2).

## v2 guidance
Preview = composite + colorize + pick over the **single baked result**; never
re-simulate. Two-tier dirty flags derived from the DAG; preview-resolution/accuracy
toggle; decals composited; per-map independent ramps; all constants tweakable (§8);
GPU via the shared resource manager; one source of truth with the bake
(`DISPATCH_INTERFACE_SPEC`, `MASKING_SPEC`, `PLACEMENT_SCATTER_SPEC`).
