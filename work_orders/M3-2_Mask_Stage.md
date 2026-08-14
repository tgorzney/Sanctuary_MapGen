# Work-Order M3-2 (REWORK) — Mask stage (`Mask_PROC`), CPU + GPU pair

*Constitution §7. Milestone M3. **Supersedes the prior M3-2** — rebuild against the
ratified ARCH §7.2/§7.4 + MASKING_SPEC Part 1. Executor: SanGen Coder (CPU+GPU parity,
GL needed). Depends only on the updated `MapFields_DATA` (already in main).*

## Title
Resolve `surfaceStratumWeights` from `materialProportions` + slope gate + stored art.

## What changed vs the old M3-2 (why this is a rework)
The old stage wrote into the single `materialMasks` field and deferred the remap to Bake.
Ratified model: **two fields** — the sims own `materialProportions` (physical), the **Mask
stage exclusively writes `surfaceStratumWeights`** (visible) and never touches proportions.

## Target files
`src/proc/Mask_PROC.h` / `.cpp` / `.glsl` / `_Test.cpp`. **Delete** `src/params/
StratumMask_PARAMS.h` (ARCH §7.1 violation) — its settings fold into `Params::Stratum`.

## Layer & accuracy
`PROC`. Preview Visual (Gpu) / **Output Exact (Cpu)** — Mask is in the Exact chain because
Placement (Exact) consumes its output (ARCH §4.6).

## Inputs / outputs (single-writer, §3.4)
In: `heightfield`, `materialProportions[0..8]`, `Params::Stratum[]`. Out:
`surfaceStratumWeights[0..8]` — **only** field written. Pure + idempotent (re-runnable).

## The combine (MASKING_SPEC 1.2 — Mask does the multiply itself)
```
slopeGradient = |grad(heightfield)| * terrainMaxHeight / cellSize
gate_s        = SlopeGateWeight(slopeGradient, stratum_s)     // 0..1, smoothstep/feather/invert
procedural_s  = materialProportions[s] * gate_s
merged_s      = Merge(procedural_s, storedArt_s, importedMaskMode_s)
surfaceStratumWeights[s] = Remap_s(merged_s)                  // clamp [maskMin,maskMax]
```
`Merge`: Disabled = gated procedural; ProceduralStart = `clamp(procedural + stored)`;
StaticOverride = replace with stored art, **not slope-gated**. (Can't be a deferred
multiply — StaticOverride replaces; §7.2.4.)

## Hard rules
- Never write `materialProportions`. **Remap happens once, here** — Bake's remap is
  deleted (that's M3-8's concern; note it). Slope **designer settings in degrees**,
  converted to gradient **once** in config flattening; `terrainMaxHeight` read from map;
  **one resampler: bilinear**. All mask settings live in `Params::Stratum`
  (`Stratum_PARAMS.h`); loaded TGA pixels are `Data::FloatField`, never PARAMS.
- Approximation in force (MASKING_SPEC 1.9): Mask consumes `materialProportions` as its
  surface-exposure stand-in; document at the call site; do **not** attempt the thickness-
  stack fix here (deferred, ARCH §7.5) — the kernel won't change when it lands.
- One math source CPU/GPU; dispatch via `Sys::Dispatch`; register per M3-8's order.

## Acceptance
CPU/GPU parity within Visual tolerance; each merge mode correct on a hand case
(StaticOverride ignores slope); Mask leaves `materialProportions` byte-identical (single-
writer); idempotent (run twice = same); degrees→gradient converted once; builds clean.

## Out of scope
Bake remap deletion + std430 re-pad (M3-8 / Bake); the thickness-stack fix (M6).
