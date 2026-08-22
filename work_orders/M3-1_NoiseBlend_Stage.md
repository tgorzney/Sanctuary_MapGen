# Work-Order M3-1 — Noise/Blend stage (`NoiseBlend_PROC`), CPU + GPU pair

*Constitution §7. Milestone M3 (PROC stages). Executor: **SanGen Coder in Claude Code**
(CPU + GPU pair, parity — GPU needs GL). Status: spec ready; NOT implemented.
This is also the M2 vertical slice: first stage proven end-to-end on both backends.*

## Title
Generate per-layer noise and blend the layer stack into the heightfield + material
masks — one kernel, CPU and GPU pair, wired through the pipeline and dispatch.

## Root problem
The first generation stage. Today the CPU noise/blend impl is missing (`Gen_NoiseAndBlend
::Process` declared, undefined), the GPU path omits blend/fractal/type from its config
(so GPU ≠ CPU), and Morton/noise are tangled. Build it clean on the v2 foundation.

## Target files
- `src/proc/NoiseBlend_PROC.h` / `.cpp` (CPU) — split across `NoiseBlend_*_PROC.cpp` if
  the ceiling is hit.
- `src/proc/NoiseBlend_PROC.glsl` (GPU compute) — paired base name (ARCH_01_04_CpuGpuKernelPairing.md §1.4).
- `src/proc/NoiseBlend_PROC_Test.cpp`.

## Layer & accuracy class
`PROC`. Preview = **Visual** (Gpu), Output = **Accurate** (Cpu) — ARCH_04_DispatchContract.md §4.2 defaults.

## Inputs / outputs
- In: `Params::LayerStack` (the enabled flat layers), `Params::Geometry` (seed,
  vertexSize).
- Out (`Data::MapFields`): `heightfield` (blended), and per-stratum `materialMasks`
  (top-down occlusion from each layer's height-blend + opacity). Cache raw per-layer
  noise + the blended result (two-level dirty hash, `NOISE_BLEND_SPEC`).

## Contract (build on the M0/M1 foundation)
- Noise per `Params::Layer` (type/fractal/frequency/octaves/gain/pingPong/cellular),
  then reshape (levels/density). Per-layer seed derived from `Geometry.seed`. Use the
  project's `FastNoiseLite` for CPU; the GPU kernel must carry the **full** layer config
  (fix the current omission) so shape matches.
- Blend layers per `Params::HeightBlendMode`; height-blend contrast/min/max + opacity
  drive `materialMasks[stratumIndex]` via top-down occlusion (see `MASKING_SPEC` — that
  math is shared with the mask stage; put the shared helper where both can call it).
- **One math source** for CPU and GPU; the GPU is the speed path, not a different result
  class. Deterministic path uses `Trigonometry_MATH` (portable) instead of `std::` /
  hardware trig.
- Dispatch via `Sys::Dispatch` (M0-8); register as a `GenerationPipeline` stage (M2-1)
  whose `computeParamHash` covers the layer-stack settings. GPU resources come from
  `GpuResource_SYS` (M0-9). No hardcoded shader paths; expose every constant as PARAMS
  (§8).

## Acceptance test
1. **CPU/GPU parity:** for a representative layer stack, the CPU and GPU heightfields
   agree within the Visual-class tolerance (document it); identical layer config on both.
2. **Blend correctness:** each `HeightBlendMode` produces the expected combination on a
   2-layer stack (hand-checked values).
3. **Dirty hash:** changing one layer's frequency re-runs the stage; changing nothing
   skips it (via the pipeline).
4. **Determinism:** with the deterministic flag, two runs (and, ideally, two machines)
   give bit-identical heightfields.
5. Builds clean in MSVC; files within §1.5 ceilings.

## Out of scope
- Mask/erosion/flow/placement/bake — their own M3 work-orders.
- Image-layer baking (a Layer as a baked image) — a follow-up once base noise works.

## Note to executor
Read `core/gen/Gen_Noise.cpp` and the current GPU noise path for the algorithm, but
rebuild against the v2 `Layer_PARAMS`/`MapFields_DATA`/`FloatVector_MATH`/dispatch —
do not port the god-object coupling. Kill the CPU/GPU config divergence.
