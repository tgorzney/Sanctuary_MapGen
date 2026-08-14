# OPTIMIZATION_REVIEW — hardware-math & memory opportunities

A living review of where SanGen is leaving performance on the table, to feed the
ARCH's optimization pillars and future work-orders. Grounded in `core/math/
Sanmath_*`, the sim survey (SIM_ALGORITHMS_SPEC), and the data model
(PARAMS_PIPELINE_SPEC). Not exhaustive — extend as more code is read.

## Foundations already good (keep)
- **Dirty-hash dependency chain** (Blend→Erosion→Flow→Placement) avoids recompute —
  excellent; keep and extend.
- **PropInstance SoA + 32×32 marker spatial grid** — solid data-oriented layout for
  the 100k-entity problem.
- **JFA distance field** (`Sanmath_Spatial.h`) — good O(n) algorithm for clearance.
- **Erosion is already per-material/layer-aware** (per-stratum soil physics).
- **Morton encode/decode helpers exist** (`Sanmath_Morton.h`).

## Hardware-math gaps
1. **The SIMD layer is nearly empty.** `Sanmath_SIMD.h` has only an AVX threshold
   compare. The hot grid passes (thermal, flow, gradient, blend, mask, slope) run
   scalar/OpenMP. **Biggest single opportunity:** vectorize the stencil/grid passes
   with AVX2/AVX-512 (process 8–16 cells per instruction).
2. **`FastInv` is a fake** — it does a real `1.0f / x` (comment admits it). The
   reciprocal-multiply pillar isn't realized. Implement `_mm_rcp_ps` (+ optional
   Newton-Raphson) and ensure loops hoist reciprocals to constants.
3. **No FMA.** Gradient, capacity, blend, levels are multiply-add chains — use
   `fmadd` to halve the op count and improve precision.
4. **Transcendentals in loops.** Stochastic clearance calls `std::cos/std::sin`
   per sample; noise/orographic use trig. Use LUTs or minimax polynomial
   approximations (accuracy-class permitting).
5. **Branchy inner loops.** Perimeter clearance checks and per-layer erosion carving
   branch heavily — convert to branchless/masked (the pillar) where the accuracy
   class allows.
6. **Precompute-once pattern is under-applied.** `GetSlopeSquaredThreshold` correctly
   precomputes cos²; do the same for every per-loop invariant (reciprocals,
   thresholds, wind vectors, gradient scales).

## Memory-storage gaps
1. **Morton is defined but not used for storage.** Grids are row-major (`FloatMask`).
   Storing/streaming the heightfield + working masks in **Morton Z-order** improves
   L1/L2 locality for the 2D stencil passes (erosion/thermal/flow/gradient) — a named
   pillar not yet realized.
2. **No SIMD alignment/padding.** Buffers aren't shown to be 32/64-byte aligned or
   width-padded to the vector register — needed before (1) above pays off, and to
   drop compiler cleanup loops.
3. **Precision tiering.** Everything is `float32`. Preview/`Visual`-class passes could
   use **FP16** (half the bandwidth) — ties directly to the accuracy classes; keep
   `Exact`/`Accurate` at f32.
4. **Per-frame allocations.** Check flow ping-pong buffers, per-thread erosion delta
   merges, and JFA `nextBuffer` copies for avoidable reallocations — prefer persistent
   arena buffers (SYS layer).
5. **Fat structs in hot arrays.** `MarkerTransform` is heavy (strings + many fields) —
   fine for the few interactive markers, but confirm props/units use the lean SoA
   (`PropInstance`) path, never the fat struct, in 100k loops.

## GPU-specific (from SIM_ALGORITHMS_SPEC)
- Non-atomic float scatter (races) → atomic or ping-pong.
- Hardcoded constants (erosion `0.3`, thermal `/2.0`) → parameters (Constitution §8).
- Hardcoded absolute shader paths → load relative.
- No GPU flow shader despite a `UseGPUFlowMap` flag → implement or remove the flag.

## How this feeds the ARCH
- Realize the named pillars that are currently only aspirational: **SIMD saturation,
  reciprocal-multiply, FMA, branchless, Morton layout, SIMD padding, LUT/approx**.
- Each item above is a candidate work-order with a benchmark-backed estimate
  (Constitution §3, §12-style basis tags). None should be applied blind — measure.
- Respect **total tweakability** (§8): exposed constants, not hardcoded.
