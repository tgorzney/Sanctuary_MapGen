# SanGen Pack Index (manifest)

Maps a topic to the spec(s) to load. Read this first, then load ONLY the
spec(s) a question needs — never the whole pack.

| Topic | Spec(s) |
| --- | --- |
| .sanmap file format, import/export, coordinate flip, schema v3 top-level sections | `specs/SANMAP_FORMAT_SPEC.md` |
| .sanmap schema version migrations — SanGenVersion gating, the migration runner/manifest, JSON transform primitives | `specs/IO_MIGRATION_SPEC.md` |
| units / props / markers, tpId scheme, factions, asset validation, .san* formats | `specs/UNIT_PROP_MARKER_DATA_SPEC.md` |
| map scripting & events, lua sandbox, Tags, AI system, modding, validators | `specs/MODDING_SCRIPTING_SPEC.md` |
| the Map Scenario system — `<MapName>_data.lua`/`<MapName>_Scenarios_Script.lua` file split, module API contract, three-tier scenario matching, `alloyMode` semantics, the mandatory-`spawns` hard requirement, execution/timing law, SanGen IO scope ruling | `specs/MAP_SCENARIO_SPEC.md` |
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
| marker/prop/unit scatter, rules & gates, symmetry (incl. Radial N-fold, ARCH §13), prop SoA, scatter determinism, global marker icon/color/scale defaults | `specs/PLACEMENT_SCATTER_SPEC.md` |
| pass-through entity PARAMS — armies/unit groups/unit transforms/map areas AND resolved/baked markers/props/decals/marker chains, incl. manual prop/decal layer authoring (`Params::Army`, `UnitGroup`, `UnitTransform`, `MapArea`, `InstancedTransform`, `MarkerInstanceGroup`, `MarkerTransform`, `PropInstanceGroup`, `PropTransform`, `DecalInstanceGroup`, `DecalTransform`, `PropInstanceLayer`, `DecalInstanceLayer`, `MarkerChain`, `ChainMarker`), distinct from procedural scatter rules; also the ratified export-time `blueprintPath` "warn, never block" ruling | `specs/ENTITY_AUTHORING_PARAMS_SPEC.md` |
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
in-game 2026-08-20) SanGen Map Scenario system as first-class law: the `<MapName>_data.lua`/
`<MapName>_Scenarios_Script.lua` file split, the `Scenario.ResolveAndApply`/
`Scenario.SpawnNavalFleets` module contract, the three-tier (`PATTERN_SCENARIOS`/
`COUNT_SCENARIOS`/`DEFAULT_SCENARIO`) matching system, the four `alloyMode` values, the
hard requirement that every scenario needing deterministic spawns declares an explicit
`spawns` table (the `.sanmap`'s one-spawn-transform-per-army shared-state failure mode
discovered live the same day), the execution/timing law, and a ruling that SanGen
Import/Export of the scenario file remains in scope but is reclassified as a distinct
IO surface (a script-tree `.lua` companion file, not a `.sanmap`-package JSON section) —
see ARCH §15. This consolidated and superseded the "what to build" content previously
inline in `MODDING_SCRIPTING_SPEC.md`'s "Scenario-script file split" section, which now
holds only that section's investigation trail (the disproven cross-tree-`Import()`
hypothesis).

⚠️ **Found during the ARCH §14 authoring pass, not fixed here:** this paragraph's own
"see ARCH §15" is a forward reference that does not yet resolve — `ARCH.md` currently ends
at §14 (the overlay-layering ruling below), with no §15 section present. The Map Scenario
IO-surface ruling this paragraph describes appears never to have been written into `ARCH.md`
itself, only recorded here in `INDEX.md`. This is a pre-existing gap, unrelated to the
overlay-layering ratification, and is out of scope for this pass to close — flagged for
whoever next touches the Map Scenario topic.

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
color-texture readback defect. Full ruling text: `ARCH.md` §14. Several items are explicitly
**left open** by this ratification, not resolved (`ARCH.md` §14.13,
`PREVIEW_COMPOSITING_SPEC.md`'s matching list): real footprint-size data source; the
cross-layer budget default and Tier B per-resolution costs (both pending a real benchmark);
whether a stable id column exists for manual sub-layers; whether Decals actually route
through `Data::PlacementInstances` today; and whether `OverlayLayer_UI::blendMode` reuses
`Ui::PreviewBlendMode` or needs a new enum (UI Expert's call).

**Standing deferred ruling:** persistent ordered thickness columns + true surface-exposure
derivation (ARCH §7.5, `LAYER_SYSTEM_SPEC` "Known gap") — an M6 DATA-shape work order.
Do not patch it inside a mask or sim work-order.

**Fixed since ARCH §13:** `DecalRule` (`src/params/ScatterRule_PARAMS.h`) now carries the
`bSymmetryUseGlobal`/`symmetryMask` pair, and `AppendDecalRules` resolves a symmetry mask for
decals via `ResolveSymmetryMask` — see `SANMAP_FORMAT_SPEC` Correction 4 and
`PLACEMENT_SCATTER_SPEC` (Defect 1, now closed).

**Standing recorded defects awaiting a coder work-order (not yet fixed):**
- `Params::symmetryOrbitMaximum = 16` (`src/params/Symmetry_PARAMS.h`) can silently overflow
  once a designer-chosen `radialSymmetryRepeatCount` combines with mirror axes — see
  `SANMAP_FORMAT_SPEC` Correction 4 and `PLACEMENT_SCATTER_SPEC` (Defect 2).
- `ComposeOnGpu()` (`PreviewComposite_Gpu_UI.cpp:78-81`) unconditionally reads back the full
  color texture even when nothing downstream consumes it on the GPU-resident hot path — up to
  256MB wasted PCIe transfer + blocking wait at the 8192² cap, every recompose. Narrow, already
  diagnosed, independent of the ARCH §14 overlay redesign; should land before it. See
  `PREVIEW_COMPOSITING_SPEC.md` / `ARCH.md` §14.10.
