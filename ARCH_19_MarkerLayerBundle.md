[← ARCH index](ARCH.md) · Part of the ratified v2 architecture; the Constitution and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 19. The Group-above-Layer container — ratified `MarkerLayerBundle` (ratifies `work_orders/DESIGN_MarkerGroupLayerRestructure_R1.md`, firms up `work_orders/DESIGN_Assembly_R1.md` where this design forces a call the unratified Assembly design didn't anticipate); extended by `work_orders/DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md` (§19.13–§19.22), `work_orders/DESIGN_MarkersUICorrectionRound2_R1.md` (§19.23–§19.27), `work_orders/DESIGN_MarkerLink_R1.md` (§19.28–§19.32), and `work_orders/BRIEF_MarkerLinkCorrection_R1.md` (§19.33)

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

**§19.23–§19.27** ratify `work_orders/DESIGN_MarkersUICorrectionRound2_R1.md` — the human's
post-STEP121-126 correction round. `TreeListWidget_UI<T,LeafKeyT>::Render` gains a two-callback
header-extra contract (§19.23); `MarkerInstanceLayer` gains `bSymmetryEnabled` (§19.24); canvas
click-pick and Markers-tab list-selection are unified onto one `OverlayInstanceKey_UI`
representation, correcting §19.20's "manual-only" framing (§19.25); the manual instance list gets
symmetry-cluster grouping (§19.26); and procedural marker instances get their own listing/selection
mechanism, overriding §19.20's earlier scope-out (§19.27).

**§19.28–§19.32** ratify `work_orders/DESIGN_MarkerLink_R1.md` §3/§5's new-field half (the Delete
key and the "+Group"/"+Layer" selection-move behavior — §1/§2 of that design — needed no ARCH
ruling and are not covered here) — the Link mechanic: a new stored `Params::MarkerLink` type
(§19.28, **field list corrected 2026-08-31, see §19.31**), independent `linkIdentifier`
back-references on `MarkerLayerBundle` and `MarkerInstanceLayer` (§19.29, **a THIRD tier —
`MarkerTransform` — added 2026-08-31, see §19.33**), the `MarkerLinks` wire array shape (§19.30),
the propagated-property ruling — **corrected 2026-08-31 by direct human ruling to ONE uniform
read-and-resolve mechanism covering every Section/Group-equivalent setting, including Name, plus the
extended set (`iconScale`, grid-snap, symmetry); the original two-mechanism ruling is retracted; a
same-day follow-up amendment adds a seventh governed field, `bLocked`, on the identical mechanism,
per direct human ruling that "everything should be cascaded down to the Groups in the Link" —
`MarkerLayerBundle` itself is explicitly ruled to have no `bLocked`-equivalent counterpart; **further
corrected 2026-08-31 (same day) by §19.33 to add an instance-tier resolution step ahead of the
Layer-tier one, for six of the seven fields (not Name)** — (§19.31) — and new `GlobalMarkerSettings`
fields `scaleSelectedAlloy/Plasma/Spawn` for the Type-section header's icon-size rework (§19.32).

**§19.33** rules `work_orders/BRIEF_MarkerLinkCorrection_R1.md`'s human-directed reversal of a named
§19.29 sentence: "+Link" no longer mints an exclusive `MarkerLayerBundle`/`MarkerInstanceLayer` or
moves instances — a Link becomes a pure per-instance tag, `MarkerTransform` gains its own
`linkIdentifier`, and the six behavioral/rendering governed-field resolvers (§19.31, everything
except Name) check the instance's own tag first, falling back to the existing Layer-tier resolution
unchanged. Ruled architecturally sound with one refinement (Name stays Layer/Bundle-tier only, no
instance-tier equivalent — a `MarkerTransform`'s own `name` is a different, pre-existing field, the
marker's own proper identity, not a Section/Group row label). No migration needed for old-mechanism
`.sanmap` data — the new check is a pure superset, falling through to the unchanged existing
mechanism. `ApplyAddLinkAction`/`DeleteMarkerLink` corrected to match.

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
| §19.20 | [ARCH_19_20_ManualOnlySelectionScope.md](ARCH_19_20_ManualOnlySelectionScope.md) | Manual-only selection scope — formal law, cross-referencing §19.9. **Narrowed by §19.25/§19.27** (Round 2 correction): `instanceIdentifier`-keyed selection stays manual-only; overall canvas/list selection no longer is |
| §19.21 | [ARCH_19_21_CategoryVsMarkerTypeNameClosed.md](ARCH_19_21_CategoryVsMarkerTypeNameClosed.md) | `MarkerRule::category` vs. `markerTypeName` — two permanently independent concepts, closed |
| §19.22 | [ARCH_19_22_ManualLayersHeaderSplit.md](ARCH_19_22_ManualLayersHeaderSplit.md) | File-size ceiling remediation, FINAL combined plan (2026-08-26 revision) — `MarkersTab_ManualLayers_UI.h` splits along BOTH the RowBody fault line and Ticket B's own required Helpers fault line, additively; supersedes this section's earlier single-split text once Ticket B's actual draft (`STEP125`) proved it added new declarations of its own |
| §19.23 | [ARCH_19_23_TreeListHeaderExtraContract.md](ARCH_19_23_TreeListHeaderExtraContract.md) | `TreeListWidget_UI<T,LeafKeyT>::Render` gains a header-extra contract — TWO callbacks (Node vs. Leaf), a deliberate divergence from `DraggableList`'s single-callback shape |
| §19.24 | [ARCH_19_24_SymmetryEnabledField.md](ARCH_19_24_SymmetryEnabledField.md) | `Params::MarkerInstanceLayer::bSymmetryEnabled` — new field, mirrors `bColorOverrideEnabled`'s shape, wire key `"SymmetryEnabled"` |
| §19.25 | [ARCH_19_25_SelectionRepresentationUnification.md](ARCH_19_25_SelectionRepresentationUnification.md) | Canvas/list selection unification — `OverlayInstanceKey_UI::bManual`, `MapCanvas`'s widened selection surface, the shell-mediated tab↔canvas callback; corrects and narrows §19.20 |
| §19.26 | [ARCH_19_26_ManualInstanceSymmetryGrouping.md](ARCH_19_26_ManualInstanceSymmetryGrouping.md) | Manual-instance symmetry-cluster grouping in the instance list — UI composition only, no PARAMS change |
| §19.27 | [ARCH_19_27_ProceduralInstanceSelectionMechanism.md](ARCH_19_27_ProceduralInstanceSelectionMechanism.md) | Procedural marker-instance listing/selection — per-frame `ruleIndex` positional index, convergence with §19.25, bucket-size symmetry-grouping rule; narrows §19.20 |
| §19.28 | [ARCH_19_28_MarkerLinkParamsType.md](ARCH_19_28_MarkerLinkParamsType.md) | New PARAMS type `Params::MarkerLink` — stored, not UI-derived, `MapRecipe::markerLinks`. **Field list corrected 2026-08-31 — see §19.31** |
| §19.29 | [ARCH_19_29_LinkIdentifierBackReferences.md](ARCH_19_29_LinkIdentifierBackReferences.md) | New `linkIdentifier` scalar fields on `MarkerLayerBundle` and `MarkerInstanceLayer` — independent, not walk-up-derived. **A third, independent tier (`MarkerTransform`) added 2026-08-31 — see §19.33** |
| §19.30 | [ARCH_19_30_MarkerLinksWireShape.md](ARCH_19_30_MarkerLinksWireShape.md) | `MarkerLinks` wire array shape — `Color` as `{r,g,b,a}`, `LinkIdentifier` merged onto `MarkerLayerBundles`/`MarkerGroups` |
| §19.31 | [ARCH_19_31_PropagatedPropertyMechanisms.md](ARCH_19_31_PropagatedPropertyMechanisms.md) | **CORRECTED 2026-08-31, then FURTHER AMENDED same day, then FURTHER CORRECTED same day (see §19.33).** Propagated-property ruling — ONE uniform read-and-resolve/master-slave mechanism covering SEVEN governed fields (name, color-override/color, `bHidden`, `iconScale`, grid-snap, symmetry, `bLocked`); six of the seven (not name) additionally resolve against the owning `MarkerTransform`'s own tag first, per §19.33 |
| §19.32 | [ARCH_19_32_MarkerSelectedScaleFields.md](ARCH_19_32_MarkerSelectedScaleFields.md) | `GlobalMarkerSettings` gains `scaleSelectedAlloy/Plasma/Spawn` — per-Type-section base+selected pair, `[0.25, 2.0]` UI clamp |
| §19.33 | [ARCH_19_33_LinkMembershipInstanceTierCorrection.md](ARCH_19_33_LinkMembershipInstanceTierCorrection.md) | **CORRECTED 2026-08-31, direct human ruling.** Link membership moves to instance-tier tagging — `MarkerTransform::linkIdentifier` (reverses a named §19.29 sentence), six-of-seven governed-field resolvers gain an instance-tier-first check (Name excluded, refinement), `ApplyAddLinkAction`/`DeleteMarkerLink` corrected to stop minting/moving Bundles/Layers, no migration needed for old-mechanism data |

Related law: `sangen_arch_pack/CONSTITUTION.md`; `ARCH_03_ModuleBoundaries.md` §3.5 (the general
MATH/PARAMS/PROC placement rule this section's math rulings apply, not re-derive);
`ARCH_01_09_IdAbbreviationBan.md` (the "Id" ban this section's field spellings apply);
`ARCH_14_08_DirtyFlagTiers.md` §14.8 (dirty-flag tiers, applied by §19.27's zero-DAG-participation
rule); `ARCH_16_MarkerLayerSymmetry.md` (the `MarkerRuleLayer`/`MarkerInstanceLayer` shapes this
section adds back-reference fields to, without reopening their existing fields).
