[← ARCH index](ARCH.md) · Part of the ratified v2 architecture; the Constitution and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 23. Procedural per-stratum mask filter chain (ARCH ruling, responds to `work_orders/DESIGN_ProceduralStratumMaskFilters_R1.md`)

Ratifies (with corrections) the Generator Expert's design for generalizing the Mask stage's
single hardcoded slope gate into an ordered, per-stratum chain of procedural filters
(slope/height/roughness, more later), each with its own settings, a blend mode, and an alpha —
exactly `MASKING_SPEC.md`'s own flagged-but-unscoped "v2 mask-of-masks" item (end of its Part 2),
now scoped for real. Responds to the design doc's §4 routing list item-for-item; also closes both
items the human left open in `work_orders/HANDOFF_TRACK_ProceduralStratumMasking.md` §E.

The design doc's own §0 ground truth is accepted as read and is not re-verified here except
where a ruling below corrects it. Two corrections of substance, both explained in their own
subsection: the PARAMS shape is a `std::vector`, not a fixed `[8]` array with a ceiling constant
(§23.1); `bSlopeUseGlobal` moves from `Params::Stratum`'s root onto the new `SlopeGateSettings`
sub-struct, a migration detail the design doc's §5 did not anticipate (§23.1).

| § | File | Ruling |
|---|------|--------|
| §23.1 | [ARCH_23_01_ParamsShapeAndFilterCount.md](ARCH_23_01_ParamsShapeAndFilterCount.md) | `StratumMaskFilter` / `Params::Stratum::maskFilters` shape; corrects the design's fixed-`[8]`-array proposal to an unbounded `std::vector`, closing the `kMaxMaskFiltersPerStratum` question by making it moot |
| §23.2 | [ARCH_23_02_BlendModeRelocatesToMath.md](ARCH_23_02_BlendModeRelocatesToMath.md) | `PreviewBlendMode`'s vocabulary and arithmetic relocate to a new `Math::BlendMode` / `Math::CombineChannel`; `Ui::PreviewBlendMode` becomes an alias |
| §23.3 | [ARCH_23_03_WindowedVariancePromotedToMath.md](ARCH_23_03_WindowedVariancePromotedToMath.md) | `PlacementStage::SampleHeightVariance`'s core promoted to `Math::WindowedHeightVariance`, mirroring `RadialClearance_MATH.h`'s raw-pointer convention; Placement becomes a thin wrapper |
| §23.4 | [ARCH_23_04_ImportedMaskModeStaysTrailingStep.md](ARCH_23_04_ImportedMaskModeStaysTrailingStep.md) | Confirmed: `ImportedMaskMode`'s stored-art merge stays a fixed step after the whole chain, never a chain blend-mode option |
| §23.5 | [ARCH_23_05_ChainSemanticsAndNewFilterTypes.md](ARCH_23_05_ChainSemanticsAndNewFilterTypes.md) | Chain fold semantics ratified; `HeightBandSettings`/`RoughnessSettings` field shapes pinned (closes both items left open in the handoff track's §E) |
| §23.6 | [ARCH_23_06_RoutingConfirmedAndSpecCatchUp.md](ARCH_23_06_RoutingConfirmedAndSpecCatchUp.md) | Format/IO Architecture/Compute Optimization/UI Expert routing confirmed unchanged from the design doc's §4; `MASKING_SPEC.md` Part 1 content catch-up flagged, not performed in this pass |

No item in this ratification is left as a blocking open question for the human — §23.1 and §23.5
each resolve one of the two items `HANDOFF_TRACK_ProceduralStratumMasking.md` §E recorded as
unanswered, by architecture reasoning rather than by asking again. Nothing here is a coder
work-order; §23.6 states exactly what still has to happen (Format/IO/Compute/UI consults, then
coder tickets) before STEP269+ can be written.
