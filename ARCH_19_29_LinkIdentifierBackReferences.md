[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.29. **Only the ARCH Expert writes this file.**

> **⚠️ CORRECTED 2026-08-31 — see `ARCH_19_33_LinkMembershipInstanceTierCorrection.md`.** The
> "Neither field is added to `MarkerRuleLayer` or `MarkerTransform`" paragraph below is **retracted
> for `MarkerTransform` only** (the `MarkerRuleLayer`/procedural-instances-out-of-scope half is
> unchanged and still correct). `§19.33` adds `int linkIdentifier = -1;` to `MarkerTransform` as a
> THIRD independent tier in this section's own two-tier pattern, consulted first by the six
> behavioral/rendering governed-field resolvers (`§19.31`) ahead of the Layer-tier resolution this
> section already establishes — because "+Link" no longer mints an exclusive Bundle/Layer or moves
> instances (a later correction to `DESIGN_MarkerLink_R1.md` §3.6/§3.7), so "every instance on this
> Layer" and "every instance in this Link" are no longer the same set, and this section's own
> reasoning for excluding `MarkerTransform` no longer holds. This file's own text is left unedited
> beneath this notice for historical record of the original ratification only — `§19.33` is
> authoritative on any conflict with what follows.

### 19.29 New `linkIdentifier` scalar fields on `MarkerLayerBundle` and `MarkerInstanceLayer` — ratified, independent (not walk-up-derived)
Responds to `work_orders/DESIGN_MarkerLink_R1.md` §3.3/§5 item 2. **Ratified as designed, no
correction.** ⚠️ **A third, independent tier is added by `§19.33` — see the notice above.**

```cpp
// MarkerLayerBundle (MarkerLayerBundle_PARAMS.h) gains:
int linkIdentifier = -1;   // organizational — which MarkerLink created this Group; drives the
                            // Links tier's own membership/ungroup walk (§19.31's Delete-Link
                            // semantics). -1 = not Link-bound, the shared sentinel this whole
                            // struct family already uses (identifier/parentBundleIdentifier/
                            // assemblyIdentifier).

// MarkerInstanceLayer (MarkerInstance_PARAMS.h) gains:
int linkIdentifier = -1;   // the ACTUAL color/visibility-resolution key (§19.31) — checked
                            // directly, never derived by walking up parentBundleIdentifier, so a
                            // later re-nest of the Layer under a different Group never silently
                            // changes which Link governs its color/visibility.
```

**Independent, two-field placement confirmed — not a re-derivation, an application of an
already-twice-ratified pattern.** `markerTypeName` already lives independently on both
`MarkerLayerBundle` (§19.3) and `MarkerRuleLayer`/`MarkerInstanceLayer` (§19.13) — two structs at
different tiers, each carrying its own copy of a cross-cutting back-reference, set independently,
with no walk-up derivation between them. `linkIdentifier` applies that identical pattern a third
time. If the two tiers' `linkIdentifier` values ever disagree (e.g. a Layer re-nested under a
different Group after both were tagged), that is a soft silent degrade — same class as every other
dangling back-reference in this family (`parentBundleIdentifier`, `assemblyIdentifier`) — never a
structural error, never validated at import (Constitution §6's soft-degrade posture, applied
again, not a new rule).

**Neither field is added to `MarkerRuleLayer` or `MarkerTransform`.** [⚠️ **RETRACTED for
`MarkerTransform` by `§19.33` — see the notice above; `MarkerTransform::linkIdentifier` now exists.
The `MarkerRuleLayer` bullet below is unaffected and still governs.**] Link membership is
Layer-tier-and-above only:
- `MarkerRuleLayer` (the procedural-rule tier) is out of scope by construction — §0 of the design
  (restating `ARCH_19_09_ManualOnlyMembership.md`'s already-binding reasoning) rules Link
  membership manual-instances-only, and `MarkerRuleLayer` has no manual member concept at all.
- ~~`MarkerTransform` (the raw per-instance leaf) needs no field of its own — color-override has
  never been a per-transform field in this codebase (it lives on `MarkerInstanceLayer` today,
  `MarkerInstance_PARAMS.h:25`), and a Link never needs to reach past the Layer down to the raw
  transform for anything this ticket requires.~~ **No longer true — see `§19.33`.** This reasoning
  held only while "+Link" minted a Link-exclusive Layer (guaranteeing Layer membership == Link
  membership); that guarantee was retracted by direct human ruling and `§19.33` adds the field.

**Additive, no `SanGenVersion` bump** — same precedent class as `markerTypeName`/
`assemblyIdentifier`, both already-shipped additive scalars on these exact two structs. `§19.33`'s
`MarkerTransform::linkIdentifier` addition is additive on the identical basis, one tier further.
