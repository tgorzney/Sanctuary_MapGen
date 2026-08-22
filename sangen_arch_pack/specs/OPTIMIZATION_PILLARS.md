# OPTIMIZATION_PILLARS — SanGen's performance law (realized, not aspirational)

The concrete techniques that realize Constitution §3. SanGen is an **offline,
GPU-allowed** tool, so it uses every applicable data-oriented + SIMD technique AND
GPU compute — and it drops the real-time/networking/fixed-point machinery a live
multiplayer sim would need. Each pillar is a benchmark-backed work-order candidate;
none is applied blind.

## Memory & data layout
1. **SoA + AoSoA.** Struct-of-arrays for linear prefetch; **AoSoA (interleaved,
   16-wide / 512-bit)** specifically for multi-layer erosion — fetch overlapping
   per-layer variables (water / sediment / elevation / material) into one AVX
   register, eliminating cache thrashing.
2. **Block-linear (tiled-Z) layout.** Store grids as **tiles row-major, tiles
   arranged in Morton/Z-order** — contiguous vector loads *within* a tile plus 2D
   cache locality *across* tiles. (Preferred over pure Morton, which scrambles the
   linear streamability wide SIMD wants — benchmark both.)
3. **Hardware-aligned + padded memory (32/64-byte).** Aligned SIMD loads
   (`load`, not `loadu`); width-pad arrays to the vector register to drop compiler
   cleanup loops.
4. **Predictive arena allocator.** One big pooled block; grow before OOM; persistent
   working buffers — no per-frame allocations in hot paths.
5. **Sparse material columns.** Most cells expose 2–3 materials, not 9 — store the
   active few + an overflow, not a dense 9-deep grid (erosion is bandwidth-bound).
6. **Precision tiering.** FP16 for **Visual**-class passes (half bandwidth); f32 for
   **Accurate**; f32 + determinism discipline for **Exact/Deterministic**.

## Compute
7. **SIMD vectorization (AVX2/AVX-512).** Process 8–16 cells per instruction across
   every grid pass (noise, blend, gradient, slope, thermal, flow, mask). The single
   biggest current gap.
8. **Branchless predication.** In hot SIMD loops compute both outcomes and select
   with a bitmask; ban `if` where the accuracy class allows (the one exception: a
   branch that skips a whole expensive block).
9. **Reciprocal-multiply + FMA.** No division inside loops — multiply a precomputed
   reciprocal; fuse multiply-adds. (Fix the fake `FastInv`.)
10. **Portable minimax transcendentals + base-2 frequencies.** Replace `std::sin/cos`
    and per-sample trig with polynomial approximations; prefer power-of-two
    frequencies for bitwise shifts over division. (Also required by DETERMINISM_SPEC.)
11. **Loop tiling / chunking.** Bound working sets to L1 for multi-step grid math.

## Parallelism
12. **Lock-free chunked multithreading.** Partition grids into non-overlapping chunks
    so cores never share memory — no mutexes.
13. **Thread-local command buffers.** Cores write deltas to isolated local arrays,
    merged once at pass end (erosion already does this — keep and generalize). For
    **Deterministic** mode the merge order is fixed.

## Scheduling & dispatch
14. **Dependency DAG / dirty-hash.** Topologically ordered recompute; re-run only the
    stages whose inputs changed (the existing Blend→Erosion→Flow→Placement hash chain
    — keep and formalize).
15. **GPU-resident compute (where allowed).** For Visual/preview and large Accurate
    grids, run compute on the GPU with data resident in SSBOs (no per-step readback).
    **Excluded from Deterministic mode** (see DETERMINISM_SPEC). This is SanGen's edge:
    a live deterministic sim can't use GPU here; SanGen can.

## Not adopted (do not port)
Fixed-point-everywhere determinism, lockstep networking, sector coordinate systems,
8-bit streaming integration, headless sim/render separation, and physics constraint
solvers — these serve real-time multiplayer engines, not an offline generator.
(Optional cross-machine determinism is handled narrowly by DETERMINISM_SPEC, not by
making the whole engine fixed-point.)

## Rule
Every pillar application is a work-order with a **benchmark-backed** estimate
(Constitution §7 basis tags) and respects **§8 total tweakability** (exposed
parameters, no hardcoded constants).
