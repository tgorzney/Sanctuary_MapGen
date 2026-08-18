# SanGen Pack Index (manifest)

Maps a topic to the spec(s) to load. Read this first, then load ONLY the
spec(s) a question needs — never the whole pack.

| Topic | Spec(s) |
| --- | --- |
| .sanmap file format, import/export, coordinate flip, schema v3 top-level sections | `specs/SANMAP_FORMAT_SPEC.md` |
| .sanmap schema version migrations — SanGenVersion gating, the migration runner/manifest, JSON transform primitives | `specs/IO_MIGRATION_SPEC.md` |
| units / props / markers, tpId scheme, factions, asset validation, .san* formats | `specs/UNIT_PROP_MARKER_DATA_SPEC.md` |
| map scripting & events, lua sandbox, Tags, AI system, modding, validators | `specs/MODDING_SCRIPTING_SPEC.md` |
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
| preview compositing — passes, coloring, picking, dirty flags, the shadow-sim fix | `specs/PREVIEW_COMPOSITING_SPEC.md` |
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
`bSymmetryUseGlobal`/`symmetryMask` override pair (it does not — recorded as a defect,
`PLACEMENT_SCATTER_SPEC`'s "Known issues" addendum, alongside a second recorded defect: the
16-slot symmetry-orbit buffer can now silently overflow under a large radial count).

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

**Standing deferred ruling:** persistent ordered thickness columns + true surface-exposure
derivation (ARCH §7.5, `LAYER_SYSTEM_SPEC` "Known gap") — an M6 DATA-shape work order.
Do not patch it inside a mask or sim work-order.

**Standing recorded defects awaiting a coder work-order (not yet fixed):**
- `DecalRule` (`src/params/ScatterRule_PARAMS.h`) has no `bSymmetryUseGlobal`/`symmetryMask`
  pair, and `AppendDecalRules` never resolves a symmetry mask for decals at all — see
  `SANMAP_FORMAT_SPEC` Correction 4 and `PLACEMENT_SCATTER_SPEC`.
- `Params::symmetryOrbitMaximum = 16` (`src/params/Symmetry_PARAMS.h`) can silently overflow
  once a designer-chosen `radialSymmetryRepeatCount` combines with mirror axes — see the same
  two specs.
