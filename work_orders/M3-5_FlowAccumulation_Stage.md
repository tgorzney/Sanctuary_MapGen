# Work-Order M3-5 — Flow/Accumulation stage (`FlowAccumulation_PROC`), CPU + GPU pair

*Constitution §7. Milestone M3. Executor: SanGen Coder (CPU+GPU parity, GL needed).
Independent of other M3 stages (own files) — parallel-safe with worktrees.*

## Title
Flow direction + drainage accumulation fields.

## Root problem
`SIM_ALGORITHMS_SPEC`: flow/accumulation feed river shading and (later) fluvial erosion.
Today `UseGPUFlowMap` just skips the CPU flow with no GPU replacement (empty map).

## Target files
`src/proc/FlowAccumulation_PROC.h` / `.cpp` / `.glsl` / `_Test.cpp`.

## Layer & accuracy
`PROC`. Preview Visual (Gpu) / **Output Exact (Cpu)** — accumulation affects pathing/river
placement.

## Inputs / outputs
In: `MapFields.heightfield`. Out: `MapFields.flow` (direction/magnitude) and
`MapFields.accumulation` (drainage area).

## Contract
- Stochastic single-flow-direction (or D-infinity) flow; accumulate drainage in a proper
  DAG order (topological / priority-flood), not an ad-hoc pass.
- One math source CPU/GPU; ordered reductions on the deterministic path; dispatch via
  `Sys::Dispatch`; register as a pipeline stage.

## Acceptance
CPU/GPU parity within tolerance; total accumulation equals cell count on a closed basin
(conservation); a single valley shows a coherent high-accumulation channel; dirty-hash
skip/re-run; builds clean.

## Out of scope
Fluvial erosion (future sim, `FUTURE_SIM_TYPES_SPEC`); placement.
