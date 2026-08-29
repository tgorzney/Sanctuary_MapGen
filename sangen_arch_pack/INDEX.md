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
| how to spawn units from a per-map Lua script — the load/execution chain, the `Import()`-cache double-execution hazard, `Import()` semantics, the one-`NewThread`-per-script rule + ordering, the `CreateUnit` call, position validation, diagnostics, known-good `tpId`s (companion to `MAP_SCENARIO_SPEC.md`, not restated there) | `specs/MAP_UNIT_SPAWNING_SPEC.md` |
| the engine's native per-navigation-layer pathing-block primitive (Navmap Modifiers) — the all-layer blocker technique (confirmed shipped, twice) and the partial/single-layer technique (confirmed shipped), the per-Lua-state execution nuance distinct from `MAP_UNIT_SPAWNING_SPEC`'s own double-execution hazard, the shared-`NewThread` ordering law (blocker work runs LAST, after unit spawning) and its `pcall`-per-call corollary, and the current manual mask-to-rectangle authoring workflow (ARCH §22) | `specs/NAVMAP_MODIFIER_BLOCKER_SPEC.md` |
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
| the Mask stage — slope gate, stored-art merge, `materialProportions` vs `surfaceStratumWeights`; see also the future-candidate forward-pointer to a SanGen-native mask-to-rectangle workflow (`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7.1) | `specs/MASKING_SPEC.md` |
| marker/prop/unit scatter, rules & gates, symmetry (incl. Radial N-fold, ARCH §13; the new layer-scoped `SymmetrySetting`/`MarkerRuleLayer`, ARCH §16, `SANMAP_FORMAT_SPEC` Correction 15; the `SymmetrySetting` retrofit onto `PropRule`/`DecalRule`/`UnitRule`, ARCH §16.11; the new Group-above-Layer container `MarkerLayerBundle`, ARCH §19, `SANMAP_FORMAT_SPEC` Correction 19; Props/Decals `RuleLayer`/`LayerBundle`/Type-Section authoring parity, ARCH §20), prop SoA, scatter determinism, global marker icon/color/scale defaults; see also the future-candidate forward-pointer to a SanGen-native mask-to-rectangle placement workflow (`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7.1) | `specs/PLACEMENT_SCATTER_SPEC.md` |
| pass-through entity PARAMS — armies/unit groups/unit transforms/map areas AND resolved/baked markers/props/decals/marker chains, incl. manual prop/decal/marker layer authoring (`Params::Army`, `UnitGroup`, `UnitTransform`, `MapArea`, `InstancedTransform`, `MarkerInstanceGroup`, `MarkerTransform`, `PropInstanceGroup`, `PropTransform`, `DecalInstanceGroup`, `DecalTransform`, `PropInstanceLayer`, `DecalInstanceLayer`, `MarkerInstanceLayer`, `MarkerChain`, `ChainMarker`), distinct from procedural scatter rules; also the ratified export-time `blueprintPath` "warn, never block" ruling; `PropInstanceLayer`/`DecalInstanceLayer` gain full field parity with `MarkerInstanceLayer` under ARCH §20 (not yet reflected in this spec's own field tables — see the §20 narrative below); `PropTransform`/`DecalTransform` gain `instanceIdentifier`/`symmetryGroupIdentifier` under ARCH §21.4 (also not yet reflected — see the §21 narrative below) | `specs/ENTITY_AUTHORING_PARAMS_SPEC.md` |
| `Params::Atmosphere` — sun/skylight/exposure-skybox/fog(×3)/wind recipe settings, promoted from the field-complete UI-only `Ui::AtmosphereSettings` | `specs/ATMOSPHERE_PARAMS_SPEC.md` |
| the canonical CPU/GPU dispatch contract — kernel/backend/policy/resource-manager | `specs/DISPATCH_INTERFACE_SPEC.md` |
| preview compositing — passes, coloring, picking, dirty flags, the shadow-sim fix; the ratified v2 screen-space overlay-layering design (six domains — Alloy/SpawnsArmies/Units/Props/Reclaim/Decals; LOD icon rendering; four dirty-flag tiers A/B/C/C2; the View toolbar's two-section popup; ARCH §14); map areas as a composited FIELD layer, not an overlay domain (`PreviewLayerKind::MapAreas`, ARCH §14.17) | `specs/PREVIEW_COMPOSITING_SPEC.md` |
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
ResolveAndApply`/`Scenario.SpawnMatchedScenarioUnits` module contract (⚠️ corrected
2026-08-28 — was `Scenario.SpawnNavalFleets`, which no longer exists; see below), the three-tier
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
- **`layerId` → `layerIdentifier` rename (ARCH §1.9, 2026-08-25).** `PropInstanceLayer::layerId`,
  `DecalInstanceLayer::layerId`, `MarkerInstanceLayer::layerId` (all shipped, confirmed live —
  STEP56/STEP60/STEP111/STEP116), `Resolve{Prop,Decal}InstanceLayerId`, and the wire key `"Id"` on
  `PropGroups`/`DecalGroups`/`MarkerGroups` all carry the "Id" abbreviation ARCH §1.9 now formally
  bans. Target spelling, migration posture (legacy-`"Id"`-fallback import, no forced version bump
  decided here), and full call-site list: `ARCH_01_09_IdAbbreviationBan.md` §1.9. Not blocking any
  currently-open ticket; new code must not repeat this spelling.
- **`BuildMarkerTransformJson` never writes `layerIndex` (confirmed still live, 2026-08-25).**
  `src/io/MapExporter_Markers_IO.cpp:17-39` has no `json["layerIndex"] = ...` line — every exported
  marker's manual-layer membership round-trips as "always absent → clamps to 0" on reimport, unlike
  Props/Decals, which both write it. Flagged originally by
  `work_orders/BRIEF_MarkerGroupLayerRestructure_R1.md`; recorded here as a standing defect per
  `ARCH_19_11_FormatSpecCorrectionBundle.md` §19.11 and `SANMAP_FORMAT_SPEC.md`'s own
  "Conversion / import-export logic" section. Whoever picks this up must also confirm the new
  `ParentBundleIdentifier` merged fields (Correction 19) don't ship with the same write-side gap.
- **`MarkersTab_ManualLayers_UI.h` was over the §1.5 150-line hard ceiling (2026-08-26), remediation
  revised same day.** STEP123 left it at 165 lines. `ARCH_19_22_ManualLayersHeaderSplit.md` §19.22
  originally ruled a single RowBody split (a new sibling header, `MarkersTab_ManualLayerRowBody_UI.h`,
  pairing `MarkersTab_ManualLayerRowBody_UI.cpp`'s existing definitions with the declaration header
  they lacked) and stated no further split was needed before Ticket B. **That premise did not hold:**
  Ticket B's actual draft (`work_orders/STEP125_MarkersTabTypeSections_UI.md`) found the RowBody split
  still unbuilt AND independently needed its own second split (a new `MarkersTab_ManualLayerHelpers_UI.h`,
  five pure standalone helpers) to fit its own three new promoted/renamed declarations under the
  ceiling. §19.22 was rewritten the same day (still §19.22 — no new subsection) to rule BOTH splits
  happen together, additively, as one authoritative shape: RowBody content in one sibling header,
  the five pure helpers plus a new `IsMarkerInstanceLayerRowSuppressed` predicate in another, the
  unused `SelectedManualMarkerLayer` explicitly staying put (dead code, out of scope for either
  split), landing the parent header at roughly 110-115 lines — comfortably under ceiling. Ruled, not
  yet built — a coder work-order.
- **`SANMAP_FORMAT_SPEC.md` Correction 15's closing paragraph is stale (2026-08-27, flagged not
  fixed).** Its sentence "`PropRule`/`DecalRule`/`UnitRule` keep the triplet exactly where it is;
  only `MarkerRule` loses it" no longer holds after `ARCH_16_11_ScatterRuleSymmetryUnification.md`
  §16.11 (below) — a full-file edit was judged too high-risk this session given the file's size
  (1,100+ lines); `PLACEMENT_SCATTER_SPEC.md`'s matching sentence was corrected in place. Whoever
  next touches `SANMAP_FORMAT_SPEC.md` should fold in the correction; the wire format itself did
  not change, so this blocks nothing.
- **`PropsTab_UI.h`/`DecalsTab_UI.h`/`DecalsTab_Manual_UI.h` cite a filename mismatch (ARCH §20.8,
  2026-08-27).** All three shipped comments (STEP159) cite `ARCH_20_DecalsTopLevelTab.md`; the real
  ratifying file is `ARCH_20_08_DecalsTopLevelTab.md` (§20.8, per this pack's own
  `ARCH_NN_MM_Topic.md` subsection-naming convention). Comment-only, not urgent — fix opportunistically
  the next time any of the three files is touched for another reason.
- **`ResolvePropsManual`/`ResolveDecalsManual` still key `OverlayInstanceKey_UI` by per-group array
  position with `bManual` defaulted false (ARCH §21.4, 2026-08-28, flagged not fixed).** The exact
  index-space collision §19.25 already fixed for Markers — confirmed still live by direct read of
  `MapCanvas_IconLayer_CullManual_UI.cpp`'s `ConsiderManualInstance` call sites for Props/Decals.
  Blocked on `ARCH_21_04_PropDecalInstanceIdentityFields.md` §21.4's new `PropTransform`/
  `DecalTransform::instanceIdentifier` fields landing first (ruled, not yet built); full fix shape
  is in §21.4 itself.

⚠️ **RETIRED 2026-08-28 — the naval-fleet paragraph that stood here is obsolete.** It recorded
the 2026-08-21 shaping of `Params::ScenarioNavalFleet` / `ScenarioNavalPondSide` /
`ScenarioNavalPondAssignment` / `ScenarioBody::navy` from the body of
`Scenario.SpawnNavalFleets(area)`. **That function no longer exists.** The 2026-08-27 rewrite
replaced the naval-only machinery with a generic path: a scenario opts in with
`spawnsUnits = true` **and** a matching branch in `Scenario.SpawnMatchedScenarioUnits`, which
dispatches to a per-scenario generator feeding one executor, `Scenario.SpawnUnits`. Every
`NAVAL_*` tuning constant is gone, and placement now derives an anchor live from each army's own
Spawn marker rather than from a pond-side assignment table. The vestigial `navy` field was removed
from the live Lua on 2026-08-28 after being confirmed to have zero readers.

Those four types are formally retired in `ARCH_15_05_ParamsScenariosType.md` §15.5, which also
records two questions deliberately left OPEN rather than guessed: where per-scenario generator and
dispatch code belongs under the ratified three-file split, and whether the placement algorithm has
any declarative PARAMS form at all. Current truth for the runtime mechanism is
`MAP_UNIT_SPAWNING_SPEC.md`; current truth for the scenario system is `MAP_SCENARIO_SPEC.md`,
whose §2 carries an as-built vs ratified-target divergence table.

Still standing from that 2026-08-21 session: `alloyMode`'s `Occupancy` default was promoted from
placeholder to ratified law (`ARCH_15_05_ParamsScenariosType.md` §15.5).

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
move — since shipped in full, see the "§16.6 fully shipped" note below), and the Generator Expert
(a mechanical `Placement_Rules_PROC.cpp` call-site update).

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
that flag is resolved. One item from §16.10's routing remained open as of this pass (since
resolved — see the "§16.6 fully shipped" note below): the IO Architecture Expert's `MarkersStack`
migration mechanics (§16.6). The Generator Expert's mechanical `Placement_Rules_PROC.cpp`
call-site update is not addressed by this note. The Format Expert also flagged,
without re-ruling, that `MarkerInstanceLayer::layerId` (STEP60/STEP56, both still undispatched)
carries the same "Id" abbreviation defect ARCH §16.5 rejected for `symmetryGroupIdentifier` —
recorded in `SANMAP_FORMAT_SPEC` Correction 16 as a probable follow-up naming correction
(`layerId` → `layerIdentifier`) for ARCH/the IO Architecture Expert to act on before STEP56/
STEP60 ship, not acted on by this pass. **Superseded by ARCH §1.9 (2026-08-25): both fields have
since shipped, so this is a real standing defect, not a pre-ship freebie — see the "Standing
recorded defects" list above.**

**§16.6 fully shipped (confirmed 2026-08-27):** the IO Architecture Expert's `MarkersStack`
migration mechanics, described as "still open" in both paragraphs above at the time they were
written, have since landed as real code, not just design — `src/io/MarkersStack_Migrate_V3_IO.h`/
`.cpp` (the `MarkersStack_Migrate_V3` grouping transform), registered under `sourceVersion = 3`
in `src/io/Sanmap_MigrationManifest_IO.cpp`, tested by
`src/io/MarkersStack_Migrate_V3_IO_Test.cpp` (covering every
`work_orders/STEP67_MarkersStackSymmetryMigration_IO.md` acceptance item, including the required
`bIndependentlySelectable`-isolation test). `ARCH_16_06_MigrationRouting.md` §16.6 carries the
full ruling plus this shipped note. Any other reference in this pack to §16.6's migration as
"still open" or "unbuilt" is stale and should be read against this note instead.

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
pair (§15.7) remains open, not touched by this pass. **Note (2026-08-25): this "Correction 17"
heading does not currently exist as its own numbered section inside `SANMAP_FORMAT_SPEC.md`
itself** — confirmed by grep during the §19 ratification pass; several other files/work-orders
cite it by number, but the spec file's own text jumps from Correction 16 to the STEP49 note to
Correction 18. Flagged here, not investigated or fixed — out of scope for the session that found
it.

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

**New `ARCH_14_15_ManualCullStableIdMigration.md` §14.15 (2026-08-24), human ruling + detail
request, not yet dispatched as a work order.** Corrects `ARCH_14_13_OpenItems.md` item 3's stale
"Work-Orders A and B remain unscheduled implementation" line — both confirmed shipped by direct
read (STEP56; `Placement_Manual_PROC.cpp` wired into `Placement_PROC.cpp:50`). Rules the human's
follow-on call: `MapCanvas_IconLayer_CullManual_UI.cpp`'s `ResolvePropsManual`/`ResolveDecalsManual`
retire their positional `layerIndex`-vs-`subLayerArrayIndex` match in favor of a stable-id
(`manualLayerId`-concept) match — but resolved **live against PARAMS**, not read from
`Data::PlacementInstances`. A real staleness hazard was confirmed by direct read before this call
was made: `PlacementStage::ComputeParameterHash()` (`Placement_Hash_PROC.cpp`) does not hash
`recipe.props`/`recipe.decals`/`recipe.propLayers`/`recipe.decalLayers`, and no manual-authoring UI
mutation site sets a PIPELINE dirty flag either — so `Data::PlacementInstances::manualLayerId` for
manual entries only refreshes incidentally, when something else dirties Placement, not on the
manual edit itself. Forcing a dirty-hash tie (so PROC would refresh on every manual edit) was
considered and rejected: it would trigger a full `RunScatter()` — every procedural rule's
`BuildRuleConfigurations`/`BuildDerivedFields`/`ScatterRule` — on every manual-prop drag frame,
violating the Tier C/C2 "zero DAG/dirty-hash involvement" cost model `ARCH_14_08_DirtyFlagTiers.md`
already ratified. Instead, two small inline helpers (`Params::ResolvePropInstanceLayerId`/
`ResolveDecalInstanceLayerId`) are promoted into `PropInstance_PARAMS.h` as the single source of
truth for the id-resolution formula, shared by `Placement_Manual_PROC.cpp`'s PROC-baked copy and
the UI cull path's live resolution, so the two never drift. Ruled dispatchable as-is — narrow, two
call sites, exact before/after text specified in §14.15 itself. **These two resolvers have since
moved to `ScatterInstanceLayer_PARAMS.h` alongside `PropInstanceLayer`/`DecalInstanceLayer`
themselves (ARCH §20.1); `PropInstance_PARAMS.h` includes that file so every existing call site
still compiles.**

**New `ARCH_19_MarkerLayerBundle.md` §19 (2026-08-25) — ratifies `work_orders/DESIGN_MarkerGroupLayerRestructure_R1.md`,
firms up open items in the still-unratified `work_orders/DESIGN_Assembly_R1.md`.** The new
Group-above-Layer container is named **`MarkerLayerBundle`** ("Bundle," not "Group" — "Group"
already names three unrelated existing concepts in this format, and a fourth silent meaning was
judged a real AI-legibility hazard, not a style preference; the UI display label stays "Group,"
a cosmetic string only, §19.1). The domain-touching-logic-vs-pure-mechanics genericity split
(repeated per-domain struct+functions for anything touching real PARAMS fields; one shared
template/callback-parameterized mechanism for pure container/graph mechanics) is promoted from
this ticket's own reasoning to standing law for all future Props/Decals/NavMesh Group work
(§19.2). The Assembly-references-Bundle question is resolved as a scalar `assemblyIdentifier` on
the Bundle (mirroring how leaf transforms already carry it), NOT a `{domain, groupIdentifier}`
forward-reference list on Assembly — keeping Assembly's own already-ratified "no members list"
rule intact one tier up (§19.5), with a new sub-rule (not in either prior document) that a nested
child Bundle carrying its own different `assemblyIdentifier` stops the recursive walk there
(§19.6). `TreeListWidget_UI<T>` is ruled a single shared, domain-agnostic UI-framework primitive,
built by the Markers Group ticket first (the human's own explicit "get Markers working first"
priority) with Assembly's leaf-body-callback extension included from day one, not bolted on later
(§19.7). **New general rule, closing a question raised three times per-feature before this
session**: `ARCH_03_ModuleBoundaries.md` gains **§3.5**, ruling that a pure function's home is
decided by whether its signature carries a `Params::` type (yes → `PARAMS`, co-located free
function, never `MATH`, never a new `PROC` file; no → `MATH`, fully generic) — settling
`BuildSymmetryOrbit` (§16.3), Assembly's rigid-rotate math, and this ticket's cycle-detection and
rotate math all under one citable rule instead of three separate design-doc judgment calls. This
confirms Bundle's rotate math and Assembly's rotate math are the exact same `MATH`-layer function,
not two copies (§19.8). The "Id" abbreviation question, raised in both this design and
`DESIGN_Assembly_R1.md` §7, is resolved once for both: **new `ARCH_01_09_IdAbbreviationBan.md`
§1.9**, which also retroactively confirms `PropInstanceLayer`/`DecalInstanceLayer`/
`MarkerInstanceLayer::layerId` are real, already-shipped naming-law violations (not the
pre-ship freebie `SANMAP_FORMAT_SPEC.md` Correction 16 previously assumed) — recorded as a new
standing defect above, routed to the IO Architecture Expert for migration mechanics, not
blocking. `SANMAP_FORMAT_SPEC.md` gains **Correction 19** (`MarkerLayerBundles`, the new
`ParentBundleIdentifier` merged fields on `MarkersStack`/`MarkerGroups` entries) and a
documentation-only correction pass closing several already-stale field-list gaps in Corrections
7/16 and the `markers[type].transforms[name]` section (full list: `ARCH_19_11_FormatSpecCorrectionBundle.md`
§19.11). Manual-layer-only Bundle membership (§19.9), v1 tab-driven-only Move/Rotate with no new
canvas gesture (§19.10), and soft (UI-only) Bundle→marker-type consistency (§19.12) are all
ratified as designed, confirmed consistent with Assembly's own parallel rulings rather than
independently drifting variants.

**`ARCH_19_MarkerLayerBundle.md` §19 extended to §19.13–§19.22 (2026-08-26) — ratifies
`work_orders/DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md` (the UI Expert's design-round
response to `BRIEF_MarkerTypeSectionsAndInstanceSelection_R1.md`) in full; every one of the
design's 14 requested rulings, plus one unrelated time-sensitive file-size-ceiling item, ratified
as proposed — none corrected.** `MarkerRuleLayer`/`MarkerInstanceLayer` gain `markerTypeName`
(§19.13, extends §19.3's field on `MarkerLayerBundle` to both Layer types, wire key
`"MarkerTypeName"`, additive). The new Markers-tab Type-section tier is confirmed **UI-derived** —
dynamic enumeration over live `markerTypeName` values, no new `Params::MarkerTypeSection` struct,
with a binding Alloy/Plasma/Spawn-first-then-alphabetical-then-"(Unassigned)" ordering rule
(§19.14), because `markerTypeName` is already an open free-form string space and
`GlobalMarkerSettings` is a genuinely closed, different-axis 3-field struct (confirmed by direct
code read, not assumed). §19.15 rules three tightly-coupled Ticket B compositions at once: the
per-type filtered-copy `TreeListWidget_UI<MarkerLayerBundle>` instantiation is confirmed safe by
`Render`'s own read-from-copy/write-by-identifier-lookup contract; a nested child Bundle whose
`markerTypeName` diverges from its parent's cross-section-cutoff is ruled the correct rendering
(the UI-tier analogue of §19.6's PARAMS-tier Assembly cutoff, cited explicitly rather than left an
unstated consequence); and `bRowSuppressed` composing two independent AND'd predicates
(Bundle-membership, type-mismatch) is signed off as within the field's documented contract, with
its widened reorder-across-invisible-rows blast radius explicitly recorded as an accepted,
inherited tradeoff rather than silently absorbed. A new `MarkerTransform::instanceIdentifier`
(§19.16, global — not per-group — uniqueness, wire key `"InstanceIdentifier"`, legacy-backfill by
sequential encounter order across the whole nested import walk, mirroring `layerId`'s own
already-shipped precedent exactly) gives manual markers the stable cross-frame identity the
selection feature needs; §1.9's "Id" ban is confirmed to already cover it correctly, recorded
there too so a future grep for every field that rule has touched finds this one. `GlobalMarkerSettings`
gains `selectColorAlloy/Plasma/Spawn` as a strict mirror of the existing 3-field pattern, plus one
signed-off, explicitly-justified 4th field, `selectColorDefault` (§19.17) — the existing pattern's
white-fallback-for-unmatched-name convention is correct for "no special type color" but would make
"selected" indistinguishable from "unselected" for any free-form group name, a real correctness
gap the deviation closes, not an arbitrary 4th field. §19.18 records the canonical selection-tint
priority order (refused-drag > selected > army-color > layer/type color) and rules "selected
replaces fill" a visual language kept permanently distinct from the drag-ghost's existing
unfilled-ring vocabulary (confirmed by direct read: the ghost uses `AddCircle`, not
`AddCircleFilled`) — two vocabularies, never colliding; **§19.18 is itself later amended by §21.5
(2026-08-28) to correct a locked-instance selectability premise — see that note below.** §19.19
confirms the static symmetric-sibling highlight computes fresh every frame via the existing
`Pipeline::BuildWorldSymmetryOrbit` plus a small one-shot inline nearest-match — deliberately NOT
`MarkerOrbitCorrespondence_UI.h`, a heavier cross-frame stability matcher solving a drift problem
this one-shot feature doesn't have — reusing `markerSymmetryFixSettings.distanceTolerance` for the
match epsilon (no new tolerance field) and wiring the canvas via a new
`SetManualMarkerSelectionSource`, confirmed the same null-safe-injection shape as the existing
`SetManualMarkerDragSource`/`SetActivePanelSource`, not a new module-boundary pattern. §19.20
formally extends §19.9's manual-only-membership law one further tier to selection scope (no
procedural-instance selection — `Data::PlacementInstances` has no stable cross-bake identity to
hang one on) — **this specific "no procedural selection" premise is itself corrected the very
next session; see the 2026-08-26 correction-round paragraph below.** §19.21 closes, with one
explicit sentence, that `MarkerRule::category` and `markerTypeName` are two permanently
independent concepts a future ticket may not silently merge.

**Unrelated, time-sensitive item from the same session — `ARCH_19_22_ManualLayersHeaderSplit.md`
§19.22 (2026-08-26; revised, same day, into its FINAL combined shape once Ticket B's own actual
draft falsified this section's original "no further split needed" premise — see the "Standing
recorded defects" entry above for the short version).** `src/ui/MarkersTab_ManualLayers_UI.h` was
left at 165 lines by a just-completed coder ticket (STEP123), over
`ARCH_01_05_FileSizeCeilings.md` §1.5's 150-line hard ceiling (it was already exactly at the
ceiling before that ticket's own change). §19.22's FINAL ruling splits the file along BOTH of its
real fault lines, additively, in one remediation: (1) a new sibling header,
`src/ui/MarkersTab_ManualLayerRowBody_UI.h`, gives the already-separately-implemented
`MarkersTab_ManualLayerRowBody_UI.cpp` the declaration header it should already have had
(`DrawLayerRowBody`, `DrawManualMarkerLayerColorOverrideHeaderControl`, their two paired
reserved-width constants); (2) a new `src/ui/MarkersTab_ManualLayerHelpers_UI.h` (required by
Ticket B's own `STEP125_MarkersTabTypeSections_UI.md` §6, adopted here as-is) relocates the five
named pure helpers (`IsMarkerInstanceLayerLocked`, `QuantizeMarkerPositionToLayerGrid`,
`EffectiveManualMarkerLayerColor`, `ManualMarkerLayerRowLabel`, `NextMarkerLayerName`) plus a new
`IsMarkerInstanceLayerRowSuppressed` predicate; the unused `SelectedManualMarkerLayer` is an
explicit carve-out, staying in the parent header untouched (confirmed dead — zero call sites —
out of scope for either split to relocate or remove). The parent header keeps its
`ManualMarkerLayersState` struct plus Ticket B's own new/renamed entry-point declarations
(`DrawLayerList` promoted, `DrawManualMarkerLayerBlockSettings` renamed/promoted from
`DrawLayerSettings`, `DrawManualMarkerLayerListBody` new, `DrawLayerListButtons` gains a
`markerTypeNameForNewLayer` parameter) and gains a `DraggableListWidget_UI.h` include it did not
previously need (for `DraggableListSignal`, `DrawLayerList`'s now-visible-outside-the-.cpp return
type) — landing at roughly 110-115 lines, well under ceiling. Every real consumer's `#include`
list is updated explicitly for both new headers, mechanically, at every call site — no reliance on
either header's transitive re-export. Ruled, not yet built — a coder work-order (implemented as
part of, or immediately ahead of, Ticket B's own diff — the two splits and Ticket B's new
declarations land together).

**`ARCH_19_MarkerLayerBundle.md` §19 extended to §19.23–§19.27 (2026-08-26, "Markers UI Correction
Round 2") — ratifies `work_orders/DESIGN_MarkersUICorrectionRound2_R1.md` (the UI Expert's response
to `work_orders/BRIEF_MarkersUICorrectionRound2_R1.md`, the human's post-STEP121-126 correction
list) for its five ARCH-flagged items, all ratified as designed after independent re-verification
against the live code (not taken on the design doc's word alone).** §19.23 signs off
`TreeListWidget_UI<T,LeafKeyT>::Render`'s new header-extra contract with TWO callbacks
(`drawNodeHeaderExtra(int)`/`drawLeafHeaderExtra(const LeafKeyT&)`), a deliberate divergence from
`DraggableList<T>::Render`'s single-callback shape — confirmed correct because the tree has two
row kinds with two distinct identity types, unlike `DraggableList`'s one; the additive overload and
thin-delegator preservation of the existing 7-callback signature were confirmed by direct read of
`TreeListWidget_UI.h`. §19.24 signs off `Params::MarkerInstanceLayer::bSymmetryEnabled` (default
`true`, wire key `"SymmetryEnabled"` — confirmed by direct read of the sibling `bLocked`/
`bGridSnapEnabled`/`bColorOverrideEnabled` IO code, all `b<Name>`→`"<Name>"`), gating the effective
symmetry mask to `None` without destructively clearing the configured axes, mirroring
`bColorOverrideEnabled`'s exact shape in the same struct. §19.25 is the round's most invasive
ruling: `OverlayInstanceKey_UI` gains `bool bManual = false` (fixing a real, independently
re-confirmed live bug — procedural `Data::PlacementInstances` array positions and manual per-group
transform indices shared one untagged number space and could collide under
`PlacementCollectionKind_UI::Markers`); `ResolveMarkersManual` switches its selection key from a
per-group index to `MarkerTransform::instanceIdentifier` (§19.16); `MapCanvas::selectedEntityIdentifier`
widens to be backed by the full `OverlayInstanceKey_UI`, with `SetSelection` gaining a canonical
full-key overload every selection-setting path (canvas pick, manual list-click, §19.27's procedural
list-click) now shares; `ApplyClick` gains a manual-marker linear hit-test fallback;
`selectionChangedCallback` widens to carry the full key; and a new shell-mediated
`MapCanvas::SelectManualMarkerByInstanceIdentifier(int)` + `Application`-bound callback lets a
Markers-tab list click drive the same selection state a canvas click drives — confirmed the same
null-safe-injection shell-mediation pattern §19.19's `SetManualMarkerSelectionSource` already uses,
not a new module-boundary class. **§19.25 formally corrects §19.20**, whose "manual-only" framing
and "OverlayInstanceKey_UI... untouched, unshared, unreferenced" claim no longer hold — see §19.20's
own file for the full correction note; §19.20's one binding sentence that still stands
(`instanceIdentifier` is never repurposed for procedural identity) is honored by both §19.25 and
§19.27. §19.26 records the manual-instance symmetry-cluster grouping UI shape (partition by
`MarkerTransform::symmetryGroupIdentifier`, `0` = ungrouped/flat-after, non-zero = collapsible
cluster, first) — no PARAMS change, recorded per the design's own request. §19.27, overriding
§19.20's prior scope-out at the human's direct instruction ("This was wrong and is now overridden:
build it"), gives procedural marker instances their own listing/selection mechanism: a session-only
per-frame `ruleIndex`→array-position index over `Data::PlacementInstances` (mirroring
`ManualInstanceLayerIndex_UI`'s shape, zero dirty-hash/DAG participation per §14.8's Tier C/C2 cost
model, no new `DATA` field), whose selection key converges onto §19.25's SAME `OverlayInstanceKey_UI`
representation (`bManual=false`) — confirmed as one shared mechanism, not built twice — plus a
symmetry-grouping rule for procedural instances that DELIBERATELY differs from §19.26's manual `== 0`
convention: `Data::PlacementInstance::symmetryIdentifier` is never `0` (confirmed by direct read,
`nextSymmetryIdentifier` starts at `1`, `Placement_PROC.cpp:44`/`Placement_Accept_PROC.cpp:43`), so
the correct predicate is bucket size `> 1`, not id value. Two lower-risk, UI-composition-only items
from the same design doc — the "(Unassigned)" Type-section becoming present-only (with a conditional
free-text Marker Type field on empty ungrouped rows, mirroring the Bundle's own existing field) and
flattening the "Ungrouped Procedural Rules"/"Ungrouped Manual Marker Layers" sub-sections into plain
rows — were reviewed and raise no ARCH objection; no new subsection was written for either, per the
design doc's own assessment that neither introduces a new field or cross-cutting contract.

**New `ARCH_16_11_ScatterRuleSymmetryUnification.md` §16.11 (2026-08-27) — takes the non-binding
follow-on §16.1 explicitly left open.** `PropRule`/`DecalRule`/`UnitRule` (`src/params/ScatterRule_PARAMS.h`)
each replace their inline `bSymmetryUseGlobal`/`symmetryMask`/`radialSymmetryRepeatCount` triplet
with a composed `Params::SymmetrySetting symmetry;` member, the same field name/type
`MarkerRuleLayer`/`MarkerInstanceLayer` already carry — closing the last of the four sites §16.1
identified as sharing the old flat convention (`MapRecipe::globalSymmetryMask`, the fifth, stays
out of scope — a top-level default, not a per-rule override). Confirmed by direct read of all six
exporter/importer files (`MapExporter_{Props,Decals,Units}Stack_IO.cpp` and their importer
counterparts) before ruling: every one already writes `SymmetryUseGlobal`/`SymmetryMask`/
`RadialSymmetryRepeatCount` as flat sibling JSON keys, the same convention Correction 15 already
documents for the composed marker types — so this is a **pure C++-internal field-grouping
refactor** with a byte-identical `.sanmap` wire shape, no `SanGenVersion` bump, no
`IO_MIGRATION_SPEC` entry, and (unlike §16.6's marker migration) compile-fail-safe rather than
silent-failure-prone, since the three flat members are removed outright. Full touch list (PARAMS,
two PROC files, three UI draw call sites, six IO files) and the one still-open documentation
follow-up (`SANMAP_FORMAT_SPEC.md` Correction 15's closing paragraph, flagged in the "Standing
recorded defects" list above, not fixed this session) are in §16.11 itself;
`PLACEMENT_SCATTER_SPEC.md`'s matching stale sentence was corrected in place this session.

**New `ARCH_20_PropsDecalsAuthoringParity.md` §20 (2026-08-27) — extends §19's Marker-specific
Group-above-Layer model to Props and Decals**, per a consult ruling grounded by direct reads of
`MarkerRule_PARAMS.h`, `MarkerInstance_PARAMS.h`, `MarkerLayerBundle_PARAMS.h`,
`PropInstance_PARAMS.h`, `ScatterRule_PARAMS.h`, `GlobalMarkerSettings_PARAMS.h`,
`MapRecipe_PARAMS.h`, `MarkerDragGesture_UI.h`, `MarkersTab_ManualLayerHelpers_UI.h`. `PropRuleLayer`/
`DecalRuleLayer`/`PropLayerBundle`/`DecalLayerBundle` are hand-written per-domain structs, not
templated — applying §19.2's already-standing rule, which named Props/Decals explicitly, rather
than re-deriving it (§20.1); new file homes `ScatterRuleLayer_PARAMS.h`/`ScatterLayerBundle_PARAMS.h`,
and `PropInstanceLayer`/`DecalInstanceLayer` move out of the now-too-small `PropInstance_PARAMS.h`
once they gain full field parity. The grid-snap/effective-symmetry resolver functions are likewise
duplicated per domain, in PARAMS this time (matching where `ResolvePropInstanceLayerId` already
lives) — with a recorded, non-blocking finding that the Marker originals are themselves misplaced
in UI by the same §3.5 rule (§20.2). `GlobalPropSettings`/`GlobalDecalSettings` are new, but scoped
to what has a real per-domain analog rather than a blind `GlobalMarkerSettings` mirror — no
icon-name fields (Props/Decals already resolve real icons from `blueprintPath`), and
`GlobalDecalSettings` skips a name-matching resolver entirely since it only ever has one value
(§20.3). **Two items were explicitly gated, not resolved by that ratification:** the Prop/Decal
drag-reposition + selection substrate needed a UI Expert design round, unified with the
separately-paused canvas click/box-select initiative, rather than a third hand-mirrored
`MarkerDragGesture_UI` copy (§20.4 — **closed by §21, see below**); and the `PropRuleLayer`/
`DecalRuleLayer` flat-to-two-tier wire restructuring is the same *class* of breaking change as
Markers' own `§16.6` migration — which has since shipped in full (see the "§16.6 fully shipped"
note above; Props/Decals still need their own version-step built against the new arrays, not a
free pass by analogy) — and is routed to the IO Architecture Expert as one shared migration-shape
consult covering all three domains together (reusing Markers' shipped `MarkersStack_Migrate_V3_IO`
as its working precedent), not an independently-invented second migration (§20.5, still open).
Type Sections reuse §19.14's UI-derived mechanism verbatim, but the field is named `propTypeName`
(never `markerTypeName` on a Prop struct) and Decals gets no equivalent field at all, since it has
exactly one implicit type (§20.6); `PropRule`/`PropInstanceGroup::bReclaimable` stays permanently
independent of `propTypeName`, the same closure §19.21 already ruled for `MarkerRule::category` vs.
`markerTypeName`. §20.7 flags, without reversing, that this keeps growing `MapRecipe_PARAMS.h`'s
flat top-level member list rather than nesting per-domain authoring data — in tension with the
opening hit-list's god-object-dismemberment direction, but ruled to stay flat for consistency with
Markers' own already-shipped flat shape. §20.8 separately ratifies an already-shipped, unrelated
fact a STEP159 comment pass had pre-emptively cited: Decals is a real standalone top-level tab
(`DecalsTab_UI.h`), not a sub-block of `PropsTab_UI.h` — closing a dangling forward-reference to a
file (`ARCH_20_DecalsTopLevelTab.md`) that did not exist until this session (the real file is
`ARCH_20_08_DecalsTopLevelTab.md`, a citation-spelling mismatch recorded above as a standing,
non-blocking defect). `PLACEMENT_SCATTER_SPEC.md` and `ENTITY_AUTHORING_PARAMS_SPEC.md` still need
fuller narrative updates naming these new types once §20.5 lands and the shape settles further —
not done yet, flagged in both the topic table above and `ARCH_20_PropsDecalsAuthoringParity.md`'s
own "Related law" so it is not lost.

**New `ARCH_21_CanvasInteractionUnification.md` §21 (2026-08-28) — closes §20.4's gate; ratifies the
UI Expert's canvas multi-select/drag-gesture/picking design round, unified as that gate required,
grounded by direct reads of `MapCanvas_UI.h`/`.cpp`, `MapCanvas_Draw_UI.cpp`,
`MapCanvas_IconLayer_UI.h`, `MapCanvas_IconLayer_CullManual_UI.cpp`, `Picking_UI.h`/`.cpp`,
`SpatialGrid_DATA.h`, `RuleBucketIndexSet_DATA.h`, `GenerationAssembler_PIPELINE.h`,
`GenerationAssembler_Stages_PIPELINE.cpp`, `MarkerDragGesture_UI.h`/`.cpp`,
`MarkerDragGesture_Frame_UI.cpp`, `MarkerOrbitCorrespondence_UI.h`, `PropInstance_PARAMS.h`,
`UniqueNameList_UI.h`, `Application_UI.cpp`.** §21.1 widens `MapCanvas`'s selection from one
`OverlayInstanceKey_UI` to an ordered `OverlayInstanceKeySet_UI` (new `MapCanvas_SelectionSet_UI.h`/
`.cpp`) with a precisely-specified MRU-primary/Replace/Toggle/Union contract and a widened
`SetSelectionChangedCallback((primary, selectedKeys))`, generalizing `Application::WireCallbacks()`'s
closure to partition the set into the already-existing `tabState.markers.selectedManualInstanceIdentifiers`
plural field (STEP141) rather than the single-element list it wrote before. §21.2 rules press-time
drag-begin-first / release-time click-or-marquee for the left button, and moves the WHOLE pan
gesture onto a brand-new, independent right-button tracker (`ImGuiMouseButton_Right`/raw
`io.MouseDown[1]`, since imgui's item-activation only tracks the left button — confirmed by direct
read) — a real, deliberate UX change, not incidental. §21.3 genericizes the Marker-only drag-gesture
machinery behind a `Traits` policy struct (mirroring `TreeListWidget_UI<T,LeafKeyT>`'s own
accessor-parameterization precedent), with two corrections this session's own direct read forced:
the gesture STATE struct itself needs no template parameter at all (every field was already
`Params::`-free), and `PropTransform`/`DecalTransform` carry no `name` field (confirmed by direct
read of `PropInstance_PARAMS.h`) — so two of the design's `Traits` hooks are ruled inert-by-
construction for Props/Decals rather than left to fail to compile. §21.4 ratifies the human's
decision to add `PropTransform`/`DecalTransform::instanceIdentifier`/`symmetryGroupIdentifier`,
verbatim mirrors of `MarkerTransform`'s own fields, PARAMS/IO-only and severable from the rest of
§21 — closing the identity-collision gap `ResolvePropsManual`/`ResolveDecalsManual` still carry
today (confirmed still live by direct read, recorded above in "Standing recorded defects"). §21.5
ratifies the human's second decision — locked instances are now excluded from click-select,
marquee-select, AND drag uniformly across Markers/Props/Decals, procedural instances unaffected —
as a direct, in-place CORRECTION to `ARCH_19_18_SelectionTintPriorityAndVisualLanguage.md` §19.18's
prior implicit "a locked instance can still be freshly selected" premise, not a new rule left
standing beside the old one. §21.6 ships `Data::SpatialGridSet` (a `RuleBucketIndexSet`-shaped
4-way mirror of the existing single `Data::SpatialGrid`), three new `SpatialGrid` accessors
exposing its private cell-coordinate split, and a region-query function — corrected in name from
the relayed design's `PickMarkersInRegion` to `PickInstancesInRegion`, since the function's own
stated contract is fully domain-generic and ARCH §1.1 does not permit a domain-generic function to
carry a specific domain's name. §21.7 flags `MapCanvas_UI.h`'s file-size ceiling as a foreseeable
consequence for the coder work-order to re-measure, per §8.4's scope law, rather than pre-ruling a
split shape. `PLACEMENT_SCATTER_SPEC.md`, `UI_FRAMEWORK_SPEC.md`, and `ENTITY_AUTHORING_PARAMS_SPEC.md`
all still need fuller narrative updates naming §21's new types once the gated §21.3 Prop/Decal
`Traits` land and the shape settles further — not done this session, flagged here so it is not lost.

**`ARCH_15_05_ParamsScenariosType.md` §15.5 amended again (2026-08-28, same-day follow-on to the
naval-fleet retirement above) — `ScenarioBody::areaName`, a named-`Area` reference.** Human-approved
design consult, independently re-verified against the real code before ratification (not
rubber-stamped): `Scenario_PARAMS.h`, `MapExporter_Scenarios_IO.cpp`,
`MapImporter_ScenarioRecord_IO.cpp`, `ScenarioScript_DataLua_IO.cpp`,
`resources/lua/SanGenScenarioRuntime.lua`, `ScenariosTab_Detail_UI.cpp`, `MapArea_PARAMS.h`,
`AreasTab_List_UI.h`, `UniqueNameList_UI.h`, `AreasTab_UI.cpp`. One new field, `ScenarioBody::areaName`
(default empty ⇒ today's exact disconnected-rectangle behavior, fully backward compatible),
resolved against `recipe.areas` by name **at export time only** into the existing flat `Area`/`area`
key every scenario record already carries — zero footprint on the Lua-rendering leg or
`SanGenScenarioRuntime.lua`, confirmed by direct read (`ResolveAndApply` returns the matched
scenario's `area` field verbatim, with no name-lookup capability of its own to feed). **Ruled: the
reference round-trips** — a new additive wire key, `AreaName`, sibling of `Area`, no `SanGenVersion`
bump (same posture as Corrections 12/14/17) — rejecting the alternative (export-only bake, no wire
key) because it silently degrades the feature to the "copy values in once" design the human
explicitly chose against. **Ruled: the four rectangle sliders go read-only** (`ImGui::BeginDisabled`,
already an established codebase idiom) whenever a reference is active, rejecting
silent-clear-on-edit as a worse authoring-safety hazard (an accidental slider nudge should not
silently detach a scenario from its named Area). Stale/duplicate-reference handling: first-name-match
resolution (mirrors `ResolveAreaColor`/`NameIsTakenBefore`'s existing idiom), a stale reference falls
back to the last live-preview `body.area` rect with a loud non-blocking export-time warning (mirrors
`ARCH_15_10` point 2's own idiom for `maxArmySlotCount`), and the Combo needs one new sentinel entry
`DrawArmyNameField` does not have (an explicit "no reference" choice, since empty `areaName` is a
real authored state here, not a transient one). Full ruling: `ARCH_15_05_ParamsScenariosType.md`
§15.5's "AMENDED 2026-08-28 — `ScenarioBody::areaName`" note (a second, later amendment than the
naval-fleet retirement recorded earlier the same day in that same file). `MAP_SCENARIO_SPEC.md`
gains a new §6.2 cross-reference note (this field has no Lua-side counterpart, so it is
deliberately NOT added as a row to §6's Lua-ground-truth table); `ARCH_15_MapScenarioSystem.md`'s
§15.5 index row is updated to mention it. **Note, not fixed by this pass:** the true `.sanmap` JSON
wire-format authority for this field is `SANMAP_FORMAT_SPEC.md`'s "Correction 17," which — per this
file's own already-recorded flag two paragraphs above — does not currently exist as written text;
whoever eventually writes it must include `AreaName` alongside `Name`/`Area`/etc. Not a work order —
the ARCH Expert does not write code or work orders; this ratifies the shape for the Format/IO
Architecture Experts' next dispatch.

**CORRECTION (2026-08-28, same day, second pass) to the `ScenarioBody::areaName` amendment
directly above — caught by the Format Expert while drafting
`work_orders/STEP209_ScenarioAreaNameReference_PARAMS_IO_UI.md`, re-verified independently before
being recorded here (not rubber-stamped).** The amendment's original "Zero changes anywhere else"
claim was right for the STRING (`areaName`/`AreaName` genuinely never needs to reach Lua — still
true) but WRONG for the NUMBERS. Direct re-read shows `ScenarioScript_DataLua_IO.cpp`'s
`AppendScenarioBodyFields` (the `<MapName>_Scenarios_Data.lua` leg — **the file the game actually
loads**, `MAP_SCENARIO_SPEC.md` §14) reads `body.area.originX/originZ/width/length` **directly and
independently** of `MapExporter_Scenarios_IO.cpp`'s `BuildScenarioRecordJson` (the `.sanmap` JSON
leg) — the two builders are never called from a shared parent, and
(`ScenarioScript_Export_IO.h`'s own header comment) are triggered by two entirely separate UI
actions with two entirely separate result types, "never merged." Left as originally ruled, the
JSON leg would re-resolve `areaName` fresh on every export while the Lua leg silently kept
rendering stale numbers whenever a referenced Area was resized without the scenario being
reselected — the two SanGen-authored artifacts would diverge, and the one that matters for
gameplay would be wrong, exactly the class of silent-wrong-result failure Constitution §6 forbids.
**Fixed by extending the ratified algorithm (never redesigning it) to the second call site**: the
identical resolved-rect-with-fallback logic is now also threaded into
`ScenarioScript_DataLua_IO.cpp`'s `AppendScenarioBodyFields` (a second, duplicated resolver,
matching this exact file family's own established per-leg-duplication precedent — it already
duplicates its alloy-mode/count-field spelling tables rather than sharing them with the JSON leg),
and the same stale-reference warning is wired into `ScenarioScript_Export_IO.cpp` via one new
shared validator header used by both legs (already specified in full by the Format Expert,
STEP209 §5 — not re-designed here). Full corrected text: `ARCH_15_05_ParamsScenariosType.md`
§15.5's own "CORRECTED 2026-08-28 (same day, second pass)" paragraph, added directly inside the
"AMENDED 2026-08-28" section (not a separate note) so a future reader sees the correction beside
the ruling it corrects; `MAP_SCENARIO_SPEC.md` §6.2 was updated in place to match (retitled "…
corrected same day," STRING/NUMBERS split spelled out explicitly). This is exactly the
self-caught-discrepancy-recorded-not-patched-over discipline §21.3's own precedent above
established for this pack.

**Missing topic-table row backfilled (2026-08-29): `MAP_UNIT_SPAWNING_SPEC.md` existed as a real,
already-heavily-cited spec file but had never been added to this file's own topic table** — a pure
documentation gap, not a content defect (every other paragraph above already cites it correctly).
Fixed as part of adding `NAVMAP_MODIFIER_BLOCKER_SPEC.md` below, since both specs govern the same
per-map `<MapName>_data.lua`/shared-`NewThread` surface.

**New `ARCH_22_NavmapModifierBlockers.md` §22 (2026-08-29) — formalizes a hand-authoring technique
proven live, in-game, twice, on Pandemonium Isthmus.** Ratifies `NAVMAP_MODIFIER_BLOCKER_SPEC.md`,
following the same process §15 used for the Map Scenario system. The engine's native
`NavmapModifierTemplate` primitive (an axis-aligned, no-rotation, per-layer world-space rectangle,
with the set of layers a prefab can ever block fixed forever at prefab-template creation time,
§22.2) has two real native consumers today — a unit template's `skirtSize` field (all layers except
Air) and the engine's own singleton `PlayableAreaBarrier` prefab (every layer, created once per map
and cached as `_G.PlayableAreaBarrierPrefabID`/`_G.PlayableAreaBarrierLayers`). **Confirmed shipped,
twice:** reusing that global prefab is the correct, sole technique for an all-layer blocker (§22.3)
— `Engine.InstantiatePrefab` + `GetNavmapModifierIDs`/`SetNavmapModifiersSize`/
`SetNavmapModifiersEnabled`, the exact sequence `playableAreaBarrier.lua`'s own
`CreateBarrier`/`SetBarrierSize`/`SetBarrierEnabled` already use, with navmap modifiers host-only
and the client touching only its local grid-modifier preview. ~~**⚠️ Designed, not yet shipped:** a
partial/single-layer blocker (e.g. Sea-only) cannot reuse that global prefab — its other five
layers' modifier children sit at real, active `disabled=false`/`size=(1,1)` template defaults and
would silently drop stray blockers — so it needs its own purpose-built prefab; §22.4 rules this may
stay a map-local `_data.lua` helper today, recommending (not mandating) promotion to a shared
`common/loading/*.lua` helper only once a second map needs the same pattern.~~ **This paragraph's
partial-layer status claim is now stale — see the correction appended at the end of this file
(2026-08-29, later same day).** The stray-1×1-blocker reasoning it states is still correct; only
the shipped-status framing needed the update. §22.5 records a
debugging-load-bearing nuance confirmed by direct read of `script.lua` — a solo/listen-server match
runs the shared per-map chunk once per **Lua state** (not per `Import`-cache-key, which is the
**different**, already-documented `MAP_UNIT_SPAWNING_SPEC.md` §2 hazard) — explicitly flagged so
the two are never conflated again, having been misdiagnosed as one bug twice this session. §22.6
extends (does not replace) the existing shared-`NewThread` ordering law with two new constraints
(after `SetPlayableArea`, before any not-yet-proven code) and records the concrete resolved order
on Pandemonium Isthmus today, explicitly flagged as differing from the illustrative/documented
order in `MAP_UNIT_SPAWNING_SPEC.md` §4 and `MAP_SCENARIO_SPEC.md` §3.1 (both now cross-reference
this ruling so neither is silently trusted as still-current on its own). **This ordering claim is
ALSO stale — see the same correction appended at the end of this file; the "differs from" framing
was wrong, and the actual live order is unit-spawn-then-blocker, not blocker-then-unit-spawn.**
§22.7 records, without
ratifying as a SanGen feature, the current ad hoc Python mask-to-rectangle authoring pipeline
(exact rectangle decomposition + tunable-overshoot agglomerative merge + mandatory zero-miss
verification) used to produce both live tests' rectangle lists, flagging it as a strong future
SanGen-native masking/placement candidate (`MASKING_SPEC.md`/`PLACEMENT_SCATTER_SPEC.md` each carry
a forward-pointer, not a restatement). §22.8 records the pixel↔world convention that workflow uses
— the *same* `SANMAP_FORMAT_SPEC.md` heightmap-sampling convention, applied in the *inverse*
direction — and explicitly rules it must never be conflated with that spec's *separate*,
still-axis-unresolved `.sanmap` entity-position convention (`SANMAP_FORMAT_SPEC.md` itself now
carries a pointer to this ruling immediately after its own heightmap-sampling paragraph, for
exactly this reason). **§22.9 rules this entire ratification is knowledge-pack law about a
hand-authored Lua technique, not a SanGen `PARAMS`/`IO`/`UI` construct** — both techniques live in
the hand-authored `<MapName>_data.lua` orchestrator, the exact file `ARCH_15_04` already forbids
SanGen from ever writing; a future SanGen-native mask-to-rectangle placement feature is recorded as
an intended future direction only, not designed or scheduled here (mirroring §15.9's own posture
for the engine-whitelist migration path).

**CORRECTION (2026-08-29, same day) to §22.6 above — ordering alone did not fix the bug it was
meant to fix; `pcall` does.** On Pandemonium Isthmus, the all-layer air blocker went through
multiple rounds of live re-ordering (blocker before/after `SetPlayableArea`; blocker before/after a
later diagnostic) that were each individually correct against §22.6's own constraints and each
individually failed to make the blocker appear. The real cause, found only afterward: a scripted
find-and-replace regenerating the rectangle table silently deleted an adjacent
`local NavmapModifiers = Import("common/navmapModifiers.lua")` line sitting in the overwritten span,
leaving `NavmapModifiers` a nil global — every `NavmapModifiers.GetNavmapModifierIDs(...)` call
then threw, invisibly, for the same reason every other throw in this thread is invisible
(`threads.lua`'s `ResumeThread` swallows it; `Warn()`/`Log()` reach a non-functional F1 console).
No re-ordering could have fixed an `Import`-omission bug. **§22.6 is amended, in place, with a new
binding rule: every blocker-spawning call must be wrapped in its own `pcall`, not merely placed in
a safe position** — ordering reduces the *chance* something upstream fails first; `pcall` removes
the *consequence* if the blocker call itself throws, for any reason, ordering-related or not. Once
every risky call is individually `pcall`-wrapped, the only ordering constraint from §22.6 that
remains genuinely load-bearing is point 1 (`SetPlayableArea` final before instantiation) — a
correctness constraint about which world-state the prefab sees, not an error-propagation one, so
`pcall` cannot substitute for it. This is also recorded as a general cautionary lesson (not specific
to navmap modifiers): a clean re-derivation of "the ordering checks out" is necessary but not
sufficient evidence a fix is correct, and any script-driven regeneration of a Lua data table
(rectangles, scenario tables, anything) should diff the whole function the table lives in, not just
the table itself, since a wide find-and-replace can silently delete adjacent real code with no
diagnostic of any kind. Full text: `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §6.1 (new);
`ARCH_22_06_NewThreadOrderingLaw.md` §22.6 carries the corresponding binding amendment.

**Second and third corrections to the §22 ratification above (2026-08-29, later same day) — one
status update, one ordering-claim retraction, both independently re-verified against the live game
files before being recorded here, not taken on a relayed report's word alone.**

1. **Partial-layer technique now also confirmed shipped.** The human has confirmed the Sea-only
   partial-layer blocker (Technique B, §22.4 / `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §4) working live
   in-game on Pandemonium Isthmus — the same evidentiary bar as §22.3's all-layer technique. Every
   "⚠️ Designed, not yet shipped" framing in the §22 paragraph above, in
   `NAVMAP_MODIFIER_BLOCKER_SPEC.md`, in `ARCH_22_NavmapModifierBlockers.md`, and in
   `ARCH_22_04_PartialLayerBlockerTechnique.md` is stale and has been updated in place at all four
   locations (plus this file's own topic-table row for the spec, above, and `ARCH.md`'s own §22.4
   master-index row). One nuance preserved, not overclaimed: the specific stray-1×1-blocker failure
   mode §22.4's own reasoning describes was never itself deliberately reproduced and observed
   failing — only the working Technique B alternative was built and confirmed; that failure-mode
   description remains reasoned from source, not independently reproduced. Still genuinely open,
   unaffected by this note: promotion of the map-local helper to a shared `common/loading/*.lua`
   engine helper (the "proven twice" bar is not yet met — Technique B has so far been confirmed
   live only once), and combining more than two blocker types on one map (never attempted).

2. **The §22.6/§6 "concrete resolved ordering" claim above was wrong and is retracted.** A direct
   read of the live `Pandemonium Isthmus_data.lua` (not assumed or relayed secondhand) shows the
   actual order inside the shared `NewThread` is `(a)` host-only `SetPlayableArea` ×2, `(b)`
   host-only scenario unit spawning (`pcall`'d), `(c)` the air and sea blocker-spawn calls, each
   independently `pcall`'d, placed LAST specifically so nothing load-bearing follows them — NOT
   "`(b)` blocker before `(d)` unit-spawn" as the §22 paragraph above and
   `ARCH_22_06_NewThreadOrderingLaw.md` previously stated. This also means the earlier claim that
   this order "differs from" `MAP_UNIT_SPAWNING_SPEC.md` §4's illustrative snippet and
   `MAP_SCENARIO_SPEC.md` §3.1's documented live order was itself wrong — the live file's actual
   order **matches** both of those specs' existing text; there was no real discrepancy to flag.
   `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §6, `ARCH_22_06_NewThreadOrderingLaw.md`,
   `MAP_SCENARIO_SPEC.md` §3.1's note, and `MAP_UNIT_SPAWNING_SPEC.md` §4's note have all been
   corrected in place to retract the wrong claim and state the verified order. Root cause: a
   stale, mid-fix debugging state relayed secondhand rather than the live file's actual final
   shape — a reminder that even a same-day "just verified" report should be re-checked against the
   live file directly before being recorded as ground truth here, exactly as this pack's own
   sourcing discipline (§15's `SCEN`/`DATA` citations, `NAVMAP_MODIFIER_BLOCKER_SPEC.md`'s own
   ground-truth table) already requires.

**New `ARCH_14_17_MapAreaFieldLayer.md` §14.17 (2026-08-29) — Map Areas fold into the real
GPU-composited preview blend pipeline as a `PreviewFieldLayer`, not a seventh overlay domain.**
Human-approved, verified against the live composite/canvas code before ratification. States a
**general rule** worth citing beyond Areas: §14's overlay-domain/field-layer separation is a rule
about `Data::PlacementInstances` (*does this re-decide something a PROC stage already resolved?*),
**not** about "anything that is not a `Data::FloatField`" — `StratumSplat` and `Water` already
answer "no" to the latter and are legal, and `MapAreas` joins them as a PARAMS-flattened analytic
per-pixel color source. Concrete shape: `PreviewLayerKind::MapAreas` appended last (+ generated
`PREVIEW_LAYER_MAP_AREAS`), `CompositeBinding::kMapAreaRectangles = 12` (binding 7 stays vacant per
its own documented hole), a 32-byte/8-scalar `PreviewMapAreaRectangle` in **cell space**, **no**
count field added to `PreviewCompositeConfiguration` (it would break the 80-byte std430 stride
mirrored by hand in two `.glsl` units — read `.length()`/`.size()` and push a degenerate sentinel
when empty), an explicit `LayerSourceField` `case … return nullptr;`, and forward-iteration
**last-match-wins** overlap so the visual and §21.8's body hit-test share one Z rule. Blend,
picking and the `.sanmap` schema are all untouched. Ownership move: `AreaColorEntry`'s single
owner becomes `PreviewCompositeSettings::areaColors` (type relocated to a new minimal
`src/ui/AreaColorTable_UI.h` so the settings header does not depend on a tab header). Defaults:
new areas Green / blend Overlay, `"PlayableArea"` pinned Green and non-editable, layer seeded
topmost and enabled via the Areas panel-catalogue row becoming a real
`PreviewVisibilityTarget::FieldLayer` (retiring one of `Application_Visibility_UI.h`'s six inert
`[O]` toggles). **Amends `ARCH_21_08_AreaCanvasGesture.md` §21.8's draw-pass ruling in place**: a
transient `PreviewCompositeSettings::mapAreaSuppressedIndex` omits the dragged area from the
composite input, so a drag/resize costs exactly **two** recomposites (begin + end) instead of one
per frame, and the immediate-mode pass draws only that one area — its border only when the layer
is enabled AND it is suppressed AND it is selected, never at all while the layer is disabled.
`PREVIEW_COMPOSITING_SPEC.md` gains a matching "Map areas are a field layer, not an overlay
domain" section plus a correction recording that its own v1 **SSBO 5** (`area bounds/colors`)
entry is the precedent this restores — SSBO **6**, not 5, was the shadow-sim defect.
