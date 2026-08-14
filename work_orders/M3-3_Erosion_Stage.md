# Work-Order M3-3 — Erosion stage (`Erosion_PROC`), CPU + GPU pair

*Constitution §7. Milestone M3. Executor: SanGen Coder (CPU+GPU parity, GL needed).
Independent of other M3 stages (own files) — parallel-safe with worktrees.*

## Title
Per-layer hydraulic (droplet) erosion with per-material physics.

## Root problem
`SIM_ALGORITHMS_SPEC`/`LAYER_SYSTEM_SPEC`: erosion is the highest-value, highest-risk
sim. Today the GPU hardcodes rates, has no meander/accumulation DAG, and does a non-
atomic float RMW race; CPU has the real algorithm. Rebuild as one kernel, two backends.

## Target files
`src/proc/Erosion_PROC.h` / `.cpp` / `.glsl` / `_Test.cpp`.

## Layer & accuracy
`PROC`. Preview Visual (Gpu) / **Output Exact (Cpu)** — shapes terrain + pathing.
Deterministic sub-mode: CPU + fixed-point accumulation for the feedback state.

## Inputs / outputs
In: `MapFields.heightfield`, per-stratum thickness (`materialMasks`/strata), per-material
physics + `ErosionFlow` params, seed. Out: eroded `heightfield` + updated strata (additive
thickness volume). Erosion is **per-layer `ErodeBeneath`**, not one global pass.

## Contract
- Mei-style droplet hydraulic: capacity/erode/deposit/evaporate driven by slope + water,
  with meander/divergence (the CPU-only term the GPU lacks) and the accumulation DAG.
- **All rates are PARAMS** (kill the hardcoded 0.3 etc., §8). One math source; no float
  RMW race on GPU (atomics or per-workgroup reduction).
- Deterministic path: `Trigonometry_MATH` + ordered reductions + fixed-point erosion
  state (`DETERMINISM_SPEC`); must pass a two-run bit-identical check.
- Dispatch via `Sys::Dispatch`; register as a pipeline stage.

## Acceptance
CPU/GPU produce visually-equivalent erosion within Visual tolerance; conservation sanity
(no runaway height); deterministic flag → bit-identical across two runs; changing a rate
re-runs the stage; builds clean. (Cross-machine bit-exact is the later gate.)

## Out of scope
Thermal/flow (separate stages); the cross-machine determinism gate (later).
