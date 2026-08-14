# Work-Order M3-7 — Bake stage (`Bake_PROC`), GPU (Visual)

*Constitution §7. Milestone M3. Executor: SanGen Coder (GPU/GL). Independent of other M3
stages (own files) — parallel-safe with worktrees.*

## Title
Bake stratum albedo / output textures from the material masks.

## Root problem
`GAMEDATA_LAYOUT_SPEC`/`PREVIEW_COMPOSITING_SPEC`: the final output textures (stratum
albedo/normal/mask composite) are baked from `materialMasks` × stratum textures. This is
the decorative, Visual-class end of the pipeline.

## Target files
`src/proc/Bake_PROC.h` / `.cpp` / `.glsl` / `_Test.cpp`.

## Layer & accuracy
`PROC`. **Visual** class, Gpu path (decorative, determinism-exempt per `DETERMINISM_SPEC`).

## Inputs / outputs
In: `MapFields.materialMasks` (the 9 weight fields), stratum textures + preview colors,
`maskRemap`. Out: the baked output texture set (the `Textures/` payload written on
export; not the settings block).

## Contract
- Composite stratum textures weighted by `materialMasks`, with `maskRemapMin/Max`.
- Gpu-resident via `GpuResource_SYS`; dispatch via `Sys::Dispatch`; register as the final
  pipeline stage. Visual-only — never on the deterministic gameplay path.

## Acceptance
A two-stratum mask blends to the expected weighted texture; remap range takes effect;
runs on the Gpu path; dirty-hash skip/re-run; builds clean.

## Out of scope
Writing the textures into a `.sanmap` on disk (IO export work-order); the preview
composite (that is UI/M4).
