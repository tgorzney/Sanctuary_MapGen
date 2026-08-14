# Work-Order M3-4 — Thermal stage (`Thermal_PROC`), CPU + GPU pair

*Constitution §7. Milestone M3. Executor: SanGen Coder (CPU+GPU parity, GL needed).
Independent of other M3 stages (own files) — parallel-safe with worktrees.*

## Title
Thermal / talus relaxation (slope-limited material slumping).

## Root problem
`SIM_ALGORITHMS_SPEC`: thermal erosion relaxes slopes past the talus angle. Today the GPU
divisor is hardcoded (`/2.0`). Rebuild as one kernel, two backends, all constants exposed.

## Target files
`src/proc/Thermal_PROC.h` / `.cpp` / `.glsl` / `_Test.cpp`.

## Layer & accuracy
`PROC`. Preview Visual (Gpu) / Output Accurate (Cpu).

## Inputs / outputs
In: `MapFields.heightfield`, per-material talus angle + thermal params. Out: relaxed
`heightfield` (+ strata thickness where material moves).

## Contract
- Where the local slope exceeds the material's talus angle, move material downslope by a
  tweakable rate until stable (iteration count is a PARAM). No hardcoded divisor (§8).
- One math source CPU/GPU; dispatch via `Sys::Dispatch`; register as a pipeline stage.

## Acceptance
CPU/GPU parity within tolerance; a steep spike relaxes toward the talus angle; flat
terrain is unchanged; iteration-count and talus-angle params take effect; builds clean.

## Out of scope
Hydraulic erosion (M3-3), flow (M3-5).
