# Work-Order M3-8 (REWORK) — pipeline integration (RUN LAST, alone)

*Constitution §7. Milestone M3. **Supersedes the prior M3-8** — the stage order changed
(ARCH §7.4). Executor: SanGen Coder. Depends on M3-1…M3-7 all existing (M3-2 reworked).
Edits shared files (CMake, assembler) — never run in parallel with the stages.*

## Title
Assemble the stages into `Generation_PIPELINE` in the ratified order, wire consumers to
the right field, build, and prove the pipeline end-to-end.

## Ratified stage order (ARCH §7.4 — supersedes the old order)
```
NoiseBlend → Erosion → Thermal → FlowAccumulation → Mask → Placement → Bake
```
Mask moves **after all sims** (gate sees final slope/proportions) and **before
Placement/Bake** (both consume resolved surface weights). Any future sim inserts before Mask.

## Target files
- `CMakeLists.txt` — add all `src/**` files + tests.
- `src/pipeline/GenerationAssembler_PIPELINE.h/.cpp` — register the 7 stages in the order
  above, each with `computeParamHash` (over its PARAMS) + `Sys::Dispatch` (its default
  `DispatchPolicy`, §4.2). Mask sits in the **Output Exact chain** (§4.6).
- `src/pipeline/GenerationAssembler_PIPELINE_Test.cpp` — end-to-end.

## DATA reshape (do FIRST in this rework, atomically)
`MapFields_DATA` currently has one `materialMasks[9]`. Reshape it to **two** families and
rename **throughout** in the same commit so the tree never breaks:
- `materialMasks[9]` → `materialProportions[9]` (rename every reference — NoiseBlend,
  Erosion, Thermal write it).
- **Add** `surfaceStratumWeights[9]` (Mask's output).
This rename mechanically touches the sim stage files (their *logic* is unchanged); it must
land together with those files, which is why it lives here, not in a standalone DATA edit.

## Consumer wiring (ARCH §7.2, single-writer)
- **Sims** (NoiseBlend seeds; Erosion/Thermal evolve) write `materialProportions`.
- **Mask** is the sole writer of `surfaceStratumWeights`.
- **Placement** gates on `surfaceStratumWeights` (visibility), **not** proportions.
- **Bake** consumes `surfaceStratumWeights` **verbatim**; its rival remap
  (`maskRemap*` / `RemapMaskWeight`) is **deleted** — Mask remaps once. **Re-pad the
  `StratumKernelConfiguration` std430 stride** after removing the two floats (48-byte /
  16-multiple), in both the C++ struct and the GLSL block (`DISPATCH_INTERFACE_SPEC §4`).

## Contract
- Two-tier dirty distinction stub (full-regen vs preview-only) for M4.
- Everything in `src/` compiles into the target; run each module's `*_Test`.

## Acceptance (end-to-end)
From a `MapRecipe` + empty `MapFields`, one `Run()` populates heightfield → proportions
(sims) → `surfaceStratumWeights` (Mask) → placement → bake, with sane values; a clean
re-run skips all stages; changing a layer frequency re-runs from that stage on; changing
a **mask** parameter re-runs **Mask alone** (proof of single-writer purity). No stage
writes a field it doesn't own. All `*_Test` pass; builds clean in MSVC.

## Out of scope
UI (M4/M5); `.sanmap` texture export (IO); the thickness-stack DATA reshape (M6).
