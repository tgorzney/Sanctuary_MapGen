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

**Planned specs** (created as the ARCH Expert populates the pack): per-module code
specs for noise/blend, erosion sims (hydraulic/thermal/fluvial/glacial/snow-melt),
flow/accumulation, masking, placement/scatter, math/SIMD, CPU-GPU dispatch, UI
imgui-bypass, preview compositing; and a deep AI / host-client pass.
