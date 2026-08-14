# MASKING_SPEC — the Mask stage: slope gate, stored-art merge, surface weights

Two parts. **Part 1 is the binding v2 contract** (ARCH §7.2 / §7.4, ratified by the
project owner) — build against this. **Part 2 is the legacy survey** of the pre-v2 code,
kept as ground truth for *why* the rules are what they are.

---

# Part 1 — the pinned v2 contract (BINDING)

## 1.1 Two fields, two meanings
The single "MaterialMasks" field of the legacy code is **split in two** (ARCH §7.2):

| DATA field | Meaning | Single writer | Read by |
| --- | --- | --- | --- |
| `materialProportions[0..8]` | **Physical** — how much of each stratum is in the column, per cell | the sims (NoiseBlend seeds; Erosion/Thermal evolve + renormalize) | the sims; Mask |
| `surfaceStratumWeights[0..8]` | **Visible** — resolved 0..1 weight at the surface, after gate + merge + remap | **Mask stage, exclusively** | Placement, Bake, preview, `.sanmap` export |

The legacy name `materialMasks` is retired: it named a role, not a quantity, and that
ambiguity is what produced the defects in Part 2. "Mask" now refers only to the on-disk
`.sanmap` stratum art and to the `surfaceStratumWeights` that generate it.

## 1.2 What the Mask stage computes
Per cell, per stratum `s`:
```
slopeGradient = |grad(heightfield)| * terrainMaxHeight / cellSize     // once per cell
gate_s        = SlopeGateWeight(slopeGradient, stratum_s)             // 0..1
procedural_s  = materialProportions[s] * gate_s
merged_s      = Merge(procedural_s, storedArt_s, importedMaskMode_s)
surfaceStratumWeights[s] = Remap_s(merged_s)                          // clamp [maskMin, maskMax]
```
**The Mask stage does the multiply and emits the combined field.** It does not emit a
bare gate for someone else to multiply, because `ImportedMaskMode` is not multiplicative:
`StaticOverride` *replaces* and `ProceduralStart` *adds*. A merge that replaces cannot be
deferred as a multiplicative weight (`visibility = art / proportion` is degenerate wherever
the proportion is zero). See ARCH §7.2.4.

## 1.3 The three hard rules
1. **Mask never writes `materialProportions`.** It reads them. Gating is never pre-baked
   into the physical field, or a renormalizing sim undoes the gate.
2. **Mask is pure and idempotent** (ARCH §3.4.2). Inputs: heightfield,
   `materialProportions`, `Params::Stratum[]`. Output: `surfaceStratumWeights`. No
   read-modify-write of any field — the dirty-hash conductor may re-run Mask alone when a
   mask parameter changes, and running it twice must give the same answer.
3. **The remap happens exactly once, here.** Bake has no remap of its own. (See 1.6.)

## 1.4 Stage position
```
NoiseBlend → Erosion → Thermal → FlowAccumulation → Mask → Placement → Bake
```
Mask runs **after every sim** so the gate is evaluated on the *final* slope and the *final*
proportions, and **before Placement/Bake** so both consume resolved weights. Any future sim
(`FUTURE_SIM_TYPES_SPEC`) is inserted in the sim block, before Mask. ARCH §7.4.

## 1.5 Merge modes (unchanged semantics, new target field)
`ImportedMaskMode` now resolves into `surfaceStratumWeights`, not into the physical field:
- `Disabled` — keep the gated procedural value.
- `ProceduralStart` — additive: `clamp(procedural + stored)`.
- `StaticOverride` — replace with the stored art; **not slope-gated** (locked to what the
  artist shipped).

Seeding the *physical* proportions from an imported `.sanmap` is an **IO** concern, not a
Mask concern: the on-disk TGAs are surface weights, and `IO` seeds `materialProportions`
from them as the best available approximation at import time, recording that it did so
(ARCH §7.2.7). IO loads a field; it never simulates.

## 1.6 Consumers
- **Placement** gates on `surfaceStratumWeights[s]` ("scatter where the grass *shows*") —
  a visibility statement, and required for WYSIWYG. This puts Mask in the Output **Exact
  chain** (ARCH §4.6), because Placement is Exact in the Output context.
- **Bake** consumes `surfaceStratumWeights` verbatim into the composite albedo and the two
  packed stratum textures (`stratums_1_4` / `stratums_5_8`). Bake's rival remap
  (`StratumBakeSource::maskRemapMinimum/Maximum`,
  `StratumKernelConfiguration::maskRemapMinimum/maskRemapRangeReciprocal`,
  `RemapMaskWeight`) is **deleted** — with both remaps live it double-remaps any `.sanmap`
  that sets both. *Removing two floats breaks `StratumKernelConfiguration`'s 48-byte /
  16-byte-multiple std430 stride; re-pad it in both the C++ struct and the GLSL block
  (`DISPATCH_INTERFACE_SPEC` §4).*
- **Preview** samples the bake. It never re-runs the mask math (ARCH §3.2).

## 1.7 Settings live in ONE per-stratum type
All mask settings — slope window, feathers, smoothstep/hard-clamp, invert, strength, the
stored-art merge mode, and the single remap — are members of `Params::Stratum`
(`src/params/Stratum_PARAMS.h`). There is **no** `StratumMask_PARAMS` and no
`StratumBakeSource` settings surface in PROC; rival per-stratum arrays are what created the
double remap. Loaded TGA **pixels** are `Data::FloatField` in `src/data/`, never PARAMS.
ARCH §7.1.

## 1.8 Pinned units and resamplers
- **Designer-facing slope settings are in DEGREES** (matching the 0/29/30 visualization
  stops). The computed slope **field is gradient magnitude** (`tan` of the angle). The
  degrees→gradient conversion happens **exactly once**, in the Mask stage's
  configuration flattening, so producer and consumer can never drift. 90° is guarded
  (`maximumSlopeDegreesLimit`, tan of 90° is infinite).
- `terrainMaxHeight` is **read from the map**, never hardcoded to 128.
- **One resampler: bilinear**, everywhere stored art is sampled (import and GUI resize
  alike).

## 1.9 Approximation currently in force
Until the persistent ordered thickness stack lands (ARCH §7.5), Mask consumes
`materialProportions` (volume fraction) as its stand-in for **surface exposure**. These are
not the same quantity — thin topsoil over deep bedrock reads ~0% under volume fraction. The
approximation is documented at the call site. When the thickness stack lands, Mask's input
binding changes to the derived surface-exposure field; **the Mask kernel itself does not
change** (same shape: 9 × `FloatField`, 0..1). Do not attempt that fix inside a mask
work-order.

---

# Part 2 — legacy survey (pre-v2 ground truth)

Source: `core/gen/Gen_Mask_Height.h`, `core/gen/Gen_Mask_Slope.h`,
`core/gen/Gen_Erosion.cpp` (live implementation), `core/params/Params_Geometry.h`
(`StratumSettings`, `SlopeSettings`, rule ranges), `core/params/Params_Enums.h`
(`ImportedMaskMode`), `PreviewRenderer.cpp` (consumption).

## Height mask (top-down occlusion)
Live in `Gen_Erosion.cpp:301-374` (the declared `Gen_Mask_Height::ApplyHeightMask` is dead
— no body, not called). Per layer, per cell: `alpha = thickness * HeightBlendContrast`,
hard-clamped to `[HeightBlendMin, HeightBlendMax]` (swap-guarded, +0.001 epsilon),
`* Opacity`; `contrib = min(alpha, remainingVisibility)` accumulated into the stratum slot;
leftover visibility falls through to the base stratum. AVX2 + OpenMP, branchless
(`_mm256_blendv_ps`). **Hard clamp only — no smoothstep, no feather, no invert.**
*In v2 this top-down occlusion fill is the NoiseBlend stage's seeding of
`materialProportions`, not part of the Mask stage.*

## Slope mask
`Gen_Mask_Slope::GenerateSlopeMap(...)` — declared, body absent. `terrainMaxHeight=128` was
a hardcoded default standing in for the terrain's vertical extent in game units. The unit
contract was **ambiguous**: the marker consumer compared gradient-squared (`tan²`) while the
header said "0-90°". Pinned in 1.8.

## Consumption (legacy)
1. Stratum/splat weights: `MaterialMasks` → SSBO to the preview
   (`PreviewRenderer.cpp:351,418`); `maskRemapMin/maskRemapMax` remapped per stratum.
2. Placement density: markers/props gated on `MinSlope/MaxSlope/MinHeight/MaxHeight`
   windows. See `PLACEMENT_SCATTER_SPEC`.
Masks did **not** gate raw noise/blend weights.

## Stored stratum masks were the same field
The `.sanmap` ships 8 masks in 2 TGAs + base stratum 0; on import they loaded into
`StratumSettings.importedMaskData` + `maskMode` and merged into the same `MaterialMasks[s]`
(`Gen_Erosion.cpp:376-399`). v2 keeps them one unified field — now
`surfaceStratumWeights` (1.5).

## Known issues (all now ruled on)
- **Dead declared bodies** (`ApplyHeightMask`, `GenerateSlopeMap`, `CalculateSlopeMask`) —
  logic lived inline in `Gen_Erosion`. → one named Mask PROC stage.
- **Slope unit ambiguity** (degrees vs gradient²) — silent-wrong-filtering hazard. → 1.8.
- **No smoothstep / feather / invert** despite the intent of smooth stratum blends. →
  present in the v2 gate, all §8-tweakable.
- **Resample inconsistency** (nearest on import, bilinear in the GUI). → 1.8, bilinear only.
- **Layer violation**: the erosion pass reached into `params.Stratums[].importedMaskData/
  maskMode` and wrote material masks mid-erosion; the slope→gradient conversion lived in
  GPU-dispatch glue. → masking is its own PROC stage, and the field split (1.1) makes the
  ownership unambiguous.
- **Duplicate/empty types**: two `StratumSettings`, two `ImportedMaskMode`, empty
  `TerrainType_Splat.h` — delete the dead copies.
- **Double remap** (Mask remap + Bake remap, both live). → 1.6, one remap in Mask.
- **In-place gate on the physical field** (Mask read-modify-wrote the field erosion then
  renormalized). → 1.1 + 1.3, the root defect this spec exists to prevent.

## v2 mask-of-masks (still open, §8 tweakability)
There is no general mask-multiply chain today. v2 should add one (a mask can multiply
another) as a tweakable op. Not required by the current work-orders; when added, it lives
inside the Mask stage and feeds `surfaceStratumWeights` — never the physical field.
