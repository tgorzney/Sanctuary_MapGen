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

**Planned specs** (created as the ARCH Expert populates the pack): noise/blend,
masking, placement/scatter, math/SIMD, CPU-GPU dispatch interface, UI imgui-bypass,
preview compositing; the future sim types (fluvial/glacial/snow-melt); and a deep
AI / host-client pass.
