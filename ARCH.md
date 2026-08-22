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
the four that exceed it are single indivisible rulings, flagged in the table below.

Section numbers are permanent and never renumbered, so every `§N` / `§N.M` citation
across the specs, work-orders, and agent charters keeps resolving forever.

**Load only what you need** — one subsection file, not a section, and never the whole
ARCH. This is the same on-demand discipline `sangen_arch_pack/INDEX.md` applies to specs.

A new ratification appends a **new file** (the next unused top-level number, e.g.
`ARCH_19_*.md`, split into `ARCH_19_NN_*.md` if it runs long) plus its rows below.
Existing files are amended in place. **Only the ARCH Expert writes any of them.**

---

## Section index

| § | File | Ruling | Size |
|---|------|--------|------|
| **§1** | [ARCH_01_NamingLaw.md](ARCH_01_NamingLaw.md) | Naming law (Constitution §2, resolved) | index → 8 subsection files |
| §1.1 | [ARCH_01_01_LiteralNames.md](ARCH_01_01_LiteralNames.md) | Literal, fully-spelled names — no abbreviations | 20 lines |
| §1.2 | [ARCH_01_02_LayerTagSuffix.md](ARCH_01_02_LayerTagSuffix.md) | Layer tag is a SUFFIX (TGUE convention) | 19 lines |
| §1.3 | [ARCH_01_03_MathNaming.md](ARCH_01_03_MathNaming.md) | Math naming — domain + optimization variant | 11 lines |
| §1.4 | [ARCH_01_04_CpuGpuKernelPairing.md](ARCH_01_04_CpuGpuKernelPairing.md) | CPU / GPU kernels pair by shared base name | 11 lines |
| §1.5 | [ARCH_01_05_FileSizeCeilings.md](ARCH_01_05_FileSizeCeilings.md) | File-size ceilings | 12 lines |
| §1.6 | [ARCH_01_06_SanmapKeyCasing.md](ARCH_01_06_SanmapKeyCasing.md) | `.sanmap` top-level key casing — game-native vs SanGen-owned (ratifies work-order SPEC-4 Correction 0) | 24 lines |
| §1.7 | [ARCH_01_07_IoMigrationFileNaming.md](ARCH_01_07_IoMigrationFileNaming.md) | IO migration file naming — schema version steps (ratifies `IO_MIGRATION_SPEC`) | 16 lines |
| §1.8 | [ARCH_01_08_ParamsFieldNamingByKind.md](ARCH_01_08_ParamsFieldNamingByKind.md) | PARAMS field naming for format-derived types — governed by data KIND, not by key presence | 43 lines |
| **§2** | [ARCH_02_LayerDirectoryMap.md](ARCH_02_LayerDirectoryMap.md) | Layer → directory map (Constitution §1/§2) | 24 lines |
| **§3** | [ARCH_03_ModuleBoundaries.md](ARCH_03_ModuleBoundaries.md) | Module boundaries & ownership (Constitution §1, resolved) | 78 lines |
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
| **§14** | [ARCH_14_PreviewOverlayLayering.md](ARCH_14_PreviewOverlayLayering.md) | Preview overlay layering — six-domain screen-space compositor (ARCH ruling, ratifies `work_orders/DESIGN_MarkerPreviewLayering_R2.md`) | index → 13 subsection files |
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
| **§15** | [ARCH_15_MapScenarioSystem.md](ARCH_15_MapScenarioSystem.md) | The SanGen Map Scenario system — formalized as first-class law (ratifies `MAP_SCENARIO_SPEC.md`) | index → 10 subsection files |
| §15.1 | [ARCH_15_01_LayerClassification.md](ARCH_15_01_LayerClassification.md) | Layer classification | 13 lines |
| §15.2 | [ARCH_15_02_IoScopeRuling.md](ARCH_15_02_IoScopeRuling.md) | IO scope ruling — corrects an earlier assumption, does not reverse it | 39 lines |
| §15.3 | [ARCH_15_03_ExportOnlyLuaRatified.md](ARCH_15_03_ExportOnlyLuaRatified.md) | Design ratified: option (c) — export-only, SanGen never parses Lua back (resolves §15.2's open question / `MAP_SCENARIO_SPEC.md` §8) | 29 lines |
| §15.4 | [ARCH_15_04_ThreeFileOnDiskShape.md](ARCH_15_04_ThreeFileOnDiskShape.md) | Three-file on-disk shape + overwrite safety (ratifies `MAP_SCENARIO_SPEC.md` §2/§2.1/§2.2) | 37 lines |
| §15.5 | [ARCH_15_05_ParamsScenariosType.md](ARCH_15_05_ParamsScenariosType.md) | `Params::Scenarios` — the new PARAMS type (shape ruling) | 118 lines |
| §15.6 | [ARCH_15_06_CountScenariosOrdering.md](ARCH_15_06_CountScenariosOrdering.md) | `COUNT_SCENARIOS` ordering — array order IS the match-priority authoring action | 18 lines |
| §15.7 | [ARCH_15_07_OwnershipSplit.md](ARCH_15_07_OwnershipSplit.md) | Ownership split — who ratifies what for the new `Params::Scenarios` family | 22 lines |
| §15.8 | [ARCH_15_08_ThirdPartyDependencyRuling.md](ARCH_15_08_ThirdPartyDependencyRuling.md) | Third-party dependency ruling — ImGuiColorTextEdit + embedded LuaJIT | 54 lines |
| §15.9 | [ARCH_15_09_EngineWhitelistMigrationPath.md](ARCH_15_09_EngineWhitelistMigrationPath.md) | Engine-whitelist migration path (recorded as intended future simplification, not built) | 19 lines |
| §15.10 | [ARCH_15_10_SlotPatternConstructionMoves.md](ARCH_15_10_SlotPatternConstructionMoves.md) | Slot-pattern construction moves into the runtime; `maxArmySlotCount` becomes authored data (ratifies the human's construction-code-belongs-in-universal-mod-code decision; amends `MAP_SCENARIO_SPEC.md` §2/§3/§4) | 119 lines |
| **§16** | [ARCH_16_MarkerLayerSymmetry.md](ARCH_16_MarkerLayerSymmetry.md) | Marker layer-scoped symmetry — `MarkerRuleLayer` / `MarkerInstanceLayer` / `SymmetrySetting` (ARCH ruling, ratifies `work_orders/DESIGN_MarkerLayerSymmetry_R1.md` + `_R2.md`) | index → 10 subsection files |
| §16.1 | [ARCH_16_01_NewParamsShapes.md](ARCH_16_01_NewParamsShapes.md) | New PARAMS shapes — ratified, with §16.5's naming amendment folded in | 68 lines |
| §16.2 | [ARCH_16_02_MarkerRuleLayersKeepsName.md](ARCH_16_02_MarkerRuleLayersKeepsName.md) | `markerRuleLayers` keeps its full name; does not shorten to `markerLayers` like `propLayers` did | 11 lines |
| §16.3 | [ARCH_16_03_ModuleBoundaryChain.md](ARCH_16_03_ModuleBoundaryChain.md) | Module boundary — solved via the existing legal `UI → PIPELINE → PROC` chain; no MATH relocation, no new UI→PROC exception | 68 lines |
| §16.4 | [ARCH_16_04_SanmapCorrection7Amendment.md](ARCH_16_04_SanmapCorrection7Amendment.md) | `SANMAP_FORMAT_SPEC` Correction 7 amendment — scoped-down one-tier shape accepted; exact key spelling deferred to the Format Expert | 28 lines |
| §16.5 | [ARCH_16_05_MarkerTransformFields.md](ARCH_16_05_MarkerTransformFields.md) | `MarkerTransform` — `symmetryGroupIdentifier` (not `symmetryGroupId`) + already-ratified `layerIndex` | 39 lines |
| §16.6 | [ARCH_16_06_MigrationRouting.md](ARCH_16_06_MigrationRouting.md) | Migration — a real breaking schema change; routed to the IO Architecture Expert, not ruled on here | 20 lines |
| §16.7 | [ARCH_16_07_NamingConfirmed.md](ARCH_16_07_NamingConfirmed.md) | Naming confirmed — `MarkerRuleLayer` / `MarkerInstanceLayer` | 14 lines |
| §16.8 | [ARCH_16_08_SpawnArmyShrink.md](ARCH_16_08_SpawnArmyShrink.md) | Spawn/Army shrink — confirmed: no new PARAMS type or field needed; corrected wording — match key is `MarkerTransform::name` only, never `alias` | 24 lines |
| §16.9 | [ARCH_16_09_NonArchItems.md](ARCH_16_09_NonArchItems.md) | Non-ARCH items — confirmed out of scope for this ratification | 9 lines |
| §16.10 | [ARCH_16_10_ConsultRoutingSummary.md](ARCH_16_10_ConsultRoutingSummary.md) | Consult routing summary — what remains before a coder work-order can be written; item 3 corrected — two PROC consumers, not one, per `STEP79_MarkerRuleLayerProcConsumer_PROC.md` | 42 lines |
| **§17** | [ARCH_17_MigrationValuesRegistry.md](ARCH_17_MigrationValuesRegistry.md) | `bLosslessIfSkipped` values for the 9 shipped `SanGenVersion` 2→3 migrations (ARCH ruling, ratifies `IO_MIGRATION_SPEC.md` §3, backfills `STEP26A_MigrationLosslessFlagAndPreview_IO.md`) | 66 lines |
| **§18** | [ARCH_18_SantpFootprintIngestion.md](ARCH_18_SantpFootprintIngestion.md) | `.santp`/`.sanprop` template ingestion — sandboxed execution primitive + determinism ruling (responds to `DESIGN_SantpFootprintIngestion_R1.md`) | index → 2 subsection files |
| §18.1 | [ARCH_18_01_SandboxedExecutionPrimitive.md](ARCH_18_01_SandboxedExecutionPrimitive.md) | `LuaTableEvaluate_SYS`/`LuaTableValue_SYS` — a sibling `SYS` primitive to `LuaSyntaxCheck_SYS`, sharing only the vendored LuaJIT library; the execution safety contract, binding | 59 lines |
| §18.2 | [ARCH_18_02_IngestedDataDeterminism.md](ARCH_18_02_IngestedDataDeterminism.md) | Determinism ruling — ingested footprint data may influence generation only after being baked into `PARAMS`; never read live by `PROC` | 69 lines |

### Oversized files — known, accepted

Four subsection files exceed the ~100-line target. Each is a single indivisible ruling
with no internal subsection structure to split on; splitting them further would break the
`§N.M` ↔ file rule for no gain.

| File | Lines | Why it stays whole |
|------|-------|--------------------|
| `ARCH_07_02_MaterialProportionVsSurfaceWeight.md` | 162 | One ratified ruling on the two-field distinction |
| `ARCH_14_13_OpenItems.md` | 137 | A flat numbered list of open items from the §14 ratification |
| `ARCH_15_10_SlotPatternConstructionMoves.md` | 119 | One ruling: slot-pattern construction moves into the runtime |
| `ARCH_15_05_ParamsScenariosType.md` | 118 | One ruling: the full `Params::Scenarios` shape |

---

## Related law

- `sangen_arch_pack/CONSTITUTION.md` — always-loaded law; this document resolves its **(TBD)** items.
- `sangen_arch_pack/INDEX.md` — the per-module specs, loaded on demand.
