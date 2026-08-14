# SIM_ALGORITHMS_SPEC — erosion, thermal & flow

Source of truth: `core/gen/Gen_Erosion.cpp`, `Gen_Hydraulic.cpp`, `Gen_Thermal.cpp`,
`Gen_FlowAndAccumulation.cpp`, `core/ErosionSimulator.cpp`, `core/ErosionCompute.cpp`,
`shaders/ErosionCompute.glsl`, `shaders/AvalancheCompute.glsl`.

## Big finding: erosion is already layer/material-aware
The current sims already operate on the **per-Stratum thickness stack**, not a single
heightmap. Per-material physics (`hardness, friction, cohesion, capacityMult,
absorptionRate`) are pulled from `Stratums[layer.StratumIndex]` and drive
erosion/deposition/thermal on both CPU and GPU. Erosion carves top-down through
erodable layers (`ErodeBeneath`/`Erodable`), deposits into the current layer, keeps
thickness ≥ 0, and recomputes `totalHeight` as the layer sum. **So the v2 layered
material erosion (LAYER_SYSTEM_SPEC) is largely already built — reuse it, don't
rewrite it.**

## Hydraulic erosion — Mei-style droplet/particle
Per-droplet trace (not a pipe/grid solver). CPU `Gen_Hydraulic.cpp`, GPU
`ErosionCompute.glsl`.
- **Spawn:** rejection-sampling against a rain map to hit exactly `DropletCount`
  (`Gen_Erosion.cpp`); drop starts speed=1, water=1, sediment=0 (or
  `InitialSedimentLoad` in deposition mode).
- **Move:** bilinear height+gradient; direction = blend(old dir, gradient) by
  `inertia = 0.05 + (1-friction)*0.1`, divided by viscosity. **CPU adds a
  meander/divergence term (`MeanderStrength`, `SlopeAdherence`, `DivergenceThreshold`)
  — GPU omits it.**
- **Capacity:** `max(-Δheight * speed * water * 4 * capacityMult * CarryingCapacityScale,
  0.01)` — same formula both paths.
- **Erode:** `BaseErosionRate*(1-hardness)`, capped at `-Δheight`, subtracted top-down
  from erodable layers. **GPU hardcodes rate 0.3.**
- **Deposit / evaporate:** bilinear splat into current layer; dump all sediment on
  dry-out; `water *= (1-EvaporationRate)*(1-absorptionRate)`.
- **Rain / orographic:** FBm rain noise (thresholded) + orographic rain-shadow (wind·slope,
  symmetry-folded wind). Deposition mode filters spawns by height band.
- **CPU-only:** `AccurateSimultaneousAccumulation` = a topological DAG spillover
  valley-fill pass (no GPU equivalent).

## Thermal / avalanche — talus repose
`Gen_Thermal.cpp` / `AvalancheCompute.glsl`. Per cell, sum positive height diffs to the
4 (von Neumann) neighbours; if `total > maxSlope` (= stratum **cohesion**), slide
`min(thickness, (total-maxSlope)/rate)` proportionally to neighbours. Per erodable layer.
**CPU uses `ThermalRate` + `ThermalIterations`; GPU hardcodes `/2.0` and takes its
iteration count from `GPUPreviewIterations`.**

## Flow & accumulation
`Gen_FlowAndAccumulation.cpp`. **Stochastic single-flow-direction** (not D8): each cell
routes to the neighbour of max *weighted* drop (`drop + rand*noiseImpact`), scattering
accumulation + velocity. Iterative (`Iterations`), AVX2/OpenMP tiled, atomic scatter,
normalized at end. Single-heightmap (not per-material). GPU flag `UseGPUFlowMap` exists
but no GPU flow shader is present.

## CPU vs GPU (the parity problem — hit-list item)
| Pass | CPU | GPU | Match? |
|---|---|---|---|
| Hydraulic | ErosionSimulator→Gen_Hydraulic | ErosionCompute→.glsl | same family, **diverges** |
| Thermal | Gen_Thermal | AvalancheCompute.glsl | same math, diff constants |
| Flow | Gen_FlowAndAccumulation | (no shader) | CPU-only today |

- **Dispatch today:** GPU runs **only** when `WYSIWYGBaking` (final bake); otherwise CPU
  (interactive). So there is no live per-calc dispatcher — just one bake-time flag.
- **They do NOT match:** GPU drops the meander/divergence + DAG spillover, hardcodes
  erosion 0.3 and thermal /2.0, and uses **non-atomic float scatter (admitted races)**.
  GPU is a faster *approximation*, not a decision-exact twin.
- **Brittle:** GPU shader paths are hardcoded absolute `D:/Projects/Sanctuary/...`.

## Reuse vs rework for v2
- **Reuse:** the Stratum thickness-stack model, per-material physics LUT, top-down
  erodable carving + deposit-into-current-layer, rain generation (FBm + orographic +
  rejection sampling), and the thermal talus model — all already v2-shaped.
- **Rework — CPU/GPU parity:** consolidate the two hydraulic and two thermal impls behind
  **one shared parameter set + one dispatch interface** (Constitution §4). Decide the
  accuracy class: today GPU = an **approximation** (Preview/fast), CPU = accurate
  (Output). Make that explicit rather than a lone `WYSIWYGBaking` flag.
- **Rework — GPU correctness:** replace non-atomic float scatter with atomic / ping-pong
  buffers; remove hardcoded shader paths.
- **Rework — flow:** currently single-heightmap + stochastic-SFD only; add material
  coupling and/or multi-flow if v2 needs it.
- **Map to LAYER_SYSTEM_SPEC:** these become the **Simulation layer** types (hydraulic,
  thermal, later fluvial/glacial/snow-melt); the Separate/Unified toggle decides whether
  they run per-GeoLayer or on the flattened stack.
