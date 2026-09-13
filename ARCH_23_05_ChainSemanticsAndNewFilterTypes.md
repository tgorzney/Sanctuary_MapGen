[← ARCH index](ARCH.md) · [§23 ARCH_23_ProceduralStratumMaskFilterChain](ARCH_23_ProceduralStratumMaskFilterChain.md) · SanGen ARCH §23.5. **Only the ARCH Expert writes this file.**

### 23.5 Chain fold semantics ratified; `HeightBandSettings`/`RoughnessSettings` pinned

#### Fold semantics — RATIFIED verbatim
The design doc's §2 left-to-right fold is ratified as the binding shape, restated here against
§23.1's `std::vector` correction and §23.4's fixed trailing merge:

```
running = materialProportions[s]
for each enabled filter in stratum.maskFilters:               // vector, in authored order
    gateValue = Evaluate(filter.type, filter.settings, <per-cell fields>)   // pure, 0..1
    combined  = Math::CombineChannel(running, gateValue, filter.blendMode)  // §23.2
    running   = lerp(running, combined, filter.alpha)
procedural_s = running
surfaceStratumWeights[s] = Merge(procedural_s, storedArt_s, importedMaskMode_s)   // §23.4, fixed
```

**Binding invariant, restated because it must appear verbatim in the coder ticket (the design
doc's own words, unchanged):** no filter evaluator ever reads another stratum's weight or
proportion. Every filter takes only *this* stratum's own settings plus the shared per-cell fields
(heightfield, slope gradient, windowed variance) — this is what keeps one stratum's chain from
ever bleeding into a sibling's mask, and what keeps Mask a pure function of its three declared
inputs (`MASKING_SPEC.md` §1.3 item 2, ARCH §3.4). The human's own worked example (a `HeightBand`
filter with `blendMode = Subtract` reading the running accumulator that already includes an
earlier `SlopeGate` filter's result) is confirmed correctly expressible by this fold with no
special-cased "reference a specific prior filter" mechanism — the design doc's own §3 reasoning
is accepted as-is.

#### `HeightBandSettings` — pure function of `heightfield`, mirrors `SlopeGateSettings`'s shape
```cpp
struct HeightBandSettings {
    float minimumHeight     = 0.0f;    // designer-facing, GAME-UNIT height (see units note below)
    float maximumHeight     = 0.0f;    // default should be a sane non-degenerate window, not 0/0 —
                                       // left to the coder ticket to pick against terrainMaxHeight
    float heightFeatherLow  = 0.0f;
    float heightFeatherHigh = 0.0f;
    bool  bUseSmoothstep    = false;
    bool  bInvertHeightGate = false;
    float heightGateStrength = 1.0f;
};
```
**Units, pinned now to prevent a repeat of the slope gate's own historical defect.**
`MASKING_SPEC.md` §1.8 pins the slope gate's designer-facing unit as degrees, converted to
gradient magnitude exactly once, in Mask's configuration-flattening step, specifically because the
legacy code's slope-unit ambiguity (degrees vs. `tan²`) was a real, shipped, silent-wrong-filtering
defect (`MASKING_SPEC.md` Part 2, "Known issues"). `HeightBandSettings` gets the same treatment
pre-emptively rather than repeating the mistake once and fixing it later: **designer-facing
`minimumHeight`/`maximumHeight` are in GAME-UNIT height** (the same unit the human authors
`terrainMaxHeight` in elsewhere), converted to the heightfield's own normalized 0..1 storage
range exactly once, in the same configuration-flattening step the slope gate already uses,
`terrainMaxHeight` read from the map per `MASKING_SPEC.md` §1.8 ("never hardcoded to 128"). The
per-cell kernel itself never sees game units, only the pre-resolved normalized threshold — same
shape as the slope gate's degrees→gradient split.

#### `RoughnessSettings` — pinned now, closing the second item left open in the handoff track
The human's second open item (`HANDOFF_TRACK_ProceduralStratumMasking.md` §E: "should
`RoughnessSettings`' exact tunables be scoped now, or left to the Generator Expert's judgment") is
an architecture-consistency call, not a creative one, and is ruled here rather than left blocking:
**`RoughnessSettings` gets the same window/feather/smoothstep/invert/strength shape as
`SlopeGateSettings`/`HeightBandSettings`, plus the one field genuinely unique to it** (the sample
window radius, which controls the *input quantity's* computation, not the gate applied to it):

```cpp
struct RoughnessSettings {
    float windowRadius          = 3.0f;   // cells; the neighborhood Math::WindowedHeightVariance
                                          // samples (§23.3) — NOT a gate-window edge
    float varianceMinimum       = 0.0f;   // gate window low edge, raw variance units (heightfield²)
    float varianceMaximum       = 1.0f;   // gate window high edge
    float varianceFeatherLow    = 0.0f;
    float varianceFeatherHigh   = 0.0f;
    bool  bUseSmoothstep        = false;
    bool  bInvertRoughnessGate  = false;
    float roughnessGateStrength = 1.0f;
};
```
This is a superset of the design doc's own minimal 6-field proposal (it adds the separate
low/high feather split and the smoothstep toggle that `SlopeGateSettings`/`HeightBandSettings`
both already carry), chosen for exact authoring parity across all three filter types rather than
a bespoke, narrower shape for Roughness alone — one UI panel shape serves all three filter types'
gate-window portion identically (a UI Expert win, not decided here, only enabled by it). Raw
variance units are left un-normalized by design: normalizing a variance value meaningfully
requires knowing the heightfield's own dynamic range, which is exactly the kind of derived
per-map quantity the config-flattening step already resolves once for other fields — left to the
coder ticket to decide whether `varianceMinimum`/`varianceMaximum` need their own map-relative
resolution step, analogous to the height/slope unit conversions above, or are authored directly
in the heightfield's native normalized-storage units. Not a blocking ambiguity: either choice is
a pure function of already-established per-map quantities, decidable by the Generator Expert at
ticket-writing time without a further ARCH ruling.

Both `HeightBand` and `Roughness` are pure functions of `heightfield` plus their own settings
(Roughness additionally calling `Math::WindowedHeightVariance`, §23.3) — both fit the Mask-stage
purity contract with no further ruling needed.
