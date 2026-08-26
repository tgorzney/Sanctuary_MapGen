[← ARCH index](ARCH.md) · SanGen ARCH §19. Part of the ratified v2 architecture; the Constitution and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 19. The Group-above-Layer container — ratified `MarkerLayerBundle` (ratifies `work_orders/DESIGN_MarkerGroupLayerRestructure_R1.md`, firms up `work_orders/DESIGN_Assembly_R1.md` where this design forces a call the unratified Assembly design didn't anticipate)

Responds to `work_orders/BRIEF_MarkerGroupLayerRestructure_R1.md` +
`work_orders/DESIGN_MarkerGroupLayerRestructure_R1.md` §7's 13-item ratification list. Assembly
(`work_orders/BRIEF_Assembly_R1.md` / `DESIGN_Assembly_R1.md`) remains a separate, still-unbuilt
feature — this ratification does not merge them, per the human's own explicit confirmation
relayed in both design docs. Where this ticket's recursion forces a ruling Assembly's own
unratified design left open or spelled wrong, this section rules it for both, cited from both
future work-orders.

**The name.** The new C++/wire type is **`MarkerLayerBundle`**, not `MarkerLayerGroup`/`Cluster`/
`Ensemble`/`Formation` (§19.1). The UI display label stays **"Group"** regardless — a cosmetic
string, not a type identity.

### Subsections of §19

| § | File | Ruling |
|---|---|---|
| §19.1 | [ARCH_19_01_NamingRatified.md](ARCH_19_01_NamingRatified.md) | Final type/wire name — `MarkerLayerBundle`; UI label stays "Group" |
| §19.2 | [ARCH_19_02_GenericitySplit.md](ARCH_19_02_GenericitySplit.md) | Domain-touching-logic-vs-pure-mechanics genericity split — general rule for all future Group/Bundle work |
| §19.3 | [ARCH_19_03_FieldSpellings.md](ARCH_19_03_FieldSpellings.md) | Field spellings for `MarkerLayerBundle` (cross-refs §1.9's "Id" ban) |
| §19.4 | [ARCH_19_04_WireShape.md](ARCH_19_04_WireShape.md) | New top-level wire key/shape/casing — `MarkerLayerBundles` |
| §19.5 | [ARCH_19_05_AssemblyReferencesBundle.md](ARCH_19_05_AssemblyReferencesBundle.md) | Assembly-references-Bundle — scalar `assemblyIdentifier` on the Bundle, not a forward-reference list on Assembly |
| §19.6 | [ARCH_19_06_NestedBundleAssemblyCutoff.md](ARCH_19_06_NestedBundleAssemblyCutoff.md) | Nested child Bundle with its own different `assemblyIdentifier` stops the recursive walk there |
| §19.7 | [ARCH_19_07_TreeListWidgetOwnership.md](ARCH_19_07_TreeListWidgetOwnership.md) | `TreeListWidget_UI<T>` — one shared, domain-agnostic widget; Markers' own Ticket B builds it first |
| §19.8 | [ARCH_19_08_SharedMathConfirmed.md](ARCH_19_08_SharedMathConfirmed.md) | Bundle's rigid-transform math and cycle-detection — same shared functions as Assembly's, per §3.5, not two copies |
| §19.9 | [ARCH_19_09_ManualOnlyMembership.md](ARCH_19_09_ManualOnlyMembership.md) | Manual-layer-only membership confirmed consistent with Assembly's own §0 ruling |
| §19.10 | [ARCH_19_10_TabDrivenV1Scoping.md](ARCH_19_10_TabDrivenV1Scoping.md) | v1 Move/Rotate is tab-driven only — no new canvas gesture, deferred until Assembly's own canvas work ships |
| §19.11 | [ARCH_19_11_FormatSpecCorrectionBundle.md](ARCH_19_11_FormatSpecCorrectionBundle.md) | `SANMAP_FORMAT_SPEC.md` staleness correction bundle — missing `MarkerGroups`/`markers[type].transforms[name]` fields, the confirmed-live `layerId` defect, the unfixed `layerIndex` export bug, the new `MarkerLayerBundles` Correction |
| §19.12 | [ARCH_19_12_SoftTypeConsistency.md](ARCH_19_12_SoftTypeConsistency.md) | Bundle→marker-type consistency stays soft (UI-enforced only) — no import-time hard validation |

Related law: `sangen_arch_pack/CONSTITUTION.md`; `ARCH_03_ModuleBoundaries.md` §3.5 (the general
MATH/PARAMS/PROC placement rule this section's math rulings apply, not re-derive);
`ARCH_01_09_IdAbbreviationBan.md` (the "Id" ban this section's field spellings apply);
`ARCH_16_MarkerLayerSymmetry.md` (the `MarkerRuleLayer`/`MarkerInstanceLayer` shapes this section
adds a back-reference field to, without reopening their existing fields).
