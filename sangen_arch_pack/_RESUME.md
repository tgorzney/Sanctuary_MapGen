# ARCH Expert — resume notes

Where the pack stands and what to do next. (Read CONSTITUTION + INDEX first; the
Setup Plan holds Appendix A's code hit-list.)

## Status: pack + ARCH COMPLETE
All planned specs are written and committed to the repo (`sangen_arch_pack/`). The
ARCH Expert charter is live at `.claude/agents/sangen-arch-expert.md`. **`ARCH.md` is
now fully authored and ratified** (6 sections: naming law/suffixes, layer→dir map,
module boundaries, dispatch contract, god-object dismemberment, rebuild order).
Constitution updated (7th layer `PIPELINE`; SYS slimmed to runtime primitives; DATA =
computed output vs PARAMS = adjustable settings; TBD markers resolved). Next step is
**implementing M0** (the foundation milestone).

## Specs in the pack (20, all current)
- Format/data: `SANMAP_FORMAT_SPEC`, `UNIT_PROP_MARKER_DATA_SPEC`,
  `GAMEDATA_LAYOUT_SPEC`, `ASSET_LOADING_SPEC`, `MODDING_SCRIPTING_SPEC`.
- Pipeline: `PARAMS_PIPELINE_SPEC`, `LAYER_SYSTEM_SPEC`, `NOISE_BLEND_SPEC`,
  `MASKING_SPEC`, `SIM_ALGORITHMS_SPEC`, `PLACEMENT_SCATTER_SPEC`,
  `PREVIEW_COMPOSITING_SPEC`.
- Systems/perf: `OPTIMIZATION_REVIEW`, `OPTIMIZATION_PILLARS`, `MATH_SIMD_SPEC`,
  `DISPATCH_INTERFACE_SPEC`, `UI_FRAMEWORK_SPEC`, `DETERMINISM_SPEC`.
- Future: `FUTURE_SIM_TYPES_SPEC` (fluvial/glacial/snow-melt), `AI_HOSTCLIENT_SPEC`.

## Key facts locked this pass
- **Entity positions = absolute world/game units** (not fractions). Map `height` =
  terrain vertical extent; a prop with Y > height floats above all terrain. A 0–1 UI
  height converts via `× maxHeight` before storage. `maxHeight` must be read from the
  map, never the hardcoded `128`. The "1 unit ≈ 10 m" ratio is an arbitrary
  scale-authoring convention, NOT coordinate math. (See `SANMAP_FORMAT_SPEC`.)
- **Unit thumbnails ARE stored** (`UI/Sprites/Icons/Units/<tpId>.dds`, 64² DXT5,
  load direct); only **prop** thumbnails must be rendered on demand + cached.
- **Cross-cutting v2 theme (the ARCH's spine):** the compute layer is doubled and
  divergent — CPU vs GPU write each kernel twice with different math, the preview
  re-simulates instead of sampling the bake, `core/data/*` is a dead duplicate
  island, and several `Gen_*` bodies are declared-but-unimplemented. Fix = one kernel
  per stage + one dispatch contract (`DISPATCH_INTERFACE_SPEC`) + one source of truth.

## Known fix-targets (from import/export & surveys)
- Exporter writes identity quaternions — rotation conversion unimplemented.
- Props export disabled (outdated prop formats fail loading) — needs fixing.
- Coordinate flip `world.z = length - z - 1` on export / inverted on import.
- Hardcoded absolute shader paths (`D:/Projects/...`); per-dispatch shader recompile;
  N ad-hoc `UseGPUx` bools; `rand()` non-determinism; mislabeled AoS-as-SoA props.

## Open items to confirm with dev
- Whether the ×10 (game-unit↔meters) or any scaling touches the heightmap texture
  itself, or only entity scale/placement (leaning: only scale authoring).
- tint_geometry.tga channel layout (was login-walled).
- Deep AI/host/client/systems lua read — only if pursuing custom AI/shared-gen depth.

## Next: implement the v2 rebuild (ARCH §6 milestones)
ARCH is authored — the work is now execution, bottom-up:
- **M0 Foundation**: `MATH` library + `SYS` primitives (`Dispatch_SYS`, `GpuResource_SYS`).
- **M1 Data model**: `*_DATA`/`*_PARAMS` replacing `GenerationParams`; delete dead
  `core/data/*`; `mapGeneratorData` round-trip.
- **M2 Dispatch + PIPELINE skeleton**: vertical slice on noise, both backends.
- **M3 PROC stages**: each built as a complete CPU+GPU pair, parity-checked together.
- **M4 Preview/WYSIWYG**, **M5 UI + assets**, **M6 determinism/future-sims/AI-host**.
Coders execute schema-valid work-orders (Constitution §7); the ARCH Expert owns any
further ARCH changes.
