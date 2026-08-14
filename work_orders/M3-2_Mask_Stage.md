# Work-Order M3-2 — Mask stage (`Mask_PROC`), CPU + GPU pair

*Constitution §7. Milestone M3. Executor: SanGen Coder (CPU+GPU parity, GL needed).
Independent of other M3 stages (own files) — parallel-safe with worktrees.*

## Title
Slope masking + stored-stratum-mask merge into the material-mask weight field.

## Root problem
`MASKING_SPEC`: slope-mask generation and the stored-mask merge are dead/inline today,
the slope unit is ambiguous (degrees vs gradient²), there's no smoothstep/feather/invert,
and the merge uses nearest-neighbor. Build it clean.

## Target files
`src/proc/Mask_PROC.h` / `.cpp` / `.glsl` / `_Test.cpp`.

## Layer & accuracy
`PROC`. Preview Visual (Gpu) / Output Accurate (Cpu).

## Inputs / outputs
In: `Data::MapFields.heightfield`, the per-stratum stored masks + `ImportedMaskMode`
(from PARAMS), slope-gate settings, `Geometry.terrainMaxHeight`. Out: `MapFields
.materialMasks` (slope-gated + stored-mask-merged).

## Contract
- Pin the slope unit (document it); derive slope from the heightfield gradient using
  `terrainMaxHeight` read from the map (not 128).
- Add smoothstep / feather / invert as tweakable ops (§8).
- Merge stored masks per `ImportedMaskMode` (Disabled / ProceduralStart additive /
  StaticOverride replace); one resampler (bilinear), not nearest.
- One math source CPU/GPU; dispatch via `Sys::Dispatch`; register as a pipeline stage.

## Acceptance
CPU/GPU parity within Visual tolerance; each merge mode correct on a hand-checked case;
smoothstep vs hard-clamp visibly differ; dirty-hash skip/re-run; builds clean.

## Out of scope
Noise/blend's initial occlusion fill (owned by M3-1); erosion.
