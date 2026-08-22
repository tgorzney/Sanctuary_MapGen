[← ARCH index](ARCH.md) · [§7 ARCH_07_M3Resolutions](ARCH_07_M3Resolutions.md) · SanGen ARCH §7.2. **Only the ARCH Expert writes this file.**

### 7.2 Material proportion vs surface weight — the two fields (RATIFIED)
*Confirmed by the project owner. This section is settled law; the previous
"dev confirmation needed" flag is cleared.*

**Ruling: there are two per-stratum fields, not one, and they mean different things.**

| DATA field | Meaning | Single writer (§3.4.1) | Read by |
| --- | --- | --- | --- |
| `materialProportions[0..8]` | **Physical.** How much of each stratum is present in the column, per cell. Sums to 1 where the column is non-empty. | the sim stages (NoiseBlend seeds it; Erosion/Thermal evolve and renormalize it) | the sims; the Mask stage |
| `surfaceStratumWeights[0..8]` | **Visible.** The resolved 0..1 weight of each stratum at the surface after gating and stored-art merge. This is what the eye and the game shader see. | the **Mask stage**, exclusively | Placement (stratum gate), Bake (composite + stratum TGA export), preview |

Consequences, all binding:

1. **Erosion owns and renormalizes `materialProportions`** when it moves material between
   strata. That was always correct and is unchanged.
2. **The slope/visibility gate is never pre-baked into `materialProportions`.** Doing so
   lets a renormalizing sim undo the gate, and makes the Mask stage a non-idempotent
   read-modify-write (§3.4.2).
3. **The Mask stage never writes `materialProportions`.** It reads them (plus the
   heightfield, for slope), and writes `surfaceStratumWeights`. It is therefore a pure,
   re-runnable function — a mask-parameter change may re-run Mask alone.
4. **The Mask stage performs the combine itself**, and emits one resolved field directly —
   there is no separate per-stratum remap step after the merge:
   ```
   gate_s        = SlopeGateWeight(slopeGradient, stratum_s)          // 0..1
   procedural_s  = materialProportions[s] * gate_s
   surfaceStratumWeights[s] = Merge(procedural_s, storedArt_s, importedMaskMode_s)
                                                          // final value. The merge's own
                                                          // output-clamp window is the only
                                                          // rescale applied — see item 5.
   ```
   **This overrides the earlier "Mask emits a bare `visibilityWeight`, Bake multiplies"
   mechanism**, for a hard technical reason: `ImportedMaskMode` is **not** a multiplicative
   gate. `StaticOverride` *replaces* the value with the artist's art and `ProceduralStart`
   *adds* to it; neither is expressible as a single multiplicative weight applied later
   (the override would require `visibility = art / proportion`, which is degenerate wherever
   the proportion is zero). The merge must therefore happen where both operands are in
   hand — inside Mask. The architectural property the original ruling was protecting
   (proportion never destroyed by the gate) is preserved in full by the *separate output
   field*, which is the part that actually mattered.
5. **CORRECTED — there is no per-stratum surface-weight remap in the Mask stage, or
   anywhere in SanGen generation.** This item originally read "the remap happens exactly
   once, in the Mask stage." That claim is **wrong** and is withdrawn — ruled by the
   Generator Expert after independently verifying the evidence directly against the real
   code (not a summary). Nothing else in this section changes: the field split, item 4's
   merge, the stage-order rulings (§7.4), and Bake consuming `surfaceStratumWeights`
   verbatim all stand exactly as ratified.

   `merged_s` from item 4's formula **is** `surfaceStratumWeights[s]`, unmodified. The
   merge's own output-clamp window (`MergeStoredMask`'s `[maskMinimum, maskMaximum]`,
   defaulting `[0,1]`) is a generic safety clamp on the merge result — it is a *different*
   mechanism from, and must not be confused with, the field discussed below.

   `Params::Stratum::maskRemapMinimum`/`maskRemapMaximum` is **per-stratum material/
   appearance pass-through data**, not a Mask-stage input or output. Confirmed by:
   - **Struct placement in the real C# format** (`SanMap.Types.cs`, ground truth): the
     field sits beside `diffuseRemap`/`tileSize`/`normalScale` — the shader-appearance
     fields — never near anything visibility-related.
   - **v1 never computed with it.** Its one touch point,
     `PreviewRenderer.cpp:426-427,505`, read channel `[0]` of stratum 0 only and applied
     it as a single global preview-shader contrast uniform — an ad-hoc debug knob, never
     wired to the real export/bake path, and code v2 explicitly supersedes.
   - **The real per-stratum visibility mechanism is the wholly separate `stratums_1_4.tga`/
     `stratums_5_8.tga` splat-weight export**, which `surfaceStratumWeights` feeds
     directly and verbatim.
   - `StratumAppearance_PARAMS.h`'s own existing scope note already buckets
     `maskRemapMinimum`/`maskRemapMaximum` as appearance data "no generation stage reads
     yet."

   The field is consumed only by the **game's own renderer**, against the stratum's own
   composite/"mask" texture (`StratumAppearance::compositeTexturePath`) — a real texture
   asset, distinct from the `stratums_1_4/5_8.tga` files the Mask stage produces. **No
   SanGen generation stage reads or writes it today.** Bake still consumes
   `surfaceStratumWeights` verbatim — that conclusion is unchanged — but the *reason* Bake
   has no remap of its own changes: it is not "the one remap lives upstream in Mask
   instead," it is "there is no per-stratum surface-weight remap anywhere in SanGen
   generation." Bake's former kernel fields for this
   (`StratumBakeSource::maskRemapMinimum/Maximum`,
   `StratumKernelConfiguration::maskRemapMinimum/maskRemapRangeReciprocal`,
   `RemapMaskWeight`) stay deleted; `StratumKernelConfiguration` keeps the two now-unused
   scalar slots as explicit padding so its std430 stride holds at a 16-byte multiple
   (`DISPATCH_INTERFACE_SPEC` §4) — this was, and remains, the correct code shape, only the
   stated reason for it has changed.

   *Field placement (the Generator Expert leaves this open; not load-bearing either way):*
   `maskRemapMinimum`/`maskRemapMaximum` **stay direct members of `Params::Stratum`**
   rather than moving into `StratumAppearance` — there is no behavioral reason to churn the
   file, and `StratumAppearance_PARAMS.h`'s existing "NOT DUPLICATED HERE" note already
   documents the split for a reader who lands there first.

   A future work-order may eventually wire this field to a real SanGen consumer — most
   plausibly composite/mask-texture processing inside Bake (`MASKING_SPEC` §1.6). That
   consumer does not exist yet and is not designed here; until then the field stays pure
   round-trip data.
6. **Placement's stratum gate reads `surfaceStratumWeights`,** not `materialProportions`.
   "Scatter trees where grass shows" is a *visibility* statement, and WYSIWYG (hit-list #4)
   requires props to follow what the preview shows. This places Mask upstream of Placement
   (§7.4) and puts Mask in the Output Exact chain (§4.6).
7. **Seeding proportions from an imported `.sanmap` is an IO concern, not a Mask concern.**
   The stratum TGAs on disk are surface weights; when a map is imported, `IO` seeds
   `materialProportions` from them as the best available approximation and records that it
   did so. `ImportedMaskMode` in the Mask stage governs only the `surfaceStratumWeights`
   output. IO loads a field; it does not simulate (§3.2).
8. **Rename, do not overload.** The field previously called `materialMasks` is renamed
   `materialProportions` throughout (§1.1: a name states the quantity, not the role).
   The word "mask" is reserved for the on-disk `.sanmap` stratum art and for the
   `surfaceStratumWeights` that produce it. `SANMAP_FORMAT_SPEC` is unaffected — the
   on-disk masks are, and always were, surface weights.
9. **Clarifications (M3 mask-rework coder judgment calls, ratified).** Two decisions the
   rework made within the spirit of §7.2 but not previously named:
   - **`strata` (the per-stratum `Params::Stratum[]`) lives on `MapRecipe`**, not as an
     assembler side-vector. Stratum settings are PARAMS (§7.1) and must round-trip in the
     `.sanmap`; `MapRecipe` is the PARAMS aggregate, so they belong on it.
   - **Placement's `DominantStratumIndex` (biome tag) is computed from
     `surfaceStratumWeights`, not `materialProportions`.** Placement is the
     visibility-consistent consumer (§7.2.6, "scatter where it shows"); its biome tag must
     match the rendered surface for WYSIWYG, and Placement reads exactly one stratum field.
     (If a future gameplay consumer needs the *physical* dominant material, it reads
     proportions directly — a separate consumer, not a change here.)
10. **Amendment — `maskRemapMinimum`/`maskRemapMaximum` are genuine 4-component fields,
    not a scalar** (ratified in a dedicated `Params::Stratum`-IO session; confirmed against
    the C# ground truth `SanMap.Types.cs` — `Stratum.maskRemapMin`/`maskRemapMax` are both
    real `Vector4`, defaulting to `(0,0,0,0)`/`(1,1,1,1)` — and against a real shipped map).
    `Params::Stratum::maskRemapMinimum`/`maskRemapMaximum` (`src/params/Stratum_PARAMS.h`)
    currently collapse this to a single `float` each, losing real format data for a
    pass-through field — the same fidelity principle §1.8 already states for hand-authored
    entity data, applied here to a format-native per-stratum field.
    - **Superseded framing note (added by the item 5 correction above).** This item's own
      framing below — "where the remap runs," "one remap site" — predates and is superseded
      by item 5's correction: there is no remap site at all, in Mask or anywhere else in
      SanGen generation. The part of this amendment that remains binding, unaffected by that
      correction, is purely the **field's shape**: `maskRemapMinimum`/`maskRemapMaximum` is a
      4-component `Vector4` pass-through field, not a scalar, exactly as the format types it
      — independent of whether, or where, any stage ever consumes it.
    - **This does NOT reopen §7.1's "no rival per-stratum settings type."** That ruling is
      about not inventing a **second per-stratum settings type** that duplicates
      `Params::Stratum` (the double-remap code defect was two rival *arrays*, not one field
      with the wrong width). Widening one field's own shape on the *same* single
      `Params::Stratum` struct is not that — it corrects the field to match what the format
      actually carries, regardless of consumption.
    - **Shape:** `float maskRemapMinimum[kStratumColorChannelCount]` /
      `float maskRemapMaximum[kStratumColorChannelCount]` — the same 4-wide convention
      `StratumAppearance::diffuseRemapColor`/`farColorRemapColor` already use
      (`kStratumColorChannelCount`, defined in `StratumAppearance_PARAMS.h`, which
      `Stratum_PARAMS.h` already includes — reused rather than a second magic `4` or a new
      constant). Defaults become `{0,0,0,0}` / `{1,1,1,1}` — the same numeric values the
      scalar fields already carried, now per-channel, matching the C# defaults exactly.
    - **Presentation is separate from shape.** The Stratum tab may expose this as up to 4
      per-channel inputs (`StratumsTab_Appearance_UI.cpp`'s `DrawMaskRemapWindow`) — a UI
      decision, not a PARAMS one; not designed here.
    - **Shape only, not wiring.** Widening the field, updating its `IO` read/write (see
      `SANMAP_FORMAT_SPEC` Correction 13), and updating its own in-code comment are separate
      coder work. **CLOSED by the item 5 correction above:** the earlier open question of
      "how does the Mask kernel's single-scalar-per-cell surface weight consume a
      4-component remap window" no longer applies — the Mask kernel does not consume
      `maskRemapMinimum`/`maskRemapMaximum` at all, and no coder needs to stop and report on
      it for that reason. (A coder still stops and reports before inventing any *new*
      consumer for this field — §8.4 — but the specific open question this amendment
      originally flagged is resolved, not merely deferred.)

