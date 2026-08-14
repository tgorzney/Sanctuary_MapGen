# Work-Order M3-8 — pipeline integration (RUN LAST, alone)

*Constitution §7. Milestone M3. Executor: SanGen Coder. **Depends on M3-1…M3-7 all
existing** — run this after the stages, NOT in parallel with them (it edits the shared
CMake + the pipeline assembler).*

## Title
Wire the stages into `Generation_PIPELINE`, add them to the build, and prove the whole
pipeline end-to-end.

## Root problem
The seven M3 stages are self-contained but not yet assembled. This is the single place
that touches shared files (CMake, the pipeline assembler) — kept as one work-order so the
parallel stage work-orders never collide on it.

## Target files
- `CMakeLists.txt` — add all `src/**` (math/data/params/proc/pipeline/sys/io) files +
  their tests.
- `src/pipeline/GenerationAssembler_PIPELINE.h/.cpp` (new) — builds a `GenerationPipeline`
  with the stages registered in order: Noise/Blend → Mask → Erosion → Thermal →
  Flow/Accumulation → Placement → Bake, each with its `computeParamHash` + dispatch.
- `src/pipeline/GenerationAssembler_PIPELINE_Test.cpp` — end-to-end.

## Layer & accuracy
`PIPELINE`. Assembles PROC stages behind the dirty-hash conductor + dispatch.

## Contract
- Register each stage's `Run` (via `Sys::Dispatch` with its default `DispatchPolicy`,
  ARCH §4.2) and `computeParamHash` (over that stage's PARAMS).
- Add the two-tier dirty distinction stub (full-regen vs preview-only) for M4 to build on.
- Everything in `src/` compiles into the target; run each module's `*_Test`.

## Acceptance (end-to-end)
From a `MapRecipe` + empty `MapFields`, one `Run()` produces a populated heightfield →
masks → eroded/thermal/flow → placement → bake, with sane values; a second `Run()` with
no change skips all stages; changing one layer's frequency re-runs from that stage
onward; all `*_Test` binaries pass; builds clean in MSVC.

## Out of scope
UI wiring (M4/M5); the `.sanmap` texture export (IO).
