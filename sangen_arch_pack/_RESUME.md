# ARCH Expert — resume notes

Where the pack stands and what to do next. (Read CONSTITUTION + INDEX first; the
Setup Plan holds Appendix A's code hit-list.)

## Status: pack + ARCH COMPLETE; SPEC-4 (.sanmap schema v3) ratified

## CORRECTION (post-hoc, supersedes the "Latest ratified session" entry below)
**ARCH §7.2 item 5 was wrong and has been corrected.** The Generator Expert, after
independently verifying the evidence against the real code, ruled that there is **no**
per-stratum surface-weight remap in the Mask stage — or anywhere in SanGen generation.
`Params::Stratum::maskRemapMinimum`/`maskRemapMaximum` is per-stratum material/
appearance **pass-through data**, consumed only by the game's own renderer against the
stratum's composite/"mask" texture — a real texture asset distinct from the
`stratums_1_4/5_8.tga` splat-weight files the Mask stage produces. No SanGen generation
stage reads or writes it today. See `ARCH_07_02_MaterialProportionVsSurfaceWeight.md` §7.2 item 5 (rewritten) and
`MASKING_SPEC` §1.6/§1.7 (corrected). Two direct consequences for the entries below,
both already applied at the cited locations:
- The "Explicitly does NOT reopen item 5 ('remap runs once, in Mask')" framing in the
  session entry just below is **stale** — item 5 no longer says that. §7.2 item 10's
  field-*shape* ruling (4-component, not scalar) is unaffected and still stands.
- The "Open items to confirm with dev" bullet about how the Mask kernel should consume
  a 4-component remap window is **CLOSED**, not open: the Mask kernel does not consume
  this field at all, so there is nothing to wire.
A future work-order may eventually wire `maskRemapMinimum`/`maskRemapMaximum` to a real
consumer (most plausibly composite/mask-texture processing inside Bake); not designed
yet.

All planned specs are written and committed to the repo (`sangen_arch_pack/`). The
ARCH Expert charter is live at `.claude/agents/sangen-arch-expert.md`. **`ARCH.md` is
now fully authored and ratified** (7 naming-law subsections incl. the schema v3 casing
law §1.6 and the IO migration-file naming law §1.7; layer→dir map; module boundaries;
dispatch contract; god-object dismemberment; rebuild order; M3/M4 rulings; §9/§10/§11
add `Params::Army`/`UnitGroup`/`UnitTransform`/`MapArea`, `Params::Atmosphere`, and
`Params::GlobalMarkerSettings`).
Constitution updated (7th layer `PIPELINE`; SYS slimmed to runtime primitives; DATA =
computed output vs PARAMS = adjustable settings; TBD markers resolved).

Work-order `SPEC-4` (`work_orders/SPEC-4_SanmapSchemaV3_DOCS.md`) is **ratified**: the
`.sanmap` format's `mapGeneratorData` blob is replaced by independently-versioned,
top-level, format-sibling SanGen-owned sections (`GeneralMapSettings`,
`HeightmapStack`, `Symmetry`, `SlopeDefaults`, `Flow`, `Accumulation`, `MarkersStack`/
`PropsStack`/`DecalsStack`/`UnitsStack`, `DetailNormal`, `SanGenVersion`,
`StratumGenerationSettings`) — see `SANMAP_FORMAT_SPEC`'s "SanGen-owned sections
(schema v3)" (Corrections 1-13). The `.sanmap` version-gating migration mechanism
(`SanGenVersion`, per-(domain,version-step) migration files, the manifest, the runner,
`JsonPrimitives_IO`) is ratified in the new `IO_MIGRATION_SPEC`.
**All are docs-only rulings** — `src/io/*`/`src/params/*` still implement the old
`mapGeneratorData` shape (plus the current, incomplete `stratumLayers` write); the
schema v3 change (and the migration mechanism that gates it) is coder-tier work for a
**future** work-order, not done yet.

## Latest ratified session — `Params::Stratum` IO ruling (3 parts, ARCH.md + 2 specs)
- **ARCH §7.2 item 10 (amendment):** `maskRemapMinimum`/`maskRemapMaximum` widen from
  `float` to `float[kStratumColorChannelCount]` (4-component) — confirmed against
  `SanMap.Types.cs::Stratum.maskRemapMin/Max` (both `Vector4`). At the time of this
  session, believed to not reopen item 5 ("remap runs once, in Mask") or §7.1 ("no
  rival per-stratum settings type") — a field-*shape* correction on the one existing
  field, not a new type. **[SUPERSEDED by the CORRECTION note at the top of this file:
  item 5 itself was later found wrong and rewritten — there is no remap site at all.
  This amendment's field-shape ruling (4-component, not scalar) is unaffected and still
  stands.]** How the Mask kernel's scalar-per-cell surface weight consumes a 4-channel
  window was left open here — **now CLOSED**: the Mask kernel does not consume this
  field, full stop.
- **`SANMAP_FORMAT_SPEC` Correction 12:** new top-level `StratumGenerationSettings[9]`
  (index-aligned with `stratumLayers[9]`) carrying `Params::Stratum::soilPhysics`'s 6
  fields + `bSlopeUseGlobal` + the 7 slope-gate fields — no new C++ type, a zero-cost
  relocation of the doomed `mapGeneratorData.Stratums` block's already-working 8-field
  read/write, plus 7 new writes (soil physics + `SlopeUseGlobal`) for fields that exist
  on `Params::Stratum` today but nothing serializes yet.
- **`SANMAP_FORMAT_SPEC` Correction 13:** `stratumLayers` appearance wiring fix — v2
  currently has **zero** `stratumLayers` importer, and the exporter writes empty
  albedo/normal/mask paths, a wrong `tileSizeFar` (reuses the near `tileCount`), and
  omits 7 fields entirely (`tileSizeTriplanar`, `tileSizeFarTriplanar`, `normalScale`,
  `normalScaleFar`, `normalFarNearBlend`, `heightFarNearBlend`, `farColorRemap`).
  Flags (does not fix) a dead/contradictory `StratumAppearance::diffuseRemapColor`
  field for coder deletion, and flags (does not resolve) `importedMaskMode`/`bEnabled`
  as fields with no ratified format home once `mapGeneratorData` is deleted.
- **`MASKING_SPEC` §1.7** gained a one-line cross-reference to Correction 12 (states
  the PARAMS-side default/override *mechanism*; Correction 12 states the IO shape).
- **Still docs-only** — `src/params/Stratum_PARAMS.h`, `src/io/MapExporter_*_IO.cpp`,
  `src/io/MapImporter_Recipe_IO.cpp` are unchanged; implementing all three parts is
  coder-tier work for a future work-order.

Next step remains **implementing M0** (the foundation milestone) — or, if SPEC-4's
schema is prioritized first, a new coder work-order against `src/io/*`/`src/params/*`
built from `SANMAP_FORMAT_SPEC` + `IO_MIGRATION_SPEC` (now including this session's
Corrections 12-13 and the ARCH §7.2 item 10 field-shape amendment).

## Specs in the pack (24, all current)
- Format/data: `SANMAP_FORMAT_SPEC`, `IO_MIGRATION_SPEC`, `UNIT_PROP_MARKER_DATA_SPEC`,
  `GAMEDATA_LAYOUT_SPEC`, `ASSET_LOADING_SPEC`, `MODDING_SCRIPTING_SPEC`,
  `ENTITY_AUTHORING_PARAMS_SPEC`, `ATMOSPHERE_PARAMS_SPEC`.
- Pipeline: `PARAMS_PIPELINE_SPEC`, `LAYER_SYSTEM_SPEC`, `NOISE_BLEND_SPEC`,
  `MASKING_SPEC`, `SIM_ALGORITHMS_SPEC`, `PLACEMENT_SCATTER_SPEC`,
  `PREVIEW_COMPOSITING_SPEC`.
- Systems/perf: `OPTIMIZATION_REVIEW`, `OPTIMIZATION_PILLARS`, `MATH_SIMD_SPEC`,
  `DISPATCH_INTERFACE_SPEC`, `UI_FRAMEWORK_SPEC`, `DETERMINISM_SPEC`.
- Future: `FUTURE_SIM_TYPES_SPEC` (fluvial/glacial/snow-melt), `AI_HOSTCLIENT_SPEC`.
(See `INDEX.md` for the authoritative, currently-maintained topic → spec map; the list
above is a coarse inventory, not a substitute for it.)

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
- `stratumLayers[9]` appearance data — exporter writes it mostly blank (see Correction
  13); **no importer exists at all**. Fix targets are itemized field-by-field there.

## Open items to confirm with dev
- Whether the ×10 (game-unit↔meters) or any scaling touches the heightmap texture
  itself, or only entity scale/placement (leaning: only scale authoring).
- tint_geometry.tga channel layout (was login-walled).
- Deep AI/host/client/systems lua read — only if pursuing custom AI/shared-gen depth.
- ~~How the Mask kernel's per-cell scalar surface weight should consume the newly
  4-component `maskRemapMinimum`/`maskRemapMaximum` (ARCH §7.2 item 10)~~ — **CLOSED**
  by the CORRECTION at the top of this file: the Mask kernel does not consume this
  field at all. Removed as an open item.

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
