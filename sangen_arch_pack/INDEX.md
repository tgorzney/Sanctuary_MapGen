# SanGen Pack Index (manifest)

Maps a topic to the spec(s) to load. Read this first, then load ONLY the
spec(s) a question needs — never the whole pack.

| Topic | Spec(s) |
| --- | --- |
| .sanmap file format, import/export, coordinate flip, schema v3 top-level sections | `specs/SANMAP_FORMAT_SPEC.md` |
| .sanmap schema version migrations — SanGenVersion gating, the migration runner/manifest, JSON transform primitives | `specs/IO_MIGRATION_SPEC.md` |
| units / props / markers, tpId scheme, factions, asset validation, .san* formats | `specs/UNIT_PROP_MARKER_DATA_SPEC.md` |
| map scripting & events, lua sandbox, Tags, AI system, modding, validators | `specs/MODDING_SCRIPTING_SPEC.md` |
| the Map Scenario system — `<MapName>_data.lua`/`<MapName>_Scenarios_Runtime.lua`/`<MapName>_Scenarios_Data.lua` three-file split, module API contract, three-tier scenario matching, `alloyMode` semantics, the mandatory-`spawns` hard requirement, execution/timing law, the ratified export-only IO design (`Params::Scenarios`, overwrite safety, ARCH §15) | `specs/MAP_SCENARIO_SPEC.md` |
| data model (GenerationParams), generation pipeline, GPU toggles, enums | `specs/PARAMS_PIPELINE_SPEC.md` |
| height/material layers, GeoLayers, sim layers, thickness model, baking, stratum masks | `specs/LAYER_SYSTEM_SPEC.md` |
| erosion (hydraulic droplet), thermal/talus, flow/accumulation, CPU-vs-GPU parity | `specs/SIM_ALGORITHMS_SPEC.md` |
| performance review — hardware-math (SIMD/FMA/reciprocal/LUT) & memory (Morton/SoA/FP16) gaps | `specs/OPTIMIZATION_REVIEW.md` |
| optimization pillars — the realized SoA/AoSoA/SIMD/tiling/GPU technique law | `specs/OPTIMIZATION_PILLARS.md` |
| cross-machine deterministic generation (competitive shared-gen from settings+seed) | `specs/DETERMINISM_SPEC.md` |
| UI framework — imgui-bypass, universal widgets, 100k-entity lists/preview, picking, dirty flags | `specs/UI_FRAMEWORK_SPEC.md` |
| asset loading — single-pass sanpack ingestion, icon atlases, on-disk icon cache | `specs/ASSET_LOADING_SPEC.md` |
| gamedata layout — folder map (units/props/stratum/icons), sprite pairs, sizes | `specs/GAMEDATA_LAYOUT_SPEC.md` |
| noise generation (FastNoiseLite types/fractals) + heightfield blend modes, layer cache | `specs/NOISE_BLEND_SPEC.md` |
| the Mask stage — slope gate, stored-art merge, `materialProportions` vs `surfaceStratumWeights` | `specs/MASKING_SPEC.md` |
| marker/prop/unit scatter, rules & gates, symmetry (incl. Radial N-fold, ARCH §13; the new layer-scoped `SymmetrySetting`/`MarkerRuleLayer`, ARCH §16, `SANMAP_FORMAT_SPEC` Correction 15), prop SoA, scatter determinism, global marker icon/color/scale defaults | `specs/PLACEMENT_SCATTER_SPEC.md` |
| pass-through entity PARAMS — armies/unit groups/unit transforms/map areas AND resolved/baked markers/props/decals/marker chains, incl. manual prop/decal/marker layer authoring (`Params::Army`, `UnitGroup`, `UnitTransform`, `MapArea`, `InstancedTransform`, `MarkerInstanceGroup`, `MarkerTransform`, `PropInstanceGroup`, `PropTransform`, `DecalInstanceGroup`, `DecalTransform`, `PropInstanceLayer`, `DecalInstanceLayer`, `MarkerInstanceLayer`, `MarkerChain`, `ChainMarker`), distinct from procedural scatter rules; also the ratified export-time `blueprintPath` "warn, never block" ruling | `specs/ENTITY_AUTHORING_PARAMS_SPEC.md` |
| `Params::Atmosphere` — sun/skylight/exposure-skybox/fog(×3)/wind recipe settings, promoted from the field-complete UI-only `Ui::AtmosphereSettings` | `specs/ATMOSPHERE_PARAMS_SPEC.md` |
| the canonical CPU/GPU dispatch contract — kernel/backend/policy/resource-manager | `specs/DISPATCH_INTERFACE_SPEC.md` |
| preview compositing — passes, coloring, picking, dirty flags, the shadow-sim fix; the ratified v2 screen-space overlay-layering design (six domains — Alloy/SpawnsArmies/Units/Props/Reclaim/Decals; LOD icon rendering; four dirty-flag tiers A/B/C/C2; the View toolbar's two-section popup; ARCH §14) | `specs/PREVIEW_COMPOSITING_SPEC.md` |
| core math library — SIMD/fast-math/Morton/spatial internals (stub reality + v2 target) | `specs/MATH_SIMD_SPEC.md` |
| future sim passes — fluvial/glacial/snow-melt design on the shared sim framework | `specs/FUTURE_SIM_TYPES_SPEC.md` |
| map AI-analyzability invariants + host/client shared-generation protocol | `specs/AI_HOSTCLIENT_SPEC.md` |

All planned specs are now written. The pack covers the full pipeline end-to-end; the
remaining depth is deep-read follow-ups noted inside individual specs (AI/host/client
lua internals in `AI_HOSTCLIENT_SPEC`; open verification items flagged per spec).
`ENTITY_AUTHORING_PARAMS_SPEC` was added later, ratifying the `Params::Army`/`MapArea`
type family the original pack left as a named gap. `ATMOSPHERE_PARAMS_SPEC` was added
later still, promoting the field-complete UI-only `Ui::AtmosphereSettings` to a real
`Params::Atmosphere` recipe type; the same ratification session also filled in the
`Params::GlobalMarkerSettings` C++ shape inside `SANMAP_FORMAT_SPEC`'s existing
`GlobalMarkerSettings` paragraph (no new spec file needed — the shape was already fully
named there). `ENTITY_AUTHORING_PARAMS_SPEC` was extended again in a third session to
ratify the remaining resolved/baked entity domains — `markers`/`props`/`decals`/`chains`
(`Params::MarkerInstanceGroup`/`MarkerTransform`, `PropInstanceGroup`, `DecalInstanceGroup`,
`MarkerChain`/`ChainMarker`, and the new shared `Params::InstancedTransform` base) — closing
the last named pass-through-instance-data gap in that family. `ENTITY_AUTHORING_PARAMS_SPEC`
was extended a fourth time (ARCH §12) to ratify manual-layer authoring for hand-placed
props/decals — `PropTransform`/`DecalTransform::layerIndex` (direct field injection) plus
the separate `PropInstanceLayer`/`DecalInstanceLayer` metadata arrays (`SANMAP_FORMAT_SPEC`
Correction 14, new `PropGroups`/`DecalGroups` top-level keys) — which superseded that spec's
earlier "props/decals need no wrapper transform type" ruling now that `layerIndex` is real
per-instance data. The same session (ARCH §13) added Radial N-fold heightmap/entity symmetry
(`SymmetryAxis::Radial`, `radialSymmetryRepeatCount`) to `SANMAP_FORMAT_SPEC` Correction 4 and
corrected that correction's prior claim that `DecalRule` already carries the
`bSymmetryUseGlobal`/`symmetryMask` override pair (at that time it did not — recorded as
Defect 1 in `PLACEMENT_SCATTER_SPEC`'s "Known issues" addendum, alongside a second recorded
defect: the 16-slot symmetry-orbit buffer can now silently overflow under a large radial
count). **Defect 1 has since shipped**, fixed by a later coder work-order — `DecalRule` now
carries the pair and `AppendDecalRules` resolves a symmetry mask for decals, confirmed by
reading `src/params/ScatterRule_PARAMS.h` and `src/proc/Placement_Rules_PROC.cpp` (see the
"Standing recorded defects" note below); the 16-slot orbit-overflow defect remains open.

`ENTITY_AUTHORING_PARAMS_SPEC` was extended a fifth time to close its own flagged item 1
(export-time `blueprintPath` validation) with a human ruling: an unresolvable `blueprintPath`
is reported — via the new IO-layer `ValidatePropAndDecalBlueprintPaths` check
(`MapExporter_IO.h`) and a `ConfirmDialog_UI` warning dialog naming the runtime risk — never
silently dropped and never silently used to hard-refuse the export; the designer sees the
warning and can choose to proceed anyway. This resolves the item's original "resolve
literally against the real pack or fail loudly (Constitution §6)" ambiguity in favor of
fail-loudly meaning "surfaced loudly to the human," not "hard-refused by the tool." Items 2-4
of that same flagged list remain open. Implemented by `work_orders/STEP4_PropsDecals_IO.md`
and `work_orders/STEP5_PropsDecalsValidation_UI.md`. The same ruling adds `ConfirmDialog_UI` —
a new generic, reusable OK/Cancel confirm-modal widget with no prior equivalent — to
`UI_FRAMEWORK_SPEC.md`'s "Universal widget library".

`MAP_SCENARIO_SPEC.md` was added later still, formalizing the now-deployed (confirmed live
in-game 2026-08-20) SanGen Map Scenario system as first-class law: the original
`<MapName>_data.lua`/`<MapName>_Scenarios_Script.lua` two-file split, the `Scenario.
ResolveAndApply`/`Scenario.SpawnNavalFleets` module contract, the three-tier
(`PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO`) matching system, the four
`alloyMode` values, the hard requirement that every scenario needing deterministic spawns
declares an explicit `spawns` table (the `.sanmap`'s one-spawn-transform-per-army shared-state
failure mode discovered live the same day), the execution/timing law, and a ruling that SanGen
Import/Export of the scenario file remains in scope but is reclassified as a distinct IO
surface (a script-tree `.lua` companion file, not a `.sanmap`-package JSON section) — see
`ARCH_15_MapScenarioSystem.md` §15. This consolidated and superseded the "what to build" content previously inline
in `MODDING_SCRIPTING_SPEC.md`'s "Scenario-script file split" section, which now holds only
that section's investigation trail (the disproven cross-tree-`Import()` hypothesis).

**`ARCH_15_MapScenarioSystem.md` §15 was later extended (§15.3–§15.9), closing the forward-reference gap flagged
below the first time this paragraph was written** (that flag is now resolved and removed — see
"Fixed since" note below). The extension ratifies the design the human settled for SanGen's own
scenario-authoring/export architecture: design option (c) (SanGen owns scenario **data**, never
parses Lua to read it back — export-only); the per-map on-disk shape becomes **three** files
(`<MapName>_data.lua` hand-authored orchestrator — never written by SanGen; the SanGen-owned,
export-copied `<MapName>_Scenarios_Runtime.lua`; the SanGen-owned, export-regenerated
`<MapName>_Scenarios_Data.lua`), replacing the original two-file `_Scenarios_Script.lua` shape
(`MAP_SCENARIO_SPEC.md` §2, §2.1 overwrite safety, §2.2 legacy-map migration); a new
`Params::Scenarios`/`PatternScenario`/`CountScenario` PARAMS family (§15.5) with the
`COUNT_SCENARIOS` array's order ratified as the match-priority authoring action itself (§15.6);
and two new third-party dependencies — ImGuiColorTextEdit (the runtime-editor widget) and an
embedded LuaJIT library used **only** for compile-check validation (`load()`/`luaL_loadstring`,
never executed) — with a binding never-execute-untrusted-Lua constraint and a corresponding
correction to `ARCH_03_ModuleBoundaries.md` §3.1's dependency table (IO gains `SYS` as an allowed dependency,
formalizing a pre-existing real-code precedent) so both `UI` and `IO` can reach the validator
(§15.8). The engine-whitelist migration path (a future one-line `LoadMapData` change that would
let the runtime read scenario data straight from `GameInfo.MapData` and retire the generated
`.lua` data file) is recorded as an intended future simplification, not current law (§15.9).

`PREVIEW_COMPOSITING_SPEC.md` was extended with a new "Overlay layering (v2, ARCH §14)"
section, ratifying `work_orders/DESIGN_MarkerPreviewLayering_R2.md` (which itself supersedes
the earlier, narrower `DESIGN_MarkerPreviewLayering_R1.md` — historical only, not current):
the six-domain (Alloy/SpawnsArmies/Units/Props/Reclaim/Decals) screen-space overlay-layer
stack (`OverlayLayer_UI`/`OverlayDomainKind_UI`/`OverlaySubLayerRef_UI`), the two-mode
(thumbnail/strategic-icon) LOD rendering rule, the four-tier dirty-flag model (adding C —
screen-space redraw, and C2 — interaction-scoped redraw — on top of the existing two-tier A/B
GPU-recomposite model), the mandatory first-work-order performance requirements (bulk vertex
writes, cross-layer visible-vertex budget + decimation, atlas page bucketing), the View
toolbar's two-section/no-crossing popup replacing "Regenerate," and a separately-recorded GPU
color-texture readback defect. Full ruling text: `ARCH_14_PreviewOverlayLayering.md` §14. Several items are explicitly
**left open** by this ratification, not resolved (`ARCH_14_13_OpenItems.md` §14.13,
`PREVIEW_COMPOSITING_SPEC.md`'s matching list): real footprint-size data source; the
cross-layer budget default and Tier B per-resolution costs (both pending a real benchmark);
whether a stable id column exists for manual sub-layers; whether Decals actually route
through `Data::PlacementInstances` today; and whether `OverlayLayer_UI::blendMode` reuses
`Ui::PreviewBlendMode` or needs a new enum (UI Expert's call). **The Alloy/SpawnsArmies row's
"blocked — no `MarkerInstanceLayer` PARAMS type exists yet" note is now stale** — `ARCH_16_MarkerLayerSymmetry.md`
§16 ratifies that type; `ARCH_14_PreviewOverlayLayering.md` §14.2/§14.5 carry forward-pointers to §16, and
`PREVIEW_COMPOSITING_SPEC.md` itself still needs the same small update (not made in this
session — flagged here so it is not lost).

**Standing deferred ruling:** persistent ordered thickness columns + true surface-exposure
derivation (ARCH §7.5, `LAYER_SYSTEM_SPEC` "Known gap") — an M6 DATA-shape work order.
Do not patch it inside a mask or sim work-order.

**Fixed since ARCH §13:** `DecalRule` (`src/params/ScatterRule_PARAMS.h`) now carries the
`bSymmetryUseGlobal`/`symmetryMask` pair, and `AppendDecalRules` resolves a symmetry mask for
decals via `ResolveSymmetryMask` — see `SANMAP_FORMAT_SPEC` Correction 4 and
`PLACEMENT_SCATTER_SPEC` (Defect 1, now closed).

**Fixed since the ARCH §14 authoring pass:** that pass flagged the "see ARCH §15" forward
reference above as unresolved (`ARCH.md` ended at §14 at the time, with no §15 present at all).
`ARCH_15_MapScenarioSystem.md` §15 was written in a later session and has since been extended to §15.9 (the Map
Scenario authoring/export ratification described above) — the gap that flag named is closed.

**Standing recorded defects awaiting a coder work-order (not yet fixed):**
- `Params::symmetryOrbitMaximum = 16` (`src/params/Symmetry_PARAMS.h`) can silently overflow
  once a designer-chosen `radialSymmetryRepeatCount` combines with mirror axes — see
  `SANMAP_FORMAT_SPEC` Correction 4 and `PLACEMENT_SCATTER_SPEC` (Defect 2).
- `ComposeOnGpu()` (`PreviewComposite_Gpu_UI.cpp:78-81`) unconditionally reads back the full
  color texture even when nothing downstream consumes it on the GPU-resident hot path — up to
  256MB wasted PCIe transfer + blocking wait at the 8192² cap, every recompose. Narrow, already
  diagnosed, independent of the ARCH §14 overlay redesign; should land before it. See
  `PREVIEW_COMPOSITING_SPEC.md` / `ARCH_14_10_GpuColorReadbackBug.md` §14.10.

**Fixed since the §15.5 ratification (2026-08-21):** the naval-fleet composition gap flagged
below the first time this note was written is closed. The live reference
`SpawnNavalFleets(area)`'s fleet-composition parameters were read directly
(`Pandemonium Isthmus_Scenarios_Script.lua`) and shaped as `Params::ScenarioNavalFleet`
(`ARCH_15_05_ParamsScenariosType.md` §15.5, `MAP_SCENARIO_SPEC.md` §5.1) — per-scenario `fleet`/`pondSideByArmy`/
`sideBiasDistance`, with the algorithm's seven tuning constants (spiral search, grid-cell
bucketing, batch/give-up cadence) explicitly excluded as runtime-owned, never per-map PARAMS.
The same session also promoted `alloyMode`'s `Occupancy` default from placeholder to ratified
law (`ARCH_15_05_ParamsScenariosType.md` §15.5).

`ARCH_16_MarkerLayerSymmetry.md` §16 ratifies the UI Expert's two-round Markers Tab + layer-scoped symmetry consult
(`work_orders/DESIGN_MarkerLayerSymmetry_R1.md` + `_R2.md`): the new `Params::SymmetrySetting`
shared struct, `Params::MarkerRuleLayer` (wraps `MarkerRule`, which loses its own per-rule
symmetry triplet — a real breaking schema change, §16.6), and `Params::MarkerInstanceLayer`
(extends the earlier-recorded Gap 1 shape, `GAP_MarkerLayerAndSymmetry_PARAMS.md`, with a
symmetry field); `MapRecipe::markerRules` → `markerRuleLayers`, new `MapRecipe::markerLayers`;
`MarkerTransform` gains `symmetryGroupIdentifier` (named in full — NOT `symmetryGroupId`, an
abbreviation the design proposed and this ratification corrected per ARCH §1.1/§1.8) alongside
the already-carried `layerIndex`. The module-boundary question (could `UI` reach
`BuildSymmetryOrbit`/`ResolveSymmetryMask`, currently PROC) is resolved **without** relocating
either function to MATH (which would have made MATH illegally depend on PARAMS,
`ARCH_16_03_ModuleBoundaryChain.md` §16.3) and **without** a new UI→PROC dependency exception — `PIPELINE` instead gains
a narrow, explicitly-scoped **stateless query passthrough** (ARCH §3.3, §16.3), the first use of
a pattern now available to future narrow PROC-purity cases. `SANMAP_FORMAT_SPEC` Correction 7's
long-deferred Group/Layer hierarchy gets its first real tier, scoped to `MarkersStack` only
(§16.4) — the exact nested-array JSON key spelling is left to the Format Expert, not asserted
here. Three items remain explicitly routed, not resolved by this ratification (§16.10): the
Format Expert (wire key spelling, `MarkerGroups` shape, the STEP49 export-warning interaction),
the IO Architecture Expert (the `MarkersStack` migration mechanics for the breaking field-tier
move), and the Generator Expert (a mechanical `Placement_Rules_PROC.cpp` call-site update).

**Fixed since the ARCH §16 ratification:** the Format Expert's wire-key/shape follow-up
flagged above has landed — `SANMAP_FORMAT_SPEC` Correction 15 (`MarkersStack`'s
Group(`MarkerRuleLayer`)→Rule(`MarkerRule`) shape, the `Rules` nested-array key spelling, the
`SymmetrySetting` flattened-sibling-keys convention) and Correction 16 (`MarkerGroups`, the
`markers[type].transforms[name]` merge of `layerIndex`/`symmetryGroupIdentifier`, and the ruled
STEP49 export-warning scope: per-`Army`, which subsumes the missing-group case). Both
`PLACEMENT_SCATTER_SPEC.md` (the "Rules — `MarkerRule`" symmetry note, the "IO wrapping"
`MarkersStack` note, and a new "Layer-scoped marker symmetry" closing section) and
`ENTITY_AUTHORING_PARAMS_SPEC.md` (`MarkerTransform::layerIndex`/`symmetryGroupIdentifier`, the
new `MarkerInstanceLayer` type, the `MapRecipe::markerLayers` field, and matching field-rename
table rows) now carry the matching narrative updates that were flagged as outstanding above —
that flag is resolved. Two items from §16.10's routing remain open, not touched by this pass:
the IO Architecture Expert's `MarkersStack` migration mechanics (§16.6), and the Generator
Expert's mechanical `Placement_Rules_PROC.cpp` call-site update. The Format Expert also flagged,
without re-ruling, that `MarkerInstanceLayer::layerId` (STEP60/STEP56, both still undispatched)
carries the same "Id" abbreviation defect ARCH §16.5 rejected for `symmetryGroupIdentifier` —
recorded in `SANMAP_FORMAT_SPEC` Correction 16 as a probable follow-up naming correction
(`layerId` → `layerIdentifier`) for ARCH/the IO Architecture Expert to act on before STEP56/
STEP60 ship, not acted on by this pass.

**Fixed since ARCH §15.7's ownership split (2026-08-21):** the Format Expert's follow-up
`SANMAP_FORMAT_SPEC` Correction defining the `Scenarios` `.sanmap` section has landed —
Correction 17, a single top-level `Scenarios` object hosting `PatternScenarios`/
`CountScenarios`/`DefaultScenario` 1:1 against `Params::Scenarios` (§15.5), with `Area`/
`Position` sub-objects reusing the format's native lowercase shapes verbatim, `AlloyMode`
matching the live Lua literal spelling, and `Field`/`Comparator` ratified as PascalCase C++
enumerator names (a symbolic-operator alternative was considered and rejected — no live Lua
literal exists for either field, and this section's own established PascalCase convention wins
by default over a UI-authoring-compactness argument that doesn't bind wire-format spelling in
the first place). No `SanGenVersion` bump — purely additive, same precedent as Corrections 12
and 14. The IO Architecture Expert's `MapExporter_Scenarios_IO`/`MapImporter_Scenarios_IO` file
pair (§15.7) remains open, not touched by this pass.

**Consolidation pass (2026-08-21) — seven ratings plus one backfill, all cross-checked against
real source before ruling:**
- **`ARCH_16_08_SpawnArmyShrink.md` §16.8 corrected.** Its "alias/name" phrasing for the
  Spawn-marker match key was loose enough to license a false negative in export-time validation.
  Verified against `src/io/MapExporter_Markers_IO.cpp`: the match key is `MarkerTransform::name`
  (the `transforms` dictionary key) only — `alias` is a Correction 11 SanGen-added field the
  engine never reads for this purpose. `work_orders/STEP82_ArmySpawnMarkerValidation_IO.md`
  independently carries the same correction in its own text; §16.8 now matches it.
- **`ARCH_16_10_ConsultRoutingSummary.md` §16.10 item 3 corrected.** It named one PROC consumer
  of the marker-rule symmetry fields; there are two — `Placement_Rules_PROC.cpp` (a compile-time
  break) and `Placement_Hash_PROC.cpp` (a silent dirty-hash regression). "Mechanical" also
  understated the migration: it forces a file split under §1.5's ceilings and carries a hard
  seed-decorrelation-counter determinism requirement. Full shape in
  `work_orders/STEP79_MarkerRuleLayerProcConsumer_PROC.md`, dispatched as one inseparable unit
  with `STEP66_MarkerRuleLayer_PARAMS.md`.
- **`GAMEDATA_LAYOUT_SPEC.md` "Top level" corrected**, per the human's direct verification against
  the real Steam Demo install: the real sanpack path is `Gamedata/<Name>.sanpack.unzipped/<Name>/…`
  (a naming, not nesting-depth, error); `Gamedata/` lives at `<root>/engine/Sanctuary_Data/Gamedata/`,
  not the install root; and `Units.sanpack` ships zipped-only, with no unzipped `Units/Units/` tree
  — the spec's own shorthand for it described a path that does not exist. Propagated consistently
  through the file's other path examples (UI/Environment/Projectiles) for internal consistency.
- **Wrong Constitution citation fixed.** `ARCH_14_09_RenderingPerformance.md` and
  `OPTIMIZATION_PILLARS.md` both cited a nonexistent Constitution "§12" for the basis-tag/benchmark
  law — the Constitution has 8 sections; the real citation is §7 (Work-order schema). Both fixed.
- **`SANMAP_FORMAT_SPEC.md` gains Correction 18** — the army engine-identity/display-name split
  ratified by `work_orders/STEP76_ArmyIdentityNaming_IO.md`: `Army::name` becomes the
  machine-owned, always-`ARMY_XX` engine identity (unchanged role as the `armies[key]` dictionary
  key); a new `Army::displayName` carries the human-authored label, merged as a lowerCamelCase
  sibling of `armyColor`/`alias` inside `armies[<ARMY_XX>]` (Correction 11 precedent). The
  confidence-limited C#-deserializer reasoning (production evidence closes a gap that could not be
  proven from the vendored ground truth alone) is carried into the Correction verbatim.
- **New `ARCH_18_SantpFootprintIngestion.md` §18**, responding to
  `work_orders/DESIGN_SantpFootprintIngestion_R1.md`. §18.1 signs off ticket 85's
  `LuaTableEvaluate_SYS`/`LuaTableValue_SYS` as a sibling `SYS` primitive to `LuaSyntaxCheck_SYS`
  (§15.8) — sharing only the vendored LuaJIT library, never widening `LuaSyntaxCheck_SYS`'s
  compile-only/never-execute contract — and states the binding sandboxed-execution safety
  contract (zero libraries, instruction-count hook, size caps, `lua_pcall` only, fresh state per
  file, owned-tree results only). §18.2 rules the reopened determinism question: ingested
  real-install footprint data may influence generation, but only after a human-triggered,
  one-shot bake into an ordinary `PARAMS` field — never a live read from `PROC`/generation, and
  never written into `DATA` (which is Constitution-defined as pure computed output, not an
  authoring input) — closing over the Exact/Deterministic chain while still letting scatter
  consume real spacing data. Binding on ticket 89 (the ingestion orchestrator); tickets 85–88 are
  governed by §18.1 alone.
- **Backfill: `ARCH_17_MigrationValuesRegistry.md` §17 written.** The 9-migration
  `bLosslessIfSkipped` values table `IO_MIGRATION_SPEC.md` §3 and
  `work_orders/STEP26A_MigrationLosslessFlagAndPreview_IO.md` both cite as "`ARCH.md` §17" now
  exists — a prior attempt died mid-write against the old monolithic `ARCH.md`; the now-split
  per-section file layout carries it without issue. Values transcribed unchanged from STEP26A's
  own audit, cross-checked against the shipped `src/io/Sanmap_MigrationManifest_IO.cpp`.

**Three-ruling pass (2026-08-22), clearing STEP97's block and closing `DESIGN_SantpFootprintIngestion_R1.md`
§7's remaining ARCH-gated open questions (Q1, Q3):**
- **New `ARCH_14_14_AlloySpawnsArmiesManualRouting.md` §14.14.** Answers STEP97's open routing
  question: `Params::MarkerInstanceLayer` gains **no** discriminator field. `MarkerInstanceLayer`
  is a cross-cutting display bucket that can legally mix Spawn- and non-Spawn-type instances under
  one `layerIndex`, so a layer-level field cannot resolve the split. The real signal already exists
  one level down — `MarkerInstanceGroup::name`, a format-reserved literal (`"Spawn"`, already
  load-bearing in `MapImporter_ArmyIdentityNormalize_IO.cpp` and `MarkersTab_Manual_UI.h`'s
  `kSpawnMarkerGroupName`) — mirroring §14.6's already-ratified procedural-side 2-way split
  (`MarkerRule::category`, Spawn vs. rest). `SeedMarkerDomains` therefore routes **per-transform**,
  not per-`markerLayers[i]` entry: a single manual layer may legally contribute Manual sub-layer
  refs to both Alloy's and SpawnsArmies' `subLayers` simultaneously. `kSpawnMarkerGroupName` is
  promoted from its current UI-only home to `Params::kSpawnMarkerGroupName`
  (`MarkerInstance_PARAMS.h`) so `IO`'s existing independent literal and this new UI consumer share
  one named source of truth instead of a third duplicated string. `ARCH_14_02_DataModel.md` §14.2's
  Alloy/SpawnsArmies table row is rewritten in place to its final (non-placeholder) shape.
- **`ARCH_15_03_ExportOnlyLuaRatified.md` §15.3 amended** with the design doc's own recommended
  Q1 clarifying sentence (option (a)): its "never parses Lua back" rule is scoped to the Map
  Scenario system's own authored content and does not bar the separate, sandboxed
  `LuaTableEvaluate_SYS` template-ingestion primitive (§18.1) from reading a different corpus in a
  different, non-round-tripping direction. Supersedes the prior session's "confirmation lives only
  in §18.1, §15.3 itself stays unamended" call — reconsidered because that left exactly the
  misreading risk the design doc warned about for a coder who reads §15.3 in isolation.
  `ARCH_18_SantpFootprintIngestion.md`'s own Q1 paragraph is updated to match.
- **New `ARCH_18_03_CatalogDataOwnership.md` §18.3.** Rules Q3 as the design recommended: footprint
  (ticket 89) and tags (ticket 92, the `bReclaimable` auto-population signal) both stay `IO`-owned,
  asset-derived lookup tables — the same category `AssetAtlasCache_*`/`WorldFootprintSizeTable`
  already occupy, read exactly once at §18.2's human-triggered `PARAMS` bake, never live by `PROC`.
  A new `DATA`-layer catalog type (option (b)) is rejected on the same Constitution §1 grounds §18.2
  already used for footprint. `economy.harvest`/`collisionInfo`/`collider`/`general.displayName`
  are explicitly deferred to the not-yet-scoped texture/asset importer, not ruled on now.
- **Verified, not ARCH-actioned: `DESIGN_SantpFootprintIngestion_R1.md`'s flagged item 4 (proposed
  ticket 93) is moot.** `STEP64_GameInstallLocation_IO.md` already shipped (commit `d84ba6e`,
  `SanGen-v3`) with its own in-place correction to the exact subpath the design doc flags; the real
  `src/io/GameInstallLocation_IO.cpp` builds `mapAssetPath` as
  `JoinExportPath(JoinExportPath(candidateRoot, "engine"), "Sanctuary_Data/Maps")` —
  `<root>/engine/Sanctuary_Data/Maps`, matching the design doc's own "correct" value, not the wrong
  one it flags. No ticket 93 needed; nothing to re-fix.
