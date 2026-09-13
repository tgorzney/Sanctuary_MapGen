# DESIGN_ProceduralStratumMaskFilters_R1.md

Scopes the human's request for automated, procedural stratum-layer masking: each stratum gets an
ordered chain of filters (slope, height, roughness, more later), each with its own settings, blend
mode (reusing the preview system's blend vocabulary), and alpha — composed left-to-right into that
stratum's own mask, never affecting any other stratum. This is exactly `MASKING_SPEC.md`'s own
flagged-but-unscoped "v2 mask-of-masks" open item, now being designed for real. Generator Expert
consult, DESIGN phase only — **not ratified.** No code has been written for this feature. An ARCH
Expert pass is required before any ticket in this doc's §4 is coder-dispatchable. Tracked from
`work_orders/HANDOFF_TRACK_ProceduralStratumMasking.md`.

## 0. Ground truth today (verified, not proposed)

- `MASKING_SPEC.md` §1.2/1.3 (binding, ARCH §7.2/§7.4): the Mask stage computes, per cell per
  stratum `s`:
  ```
  gate_s        = SlopeGateWeight(slopeGradient, stratum_s)        // the ONLY filter that exists today
  procedural_s  = materialProportions[s] * gate_s
  surfaceStratumWeights[s] = Merge(procedural_s, storedArt_s, importedMaskMode_s)
  ```
  Mask is pure and idempotent: reads `heightfield` + `materialProportions` + `Params::Stratum[]`,
  writes only `surfaceStratumWeights`, **never** writes `materialProportions`. No read-modify-write.
- ARCH §7.1 (`ARCH_07_01_ParamsPerStratum.md`): exactly ONE per-stratum settings type,
  `Params::Stratum` (`src/params/Stratum_PARAMS.h`) — composition of sub-structs is allowed, a rival
  top-level per-stratum array is not.
- Today's only filter (slope gate) is 9 flat scalar fields directly on `Params::Stratum`
  (`Stratum_PARAMS.h:31-39`), plus the `SlopeDefaults`/`bSlopeUseGlobal` shared-default split
  (`MASKING_SPEC.md` §1.7). **It has no UI control anywhere** — `StratumsTab_UI.h` SCOPE NOTE 4
  documents this as a known, un-ticketed gap.
- `PreviewBlendMode` (the vocabulary this feature reuses) is a **UI-layer** type
  (`PreviewComposite_Settings_UI.h:35-38`, 12 modes: Replace, AlphaBlend, Add, Multiply, Maximum,
  Minimum, Subtract, Divide, Overlay, Screen, SoftLight, HardLight); its arithmetic lives in
  `PreviewComposite_Color_UI.h`. Mask is PROC and may not include UI headers — reusing the
  *vocabulary/semantics* is fine, reusing that header directly is a layer violation (§1 below).
- A windowed local-neighborhood height-variance computation already exists —
  `PlacementStage::SampleHeightVariance` (`Placement_Metrics_PROC.cpp:27-52`) — but it is private to
  Placement (used only for scatter ranking), not a shared MATH function, and never emitted as a 0..1
  field.
- `MASKING_SPEC.md`'s own "v2 mask-of-masks" note (end of Part 2): *"There is no general mask-multiply
  chain today. v2 should add one (a mask can multiply another) as a tweakable op... when added, it
  lives inside the Mask stage and feeds `surfaceStratumWeights` — never the physical field."* This
  design doc is that item, scoped for real.

## 1. Proposed PARAMS shape

One composed member on `Params::Stratum`, not a rival array — same pattern as its existing
`appearance`/`soilPhysics` sub-structs:

```cpp
enum class MaskFilterType { SlopeGate, HeightBand, Roughness };

struct StratumMaskFilter {
    MaskFilterType type    = MaskFilterType::SlopeGate;
    bool           bEnabled = true;
    BlendMode      blendMode = BlendMode::AlphaBlend;   // relocated PROC-legal enum, see below
    float          alpha    = 1.0f;
    SlopeGateSettings   slope;      // today's 9 fields, moved here verbatim (see §5 migration)
    HeightBandSettings  height;     // same 7-field shape as slope, but gating on height not slope
    RoughnessSettings   roughness;  // windowRadius, varianceMin/Max, feather, invert, strength
};

// on Params::Stratum:
static constexpr int kMaxMaskFiltersPerStratum = 8;
StratumMaskFilter maskFilters[kMaxMaskFiltersPerStratum];
int                maskFilterCount = 0;
```

Fixed-capacity array, not `std::vector` — matches the existing `maskRemapMinimum[kStratumColorChannelCount]`
convention and avoids heap traffic in a per-cell-per-stratum hot loop (Compute Optimization Expert's
lane to confirm, §4). ❓ **ARCH: pin `kMaxMaskFiltersPerStratum`'s value (8 proposed, matching the
9-stratum-array convention's spirit) and rule on where `BlendMode` lives** — it cannot stay defined
inside a UI-only header (`PreviewComposite_Settings_UI.h`) once PROC references it. Likely needs to
move to (or be mirrored in) a MATH/PARAMS-legal shared header, with the UI-side preview enum either
becoming an alias or the two staying independently-defined-but-identical — ARCH's call, not
pre-decided here.

## 2. Chain semantics

In-order left-to-right fold, mirroring `PreviewFieldLayer`'s own documented compositing rule ("Layers
are applied in vector order"):

```
running = materialProportions[s]
for each enabled filter in maskFilters[0 .. maskFilterCount):
    gateValue = Evaluate(filter.type, filter.settings, heightfield/slopeGradient at cell)  // pure, 0..1
    running   = lerp(running, Combine(running, gateValue, filter.blendMode), filter.alpha)
procedural_s = running
surfaceStratumWeights[s] = Merge(procedural_s, storedArt_s, importedMaskMode_s)   // unchanged, fixed final step
```

Stays pure/idempotent — reads only `heightfield` / `materialProportions` / `Params::Stratum` (now
including `maskFilters`), writes only `surfaceStratumWeights`. **Hard invariant, must be stated
verbatim in the eventual coder ticket: no filter evaluator ever reads another stratum's weight or
proportion** — every filter takes only *this* stratum's own settings plus the shared per-cell fields
(heightfield, slope gradient, etc.), which is what keeps a stratum's chain from ever bleeding into a
sibling stratum's mask.

`ImportedMaskMode`'s existing merge (stored art vs. procedural) **stays a fixed trailing step, not a
chain blend-mode option** — folding it into the chain would let `StaticOverride` ("not slope-gated,"
`MASKING_SPEC.md` §1.5) get reordered relative to the filter chain, breaking a pinned rule. ❓ **ARCH:
confirm this stays out of the chain** (default assumption here — flag if the human wants that
reopened).

## 3. New filter types

- **HeightBand** — pure function of `heightfield` + settings, identical 7-field shape to the slope
  gate (min/max, feathers, smoothstep, invert, strength) but gating on height instead of slope
  gradient. Clean fit, no new spec question.
- **Roughness** — needs `SampleHeightVariance`'s windowed-variance math. Proposal: promote it from
  Placement-private to a shared MATH function both Placement and Mask call, rather than duplicating
  the window-sum logic. Still a pure function of `heightfield` + settings (radius, variance
  min/max threshold, feather) — fits the Mask-stage purity contract. ❓ **Compute Optimization
  Expert:** O(window²) per stratum per filter is real cost; if multiple strata request the same
  window radius, a shared precomputed variance field (computed once, read by every stratum's
  Roughness filter) would avoid recomputing it per stratum — worth ruling on before this becomes a
  coder ticket, not an afterthought.
- The human's worked example (subtract-if-height>Y from a sibling filter's own running mask, within
  one stratum's chain) is already expressible by §2's fold: a `HeightBand` filter with
  `blendMode = Subtract` reads the *running* accumulator (already includes the earlier slope-band
  filters) and subtracts wherever its own gate is high. No special-cased "reference another filter's
  output" mechanism is needed — the left-to-right fold already gives every filter the prior filters'
  combined result as its own `running` input.

## 4. Routing — what this design doc does NOT settle

- **ARCH Expert** — ratify: the `maskFilters`/`StratumMaskFilter` PARAMS shape (§1),
  `kMaxMaskFiltersPerStratum`'s value, where the (now PROC-referenced) `BlendMode` enum lives,
  promoting `SampleHeightVariance`'s ownership out of Placement, and confirming `ImportedMaskMode`
  stays outside the chain (§2).
- **Format Expert** — `.sanmap` round-trip shape for the new per-filter list; likely extends
  `StratumGenerationSettings` (`MASKING_SPEC.md` §1.7 Correction 12), which is already the
  index-aligned-with-`stratumLayers[9]` home for the existing slope-gate fields this feature
  generalizes.
- **IO Architecture Expert** — migration path: an old `.sanmap` carries only the flat slope-gate
  fields (no `maskFilters` array at all); on load, synthesize a single
  `maskFilters[0] = {type: SlopeGate, blendMode: AlphaBlend, alpha: 1.0, slope: <old fields>}`,
  `maskFilterCount = 1`, so existing maps render identically post-migration. Needs its own migration
  version step and IO code structure per `IO_MIGRATION_SPEC.md`'s law.
- **Compute Optimization Expert** — evaluation strategy for a variable-length (0..8), per-stratum,
  tagged-union filter chain: branchy per-cell dispatch vs. per-type batched passes, SIMD-friendly
  layout for the fixed-capacity filter array, the Roughness-filter windowed-variance cost/caching
  question from §3, and the GPU kernel shape (`DISPATCH_INTERFACE_SPEC`) for a chain whose length
  varies per stratum.
- **UI Expert** — authoring UI for an ordered, per-stratum filter list (type picker, blend-mode
  picker, alpha slider, per-type settings, reorder/add/remove) — likely reusing
  `DraggableList`/`TreeListWidget_UI<T>` rather than a bespoke widget, per this codebase's own
  "proven twice" reuse discipline. This finally gives the slope gate (and its new siblings) the
  authoring surface `StratumsTab_UI.h` SCOPE NOTE 4 flags as currently missing entirely.

## 5. Migration note (non-binding, for the eventual IO ticket)

Today's flat slope-gate fields on `Params::Stratum` become filter-slot 0's `SlopeGateSettings` under
this design — a real breaking PARAMS shape change, not additive. This is IO Architecture Expert
territory (versioning) and Format Expert territory (exact wire shape), not decided here; flagged so
neither expert is surprised by it when consulted.
