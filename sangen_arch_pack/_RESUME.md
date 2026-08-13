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
1. Per-module code specs from `core/` + `gui/` + `shaders/`: terrain synth,
   erosion (hydraulic/thermal/flow), masking, placement/scatter, math/SIMD,
   CPU-GPU dispatch, UI imgui-bypass, preview, and the PARAMS/GenerationParams
   model (reconcile the two data-model families).
2. Deep AI/host/client/systems read (AIFunctions 233KB, platoon functions 614KB)
   — only if pursuing custom AI; tangential to core map-gen.
3. Read leftover `SanMap File Format/MapUtils.cs` + `Colors.cs` (save/load +
   palette) to finish the import/export spec.

## Then: author the ARCH
With the user, work Appendix A's hit-list into `ARCH.md`: naming law + file-size
ceilings, the layer boundaries, the CPU/GPU dispatch field names/defaults, and
the god-object dismemberment plan. This is the interactive, decision-heavy step.
