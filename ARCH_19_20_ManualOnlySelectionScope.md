[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.20. **Only the ARCH Expert writes this file.**

### 19.20 Manual-only selection scope — formal law, cross-referencing §19.9 (**narrowed by §19.25/§19.27, 2026-08-26 correction round — read the note below before applying this section**)
Responds to `DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md` Open Q5. Originally ratified:
manual-only, extending §19.9's already-ratified manual-only-membership reasoning one further tier,
for the identical reason: `Data::PlacementInstances` is PROC-regenerated every bake with no
cross-bake stable identity a selection could hang on (Constitution's DATA-is-pure-computed-output
rule; §14.8's dirty-flag tiers). A procedurally-placed instance has no `instanceIdentifier` —
§19.16 is a `MarkerTransform`-only field; `Data::PlacementInstances` carries no such field and gains
none from this ruling.

> **CORRECTED (Markers UI Correction Round 2, item 13 of `DESIGN_MarkersUICorrectionRound2_R1.md`;
> full rulings in §19.25 and §19.27).** The human explicitly overrode this section's "manual-only"
> scoping after checking it against his own original hierarchy diagram, which showed procedural
> instances listed and selectable exactly like manual ones. Two sentences below are now **superseded,
> not current law**:
> - *"a procedurally-placed instance ... cannot be selected by this mechanism"* — false as a whole-
>   feature statement. §19.27 gives procedural instances their OWN selection mechanism: a session-only
>   per-frame positional index over `Data::PlacementInstances` (no persisted identity needed, since
>   the index is rebuilt every frame and simply stops resolving after a rebake — the identity-
>   stability concern this section originally raised turned out to be solvable without a persisted
>   field).
> - *"`OverlayInstanceKey_UI`'s existing procedural-only selection-key pipeline is untouched,
>   unshared, and unreferenced by this feature"* — false. §19.25 widens `OverlayInstanceKey_UI` itself
>   (new `bManual` field) and §19.27 explicitly CONVERGES procedural list-selection onto that same
>   representation, by design, as one shared mechanism with canvas click-pick.
>
> **What still stands, unchanged, from this section's original ruling — the one binding sentence
> below that was never in question:** `instanceIdentifier` (§19.16) is a `MarkerTransform`-only field
> and is NEVER repurposed for procedural identity. §19.27's procedural mechanism keys instances by
> array position (`bManual=false`), a separate, `DATA`-scoped, session-only identity — not
> `instanceIdentifier`. The static symmetric-sibling highlight (§19.19) and the select-color surface
> (§19.17/§19.18) also remain manual-only, untouched by the Round 2 correction — only
> listing/selection itself was extended to procedural instances, not the full manual-marker feature
> set this section originally scoped.

`instanceIdentifier`-keyed selection stays formally manual-only for the reason originally given here
— see §19.16/§19.19 — this section's title is kept for that reason. Treat any reference to this
section as scoped narrowly to the `instanceIdentifier` mechanism from this point forward; for the
canvas/list selection REPRESENTATION as a whole, see §19.25 (canonical) and §19.27 (procedural).

**Binding, recorded so a future reader doesn't re-derive it (still current):** any future ticket
extending the `instanceIdentifier`-keyed highlight/select-color surface (§19.16-§19.19) to procedural
instances needs its own new PARAMS/DATA-layer identity mechanism for that surface specifically — it
may not repurpose `instanceIdentifier` for that, exactly as this section originally ruled.
