# SanGen ARCH — the authoritative v2 architecture

The binding architecture for the SanGen v2 rebuild. Authored and ratified with the
human by the SanGen ARCH Expert. **Only the ARCH Expert writes this file.** It
resolves the Constitution's **(TBD)** items and executes the opening hit-list. Read
`sangen_arch_pack/CONSTITUTION.md` (always-loaded law) first; load specs from
`sangen_arch_pack/INDEX.md` as needed.

## Opening hit-list (the v2 mandate)
1. Reconcile the two data-model families — dead `core/data/*` + `GenParams_*` vs live
   `params/Params_*`.
2. Dismember the `GenerationParams` god object; evict GPU/GL state from DATA.
3. Unify the CPU/GPU twins behind one dispatch interface; retire rival toggles.
4. Eliminate the preview's shadow reimplementation of the sim (WYSIWYG).

---

## How this document is organized

`ARCH.md` is the **index**. No ruling text lives here.

Every ratified section lives in its own file at the repo root, numbered to match its
section number, so a citation resolves by arithmetic:

- **§14** → `ARCH_14_PreviewOverlayLayering.md`
- **§14.9** → `ARCH_14_09_RenderingPerformance.md`

A section long enough to need it is itself split one-file-per-subsection; its
`ARCH_NN_<Topic>.md` file then holds only the ruling's framing plus a table of its
subsection files. Short sections stay whole. **Target: no ARCH file over ~100 lines** —
the files that exceed it are single indivisible rulings, flagged in the table below.

Section numbers are permanent and never renumbered, so every `§N` / `§N.M` citation
across the specs, work-orders, and agent charters keeps resolving forever.

**Load only what you need** — one subsection file, not a section, and never the whole
ARCH. This is the same on-demand discipline `sangen_arch_pack/INDEX.md` applies to specs.

A new ratification appends a **new file** (the next unused top-level number, e.g.
`ARCH_20_*.md`, split into `ARCH_20_NN_*.md` if it runs long) plus its rows below.
Existing files are amended in place. **Only the ARCH Expert writes any of them.**

---

## Section index

| § | File | Ruling | Size |
|---|------|--------|------|
| **§1** | [ARCH_01_NamingLaw.md](ARCH_01_NamingLaw.md) | Naming law (Constitution §2, resolved) | index → 9 subsection files |
| §1.1 | [ARCH_01_01_LiteralNames.md](ARCH_01_01_LiteralNames.md) | Literal, fully-spelled names — no abbreviations | 20 lines |
| §1.2 | [ARCH_01_02_LayerTagSuffix.md](ARCH_01_02_LayerTagSuffix.md) | Layer tag is a SUFFIX (TGUE convention) | 19 lines |
| §1.3 | [ARCH_01_03_MathNaming.md](ARCH_01_03_MathNaming.md) | Math naming — domain + optimization variant | 11 lines |
| §1.4 | [ARCH_01_04_CpuGpuKernelPairing.md](ARCH_01_04_CpuGpuKernelPairing.md) | CPU / GPU kernels pair by shared base name | 11 lines |
| §1.5 | [ARCH_01_05_FileSizeCeilings.md](ARCH_01_05_FileSizeCeilings.md) | File-size ceilings | 12 lines |
| §1.6 | [ARCH_01_06_SanmapKeyCasing.md](ARCH_01_06_SanmapKeyCasing.md) | `.sanmap` top-level key casing — game-native vs SanGen-owned (ratifies work-order SPEC-4 Correction 0) | 24 lines |
| §1.7 | [ARCH_01_07_IoMigrationFileNaming.md](ARCH_01_07_IoMigrationFileNaming.md) | IO migration file naming — schema version steps (ratifies `IO_MIGRATION_SPEC`) | 16 lines |
| §1.8 | [ARCH_01_08_ParamsFieldNamingByKind.md](ARCH_01_08_ParamsFieldNamingByKind.md) | PARAMS field naming for format-derived types — governed by data KIND, not by key presence | 43 lines |
| §1.9 | [ARCH_01_09_IdAbbreviationBan.md](ARCH_01_09_IdAbbreviationBan.md) | "Id" is banned — resolved once, binding on every current and future field; retroactively confirms the shipped `layerId` defect | 62 lines |
| **§2** | [ARCH_02_LayerDirectoryMap.md](ARCH_02_LayerDirectoryMap.md) | Layer → directory map (Constitution §1/§2) | 24 lines |
| **§3** | [ARCH_03_ModuleBoundaries.md](ARCH_03_ModuleBoundaries.md) | Module boundaries & ownership (Constitution §1, resolved) — §3.5 (new) is the general MATH/PARAMS/PROC placement rule for pure `Params::`-shaped math | 126 lines |
| **§4** | [ARCH_04_DispatchContract.md](ARCH_04_DispatchContract.md) | Dispatch contract (Constitution §4, resolved) | 81 lines |
| **§5** | [ARCH_05_GodObjectDismemberment.md](ARCH_05_GodObjectDismemberment.md) | God-object dismemberment (hit-list #1–2) | 41 lines |
| **§6** | [ARCH_06_RebuildOrder.md](ARCH_06_RebuildOrder.md) | v2 rebuild order (dependency-ordered milestones) | 37 lines |
| **§7** | [ARCH_07_M3Resolutions.md](ARCH_07_M3Resolutions.md) | M3 design resolutions (ARCH rulings) | index → 5 subsection files |
| §7.1 | [ARCH_07_01_ParamsPerStratum.md](ARCH_07_01_ParamsPerStratum.md) | Where the remaining PARAMS live — one settings type per stratum | 33 lines |
| §7.2 | [ARCH_07_02_MaterialProportionVsSurfaceWeight.md](ARCH_07_02_MaterialProportionVsSurfaceWeight.md) | Material proportion vs surface weight — the two fields (RATIFIED) | 162 lines |
| §7.3 | [ARCH_07_03_VendoredThirdPartyHeaders.md](ARCH_07_03_VendoredThirdPartyHeaders.md) | Vendored third-party headers | 9 lines |
| §7.4 | [ARCH_07_04_PipelineStageOrder.md](ARCH_07_04_PipelineStageOrder.md) | Pipeline stage order (binding on M3-8) | 29 lines |
| §7.5 | [ARCH_07_05_TriagedFollowUps.md](ARCH_07_05_TriagedFollowUps.md) | Triaged follow-ups — NOT in scope for the M3-2/M3-8 rework | 39 lines |
| **§8** | [ARCH_08_M4Resolutions.md](ARCH_08_M4Resolutions.md) | M4 design resolutions (ARCH rulings) | index → 4 subsection files |
| §8.1 | [ARCH_08_01_GradientLutBakeIsUi.md](ARCH_08_01_GradientLutBakeIsUi.md) | The gradient LUT bake is `UI`, not `PROC` (corrects §5.4) | 33 lines |
| §8.2 | [ARCH_08_02_GradientRampParams.md](ARCH_08_02_GradientRampParams.md) | `GradientRamp_PARAMS` — the missing v2 settings type | 29 lines |
| §8.3 | [ARCH_08_03_SpatialGridVsSpacingGrid.md](ARCH_08_03_SpatialGridVsSpacingGrid.md) | `SpatialGrid_DATA` vs `Placement_SpacingGrid_PROC` — two different structures | 39 lines |
| §8.4 | [ARCH_08_04_CoderScopeLaw.md](ARCH_08_04_CoderScopeLaw.md) | Scope law — a coder never invents a missing type | 17 lines |
| **§9** | [ARCH_09_ArmyUnitGroupMapArea.md](ARCH_09_ArmyUnitGroupMapArea.md) | `Params::Army` / `UnitGroup` / `UnitTransform` / `MapArea` (ARCH ruling, ratifies `ENTITY_AUTHORING_PARAMS_SPEC`) | 23 lines |
| **§10** | [ARCH_10_AtmosphereParams.md](ARCH_10_AtmosphereParams.md) | `Params::Atmosphere` (ARCH ruling, ratifies `ATMOSPHERE_PARAMS_SPEC`) | 42 lines |
| **§11** | [ARCH_11_GlobalMarkerSettings.md](ARCH_11_GlobalMarkerSettings.md) | `Params::GlobalMarkerSettings` (ARCH ruling, completes `SANMAP_FORMAT_SPEC` Correction 7) | 40 lines |
| **§12** | [ARCH_12_ManualPropDecalLayers.md](ARCH_12_ManualPropDecalLayers.md) | Manual-layer authoring for props/decals — `layerIndex` + `PropGroups`/`DecalGroups` (ARCH ruling, revises `ENTITY_AUTHORING_PARAMS_SPEC`) | 59 lines |
| **§13** | [ARCH_13_RadialSymmetry.md](ARCH_13_RadialSymmetry.md) | Radial N-fold symmetry — `SymmetryAxis::Radial` + `radialSymmetryRepeatCount` (ARCH ruling, amends `Symmetry_PARAMS.h`, `SANMAP_FORMAT_SPEC` Correction 4) | 70 lines |
| **§14** | [ARCH_14_PreviewOverlayLayering.md](ARCH_14_PreviewOverlayLayering.md) | Preview overlay layering — six-domain screen-space compositor (ARCH ruling, ratifies `work_orders/DESIGN_MarkerPreviewLayering_R2.md`) | index → 16 subsection files |
| §14.1 | [ARCH_14_01_ModuleBoundaryDataVsParams.md](ARCH_14_01_ModuleBoundaryDataVsParams.md) | Module boundary and the DATA-vs-PARAMS split | 16 lines |
| §14.2 | [ARCH_14_02_DataModel.md](ARCH_14_02_DataModel.md) | Data model (binding shape) | 43 lines |
| §14.3 | [ARCH_14_03_IconRenderingLod.md](ARCH_14_03_IconRenderingLod.md) | Icon rendering — two-mode LOD, not constant-screen-size-only | 32 lines |
| §14.4 | [ARCH_14_04_NestedUnitGroupAddressing.md](ARCH_14_04_NestedUnitGroupAddressing.md) | Nested `UnitGroup` addressing is flat | 12 lines |
| §14.5 | [ARCH_14_05_ViewStackState.md](ARCH_14_05_ViewStackState.md) | View-stack state — split by field, not one blanket policy | 16 lines |
| §14.6 | [ARCH_14_06_OverlayDomainKindCoexistence.md](ARCH_14_06_OverlayDomainKindCoexistence.md) | `OverlayDomainKind_UI` vs `MarkerCategory`/`PlacementResults` — sits alongside, changes neither | 9 lines |
| §14.7 | [ARCH_14_07_ViewToolbar.md](ARCH_14_07_ViewToolbar.md) | View toolbar — replaces "Regenerate," one popup / two non-crossing sections | 43 lines |
| §14.8 | [ARCH_14_08_DirtyFlagTiers.md](ARCH_14_08_DirtyFlagTiers.md) | Dirty-flag tiers — four, not two (extends `PREVIEW_COMPOSITING_SPEC`'s existing two-tier model) | 23 lines |
| §14.9 | [ARCH_14_09_RenderingPerformance.md](ARCH_14_09_RenderingPerformance.md) | Rendering/performance — mandatory in the first work-order, not deferrable | 42 lines |
| §14.10 | [ARCH_14_10_GpuColorReadbackBug.md](ARCH_14_10_GpuColorReadbackBug.md) | GPU color-texture readback bug (recorded, separate narrow fix, lands first) | 13 lines |
| §14.11 | [ARCH_14_11_Determinism.md](ARCH_14_11_Determinism.md) | Determinism | 10 lines |
| §14.12 | [ARCH_14_12_Naming.md](ARCH_14_12_Naming.md) | Naming | 6 lines |
| §14.13 | [ARCH_14_13_OpenItems.md](ARCH_14_13_OpenItems.md) | Open items — status as of this ratification (closed items marked) | 137 lines |
| §14.14 | [ARCH_14_14_AlloySpawnsArmiesManualRouting.md](ARCH_14_14_AlloySpawnsArmiesManualRouting.md) | Alloy/SpawnsArmies Manual sub-layer routing — no discriminator field on `MarkerInstanceLayer`; routed per-transform by the reserved `"Spawn"` group name (responds to `STEP97_AlloySpawnsArmiesManualSubLayers_UI.md`) | 51 lines |
| §14.15 | [ARCH_14_15_ManualCullStableIdMigration.md](ARCH_14_15_ManualCullStableIdMigration.md) | Manual props/decals cull-path stable-id migration — corrects §14.13 item 3's stale "unscheduled" line (both work-orders shipped); rules `MapCanvas_IconLayer_CullManual_UI.cpp` migrates its match key from positional `layerIndex` to stable `manualLayerId`, resolved live against PARAMS (not read from `Data::PlacementInstances`, a confirmed staleness hazard) | 106 lines |
| §14.16 | [ARCH_14_16_PerArmyUnitsOverlayRows.md](ARCH_14_16_PerArmyUnitsOverlayRows.md) | Per-army Units overlay rows — dynamic row-per-army (not a fixed enum), real `UnitRule::armyIndex` plumbing for procedural units (corrects a relayed "Faction-only" premise), per-army tint reads `Army::armyColor` directly, plus the v1 default-color-palette port this ruling needs to not be inert | 114 lines |
| **§15** | [ARCH_15_MapScenarioSystem.md](ARCH_15_MapScenarioSystem.md) | The SanGen Map Scenario system — formalized as first-class law (ratifies `MAP_SCENARIO_SPEC.md`) | index → 10 subsection files |
| §15.1 | [ARCH_15_01_LayerClassification.md](ARCH_15_01_LayerClassification.md) | Layer classification | 13 lines |
| §15.2 | [ARCH_15_02_IoScopeRuling.md](ARCH_15_02_IoScopeRuling.md) | IO scope ruling — corrects an earlier assumption, does not reverse it | 39 lines |
| §15.3 | [ARCH_15_03_ExportOnlyLuaRatified.md](ARCH_15_03_ExportOnlyLuaRatified.md) | Design ratified: option (c) — export-only, SanGen never parses Lua back (resolves §15.2's open question / `MAP_SCENARIO_SPEC.md` §8); amended with a template-ingestion scope-clarifying sentence, §18 | 38 lines |
| §15.4 | [ARCH_15_04_ThreeFileOnDiskShape.md](ARCH_15_04_ThreeFileOnDiskShape.md) | Three-file on-disk shape + overwrite safety (ratifies `MAP_SCENARIO_SPEC.md` §2/§2.1/§2.2); corrected 2026-08-28 re: the retired `SpawnNavalFleets` reference | 42 lines |
| §15.5 | [ARCH_15_05_ParamsScenariosType.md](ARCH_15_05_ParamsScenariosType.md) | `Params::Scenarios` — the new PARAMS type (shape ruling); naval-fleet types (`ScenarioNavalFleet`/`ScenarioNavalFleetEntry`/`ScenarioNavalPondSide`/`ScenarioNavalPondAssignment`, `ScenarioBody::navy`) RETIRED 2026-08-28, replaced by `spawnsUnits` + name-keyed dispatch; two items left explicitly OPEN, not guessed at | 154 lines |
| §15.6 | [ARCH_15_06_CountScenariosOrdering.md](ARCH_15_06_CountScenariosOrdering.md) | `COUNT_SCENARIOS` ordering — array order IS the match-priority authoring action | 18 lines |
| §15.7 | [ARCH_15_07_OwnershipSplit.md](ARCH_15_07_OwnershipSplit.md) | Ownership split — who ratifies what for the new `Params::Scenarios` family | 22 lines |
| §15.8 | [ARCH_15_08_ThirdPartyDependencyRuling.md](ARCH_15_08_ThirdPartyDependencyRuling.md) | Third-party dependency ruling — ImGuiColorTextEdit + embedded LuaJIT | 54 lines |
| §15.9 | [ARCH_15_09_EngineWhitelistMigrationPath.md](ARCH_15_09_EngineWhitelistMigrationPath.md) | Engine-whitelist migration path (recorded as intended future simplification, not built) | 19 lines |
| §15.10 | [ARCH_15_10_SlotPatternConstructionMoves.md](ARCH_15_10_SlotPatternConstructionMoves.md) | Slot-pattern construction moves into the runtime; `maxArmySlotCount` becomes authored data (ratifies the human's construction-code-belongs-in-universal-mod-code decision; amends `MAP_SCENARIO_SPEC.md` §2/§3/§4) | 119 lines |
| **§16** | [ARCH_16_MarkerLayerSymmetry.md](ARCH_16_MarkerLayerSymmetry.md) | Marker layer-scoped symmetry — `MarkerRuleLayer` / `MarkerInstanceLayer` / `SymmetrySetting` (ARCH ruling, ratifies `work_orders/DESIGN_MarkerLayerSymmetry_R1.md` + `_R2.md`) | index → 11 subsection files |
| §16.1 | [ARCH_16_01_NewParamsShapes.md](ARCH_16_01_NewParamsShapes.md) | New PARAMS shapes — ratified, with §16.5's naming amendment folded in | 68 lines |
| §16.2 | [ARCH_16_02_MarkerRuleLayersKeepsName.md](ARCH_16_02_MarkerRuleLayersKeepsName.md) | `markerRuleLayers` keeps its full name; does not shorten to `markerLayers` like `propLayers` did | 11 lines |
| §16.3 | [ARCH_16_03_ModuleBoundaryChain.md](ARCH_16_03_ModuleBoundaryChain.md) | Module boundary — solved via the existing legal `UI → PIPELINE → PROC` chain; no MATH relocation, no new UI→PROC exception | 68 lines |
| §16.4 | [ARCH_16_04_SanmapCorrection7Amendment.md](ARCH_16_04_SanmapCorrection7Amendment.md) | `SANMAP_FORMAT_SPEC` Correction 7 amendment — scoped-down one-tier shape accepted; exact key spelling deferred to the Format Expert | 28 lines |
| §16.5 | [ARCH_16_05_MarkerTransformFields.md](ARCH_16_05_MarkerTransformFields.md) | `MarkerTransform` — `symmetryGroupIdentifier` (not `symmetryGroupId`) + already-ratified `layerIndex`; superseded as the canonical "Id" ban citation by §1.9, not deleted | 39 lines |
| §16.6 | [ARCH_16_06_MigrationRouting.md](ARCH_16_06_MigrationRouting.md) | Migration — a real breaking schema change; routed to the IO Architecture Expert, not ruled on here | 20 lines |
| §16.7 | [ARCH_16_07_NamingConfirmed.md](ARCH_16_07_NamingConfirmed.md) | Naming confirmed — `MarkerRuleLayer` / `MarkerInstanceLayer` | 14 lines |
| §16.8 | [ARCH_16_08_SpawnArmyShrink.md](ARCH_16_08_SpawnArmyShrink.md) | Spawn/Army shrink — confirmed: no new PARAMS type or field needed; corrected wording — match key is `MarkerTransform::name` only, never `alias` | 24 lines |
| §16.9 | [ARCH_16_09_NonArchItems.md](ARCH_16_09_NonArchItems.md) | Non-ARCH items — confirmed out of scope for this ratification | 9 lines |
| §16.10 | [ARCH_16_10_ConsultRoutingSummary.md](ARCH_16_10_ConsultRoutingSummary.md) | Consult routing summary — what remains before a coder work-order can be written; item 3 corrected — two PROC consumers, not one, per `STEP79_MarkerRuleLayerProcConsumer_PROC.md` | 42 lines |
| §16.11 | [ARCH_16_11_ScatterRuleSymmetryUnification.md](ARCH_16_11_ScatterRuleSymmetryUnification.md) | The §16.1 non-binding follow-on, now taken — `SymmetrySetting symmetry;` retrofitted onto `PropRule`/`DecalRule`/`UnitRule`; no JSON/migration change (C++-internal refactor only) | 55 lines |
| **§17** | [ARCH_17_MigrationValuesRegistry.md](ARCH_17_MigrationValuesRegistry.md) | `bLosslessIfSkipped` values for the 9 shipped `SanGenVersion` 2→3 migrations (ARCH ruling, ratifies `IO_MIGRATION_SPEC.md` §3, backfills `STEP26A_MigrationLosslessFlagAndPreview_IO.md`) | 66 lines |
| **§18** | [ARCH_18_SantpFootprintIngestion.md](ARCH_18_SantpFootprintIngestion.md) | `.santp`/`.sanprop` template ingestion — sandboxed execution primitive + determinism ruling (responds to `DESIGN_SantpFootprintIngestion_R1.md`) | index → 3 subsection files |
| §18.1 | [ARCH_18_01_SandboxedExecutionPrimitive.md](ARCH_18_01_SandboxedExecutionPrimitive.md) | `LuaTableEvaluate_SYS`/`LuaTableValue_SYS` — a sibling `SYS` primitive to `LuaSyntaxCheck_SYS`, sharing only the vendored LuaJIT library; the execution safety contract, binding | 59 lines |
| §18.2 | [ARCH_18_02_IngestedDataDeterminism.md](ARCH_18_02_IngestedDataDeterminism.md) | Determinism ruling — ingested footprint data may influence generation only after being baked into `PARAMS`; never read live by `PROC` | 69 lines |
| §18.3 | [ARCH_18_03_CatalogDataOwnership.md](ARCH_18_03_CatalogDataOwnership.md) | Q3 ruled — richer catalog data (footprint + tags, the two artifacts tickets 89/92 need) stays `IO`-owned, asset-derived, matching `AssetAtlasCache_*`; no new `DATA`-layer catalog type; `economy.harvest`/`collisionInfo`/`displayName` explicitly deferred | 33 lines |
| **§19** | [ARCH_19_MarkerLayerBundle.md](ARCH_19_MarkerLayerBundle.md) | The Group-above-Layer container — `MarkerLayerBundle` (ARCH ruling, ratifies `work_orders/DESIGN_MarkerGroupLayerRestructure_R1.md`, firms up open items in the still-unratified `work_orders/DESIGN_Assembly_R1.md`); extended by `work_orders/DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md` and `work_orders/DESIGN_MarkersUICorrectionRound2_R1.md` | index → 27 subsection files |
| §19.1 | [ARCH_19_01_NamingRatified.md](ARCH_19_01_NamingRatified.md) | Final type/wire name — `MarkerLayerBundle`, not `MarkerLayerGroup`/`Cluster`/`Ensemble`/`Formation`; UI label stays "Group" | 39 lines |
| §19.2 | [ARCH_19_02_GenericitySplit.md](ARCH_19_02_GenericitySplit.md) | Domain-touching-vs-pure-mechanics genericity split — general rule for all future Group/Bundle work (Props, Decals, NavMesh) | 33 lines |
| §19.3 | [ARCH_19_03_FieldSpellings.md](ARCH_19_03_FieldSpellings.md) | `MarkerLayerBundle` field spellings — `identifier`/`parentBundleIdentifier`/`markerTypeName`/`assemblyIdentifier`, applying §1.9 | 44 lines |
| §19.4 | [ARCH_19_04_WireShape.md](ARCH_19_04_WireShape.md) | New top-level wire key/shape — `MarkerLayerBundles`, PascalCase, additive, no `SanGenVersion` bump | 39 lines |
| §19.5 | [ARCH_19_05_AssemblyReferencesBundle.md](ARCH_19_05_AssemblyReferencesBundle.md) | Assembly-references-Bundle — scalar `assemblyIdentifier` on the Bundle, not a `{domain, groupIdentifier}` forward-reference list on Assembly | 41 lines |
| §19.6 | [ARCH_19_06_NestedBundleAssemblyCutoff.md](ARCH_19_06_NestedBundleAssemblyCutoff.md) | Nested child Bundle with its own different `assemblyIdentifier` stops the recursive walk there — new rule | 27 lines |
| §19.7 | [ARCH_19_07_TreeListWidgetOwnership.md](ARCH_19_07_TreeListWidgetOwnership.md) | `TreeListWidget_UI<T>` — one shared, domain-agnostic widget; Markers' own Ticket B builds it first | 34 lines |
| §19.8 | [ARCH_19_08_SharedMathConfirmed.md](ARCH_19_08_SharedMathConfirmed.md) | Bundle's rigid-transform math and cycle-detection placement — applies §3.5; one shared MATH function with Assembly, not two copies | 37 lines |
| §19.9 | [ARCH_19_09_ManualOnlyMembership.md](ARCH_19_09_ManualOnlyMembership.md) | Manual-layer-only membership confirmed consistent with Assembly's own §0 ruling, not a drifting variant | 26 lines |
| §19.10 | [ARCH_19_10_TabDrivenV1Scoping.md](ARCH_19_10_TabDrivenV1Scoping.md) | v1 Move/Rotate is tab-driven only — no new canvas gesture, deferred until Assembly's own canvas work ships | 28 lines |
| §19.11 | [ARCH_19_11_FormatSpecCorrectionBundle.md](ARCH_19_11_FormatSpecCorrectionBundle.md) | `SANMAP_FORMAT_SPEC.md` staleness correction bundle — landed directly in that file this session | 42 lines |
| §19.12 | [ARCH_19_12_SoftTypeConsistency.md](ARCH_19_12_SoftTypeConsistency.md) | Bundle→marker-type consistency stays soft (UI-enforced only) — no import-time hard validation | 28 lines |
| §19.13 | [ARCH_19_13_MarkerRuleLayerTypeName.md](ARCH_19_13_MarkerRuleLayerTypeName.md) | `markerTypeName` on `MarkerRuleLayer`/`MarkerInstanceLayer` — additive, wire key `"MarkerTypeName"`, extends §19.3 | 21 lines |
| §19.14 | [ARCH_19_14_TypeSectionUiDerived.md](ARCH_19_14_TypeSectionUiDerived.md) | The Type-section tier is UI-derived — dynamic enumeration over `markerTypeName`, not a stored `Params` container; ordering rule | 26 lines |
| §19.15 | [ARCH_19_15_TypeSectionTreeComposition.md](ARCH_19_15_TypeSectionTreeComposition.md) | Type-section × Bundle-tree composition — filtered-copy `TreeListWidget_UI` per type, the cross-Type-section nesting cutoff, `bRowSuppressed`'s two-predicate composition | 52 lines |
| §19.16 | [ARCH_19_16_InstanceIdentifier.md](ARCH_19_16_InstanceIdentifier.md) | `MarkerTransform::instanceIdentifier` — global uniqueness, wire key `"InstanceIdentifier"`, legacy-backfill mirrors `layerId`'s precedent | 37 lines |
| §19.17 | [ARCH_19_17_SelectColorFields.md](ARCH_19_17_SelectColorFields.md) | `GlobalMarkerSettings` select-color fields — strict 3-field mirror plus the signed-off `selectColorDefault` deviation | 36 lines |
| §19.18 | [ARCH_19_18_SelectionTintPriorityAndVisualLanguage.md](ARCH_19_18_SelectionTintPriorityAndVisualLanguage.md) | Selection tint — canonical priority order; "selected replaces fill" distinct from the drag-ghost's unfilled-ring vocabulary; **amended by §21.5** (locked-item exclusion) | 40 lines |
| §19.19 | [ARCH_19_19_StaticHighlightComputationAndWiring.md](ARCH_19_19_StaticHighlightComputationAndWiring.md) | Static selection-highlight — one-shot orbit computation (not `MarkerOrbitCorrespondence_UI.h`), tolerance reuse, canvas wiring | 57 lines |
| §19.20 | [ARCH_19_20_ManualOnlySelectionScope.md](ARCH_19_20_ManualOnlySelectionScope.md) | Manual-only selection scope — formal law, cross-referencing §19.9; **narrowed by §19.25/§19.27** | 20 lines → revised |
| §19.21 | [ARCH_19_21_CategoryVsMarkerTypeNameClosed.md](ARCH_19_21_CategoryVsMarkerTypeNameClosed.md) | `MarkerRule::category` vs. `markerTypeName` — two permanently independent concepts, closed | 17 lines |
| §19.22 | [ARCH_19_22_ManualLayersHeaderSplit.md](ARCH_19_22_ManualLayersHeaderSplit.md) | File-size ceiling remediation — `MarkersTab_ManualLayers_UI.h` split, resolved ahead of Ticket B | 67 lines |
| §19.23 | [ARCH_19_23_TreeListHeaderExtraContract.md](ARCH_19_23_TreeListHeaderExtraContract.md) | `TreeListWidget_UI<T,LeafKeyT>::Render` header-extra contract — two callbacks (Node vs. Leaf), a deliberate divergence from `DraggableList`'s single-callback shape | 45 lines |
| §19.24 | [ARCH_19_24_SymmetryEnabledField.md](ARCH_19_24_SymmetryEnabledField.md) | `Params::MarkerInstanceLayer::bSymmetryEnabled` — new field, mirrors `bColorOverrideEnabled`'s shape, wire key `"SymmetryEnabled"` | 33 lines |
| §19.25 | [ARCH_19_25_SelectionRepresentationUnification.md](ARCH_19_25_SelectionRepresentationUnification.md) | Canvas/list selection unification — `OverlayInstanceKey_UI::bManual`, `MapCanvas`'s widened selection surface, the shell-mediated tab↔canvas callback; corrects and narrows §19.20 | 51 lines |
| §19.26 | [ARCH_19_26_ManualInstanceSymmetryGrouping.md](ARCH_19_26_ManualInstanceSymmetryGrouping.md) | Manual-instance symmetry-cluster grouping in the instance list — UI composition only, no PARAMS change | 21 lines |
| §19.27 | [ARCH_19_27_ProceduralInstanceSelectionMechanism.md](ARCH_19_27_ProceduralInstanceSelectionMechanism.md) | Procedural marker-instance listing/selection — per-frame `ruleIndex` positional index, convergence with §19.25, bucket-size symmetry-grouping rule; narrows §19.20 | 39 lines |
| **§20** | [ARCH_20_PropsDecalsAuthoringParity.md](ARCH_20_PropsDecalsAuthoringParity.md) | Props/Decals authoring parity with Markers — `RuleLayer`/`InstanceLayer` field parity, the `LayerBundle` tree, Type Sections (ARCH ruling; §20.4's gate is **closed by §21**, §20.5 item 3 remains gated on a not-yet-done IO Architecture Expert consult) | index → 8 subsection files |
| §20.1 | [ARCH_20_01_ParamsGenericitySplit.md](ARCH_20_01_ParamsGenericitySplit.md) | `PropRuleLayer`/`DecalRuleLayer`/`PropLayerBundle`/`DecalLayerBundle` — hand-written per domain, not templated; new file homes | 51 lines |
| §20.2 | [ARCH_20_02_ConsumingLogicPlacement.md](ARCH_20_02_ConsumingLogicPlacement.md) | Grid-snap / effective-symmetry resolvers — duplicated per domain, in PARAMS; a Marker-side placement finding | 28 lines |
| §20.3 | [ARCH_20_03_GlobalPropDecalSettings.md](ARCH_20_03_GlobalPropDecalSettings.md) | `GlobalPropSettings`/`GlobalDecalSettings` — scoped to what has a real analog, not a blind mirror | 36 lines |
| §20.4 | [ARCH_20_04_DragGestureSubstrateRouting.md](ARCH_20_04_DragGestureSubstrateRouting.md) | Drag-gesture/selection substrate — routing record; **gate closed by §21** | 42 lines + closing note |
| §20.5 | [ARCH_20_05_RuleLayerMigrationRouting.md](ARCH_20_05_RuleLayerMigrationRouting.md) | IO — additive parts confirmed no-bump; `RuleLayer` wrapping tier **gated on an IO Architecture Expert consult, not yet done** | 42 lines |
| §20.6 | [ARCH_20_06_TypeSectionReuse.md](ARCH_20_06_TypeSectionReuse.md) | Type Sections — reuse §19.14's mechanism verbatim; field named per domain (`propTypeName`, never `markerTypeName` on a Prop/Decal struct; Decals gets no field at all) | 30 lines |
| §20.7 | [ARCH_20_07_Housekeeping.md](ARCH_20_07_Housekeeping.md) | Naming / file-size / `MapRecipe` flatness housekeeping | 22 lines |
| §20.8 | [ARCH_20_08_DecalsTopLevelTab.md](ARCH_20_08_DecalsTopLevelTab.md) | Decals is a standalone top-level tab — ratifies the already-shipped split (STEP159), closes a dangling forward-reference | 27 lines |
| **§21** | [ARCH_21_CanvasInteractionUnification.md](ARCH_21_CanvasInteractionUnification.md) | Canvas interaction unification — multi-select, drag-gesture genericization, uniform locked-item exclusion, shared picking substrate, Area authoring on the canvas (ARCH ruling, closes §20.4's gate) | index → 8 subsection files |
| §21.1 | [ARCH_21_01_MultiSelectRepresentation.md](ARCH_21_01_MultiSelectRepresentation.md) | Multi-select representation — `OverlayInstanceKeySet_UI`, the widened `MapCanvas` selection surface, the callback signature | 69 lines |
| §21.2 | [ARCH_21_02_GestureOwnership.md](ARCH_21_02_GestureOwnership.md) | Gesture ownership — press-time drag-begin-first, release-time click/marquee, the independent right-button pan | 49 lines |
| §21.3 | [ARCH_21_03_DragGestureGenericization.md](ARCH_21_03_DragGestureGenericization.md) | Drag-gesture genericization — `InstanceDragGestureState`, `Begin/Update/EndInstanceDragGesture<Traits>`, `MarkerDragTraits`/`PropDragTraits`/`DecalDragTraits`, `HitTestManualInstances<GroupT>`/`CollectManualInstancesInWorldRegion<GroupT>` | 122 lines |
| §21.4 | [ARCH_21_04_PropDecalInstanceIdentityFields.md](ARCH_21_04_PropDecalInstanceIdentityFields.md) | `PropTransform`/`DecalTransform` gain `instanceIdentifier`/`symmetryGroupIdentifier` — mirrors `MarkerTransform` verbatim | 49 lines |
| §21.5 | [ARCH_21_05_LockedItemExclusionCorrection.md](ARCH_21_05_LockedItemExclusionCorrection.md) | Locked-item exclusion — corrects §19.18; uniform across click/marquee/drag; procedural instances unaffected | 39 lines |
| §21.6 | [ARCH_21_06_PickingInfrastructure.md](ARCH_21_06_PickingInfrastructure.md) | Picking infrastructure — `Data::SpatialGridSet`, `BuildSpatialGridSet`, three new `SpatialGrid` accessors, `PickInstancesInRegion` (renamed from the design's `PickMarkersInRegion`) | 64 lines |
| §21.7 | [ARCH_21_07_FileSizeCeilingFlag.md](ARCH_21_07_FileSizeCeilingFlag.md) | File-size ceiling flag — `MapCanvas_UI.h` | 15 lines |
| §21.8 | [ARCH_21_08_AreaCanvasGesture.md](ARCH_21_08_AreaCanvasGesture.md) | Area canvas gesture — create-by-drag, 8-handle resize + body-move for `Params::MapArea`, its own hand-written (non-`Traits`) substrate; independently dispatchable, not part of §21.1-§21.7's interlocking mechanism | 320 lines |
| **§22** | [ARCH_22_NavmapModifierBlockers.md](ARCH_22_NavmapModifierBlockers.md) | Navmap Modifier blockers — formalizes a hand-authoring technique proven live twice on Pandemonium Isthmus (ARCH ruling, ratifies `NAVMAP_MODIFIER_BLOCKER_SPEC.md`); not yet a SanGen `PARAMS`/`IO`/`UI` construct (§22.9) | index → 9 subsection files |
| §22.1 | [ARCH_22_01_LayerClassification.md](ARCH_22_01_LayerClassification.md) | Layer classification — game-side Lua, not SanGen C++ (mirrors §15.1) | 14 lines |
| §22.2 | [ARCH_22_02_NativePrimitiveGroundTruth.md](ARCH_22_02_NativePrimitiveGroundTruth.md) | The native `NavmapModifierTemplate` primitive — recorded ground truth, binding on both techniques | 26 lines |
| §22.3 | [ARCH_22_03_AllLayerBlockerTechnique.md](ARCH_22_03_AllLayerBlockerTechnique.md) | All-layer blocker via the global `PlayableAreaBarrier` prefab — ratified, confirmed shipped | 21 lines |
| §22.4 | [ARCH_22_04_PartialLayerBlockerTechnique.md](ARCH_22_04_PartialLayerBlockerTechnique.md) | Partial/single-layer blocker requires a purpose-built prefab — ⚠️ designed, not shipped; promotion-to-shared-helper guidance | 23 lines |
| §22.5 | [ARCH_22_05_PerLuaStateExecutionNuance.md](ARCH_22_05_PerLuaStateExecutionNuance.md) | Per-Lua-state execution nuance — distinct from `MAP_UNIT_SPAWNING_SPEC` §2's `Import`-cache double-execution hazard | 19 lines |
| §22.6 | [ARCH_22_06_NewThreadOrderingLaw.md](ARCH_22_06_NewThreadOrderingLaw.md) | Ordering law inside the shared `NewThread` — extends, does not replace, existing ordering law; **binding `pcall`-per-call rule** — ordering reduces the chance of a failure, `pcall` removes the consequence, and only one ordering constraint (playable area final before instantiation) survives as load-bearing once every call is wrapped | 38 lines |
| §22.7 | [ARCH_22_07_MaskToRectangleWorkflow.md](ARCH_22_07_MaskToRectangleWorkflow.md) | Mask-to-rectangle authoring workflow — recorded as the current manual (non-SanGen) process | 14 lines |
| §22.8 | [ARCH_22_08_CoordinateConventionDistinction.md](ARCH_22_08_CoordinateConventionDistinction.md) | Pixel↔world coordinate convention for mask-derived rectangles — distinct from the `.sanmap` entity-position convention | 16 lines |
| §22.9 | [ARCH_22_09_OwnershipScopeRuling.md](ARCH_22_09_OwnershipScopeRuling.md) | Ownership/scope ruling — not yet a SanGen-owned `PARAMS`/`IO`/`UI` construct; future-direction note only | 17 lines |

### Oversized files — known, accepted

Files exceeding the ~100-line target are listed below. Six are single indivisible
rulings with no internal subsection structure to split on; splitting them further
would break the `§N.M` ↔ file rule for no gain. The seventh, `ARCH_03_ModuleBoundaries.md`,
is §3 itself — a section this pack has never split into per-subsection files (unlike
§1/§7/§14/§15/§16/§18) — accepting one additional general rule (§3.5) appended in place
rather than forking that established non-split pattern for a single new subsection. The
eighth, `ARCH_21_03_DragGestureGenericization.md`, is one interlocking ruling (the
`Traits` contract, the state-struct de-templating refinement, and the name-field
refinement all depend on each other for context) — splitting it would scatter a single
coherent correction across files with no citation gain. The ninth, `ARCH_21_08_AreaCanvasGesture.md`,
is by far the largest: a whole new, self-contained canvas-authoring subsystem (its own
gesture substrate, injected-pointer setter, press/release dispatch wiring, draw pass, and
a full page of explicit open-question rulings a coder work-order needs with no ambiguity
left) that the human's own request scoped as ONE new subsection file, not a further
`§21.8.N` breakdown this pack's numbering scheme does not otherwise use — kept whole
rather than invent a nesting depth no other section has.

| File | Lines | Why it stays whole |
|------|-------|--------------------|
| `ARCH_03_ModuleBoundaries.md` | 126 | §3.5 (the general MATH/PARAMS/PROC placement rule) appended to the section's existing non-split file rather than forking a new subsection-file pattern this section has never used |
| `ARCH_07_02_MaterialProportionVsSurfaceWeight.md` | 162 | One ratified ruling on the two-field distinction |
| `ARCH_14_13_OpenItems.md` | 137 | A flat numbered list of open items from the §14 ratification |
| `ARCH_15_10_SlotPatternConstructionMoves.md` | 119 | One ruling: slot-pattern construction moves into the runtime |
| `ARCH_15_05_ParamsScenariosType.md` | 154 | One ruling: the full `Params::Scenarios` shape, plus the 2026-08-28 naval-fleet retirement note and two explicitly-flagged OPEN items — kept in one file since all three depend on the same shape for context |
| `ARCH_14_16_PerArmyUnitsOverlayRows.md` | 114 | One ruling: per-army row seeding + procedural-unit routing + tint source + default-palette prerequisite, deliberately kept together as one shippable unit |
| `ARCH_21_03_DragGestureGenericization.md` | 122 | One ruling: the `Traits` contract plus its two load-bearing refinements (state de-templating, the missing-`name`-field fix) |
| `ARCH_14_15_ManualCullStableIdMigration.md` | 106 | One ruling: the cull-path stable-id migration |
| `ARCH_21_08_AreaCanvasGesture.md` | 320 | One whole, self-contained canvas-authoring subsystem — substrate, setter, dispatch wiring, draw pass, and every open-question ruling a coder needs, deliberately kept together as one shippable unit rather than forking a new `§N.M.K` nesting depth |

---

## Related law

- `sangen_arch_pack/CONSTITUTION.md` — always-loaded law; this document resolves its **(TBD)** items.
- `sangen_arch_pack/INDEX.md` — the per-module specs, loaded on demand.
