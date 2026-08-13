# SanGen Pack Index (manifest)

Maps a topic to the spec(s) to load. Read this first, then load ONLY the
spec(s) a question needs — never the whole pack.

| Topic | Spec(s) |
| --- | --- |
| .sanmap file format, import/export target, map entities on disk, mapGeneratorData round-trip | `specs/SANMAP_FORMAT_SPEC.md` |
| units / props / markers, tpId scheme, factions, unit catalog, resource spots, areas, asset validation, .san* formats | `specs/UNIT_PROP_MARKER_DATA_SPEC.md` |
| map scripting & events, lua sandbox, Tags system, AI system, modding, blueprint validators | `specs/MODDING_SCRIPTING_SPEC.md` |

**Planned specs** (created as the ARCH Expert populates the pack): terrain /
noise, erosion (hydraulic / thermal / flow), masking, placement / scatter,
math / SIMD, CPU-GPU dispatch, UI imgui-bypass, preview, params (the
GenerationParams / mapGeneratorData data model), and a deep AI / host-client
pass.
