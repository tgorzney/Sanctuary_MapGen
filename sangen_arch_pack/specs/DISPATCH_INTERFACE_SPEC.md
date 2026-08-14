# DISPATCH_INTERFACE_SPEC — the canonical CPU/GPU dispatch contract

Source (current state): `core/Parameters.h` (the ad-hoc flags), `core/gen/
Gen_Erosion.cpp`, `core/ErosionCompute.cpp`, `shaders/ErosionCompute.glsl` /
`AvalancheCompute.glsl`, `core/TerrainCompute.cpp`, `core/gen/LayerBakeCompute.h`,
`gui/PreviewRenderer.cpp`. This spec **prescribes** the one interface every compute
stage uses. It realizes Constitution §4 (CPU = accuracy path, GPU = speed path;
Exact/Accurate/Visual accuracy classes) as an actual code contract.

## Current state (what v2 replaces)
- **No unified selector.** Selection is N independent booleans, each branched at a
  different call site, sometimes on a differently-named field:
  `UseGPUTerrain` (orphaned — `DispatchTerrain` is implemented but never called),
  `UseGPUMarkers` (orphaned — `DispatchMarkers` never called), `UseGPUFlowMap`
  (setting it just **skips** the CPU flow with no GPU replacement → empty flow map),
  and erosion branches on `WYSIWYGBaking` (not a "UseGPU" name at all). Plus two
  **dead** duplicate flags (`FlowSettings::UseGPU`, `GenParams_Flow.h::UseGPU`) wired
  to UI but read by nothing.
- **Kernel written twice, divergent.** The droplet kernel exists as CPU
  (`Gen_Hydraulic.cpp`) and GPU (`ErosionCompute.glsl`) with different math: GPU
  **hardcodes** erosion/deposition rates (`0.3*(1-hardness)`), has **no meander/
  divergence** and **no accumulation DAG**, and does a **non-atomic float RMW race**
  ("no atomics for floats") → non-deterministic and lossy. `CalculateGradient` is
  copy-pasted a **third** time. This is the WYSIWYG "1:1 lossy GPU preview" vs the
  accurate CPU bake — an *intended* speed/accuracy split, but today it leaks into
  *wrong* results, not just faster ones.
- **Inconsistent resource management.** Erosion **recompiles the shader from a
  hardcoded absolute path and re-uploads every dispatch**, then blocks on
  `glMapBuffer` and tears everything down — no persistent buffers, no async.
  `PreviewRenderer` and `TerrainCompute` *do* cache programs/SSBOs. `LayerBakeCompute::
  Dispatch` is declared + called but **undefined**. GL boilerplate is triplicated.
- **Layer violation.** `gui/PreviewRenderer.cpp` owns its own `glDispatchCompute` +
  SSBOs — a second GPU pipeline living in the UI layer, sharing nothing with `core/`.

## The v2 contract

### 1. One kernel, two backends, shared math
Each compute stage (noise/blend, mask, erosion, thermal, flow/accumulation,
placement, bake) is **one kernel** with a single source of truth for its math and
two backends implementing the *same* algorithm:
- **CPU backend** = the **accuracy path** (Constitution §4): full-precision, ordered
  reductions, the complete algorithm (meander, accumulation DAG, thermal cohesion).
- **GPU backend** = the **speed path**: same algorithm, same parameters, allowed to
  use a faster/looser accuracy *class* (Visual for live preview) but **never a
  different result class for the same class**. GPU carries the **full** parameter
  set (the current `LayerConfigGLSL` omissions are the bug), and any approximation is
  a declared accuracy class, not a silent hardcode. Float RMW races are not permitted
  — use atomics or per-workgroup reduction.

### 2. One dispatch policy (replaces the N booleans)
A single policy object resolves backend per stage from a **global default +
per-stage override + context**:
- **Backend** ∈ `{ CPU, GPU, Auto }`, one global default, overridable per stage.
- **Context** ∈ `{ Preview, Output }` (Constitution §4): Preview → Visual/GPU by
  default, escalating to Accurate on idle; Output → the stage's declared accuracy
  class (Exact where required). This is the "toggle for preview and a toggle for
  output, global and per-calculation" the design calls for.
- **Deterministic sub-mode** (`DETERMINISM_SPEC`): forces CPU + ordered reductions +
  portable transcendentals for the shared-generation path.
No stage reads a raw `bool UseGPUx`; every stage asks the policy. Orphaned/dead flags
are deleted.

### 3. One resource manager (no per-dispatch churn)
A GPU resource manager owns: shader programs (compiled **once**, from a resolved
path — never a hardcoded absolute path, never recompiled per dispatch), **persistent
SSBOs/textures** reallocated only on resize, and **async** dispatch (fences, no
blocking `glMapBuffer` on the hot path — follow `PreviewRenderer`'s persistent
`s_SSBOs` + `GL_MAP_UNSYNCHRONIZED` pattern, not erosion's tear-down-every-call). GL
extension loading and the boilerplate live in **one** place. Workgroup sizes are
named constants shared between the GLSL `local_size` and the dispatch math (today
they are duplicated literals: erosion 256, avalanche/terrain 16², markers 8²).

### 4. Buffer handoff contract
CPU `FloatMask`/SoA ↔ GPU SSBO conversion is one documented step per data type
(flatten/upload/dispatch/barrier/readback), reused by every stage; struct layouts
(std430) are declared once and shared, killing the current hazard where erosion's
8-float `LayerPhysics` and avalanche's `vec4 physics[]` alias the same binding with
different strides.

### 5. Determinism & parity classes
Precision tiering per Constitution §4 / `OPTIMIZATION_PILLARS` (fp32 accurate, fp16
visual). Same accuracy class ⇒ CPU and GPU agree within the class's tolerance;
different class ⇒ different *cost/quality*, documented, never accidental. The current
provable non-parity (meander/DAG CPU-only, GPU hardcodes + races) is exactly what
this contract forbids.

## Known issues folded in (hit-list)
No central dispatch; orphaned/dead GPU paths (`DispatchTerrain`/`DispatchMarkers`
never called, `UseGPUFlowMap` = silent no-op); missing `LayerBakeCompute` impl;
per-dispatch recompile + hardcoded absolute shader paths; no async; triplicated GL
boilerplate and gradient math; UI-layer GPU pipeline; std430 struct-alias hazard.

## v2 guidance
Build the kernel/backend/policy/resource-manager quartet **first** in the v2 rebuild
order — every other stage (noise, mask, erosion, flow, placement, preview) plugs into
it. One math source per kernel; CPU authoritative; GPU carries full params and only
varies by declared accuracy class; one policy replaces every `UseGPUx` bool; one
resource manager; deterministic mode is just a policy setting.
