# ARCH Expert — resume notes

Where the pack stands and what to do next. (Read CONSTITUTION + INDEX first; the
Setup Plan holds Appendix A's code hit-list.)

## Status: pack + ARCH COMPLETE; SPEC-4 (.sanmap schema v3) ratified
All planned specs are written and committed to the repo (`sangen_arch_pack/`). The
ARCH Expert charter is live at `.claude/agents/sangen-arch-expert.md`. **`ARCH.md` is
now fully authored and ratified** (7 naming-law subsections incl. the schema v3 casing
law §1.6 and the IO migration-file naming law §1.7; layer→dir map; module boundaries;
dispatch contract; god-object dismemberment; rebuild order; M3/M4 rulings).
Constitution updated (7th layer `PIPELINE`; SYS slimmed to runtime primitives; DATA =
computed output vs PARAMS = adjustable settings; TBD markers resolved).

Work-order `SPEC-4` (`work_orders/SPEC-4_SanmapSchemaV3_DOCS.md`) is **ratified**: the
`.sanmap` format's `mapGeneratorData` blob is replaced by independently-versioned,
top-level, format-sibling SanGen-owned sections (`GeneralMapSettings`,
`HeightmapStack`, `Symmetry`, `SlopeDefaults`, `Flow`, `Accumulation`, `MarkersStack`/
`PropsStack`/`DecalsStack`/`UnitsStack`, `DetailNormal`, `SanGenVersion`) — see
`SANMAP_FORMAT_SPEC`'s "SanGen-owned sections (schema v3)". The `.sanmap` version-gating
migration mechanism (`SanGenVersion`, per-(domain,version-step) migration files, the
manifest, the runner, `JsonPrimitives_IO`) is ratified in the new `IO_MIGRATION_SPEC`.
**Both are docs-only rulings** — `src/io/*`/`src/params/*` still implement the old
`mapGeneratorData` shape; implementing the schema v3 change (and the migration
mechanism that gates it) is coder-tier work for a **future** work-order, not done yet.

Next step remains **implementing M0** (the foundation milestone) — or, if SPEC-4's
schema is prioritized first, a new coder work-order against `src/io/*`/`src/params/*`
built from `SANMAP_FORMAT_SPEC` + `IO_MIGRATION_SPEC`.

## Specs in the pack (21, all current)
- Format/data: `SANMAP_FORMAT_SPEC`, `IO_MIGRATION_SPEC`, `UNIT_PROP_MARKER_DATA_SPEC`,
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
- **`SanGenVersion` must actually gate imports** (it does not today —
  `MapGeneratorDataVersion` is written, never read). `IO_MIGRATION_SPEC` is the ratified
  mechanism: one `<Domain>_Migrate_V<N>_IO` file per (domain, version-step), a single
  `Sanmap_MigrationManifest_IO` wiring point per version bump, a `Sanmap_MigrationRunner_IO`
  that refuses a too-new document rather than best-effort-parsing it, and a shared
  `JsonPrimitives_IO.h` toolkit (also the fix for the mis-homed `ReadJson*` accessors
  currently stuck in `MapImporter_Recipe_IO.h`).

## Known fix-targets (from import/export & surveys)
- Exporter writes identity quaternions — rotation conversion unimplemented.
- Props export disabled (outdated prop formats fail loading) — needs fixing.
- Coordinate flip `world.z = length - z - 1` on export / inverted on import.
- Hardcoded absolute shader paths (`D:/Projects/...`); per-dispatch shader recompile;
  N ad-hoc `UseGPUx` bools; `rand()` non-determinism; mislabeled AoS-as-SoA props.
- `mapGeneratorData` (~40 keys, ~60% duplicate, two incompatible dialects, no working
  version gate) — superseded by the schema v3 sections; not yet implemented in `src/io/*`.

## Open items to confirm with dev
- Whether the ×10 (game-unit↔meters) or any scaling touches the heightmap texture
  itself, or only entity scale/placement (leaning: only scale authoring).
- tint_geometry.tga channel layout (was login-walled).
- Deep AI/host/client/systems lua read — only if pursuing custom AI/shared-gen depth.

## Next: implement the v2 rebuild (ARCH §6 milestones)
ARCH is authored — the work is now execution, bottom-up:
- **M0 Foundation**: `MATH` library + `SYS` primitives (`Dispatch_SYS`, `GpuResource_SYS`).
- **M1 Data model**: `*_DATA`/`*_PARAMS` replacing `GenerationParams`; delete dead
  `core/data/*`; `.sanmap` schema v3 round-trip (`SANMAP_FORMAT_SPEC`,
  `IO_MIGRATION_SPEC`) replacing the `mapGeneratorData` blob.
- **M2 Dispatch + PIPELINE skeleton**: vertical slice on noise, both backends.
- **M3 PROC stages**: each built as a complete CPU+GPU pair, parity-checked together.
- **M4 Preview/WYSIWYG**, **M5 UI + assets**, **M6 determinism/future-sims/AI-host**.
Coders execute schema-valid work-orders (Constitution §7); the ARCH Expert owns any
further ARCH changes.
