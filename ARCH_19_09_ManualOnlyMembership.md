[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.9. **Only the ARCH Expert writes this file.**

### 19.9 Manual-layer-only membership — confirmed consistent with Assembly's own §0 ruling, not a drifting variant
**Confirmed, not a new rule.** `DESIGN_Assembly_R1.md` §0 already ruled `AssemblyId` (now
`assemblyIdentifier`, §19.3) membership manual-instances-only, on the grounds that
`Data::PlacementInstances` is PROC-regenerated every bake with no cross-bake stable identity to
hang a persisted tag on (Constitution's DATA-is-pure-computed-output rule; ARCH §14.8's dirty-flag
tiers). The identical reasoning applies one tier up, unchanged: a Bundle containing a Procedural
`MarkerRuleLayer` shows that Layer in the tree (organizational membership — the Bundle legitimately
groups both Procedural and Manual Layers, per the human's own worked example), but that Layer
contributes **zero** members to a Move/Rotate Apply, and zero members to an Assembly's resolved
membership reached transitively through that Bundle (§19.5).

This is the same restriction, at the same layer boundary, for the same reason — recorded here so a
future reader of either feature's design does not need to reconcile two independently-worded
rulings that could plausibly have drifted. They do not: one ruling, cited from both.

**Practical consequence, restated for implementers.** `CollectMarkerLayerBundleRecursiveLayerIndices`
(§19.3) enumerates BOTH `MarkerRuleLayer` and `MarkerInstanceLayer` indices under a Bundle — this
is the tree's organizational/display enumeration, correctly including Procedural layers.
`CollectMarkerLayerBundleRecursiveManualMembers` (§19.3) is a **separate, narrower** function that
only walks `MarkerInstanceLayer` and the transforms indexing them — this is the one Move/Rotate
Apply and the Assembly-membership walk (§19.5) both call. Two functions, deliberately, not one
function with a filter flag — the tree needs the wide enumeration, the transform/membership walks
need the narrow one, and conflating them into one flag-gated function would make it too easy for a
future call site to pass the wrong flag silently.
