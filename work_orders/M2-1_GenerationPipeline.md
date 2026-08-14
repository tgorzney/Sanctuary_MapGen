# Work-Order M2-1 — `Generation_PIPELINE` (dirty-hash stage conductor)

*Constitution §7. Milestone M2 (Dispatch + PIPELINE skeleton). Status: implemented +
verified (ALL PASS).*

## Root problem
The v2 needs one conductor that owns the generation stage order and re-runs only what
changed — replacing the `TerrainGenerator` god + the `main.cpp` regen loop, and
formalizing the ad-hoc dirty flags. With `Dispatch_SYS` (M0-8) this is the spine the M3
stages plug into.

## Target files
- `src/pipeline/Generation_PIPELINE.h` (+ test).

## Layer & accuracy
`PIPELINE`. Pure ordering + dirty tracking; no dependency on any concrete stage (stages
are injected as closures), no GPU.

## Solution
`GenerationPipeline::AddStage(name, computeParamHash, run)` registers ordered stages.
`Run()` walks them in order: each stage's combined hash = `HashCombine(upstreamHash,
ownParamHash)`; a stage runs only if its combined hash changed, and because the combined
hash mixes forward, changing an early stage dirties it and everything downstream while an
unchanged prefix is skipped (reuses cached output). `InvalidateAll()` forces a full
re-run (e.g. after a resize). The stage `run` closure is where CPU/GPU dispatch happens
(via `Dispatch_SYS`) — the pipeline stays stage-agnostic.

## Acceptance — PASSED
Three mock stages: first run → all 3; unchanged rerun → none; change the middle → the
middle + downstream only (not upstream); change the first → all; change the last → only
the last; `InvalidateAll()` → all. **ALL PASS under ASan+UBSan**; 70-line header.

## Out of scope
- The two-tier dirty flags (full-regen vs preview-only) — layered on once preview
  exists (M4).
- Wiring the real PROC stages (noise→…→bake) — that is M3, one stage at a time.
- Per-stage default `DispatchPolicy` construction (PIPELINE builds these when stages are
  registered in M3).
