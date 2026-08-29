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
| `surfaceStratumWeights[0..8]` | **Visible** — resolved 0..1 weight at the surface, after gate + merge | **Mask stage, exclusively** | Placement, Bake, preview, `.sanmap` export |

The legacy name `materialMasks` is retired: it named a role, not a quantity, and that
ambiguity is what produced the defects in Part 2. "Mask" now refers only to the on-disk
`.sanmap` stratum art and to the `surfaceStratumWeights` that generate it.

## 1.2 What the Mask stage computes
Per cell, per stratum `s`:
```
slopeGradient = |grad(heightfield)| * terrainMaxHeight / cellSize     // once per cell
gate_s        = SlopeGateWeight(slopeGradient, stratum_s)             // 0..1
procedural_s  = materialProportions[s] * gate_s
surfaceStratumWeights[s] = Merge(procedural_s, storedArt_s, importedMaskMode_s)
                                                    // final value. The merge's own
                                                    // output-clamp window (maskMinimum/
                                                    // maskMaximum, default [0,1]) is the
                                                    // only rescale applied — see 1.6.
```
**The Mask stage does the multiply and emits the combined field directly — there is no
separate per-stratum remap step after the merge.** It does not emit a bare gate for
someone else to multiply, because `ImportedMaskMode` is not multiplicative:
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
3. **There is no per-stratum surface-weight remap — not here, not anywhere in SanGen
   generation.** (ARCH §7.2 item 5, corrected.) See 1.6 for what
   `maskRemapMinimum`/`maskRemapMaximum` actually is.

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
  packed stratum textures (`stratums_1_4` / `stratums_5_8`). Bake has no remap of its
  own — not because a remap "moved" here from Bake into Mask, but because **no
  per-stratum surface-weight remap exists anywhere in SanGen generation** (ARCH §7.2 item
  5, corrected). Bake's former kernel fields for this
  (`StratumBakeSource::maskRemapMinimum/Maximum`,
  `StratumKernelConfiguration::maskRemapMinimum/maskRemapRangeReciprocal`,
  `RemapMaskWeight`) stay **deleted**; `StratumKernelConfiguration` keeps the two
  now-unused scalar slots as explicit padding so its std430 stride holds at a 16-byte
  multiple (`DISPATCH_INTERFACE_SPEC` §4).
- **Preview** samples the bake. It never re-runs the mask math (ARCH §3.2).
- **`Params::Stratum::maskRemapMinimum`/`maskRemapMaximum` is NOT a Mask-stage input or
  output, and not consumed by any SanGen generation stage today.** It is per-stratum
  material/appearance **pass-through data** — round-trip only
  (`SANMAP_FORMAT_SPEC` Correction 13). Its real consumer is the **game's own renderer**,
  applied against the stratum's own composite/"mask" texture
  (`StratumAppearance::compositeTexturePath`) — a genuine texture asset, wholly distinct
  from the `stratums_1_4.tga`/`stratums_5_8.tga` splat-weight files this Mask stage
  produces. A future work-order may eventually wire this field to a real SanGen consumer
  — most plausibly composite/mask-texture processing inside Bake — but that consumer does
  not exist yet and is not designed here; until then the field stays pure round-trip
  data.

## 1.7 Settings live in ONE per-stratum type — plus a shared-default layer (SPEC-4 Correction 5)
All mask settings — slope window, feathers, smoothstep/hard-clamp, invert, strength, and
the stored-art merge mode — are members of `Params::Stratum` (`src/params/Stratum_PARAMS.h`),
evaluated once per stratum by design — that mechanism is the entire reason different strata
can occupy different slope bands (grass on shallow ground, rock on cliffs). There is **no**
`StratumMask_PARAMS` and no `StratumBakeSource` settings surface in PROC; rival per-stratum
arrays are what created the historical double-remap **code** defect — two rival structs
both carrying remap-shaped fields for the same stratum, now resolved: neither computes a
remap at all (ARCH §7.2 item 5). Loaded TGA **pixels** are `Data::FloatField` in
`src/data/`, never PARAMS. ARCH §7.1.

*`Params::Stratum::maskRemapMinimum`/`maskRemapMaximum` also lives on this same type, as a
4-component field (`kStratumColorChannelCount` wide), not a scalar — ARCH §7.2 item 10. It
is **not** a mask setting: it is per-stratum appearance pass-through data the Mask stage
never reads (1.6 above). It is documented here only because it shares the `Params::Stratum`
type this section is about, per ARCH §7.1's "composition is allowed" — not because it is
part of the mask gate/merge mechanism described above.*

**Default/override split (`SANMAP_FORMAT_SPEC` `SlopeDefaults`, ratified by work-order
`SPEC-4` Correction 5).** Per-stratum slope gates remain **ground truth** — the mechanism
above is unchanged. A flat global window is not a simplification of it; deleting per-stratum
control would delete the feature, since every stratum's gate would then open and close
together. Instead:

- A new top-level `.sanmap` section, **`SlopeDefaults`**, carries the same seven fields
  (`bSlopeGateEnabled`, `minimumSlopeDegrees`, `maximumSlopeDegrees`,
  `slopeFeatherDegreesLow/High`, `bUseSmoothstep`, `bInvertSlopeGate`, `slopeGateStrength`)
  as **shared defaults only** — it does not gate anything itself and is not a per-stratum
  type.
- `Params::Stratum` gains one new field, **`bSlopeUseGlobal`** (default `true`):
  - `bSlopeUseGlobal == true` (the default for a newly created stratum) → the stratum's
    slope-gate fields are populated from `SlopeDefaults` at config-flattening time. New
    strata inherit shared defaults for free.
  - `bSlopeUseGlobal == false` → the stratum's own slope-gate fields (already on
    `Params::Stratum`) override the defaults. A stratum that needs its own window flips
    one bool.
- This is resolved entirely in the Mask stage's **existing** config-flattening step (§1.8)
  — default-vs-override is a **config source** decision, not a PROC change. The per-stratum
  kernel (§1.2) is unaffected: it still reads one resolved `Params::Stratum` per stratum and
  has no knowledge of whether a value came from a default or an override.
- No new rival settings type is created (ARCH §7.1 still holds): `SlopeDefaults` is a single
  global record read by the flattening step alongside each stratum's own fields, not a
  per-stratum type a stage reaches independently.
- **Where these fields (plus the per-stratum soil physics) round-trip on disk:**
  `SANMAP_FORMAT_SPEC` Correction 12, `StratumGenerationSettings` — a new top-level array,
  index-aligned with `stratumLayers[9]`, carrying `bSlopeUseGlobal`, the 7 slope-gate
  fields above, and `Params::Stratum::soilPhysics`'s 6 fields. This section (§1.7) states
  only the PARAMS-side mechanism; Correction 12 states the IO shape.

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
   (`PreviewRenderer.cpp:349-355`).
2. Placement density: markers/props gated on `MinSlope/MaxSlope/MinHeight/MaxHeight`
   windows. See `PLACEMENT_SCATTER_SPEC`.

Masks did **not** gate raw noise/blend weights.

`maskRemapMin[0]`/`maskRemapMax[0]` (`PreviewRenderer.cpp:426-427,505`) is **not** evidence
of a real per-stratum remap mechanism, and the original Part 1 ruling that cited it as such
was wrong (ARCH §7.2 item 5, corrected). What that code actually does: the loop builds one
`[min,max]` pair per stratum (index `0..8`), but only `stratRemaps[0]`/`stratRemaps[1]` —
stratum **0**'s pair — is ever read, uploaded as a single global `loc_stratumRemaps`
uniform applied identically to the *whole* preview splat blend. It is an ad-hoc, global,
single-channel preview contrast knob, never per-stratum, never connected to the real
export/bake path, and v2 supersedes this preview code outright (WYSIWYG, hit-list #4).

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
- **Double remap** (an early v2 draft carried a per-stratum remap step in both the Mask
  kernel and Bake's kernel config, live simultaneously — a real code-duplication defect).
  → **corrected further, 1.6/1.3**: neither remap was ever a real generation-stage
  computation to begin with. `maskRemapMinimum`/`maskRemapMaximum` is appearance
  pass-through data no generation stage reads; the original "one remap, in Mask" framing
  this bullet used to point to has been withdrawn (ARCH §7.2 item 5).
- **In-place gate on the physical field** (Mask read-modify-wrote the field erosion then
  renormalized). → 1.1 + 1.3, the root defect this spec exists to prevent.

## v2 mask-of-masks (still open, §8 tweakability)
There is no general mask-multiply chain today. v2 should add one (a mask can multiply
another) as a tweakable op. Not required by the current work-orders; when added, it lives
inside the Mask stage and feeds `surfaceStratumWeights` — never the physical field.

## Future candidate — SanGen-native mask-to-rectangle placement workflow (not designed here)
A related but distinct mask concept exists outside this stage entirely:
`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7 records the **current, ad hoc, non-SanGen** manual
process (a hand-run Python pipeline) used to turn a hand-authored `Textures/`-folder mask
into a list of axis-aligned `NavmapModifierTemplate` rectangles for pathing-blocker
placement — a different consumer than this stage's `surfaceStratumWeights` (which feed
Placement/Bake, not the engine's navigation layers). That spec's §7.1 flags this as a
strong candidate for a future SanGen-native masking/placement feature — author a blocker
mask the same way a stratum mask is authored, decompose it to rectangles automatically —
but it is **not designed here**: no PARAMS shape, no PROC stage, and no interaction with
this stage's `surfaceStratumWeights`/`materialProportions` fields is ruled on by either
spec. Cross-referenced here only so a reader of this stage's own masking concept finds the
pointer to the unrelated one.
