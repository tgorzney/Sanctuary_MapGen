# NOISE_BLEND_SPEC — noise generation & the heightfield blend stack

Source: `core/params/Params_Geometry.h` (`NoiseLayer`), `core/params/Params_Enums.h`
(`NoiseType`/`FractalType`/`BlendMode`), `core/gen/Gen_Noise.h`,
`core/gen/Gen_NoiseAndBlend.h`, `core/TerrainCompute.cpp` (GPU),
`TerrainGenerator.*` (cache). This is the **first stage** of the pipeline —
everything downstream (erosion, flow, placement, preview) reads what this produces.
See `LAYER_SYSTEM_SPEC` for the layer/GeoLayer model and `PARAMS_PIPELINE_SPEC`
for the dirty-hash order.

## Noise (the per-layer source)
Every heightfield layer is a `NoiseLayer` (`Params_Geometry.h:31`). Noise is backed
by **FastNoiseLite**. The controlling fields, VERBATIM:
- `NoiseType Type` — `OpenSimplex2, OpenSimplex2S, Cellular, Perlin, ValueCubic,
  Value, None` (`Params_Enums.h:52`).
- `FractalType Fractal` — `None, FBm, Ridged, PingPong` (`Params_Enums.h:62`).
- `float Frequency (0.005)`, `int Octaves (5)`, `float Gain (0.5, =persistence)`,
  `float PingPongStrength (2.0, PingPong only)`, `float CellularJitter (1.0,
  Cellular only)`.
- **Post-noise reshaping** (Photoshop "Levels"): `LevelsShadows/LevelsMidtones/
  LevelsHighlights/LevelsOutputBlack/LevelsOutputWhite`.
- **Density shaping**: `LandDensity/PlateauDensity/MountainDensity/RampDensity`.
- **Image mode**: `UseImage/ImagePath/ImageData` + bake state
  `IsBaked/BakeRequested/BakedImageData` (a layer can be a baked image instead of
  live noise — see `LAYER_SYSTEM_SPEC` baking).

**Seed**: one global `params.Seed`; each layer's effective seed is derived by stack
index — `layer.GetNoiseHash(params.Seed + i, …)`. `GetNoiseHash` deliberately
**excludes** Levels, Density and Opacity so cheap reshaping does not invalidate the
cached structural noise (see Cache below).

## Blend (combining layers into one heightfield)
`BlendMode Blend` per `NoiseLayer` — `Add, Subtract, Multiply, Overlay, Max, Min`
(`Params_Enums.h:43`). Per-layer combine controls: `Opacity (1.0)`, and the
**height-blend** group `HeightBlendContrast (1.0)/HeightBlendMin (0.0)/HeightBlendMax
(1.0)` — "how deep this layer sits on top of the one below" (this is also the
top-down occlusion the height mask uses — see `MASKING_SPEC`). `StratumIndex`
routes the layer into one of 9 stratums / its GeoLayer group.

Layers are grouped by GeoLayer but blended over a **flat cross-GeoLayer stack** in
calculation order (`GetFlatLayers()`); the Separate/Unified sim toggle
(`LAYER_SYSTEM_SPEC`) decides whether GeoLayers are simulated apart or as one.

> **Do not confuse** `BlendMode` (heightfield geometry, this spec) with
> `GenerationParams::LayerBlendMode` (`None/Normal/Add/Subtract/Multiply/Divide/
> Overlay/Screen/SoftLight/HardLight`) — that second enum is **preview-composite
> Z-order only** (`PREVIEW_COMPOSITING_SPEC`), not terrain. v2 must name them so
> they can never be cross-wired.

## Cache (dirty-hash)
`GenerationResult` holds `CachedRawNoise[]` + `CachedNoiseHashes[]` (per-layer,
keyed by `GetNoiseHash`) and `CachedBlendedMap`/`CachedBlendedStratums` +
`CachedBlendHash`. The GPU sets `needsNoiseGen` per layer only when its hash
changed, and skips the whole noise pass if nothing changed. Structural-noise vs
reshape separation is the whole point — **preserve it in v2**.

## CPU vs GPU (current divergence — a hit-list item)
Both paths exist, gated by `bool UseGPUTerrain` (currently orphaned — see below).
The GPU config `LayerConfigGLSL` carries only freq/octaves/gain/densities/levels/
opacity/stratumIdx — it **omits** `Blend`, `HeightBlend*`, `Type`, `Fractal`,
`PingPongStrength`, `CellularJitter`, symmetry. **So the GPU cannot reproduce blend
mode, fractal/noise type, ping-pong/cellular, or height-blend** → GPU ≠ CPU for any
non-default layer. Per Constitution §4 (CPU = accuracy, GPU = speed) the two are
allowed to differ in *cost*, never in *result class* — the GPU noise/blend kernel
must carry the full `NoiseLayer` config so a layer bakes the same shape either way.

## Known issues to fix in v2 (from the survey)
- **CPU impl is missing**: `Gen_NoiseAndBlend::Process` and `Gen_Noise::
  ProcessLayerChunk` are declared + called but have **no definition** in the tree —
  the switch-over-`BlendMode` math does not exist here; v2 authors it once.
- **Hardcoded absolute shader paths** (`D:/Projects/.../TerrainCompute.glsl`) and
  **missing shader files** — GPU terrain path is non-functional as delivered.
- **`NoiseLayer` is a god object** (~90 fields): noise + image baking + per-layer
  erosion + prop/decal placement + physics tags all in one struct. v2 splits by
  layer (Constitution §1): noise/blend fields stay; placement/physics move out.
- **Dead/conflicting types**: `data/LayerType_*.h` redefine `LayerType` and
  duplicate marker structs, included by nothing — delete.
- **Morton code triplicated** (`Gen_Noise.h`, `Sanmath_Morton.h`, inline in
  `TerrainGenerator.cpp`) — one copy (Constitution §2 naming law).
- **Missing capability** vs stated intent (total tweakability, §8): no lacunarity,
  no domain-warp, no explicit per-layer blend mask. v2 should add these as tweakable
  fields, not bury them.

## v2 guidance
1. One noise/blend kernel, CPU + GPU sharing the full `NoiseLayer` config
   (`DISPATCH_INTERFACE_SPEC`); GPU carries every field so shape is identical.
2. Keep the two-level hash (raw-noise vs blend) and the reshape-excluded-from-hash
   rule.
3. Every noise/blend constant exposed and tweakable (§8) — no baked densities/tile
   sizes; add lacunarity + domain-warp as first-class.
4. Rename the geometry vs preview blend enums so they are unmistakable.
