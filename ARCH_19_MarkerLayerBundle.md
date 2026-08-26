[← ARCH index](ARCH.md) · Part of the ratified v2 architecture; the Constitution and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 19. The Group-above-Layer container — ratified `MarkerLayerBundle` (ratifies `work_orders/DESIGN_MarkerGroupLayerRestructure_R1.md`, firms up `work_orders/DESIGN_Assembly_R1.md` where this design forces a call the unratified Assembly design didn't anticipate); extended by `work_orders/DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md` (§19.13–§19.22)

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

**§19.13–§19.22** ratify `work_orders/DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md` — the
dynamic, UI-derived Type-section tier over `markerTypeName`; the per-type filtered
`TreeListWidget_UI<MarkerLayerBundle>` composition and its cross-Type-section nesting cutoff; the
new `MarkerTransform::instanceIdentifier` and manual-instance selection highlight (priority order,
visual language, sibling-orbit computation, canvas wiring); the `GlobalMarkerSettings` select-color
fields; and a `§1.5` file-size-ceiling remediation this extension required ahead of its own Ticket B.

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
| §19.13 | [ARCH_19_13_MarkerRuleLayerTypeName.md](ARCH_19_13_MarkerRuleLayerTypeName.md) | `markerTypeName` on `MarkerRuleLayer`/`MarkerInstanceLayer` — additive, wire key `"MarkerTypeName"`, extends §19.3 |
| §19.14 | [ARCH_19_14_TypeSectionUiDerived.md](ARCH_19_14_TypeSectionUiDerived.md) | The Type-section tier is UI-derived — dynamic enumeration over `markerTypeName`, not a stored `Params` container; ordering rule |
| §19.15 | [ARCH_19_15_TypeSectionTreeComposition.md](ARCH_19_15_TypeSectionTreeComposition.md) | Type-section × Bundle-tree composition — filtered-copy `TreeListWidget_UI` per type, the cross-Type-section nesting cutoff, `bRowSuppressed`'s two-predicate composition |
| §19.16 | [ARCH_19_16_InstanceIdentifier.md](ARCH_19_16_InstanceIdentifier.md) | `MarkerTransform::instanceIdentifier` — global uniqueness, wire key `"InstanceIdentifier"`, legacy-backfill mirrors `layerId`'s precedent |
| §19.17 | [ARCH_19_17_SelectColorFields.md](ARCH_19_17_SelectColorFields.md) | `GlobalMarkerSettings` select-color fields — strict 3-field mirror plus the signed-off `selectColorDefault` deviation |
| §19.18 | [ARCH_19_18_SelectionTintPriorityAndVisualLanguage.md](ARCH_19_18_SelectionTintPriorityAndVisualLanguage.md) | Selection tint — canonical priority order; "selected replaces fill" distinct from the drag-ghost's unfilled-ring vocabulary |
| §19.19 | [ARCH_19_19_StaticHighlightComputationAndWiring.md](ARCH_19_19_StaticHighlightComputationAndWiring.md) | Static selection-highlight — one-shot orbit computation (not `MarkerOrbitCorrespondence_UI.h`), tolerance reuse, canvas wiring |
| §19.20 | [ARCH_19_20_ManualOnlySelectionScope.md](ARCH_19_20_ManualOnlySelectionScope.md) | Manual-only selection scope — formal law, cross-referencing §19.9 |
| §19.21 | [ARCH_19_21_CategoryVsMarkerTypeNameClosed.md](ARCH_19_21_CategoryVsMarkerTypeNameClosed.md) | `MarkerRule::category` vs. `markerTypeName` — two permanently independent concepts, closed |
| §19.22 | [ARCH_19_22_ManualLayersHeaderSplit.md](ARCH_19_22_ManualLayersHeaderSplit.md) | File-size ceiling remediation — `MarkersTab_ManualLayers_UI.h` split, resolved ahead of Ticket B |

Related law: `sangen_arch_pack/CONSTITUTION.md`; `ARCH_03_ModuleBoundaries.md` §3.5 (the general
MATH/PARAMS/PROC placement rule this section's math rulings apply, not re-derive);
`ARCH_01_09_IdAbbreviationBan.md` (the "Id" ban this section's field spellings apply);
`ARCH_16_MarkerLayerSymmetry.md` (the `MarkerRuleLayer`/`MarkerInstanceLayer` shapes this section
adds back-reference fields to, without reopening their existing fields).
