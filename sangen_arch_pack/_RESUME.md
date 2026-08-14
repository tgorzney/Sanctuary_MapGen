# ARCH Expert — resume notes

Where the pack stands and what to do next. (Read the Setup Plan + CONSTITUTION
+ INDEX first.)

## Done
- **Constitution** seeded (layers, naming precedents, GPU/accuracy standard,
  input/asset-safety pillar, work-order schema). TBD items marked inside.
- **Specs written:**
  - `SANMAP_FORMAT_SPEC` — .sanmap JSON (v3, 9 strata), entities, validated on
    ~23 official maps; terrain lives in `Textures/` not the JSON; `mapGeneratorData`
    round-trips the full generator state (= GenerationParams on disk).
  - `UNIT_PROP_MARKER_DATA_SPEC` — factions Chosen/Guard/EDA (army 0/1/2),
    authoritative 7-char tpId scheme, unit template model, `.san*` formats,
    asset-validation requirement.
  - `MODDING_SCRIPTING_SPEC` — lua sandbox, map lifecycle/event API, Tags system,
    AI mods (`AI/mods/<name>`), AIMarkerGenerator invariants.
- **Code survey** captured in Setup Plan Appendix A (god object, dead data model,
  CPU/GPU twins, preview shadow logic).

## Next (deep passes still needed)
1. Per-module code specs from `core/` + `gui/` + `shaders/`:
   - DONE: PARAMS_PIPELINE_SPEC (data model + Blend→Erosion→Flow→Placement
     pipeline; live model = params/Params_*, dead = data/*).
   - DONE: LAYER_SYSTEM_SPEC — the v2 height/material layer design (authored with
     owner): author-in-height / simulate-in-thickness, GeoLayers (Material vs
     Shaper mode), sim layers per sim type, additive-thickness volume, baking,
     stratum masks (8 masks + base). Erosion becomes its own layer type in v2.
   - DONE: SIM_ALGORITHMS_SPEC (hydraulic droplet, thermal talus, flow; already
     layer/material-aware; CPU/GPU diverge — parity is the rework).
   - DONE: OPTIMIZATION_REVIEW + OPTIMIZATION_PILLARS (HPC law), DETERMINISM_SPEC
     (optional cross-machine competitive gen), UI_FRAMEWORK_SPEC (imgui-bypass).
   - TODO: noise/blend, masking, placement/scatter, the unified CPU-GPU dispatch
     interface, preview compositing; future sim types (fluvial/glacial/snow-melt).

## Open design items (from LAYER_SYSTEM_SPEC)
- tint_geometry.tga channel layout (login-walled resource — owner to supply).
- Stratum chosen by an Add/raise Shaper GeoLayer (confirm).
- (Resolved: multi-Material-GeoLayer combine = the global Separate/Unified sim toggle.)
2. Deep AI/host/client/systems read (AIFunctions 233KB, platoon functions 614KB)
   — only if pursuing custom AI; tangential to core map-gen.

## Known fix-targets (from import/export)
- Exporter writes identity quaternions — rotation conversion is unimplemented.
- Props export is disabled (outdated prop formats fail loading) — needs fixing.
- Coordinate flip `world.z = length - z - 1` must be applied on export / inverted
  on import.

## Then: author the ARCH
With the user, work Appendix A's hit-list into `ARCH.md`: naming law + file-size
ceilings, the layer boundaries, the CPU/GPU dispatch field names/defaults, and
the god-object dismemberment plan. This is the interactive, decision-heavy step.
