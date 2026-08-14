# MASKING_SPEC — height/slope masks & the unified stratum weight field

Source: `core/gen/Gen_Mask_Height.h`, `core/gen/Gen_Mask_Slope.h`,
`core/gen/Gen_Erosion.cpp` (live implementation), `core/params/Params_Geometry.h`
(`StratumSettings`, `SlopeSettings`, rule ranges), `core/params/Params_Enums.h`
(`ImportedMaskMode`), `PreviewRenderer.cpp` (consumption). Masks are the **weight
fields** that decide where a stratum shows and where scatter is allowed. They feed
`PLACEMENT_SCATTER_SPEC`, `PREVIEW_COMPOSITING_SPEC`, and the stratum export
(`SANMAP_FORMAT_SPEC`).

## The one weight field: `MaterialMasks[0..8]`
There is a single per-stratum 0..1 visibility/weight field, `MaterialMasks` (9
slots). Both procedural masking and the imported stored masks write into the **same**
field. This is the thing exported as the stratum TGAs and sampled by the preview.

## Height mask (top-down occlusion)
Live in `Gen_Erosion.cpp:301-374` (the declared `Gen_Mask_Height::ApplyHeightMask`
is dead — no body, not called). Per layer, per cell:
`alpha = thickness * HeightBlendContrast`, hard-clamped to
`[HeightBlendMin, HeightBlendMax]` (swap-guarded, +0.001 epsilon), `* Opacity`;
`contrib = min(alpha, remainingVisibility)` accumulated into
`MaterialMasks[StratumIndex]`; leftover visibility falls through to the base
stratum. AVX2 + OpenMP, branchless (`_mm256_blendv_ps`). Fields:
`HeightBlendContrast/HeightBlendMin/HeightBlendMax/Opacity/StratumIndex` on
`NoiseLayer`. **Hard clamp only — no smoothstep, no feather, no invert.**

## Slope mask
`Gen_Mask_Slope::GenerateSlopeMap(heightMap, outSlopeMap, bUseEngineParityMath,
result, terrainMaxHeight=128, cellSize=1)` — declared, body absent. (Note:
`terrainMaxHeight=128` is the terrain's **vertical extent in game units** — the
correct slope scale — but it must be **read from the map, not hardcoded**. Entity
positions are absolute world/game units, not fractions; see `SANMAP_FORMAT_SPEC`
entity-position encoding.) Slope is a
finite-difference gradient of the heightmap scaled by `terrainMaxHeight/cellSize`.
`SlopeSettings{ bUseEngineParityMath }`; the visualization gradient stops are in
**degrees** (0/29/30). **Unit contract is ambiguous** (see Issues): the marker
consumer compares **gradient-squared** (`tan²` of `MinSlope/MaxSlope`), implying the
map stores gradient magnitude, while the header says "0-90°". v2 must pin one unit.

## Consumption — masks gate two things
1. **Stratum/splat weights**: `MaterialMasks` → SSBO to the preview
   (`PreviewRenderer.cpp:351,418`); `maskRemapMin/maskRemapMax` remap per stratum.
2. **Placement density**: markers/props gate on `MinSlope/MaxSlope/MinHeight/
   MaxHeight` range windows (`MarkerRule`/`PropRule`/`DecalRule`, and
   `NoiseLayer.MinSlope..MaxHeight`). See `PLACEMENT_SCATTER_SPEC`.
Masks do **not** currently gate raw noise/blend weights.

## Stored stratum masks are the SAME system
The `.sanmap` ships 8 masks in 2 TGAs (`stratums_1_4.tga` / `stratums_5_8.tga`) +
base stratum 0. On import they load into `StratumSettings.importedMaskData` +
`maskMode` and merge into the **same `MaterialMasks[sIdx]`** (`Gen_Erosion.cpp:
376-399`). `ImportedMaskMode` (`Params_Enums.h`):
- `Disabled` — ignore the stored mask.
- `ProceduralStart` — **additive**: `clamp(procedural + imported, 0, 1)` (stored
  mask seeds/adds onto procedural output).
- `StaticOverride` — **replace**: `MaterialMasks[sIdx] = imported` (locked to the
  stored art).
So procedural and stored masks are one unified weight field with a per-stratum merge
mode — not two pipelines.

## Combining
Multiple layers accumulate additively (top-down occlusion) into a stratum slot;
stored masks merge per the mode above. Marker slope+height are **ANDed range
gates**. There is no general mask-of-masks multiply chain today — v2 should add one
(a mask can multiply another) for §8 tweakability.

## CPU vs GPU
Height mask / `MaterialMasks`: CPU AVX2+OpenMP, branchless. Imported-mask merge:
CPU, scalar, **nearest-neighbor** resample. Slope/marker gating: intended GPU.
Slope-map generation: intended GPU ("1:1 to a compute shader") but body absent.

## Known issues to fix in v2
- **Dead declared bodies** (`ApplyHeightMask`, `GenerateSlopeMap`,
  `CalculateSlopeMask`) — logic lives inline in `Gen_Erosion`; consolidate into one
  named mask module (Constitution §1 MATH/PROC split).
- **Slope unit ambiguity** (degrees vs gradient²) across producer/consumer/viz — a
  silent-wrong-filtering hazard. Pin the unit and document it.
- **No smoothstep / feather / invert** for either mask despite the intent of smooth
  stratum blends — add them (`LayerBakeCompute` even advertises "step").
- **Resample inconsistency**: import→generation uses nearest-neighbor (aliasing)
  while the GUI resize of the same data uses bilinear — unify on one resampler.
- **Layer violation**: the erosion pass reaches into `params.Stratums[].
  importedMaskData/maskMode` and writes material masks mid-erosion; the slope→gradient
  unit conversion lives in GPU-dispatch glue. Masking is its own PROC stage
  (Constitution §1), not a side effect of erosion or dispatch.
- **Duplicate/empty types**: two `StratumSettings`, two `ImportedMaskMode`, empty
  `TerrainType_Splat.h` — delete the dead copies (§2).

## v2 guidance
One masking module owning height + slope + stored-mask merge, producing the single
`MaterialMasks` field; smoothstep/feather/invert + mask-multiply as tweakable ops
(§8); one pinned slope unit; one resampler; CPU/GPU share the mask math
(`DISPATCH_INTERFACE_SPEC`) so preview masks == baked masks
(`PREVIEW_COMPOSITING_SPEC`).
