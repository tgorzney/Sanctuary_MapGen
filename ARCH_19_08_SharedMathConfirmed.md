[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.8. **Only the ARCH Expert writes this file.**

### 19.8 Module-boundary placement + shared-function confirmation — applies §3.5, not a separate ruling
This is the third time this class of question was raised per-feature (`BuildSymmetryOrbit` →
§16.3; Assembly's rigid-rotate math → `DESIGN_Assembly_R1.md` §4/§6; Bundle's cycle-detection and
rigid-transform math → `DESIGN_MarkerGroupLayerRestructure_R1.md` §7 items 8-9). **§3.5 settles
it generally; this subsection only applies that rule to this ticket's specific functions, per
the requirement not to re-rule per feature.**

**Placement, by §3.5's signature test:**
- `WouldReparentMarkerLayerBundleCreateCycle(int candidateId, int newParentId, const
  std::vector<MarkerLayerBundle>& bundles)`, `CollectMarkerLayerBundleRecursiveLayerIndices(...)`,
  `CollectMarkerLayerBundleRecursiveManualMembers(...)` — all carry a `Params::`-typed
  parameter → **`PARAMS`**, hand-written, co-located with `MarkerLayerBundle` in
  `MarkerLayerBundle_PARAMS.h` (§19.3). Same file, same layer as
  `WouldReparentCreateCycle`/`CollectAssemblyRecursiveMembership`/`ResolveAssemblyRootAncestor`
  (Assembly's own equivalents, `DESIGN_Assembly_R1.md` §1/§6) — confirmed as PARAMS-resident by
  the same rule, not left as an open question for Assembly's own eventual ticket to re-ask.
- The rigid rotate/translate-around-centroid math, kept as **plain scalars in/out with zero
  `Params::` types in its signature** (`RotatePointAroundPivot(float x, float z, float pivotX,
  float pivotZ, float angleRadians, float& outX, float& outZ)` or equivalent) → **`MATH`**, per
  §3.5's first bullet. This is a **correction, not a restatement**, of both prior design docs —
  neither `DESIGN_Assembly_R1.md` §4/§6 nor `DESIGN_MarkerGroupLayerRestructure_R1.md` §5/§8
  proposed a MATH-layer home; both flagged the placement as genuinely open. §3.5 resolves it: the
  pure geometry has no reason to carry a `Params::` type in its signature, so it belongs in MATH,
  reusable by symmetry-orbit rotation, Assembly's rotate, and Bundle's rotate identically.
- The per-domain orchestration that resolves a selection, calls the MATH function once per member,
  and writes the result back into `recipe.markers[...].transform`/`recipe.props[...].transform`/
  `recipe.decals[...].transform` — **`UI`**, per §3.5's third bullet (a button-triggered live-PARAMS
  edit, not a PROC stage, not a pure resolver).

**Item 9 confirmed: one shared function, not two copies.** Because the MATH-layer rotate function
carries zero `Params::` dependency, Assembly's and Bundle's move/rotate both call the exact same
`MATH` function — there is no domain-specific reason for two implementations, and §3.5's rule
structurally prevents the duplication risk both design docs correctly worried about (a
Params::-carrying version would have forced either a shared PARAMS type neither domain wants, or
genuine duplication).
