# Work-Order M4-5 — preview integration + two-tier dirty flags (RUN LAST, alone)

*Constitution §7. Milestone M4. **SEQUENTIAL — run last, single agent.** Depends on
M4-0a, M4-0b, M4-1…M4-4. Edits shared files (`Generation_PIPELINE` / assembler, CMake) —
never in parallel with the others. Executor: SanGen Coder.*

## Title
Wire the composite + picking behind the two-tier dirty model, driven off the DAG.

## Root problem
`UI_FRAMEWORK_SPEC` / `ARCH_14_08_DirtyFlagTiers.md` §14.8 (corrected 2026-08-22 — was
previously miscited as §6.1, a PROC-stage completion checklist unrelated to this claim): a
cheap visual tweak must not trigger a full regen. Two
flags, **derived from the dependency DAG, not by hand**:
- `bNeedsMapUpdate` → full `GenerationPipeline::Run()` (heightfield → … → bake).
- `bNeedsPreviewRender` → `PreviewComposite` only (recolor/recomposite the existing bake).

## Target files
- `src/pipeline/GenerationAssembler_PIPELINE.*` (or a small `PreviewDriver_PIPELINE.*`) —
  own the two flags + which one each parameter trips, derived from stage ownership; and
  call `Data::SpatialGrid::Build(...)` immediately after the Placement stage (ARCH_08_03_SpatialGridVsSpacingGrid.md §8.3:
  PIPELINE is the grid's single writer).
- `CMakeLists.txt` — add `src/params/GradientRamp_PARAMS*`, `src/data/SpatialGrid_DATA*`,
  `src/data/EntityIdBuffer_DATA*`, `src/ui/*` (M4-2…M4-4 files) + their tests.
- End-to-end test file.

## Layer & accuracy
`PIPELINE` drives; `UI` composites. GPU via `GpuResource_SYS`.

## Contract
- A parameter that feeds a PROC stage trips `bNeedsMapUpdate`; a pure-visual control
  (gradient ramp / tint / preview-resolution) trips only `bNeedsPreviewRender`. Derive the
  mapping from which stage owns the field the parameter feeds — no hand-maintained
  per-widget list.
- On `bNeedsMapUpdate`: run the pipeline (Placement → grid `Build`), then one composite.
  On `bNeedsPreviewRender`: composite only. Preview **samples the bake** (WYSIWYG) —
  assert it never re-runs a sim.

## Acceptance (end-to-end)
Change a layer frequency → `bNeedsMapUpdate` → pipeline re-runs from that stage → composite
updates. Change a gradient ramp color → `bNeedsPreviewRender` only → composite updates,
**no** pipeline re-run (assert stage run-count unchanged) and the spatial grid is **not**
rebuilt. A click resolves the right entity (picking) against the grid the pipeline built.
Preview matches the bake. All `*_Test` pass; builds clean in MSVC.

## Out of scope
The full widget library + tabs + MapCanvas (M5); asset atlas/cache (M5); serializing
gradient ramps into `mapGeneratorData` (needs its own IO work-order).
