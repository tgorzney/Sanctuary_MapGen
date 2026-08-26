[← ARCH index](ARCH.md) · [§19 ARCH_19_MarkerLayerBundle](ARCH_19_MarkerLayerBundle.md) · SanGen ARCH §19.5. **Only the ARCH Expert writes this file.**

### 19.5 Assembly-references-Bundle — ratified: scalar `assemblyIdentifier` on the Bundle, NOT a `{domain, groupIdentifier}` forward-reference list on Assembly
**Ratified as reframed. The brief's own original ask (`{domain, groupIdentifier}` living on
`Assembly`) is explicitly rejected; the design's §3 reframing is accepted as the correct
resolution and is now binding on both features.**

The brief's suggested shape would have directly contradicted Assembly's own already-decided rule
(`DESIGN_Assembly_R1.md` §0, restating `BRIEF_Assembly_R1.md`'s ground truth): `Params::Assembly`
carries **no members list at all** — "duplicates membership truth in two places that can go out of
sync." A `{domain, groupIdentifier}` array on `Assembly` is exactly that same forward-reference
shape, one tier up. It does not need reopening; the existing backward-tag pattern applies one
tier higher instead:

- **`MarkerLayerBundle` carries its own scalar `assemblyIdentifier`** (§19.3) — the identical
  shape every leaf transform (`MarkerTransform`/`PropTransform`/`DecalTransform`) already carries
  per `DESIGN_Assembly_R1.md` §5. A Bundle belongs to at most one Assembly — same
  no-multi-membership invariant, one tier up, not a new rule.
- **`Assembly` itself needs no new field and no new reference type.** Its
  `{identifier, name, parentIdentifier}` record (`DESIGN_Assembly_R1.md` §5) stays exactly as
  designed — confirmed unchanged by this ruling, not merely left alone by omission.
- **No `GroupDomain` enum or discriminated-union reference type is needed anywhere.** Each
  domain's Bundle table is its own typed array (`markerLayerBundles` today; `propLayerBundles`/
  `decalLayerBundles` later, §19.2) — there is no forward reference to discriminate, because there
  is no forward reference.

**"Live, not snapshot" is confirmed, and the mechanism that gives it that property is named.**
`CollectAssemblyRecursiveMembership` (flagged for a PARAMS home in `DESIGN_Assembly_R1.md` §6,
confirmed as a PARAMS-resident function by §3.5/§19.8) **must be extended** to also scan each
domain's Bundle table for `assemblyIdentifier` matches, and for every match, recursively fold in
that Bundle's resolved Layer→member set via `CollectMarkerLayerBundleRecursiveManualMembers`
(§19.3). Because this is a query-time walk over live `Params::MapRecipe` state — never a
cached/snapshotted list — a marker added to a Bundle-tagged Layer tomorrow is included the next
time the query runs, with no extra machinery and no re-tagging step. This is the concrete
mechanism behind the brief's own "should just work" suspicion, confirmed rather than assumed.

**Sequencing note, not a new ruling.** This extension to `CollectAssemblyRecursiveMembership` is
explicitly **not** part of Ticket A/B (§19 covers Markers-only PARAMS+IO+UI) — it depends on both
Assembly's own ticket and at least one domain's Bundle ticket existing, and is sequenced after
both, exactly as the design's §6 delivery-scoping already stated. Restated here only so the
dependency is visible from the ARCH side too, not to add scope.
