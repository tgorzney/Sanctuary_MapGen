[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.7. **Only the ARCH Expert writes this file.**

### 22.7 Mask-to-rectangle authoring workflow — recorded as the current manual process

**Recorded, not ratified as a SanGen feature.** The rectangle lists used for both live tests were
produced by an ad hoc Python pipeline, run entirely outside SanGen, by hand. Full six-step
procedure (mask authoring at `heightmapResolution`, threshold + connected-component labeling,
exact largest-rectangle decomposition, greedy agglomerative merge with a human-tunable
overshoot/count tradeoff threshold, mandatory zero-missed-pixel verification, and the inherent
no-rotation limitation): `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7.

**Ruled: this is flagged as a strong candidate for a future SanGen-native masking/placement
feature, not designed here.** No `PARAMS` shape, `PROC` stage, or `IO`/export surface is ruled on
by this subsection. Natural future home: `MASKING_SPEC.md` (the mask-authoring/raster-asset half)
and `PLACEMENT_SCATTER_SPEC.md` (the rectangle-decomposition/placement half) — both specs carry a
short forward-pointer to `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7.1 for this reason, not a
restatement of the workflow.
