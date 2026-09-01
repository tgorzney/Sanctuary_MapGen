# STEP246 — Instance-tier resolvers for the 6 governed Link fields + wire into real render/gesture consumers

**Layer:** UI. **Domain:** `src/ui/MarkersTab_ManualLayerHelpers_UI.h`, a new sibling file for the
remaining instance-tier resolvers (ARCH §1.5 sizing left to the coder — `MarkersTab_MarkerLinkResolvers_UI.h`
is already near its soft ceiling, do not add 8 more functions to it in place), `src/ui/MarkersTab_ManualInstance_UI.cpp`,
`src/ui/MapCanvas_MarkerRosterDraw_UI.cpp`, `src/ui/MapCanvas_IconLayer_CullManual_UI.cpp`.
**Sequence:** depends on STEP244.

Ratifies `ARCH_19_33_LinkMembershipInstanceTierCorrection.md` (the resolver contract) and
`ARCH_21_09_LinkTierContractWidening.md` (the concrete `Locked`/`QuantizeMarkerPositionToLayerGrid`/
`ResolveEffectiveMarkerSymmetry` shapes, which supersede §19.33's own placeholder wording for those
three). Read both ARCH sections in full before starting — this ticket is a restatement, not the
source of truth.

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file above.

## Fix

1. **`MarkersTab_ManualLayerHelpers_UI.h`** — widen these two EXISTING functions in place (per
   `ARCH_21_09`, these are widened, not doubled — every real call site always has a transform in hand):
   ```cpp
   void QuantizeMarkerPositionToLayerGrid(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                          const Params::MarkerTransform& transform,
                                          const std::vector<Params::MarkerLink>& links,
                                          float& worldX, float& worldZ);
   void ResolveEffectiveMarkerSymmetry(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                       const Params::MarkerTransform& transform,
                                       const std::vector<Params::MarkerLink>& links,
                                       int globalSymmetryMask, int globalRadialRepeatCount,
                                       int& outMask, int& outRadialRepeatCount);
   ```
   Resolution order inside both (identical, ARCH §19.33/§21.9): (1) `transform.linkIdentifier >= 0`
   and resolves → that Link's field; (2) else `layer.linkIdentifier >= 0` and resolves → that Link's
   field (existing Layer-tier mechanism, unchanged); (3) else → the Layer's own stored field. Update
   every existing call site of both functions to pass the transform + `recipe.markerLinks` instead of
   a bare `layerIndex`.

   Also add, same file:
   ```cpp
   bool EffectiveManualMarkerInstanceLocked(const Params::MarkerTransform& transform,
                                            const Params::MarkerInstanceLayer& layer,
                                            const std::vector<Params::MarkerLink>& links);
   bool IsMarkerInstanceLocked(const Params::MarkerTransform& transform,
                               const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                               const std::vector<Params::MarkerLink>& links);
   // IsMarkerInstanceLayerLocked(markerLayers, layerIndex) stays UNCHANGED, NOT deleted — still the
   // correct call for a layerIndex-only site (a Layer-row header lock toggle).
   ```

2. **New sibling file** (name it per your own ARCH §1.5 sizing judgment, e.g.
   `MarkersTab_MarkerLinkInstanceResolvers_UI.h`) — the remaining 4 governed-field resolver pairs,
   same 3-parameter `(transform, layer, links)` shape as `EffectiveManualMarkerInstanceLocked` above,
   each checking `transform.linkIdentifier` first, falling back to the existing 2-parameter
   `MarkersTab_MarkerLinkResolvers_UI.h` Layer-tier resolver when the instance itself isn't tagged:
   - `EffectiveManualMarkerInstanceHidden`
   - `EffectiveManualMarkerInstanceIconScale`
   - `EffectiveManualMarkerInstanceColorOverrideEnabled` / `EffectiveManualMarkerInstanceColor`
     (falls back to `MarkersTab_ManualLayerHelpers_UI.h`'s existing STEP239
     `EffectiveManualMarkerLayerColorOverrideEnabled`/`Color`)
   - Grid-snap-enabled and symmetry-enabled BOOLEAN gates (`EffectiveManualMarkerInstanceGridSnapEnabled`,
     `EffectiveManualMarkerInstanceSymmetryEnabled`) — distinct from the two WIDENED functions in step
     1 above, which resolve the actual grid-size/mask VALUES; these resolve whether the feature is
     toggled on at all, mirroring the existing 2-parameter `EffectiveManualMarkerLayerGridSnapEnabled`/
     `SymmetryEnabled` split.
   **`name` gets NO instance-tier resolver** — `ARCH_19_33`'s explicit refinement: `MarkerTransform::name`
   is the marker's own proper identity (e.g. "Mex 0"), not a label mirror; do not invent
   `EffectiveMarkerTransformName` or anything resolving a Link's name onto it.

3. **Wire the new resolvers into the three real, previously Link-blind consumers** (confirmed by
   direct grep during design: these are the ONLY non-UI-row-display consumers of the 7 governed
   fields — everything else touching them is the disabled-row-mirror display in the Bundle tree,
   already correctly reading the OLD 2-parameter resolvers and needing no change here):
   - `MarkersTab_ManualInstance_UI.cpp` (`DrawSelectedMarkerInstance`, lines ~129,143) — already
     holds `transform`/`markerLayers` in scope; add a `links` parameter, swap in the new
     `IsMarkerInstanceLocked`/widened `QuantizeMarkerPositionToLayerGrid` calls.
   - `MapCanvas_MarkerRosterDraw_UI.cpp` (`ManualMarkerTint`, `ManualMarkerDotRadius`) — both already
     iterate one `transform` at a time inside `DrawManualMarkerRoster`'s loop; thread
     `recipe.markerLinks` down (a `recipe`-adjacent pointer is already threaded into this call site
     via `manualMarkerDragRecipe`, `MapCanvas_MarkerDrag_UI.cpp` — reuse it, don't add a new field).
     Swap in `EffectiveManualMarkerInstanceColorOverrideEnabled`/`Color`/`IconScale`.
   - `MapCanvas_IconLayer_CullManual_UI.cpp` (`ResolveMarkersManual`) — **real restructuring, not a
     rename**: the raw `bHidden`/`iconScale`/color reads are currently hoisted ONCE PER LAYER, outside
     the per-transform loop (lines ~145-163). Under this correction different transforms on the same
     Layer can resolve differently (that's the whole point of instance-tier tagging), so these three
     reads must move INSIDE the existing per-transform loop (lines ~179-233), each call now passing
     that iteration's own `transform`. This is a perf-shape change (hoisted-once → per-instance) —
     call it out explicitly in the PR/commit, don't let it read as an accidental regression.

## Verify

- Extend/add resolver unit tests: an instance tagged to a Link resolves that Link's field even when
  its owning Layer has a DIFFERENT (or no) `linkIdentifier`; an untagged instance on a Link-bound
  Layer still resolves the Layer's Link (existing §19.31 behavior, unchanged); an instance with a
  dangling `linkIdentifier` (no matching `Params::MarkerLink`) soft-degrades to the Layer-tier result,
  never a crash/refusal.
- A live regression this correction is explicitly FOR: two markers sharing one Layer, one tagged to a
  Link with `bHidden = true`, the other untagged — confirm only the tagged one hides in the canvas
  render, the other stays visible. (This exact scenario was impossible to express correctly before
  this ticket — write it as a new test, not a modification of an existing one.)
- Confirm `ResolveMarkersManual`'s restructuring doesn't change output for the common case (no Links
  in use at all) — every existing `MapCanvas_IconLayer_Cull_UI_Test`/`MapCanvas_IconLayer_Microbenchmark_UI_Test`
  case stays green and, per the microbenchmark test's own name, note whether the hoist-to-per-instance
  change shows any measurable throughput delta (expected negligible at authored-map instance counts,
  per `DESIGN_MarkerLink_R1.md` §7's own "authoring-scale only" framing, but don't assume — check).
- Full `MarkersTab_UI_Test`/`MapCanvas_*_UI_Test` suites stay green.

## Out of scope

- The drag-gesture (`InstanceDragGesture_UI.h`) and hit-test/delete (`ManualInstanceHitTest_UI.h`/
  `ManualInstanceDelete_UI.h`) shared cross-domain generic widening — that's STEP249, ratified
  separately by `ARCH_21_09` because it touches Props/Decals code paths too.
- `ApplyAddLinkAction`/`DeleteMarkerLink`/the no-op guard (STEP247) and the Links-Section UI body
  (STEP248).
