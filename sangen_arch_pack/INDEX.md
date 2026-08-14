# SanGen Pack Index (manifest)

Maps a topic to the spec(s) to load. Read this first, then load ONLY the
spec(s) a question needs — never the whole pack.

| Topic | Spec(s) |
| --- | --- |
| .sanmap file format, import/export, coordinate flip, mapGeneratorData round-trip | `specs/SANMAP_FORMAT_SPEC.md` |
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
| marker/prop/unit scatter, rules & gates, symmetry, prop SoA, scatter determinism | `specs/PLACEMENT_SCATTER_SPEC.md` |
| the canonical CPU/GPU dispatch contract — kernel/backend/policy/resource-manager | `specs/DISPATCH_INTERFACE_SPEC.md` |
| preview compositing — passes, coloring, picking, dirty flags, the shadow-sim fix | `specs/PREVIEW_COMPOSITING_SPEC.md` |
| core math library — SIMD/fast-math/Morton/spatial internals (stub reality + v2 target) | `specs/MATH_SIMD_SPEC.md` |
| future sim passes — fluvial/glacial/snow-melt design on the shared sim framework | `specs/FUTURE_SIM_TYPES_SPEC.md` |
| map AI-analyzability invariants + host/client shared-generation protocol | `specs/AI_HOSTCLIENT_SPEC.md` |

All planned specs are now written. The pack covers the full pipeline end-to-end; the
remaining depth is deep-read follow-ups noted inside individual specs (AI/host/client
lua internals in `AI_HOSTCLIENT_SPEC`; open verification items flagged per spec).

**Standing deferred ruling:** persistent ordered thickness columns + true surface-exposure
derivation (ARCH §7.5, `LAYER_SYSTEM_SPEC` "Known gap") — an M6 DATA-shape work order.
Do not patch it inside a mask or sim work-order.
