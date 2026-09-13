[← ARCH index](ARCH.md) · [§23 ARCH_23_ProceduralStratumMaskFilterChain](ARCH_23_ProceduralStratumMaskFilterChain.md) · SanGen ARCH §23.1. **Only the ARCH Expert writes this file.**

### 23.1 PARAMS shape — `StratumMaskFilter` on `Params::Stratum`, `std::vector` not a fixed array

**The composition shape is RATIFIED as proposed, with two corrections.** One new member on
`Params::Stratum`, same pattern as its existing `appearance`/`soilPhysics` sub-structs
(`Stratum_PARAMS.h`) — this satisfies ARCH §7.1's "exactly one per-stratum settings type,
composition allowed, no rival top-level array" rule exactly: `maskFilters` is reached only
through `Params::Stratum`, never independently by a stage.

```cpp
enum class MaskFilterType { SlopeGate, HeightBand, Roughness };

struct StratumMaskFilter {
    MaskFilterType   type      = MaskFilterType::SlopeGate;
    bool             bEnabled  = true;
    Math::BlendMode  blendMode = Math::BlendMode::AlphaBlend;   // §23.2
    float            alpha     = 1.0f;
    SlopeGateSettings   slope;
    HeightBandSettings  height;
    RoughnessSettings   roughness;
};

// on Params::Stratum:
std::vector<StratumMaskFilter> maskFilters;   // ORDERED — left-to-right fold is semantically
                                               // meaningful (§23.5), same convention as
                                               // MarkerChain::markers, CountScenario::conditions.
```

**Correction 1 — `std::vector`, not `StratumMaskFilter maskFilters[8]` + a count field.** The
design doc's fixed-capacity proposal cited `maskRemapMinimum[kStratumColorChannelCount]` as
precedent and reasoned the fixed shape "avoids heap traffic in a per-cell-per-stratum hot loop."
Both premises don't hold under a direct read of `src/params/`:

- Every genuinely fixed-cardinality PARAMS array in this codebase sizes to a **real fixed
  domain** — 4 color channels, 9 stratum slots (and even that one, `MapRecipe_PARAMS.h:56`, is
  itself `std::vector<Stratum> strata`, not a C array). There is no precedent anywhere in
  `src/params/` for a fixed-capacity array paired with a companion count field to represent an
  author-driven, variable-length **ordered list of composable items** — that shape is
  universally `std::vector<T>` in this codebase: `GradientRamp::stops`, `MarkerRuleLayer::rules`,
  `MarkerChain::markers` ("ORDERED — semantically meaningful sequence"), `CountScenario::conditions`,
  `PreviewCompositeSettings::fieldLayers` (the UI-side layer stack this feature explicitly
  mirrors — also blend-mode-bearing, also ordered, also a plain `std::vector`). `maskFilters` is
  this same shape and should be this same container.
- The heap-traffic concern is moot for the reason it matters here: `Params::Stratum` instances
  are **per-stratum** (9 of them, total, per map), never per-cell. The Mask stage's actual
  per-cell kernel does not walk `Params::Stratum`/`maskFilters` live — it consumes a
  config-flattened POD record built once per generation (`MASKING_SPEC.md` §1.7/§1.8's existing
  "configuration flattening" step, the same mechanism that already turns per-stratum degrees into
  a resolved gradient threshold before the per-cell loop runs). A `std::vector`'s one-time
  allocation happens at config-build time, 9 times per generation, not once per cell — negligible
  by any measure. The PARAMS-side container choice has **zero** per-cell performance
  consequence; only the PROC-side kernel-config layout does (§23.6 routes that to Compute
  Optimization Expert, where a genuine fixed-capacity concern legitimately belongs — a GPU SSBO
  record needs a fixed stride, a `std::vector<StratumMaskFilter>` does not).

**This closes `kMaxMaskFiltersPerStratum` by making the question moot, not by picking a number.**
No PARAMS-side ceiling constant exists. "As complex as needed" (the human's own phrase) is
satisfied natively by an unbounded vector — closing the first item `HANDOFF_TRACK_
ProceduralStratumMasking.md` §E left open. If the Compute Optimization Expert's GPU kernel-config
pass (§23.6) needs its own fixed per-stratum filter-slot cap for buffer layout, that is a distinct,
later, PROC/GPU-dispatch-shape number — not this one, and not an ARCH/PARAMS ruling.

**Correction 2 — `bSlopeUseGlobal` moves onto `SlopeGateSettings`, not left on `Params::Stratum`'s
root.** The design doc's §5 migration note says "today's flat slope-gate fields... become filter
slot 0's `SlopeGateSettings` verbatim" but never addresses `bSlopeUseGlobal`
(`MASKING_SPEC.md` §1.7's shared-default flag) or `SlopeDefaults`'s continued role. Left on
`Params::Stratum`'s root, `bSlopeUseGlobal` would apply to the stratum as a whole while
`maskFilters` may now contain **more than one** `SlopeGate`-type filter (e.g. the human's own
worked example: two chained slope bands) — a single root-level flag cannot mean "use global
defaults" independently per filter instance. `bSlopeUseGlobal` becomes a field on
`SlopeGateSettings` itself, meaningful only when `type == SlopeGate`, so each `SlopeGate` filter
in a stratum's chain independently chooses default-vs-override — a strict generalization of the
old per-stratum semantics (a stratum with exactly one `SlopeGate` filter behaves identically to
today), not a behavior change for any existing single-slope-gate map. `SlopeDefaults` itself is
unchanged; it stays the shared-default source the flattening step reads from when a filter's own
`bSlopeUseGlobal` is true.

**File-size consequence (ARCH §1.5).** `Stratum_PARAMS.h` cannot hold `StratumMaskFilter` plus its
three settings sub-structs and stay under the ceiling. Per §7.1's existing rule ("composition is
allowed... split into its own header when the §1.5 ceiling forces it... a member file, never a
settings type a stage reaches independently"), the new types land in a sibling file —
e.g. `StratumMaskFilter_PARAMS.h` — `#include`d by `Stratum_PARAMS.h`, exactly like
`StratumAppearance_PARAMS.h`/`StratumSoilPhysics_PARAMS.h` already are. Left to the coder ticket
to size correctly against the real line count once written; not pre-decided here beyond "it is a
member file, not a rival type."
