[← ARCH index](ARCH.md) · SanGen ARCH §5. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 5. God-object dismemberment (hit-list #1–2)

Applying §3 boundaries and the input/output split (settings → `PARAMS`, computed arrays
→ `DATA`). Four offenders and where their pieces land.

### 5.1 `GenerationParams` → typed modules
- **settings → `PARAMS`:** `Layers_PARAMS` (the editable layer stack), `Stratum_PARAMS`,
  `MarkerRules_PARAMS`, `PropRules_PARAMS`, `Water_PARAMS`, `Atmosphere_PARAMS`,
  `Geometry_PARAMS` (dimensions + seed), `ErosionFlow_PARAMS`, `Symmetry_PARAMS`,
  `Environment_PARAMS`, `Enums_PARAMS`.
- **computed arrays → `DATA`:** `Heightfield_DATA`, `BlendedMap_DATA`,
  `MapFields_DATA` (heightfield, flow, accumulation, `materialProportions`,
  `surfaceStratumWeights` — §7.2), `Markers_DATA`, `Props_DATA`, `Units_DATA`,
  `Areas_DATA`, `EntityIdBuffer_DATA`, `SpatialGrid_DATA`, cached noise.
- **dispatch toggles → `DispatchPolicy`** (PIPELINE sets, SYS reads).
- **dirty flags + hashes → `DirtyHash_PIPELINE`.**
- **GPU buffers / GL handles → `GpuResource_SYS`.**

### 5.2 `NoiseLayer` (~90 fields) → `Layers_PARAMS`
Keeps only identity + noise + blend + stratum-index. Evicted: image-bake state → a bake
concern in `PROC`; per-layer erosion → `ErosionFlow_PARAMS`; placement fields
(`AvoidWater`, `NearCliffs`, blueprint) → `PropRules_PARAMS`; physics tags → material/
stratum physics. Its computed noise output → the `DATA` cache.

### 5.3 `Widget_MapCanvas` (~720 lines) → `MapCanvas_UI`
Keeps draw + input only. Evicted: triangle height-interpolation → `Interpolation_MATH`;
unit-grid / symmetry **spawning + army creation** → `Placement_PROC` (UI only requests
it via `PIPELINE`); picking → reads `EntityIdBuffer_DATA` produced by `SYS`.

### 5.4 `PreviewRenderer` (~300 lines) → `PreviewComposite_UI`
Keeps pass ordering only. Evicted: GL load / shader compile / SSBO packing →
`GpuResource_SYS`; gradient LUT bake → **`GradientLut_UI`** (a UI-layer colorization
helper — **not** a PROC stage; corrected in §8.1); **sim/rule re-filtering → deleted**
(samples the bake, §3.2); picking readback → `Picking_UI`.

### 5.5 Retire outright
`TerrainGenerator` god + the `main.cpp` regen loop → `Generation_PIPELINE`. The dead
`core/data/*` + `GenParams_*` duplicate family → deleted (hit-list #1). Every hardcoded
GPU constant (erosion `0.3`, thermal `/2.0`) → a `PARAMS` field (Constitution §8).
