[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.20. **Only the ARCH Expert writes this file.**

### 19.20 Manual-only selection scope — formal law, cross-referencing §19.9
Responds to `DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md` Open Q5. **Ratified: manual-only**,
extending §19.9's already-ratified manual-only-membership reasoning one further tier, for the
identical reason: `Data::PlacementInstances` is PROC-regenerated every bake with no cross-bake
stable identity a selection could hang on (Constitution's DATA-is-pure-computed-output rule; §14.8's
dirty-flag tiers). A procedurally-placed instance has no `instanceIdentifier` — §19.16 is a
`MarkerTransform`-only field; `Data::PlacementInstances` carries no such field and gains none from
this ruling — and cannot be selected by this mechanism.

`OverlayInstanceKey_UI`'s existing procedural-only selection-key pipeline is untouched, unshared, and
unreferenced by this feature — the new `instanceIdentifier`/select-color/highlight surface is
Manual-marker-specific end to end, with zero shared plumbing and zero risk the two selection concepts
drift into each other.

**Binding, recorded so a future reader doesn't re-derive it:** any future ticket proposing
procedural-instance selection needs its own new PARAMS/DATA-layer identity mechanism (out of scope
here); it may not repurpose `instanceIdentifier` for that.
