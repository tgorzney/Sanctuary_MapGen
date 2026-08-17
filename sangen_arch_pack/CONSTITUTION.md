# SanGen Constitution — Tier-1 Law (always loaded)

The non-negotiable law every SanGen agent and coder obeys. Kept deliberately
short. Authored and ratified with the human by the SanGen ARCH Expert. Items
marked **(TBD)** are settled during the ARCH Expert's setup conversation, after
it has read the code.

## 1. The layers
SanGen is divided by technical layer, not by feature:

- **MATH / SIMD** — pure, stateless math (noise, vector, gradient, PRNG).
- **DATA / SOA** — the struct-of-arrays *computed output* of generation (heightfield,
  blended map, material proportions, surface stratum weights, flow/accumulation, resolved
  marker/prop/unit instance arrays, entity-id buffer, spatial grid, cached noise). Holds no
  GPU/GL state.
- **PARAMS** — the *adjustable settings* (the recipe): the layer stack, stratum/
  marker/prop rules, seed, dimensions, erosion/flow/thermal constants, symmetry,
  environment, enums. Its own folder. This is what the `.sanmap`'s SanGen-owned schema v3
  sections (`SANMAP_FORMAT_SPEC`) serialize; DATA is regenerated from it (input vs output).
- **PROC** — applied processors (terrain synthesis, erosion, masking, placement);
  each CPU processor paired with its GPU kernel.
- **PIPELINE** — the conductor: owns the dirty-hash dependency DAG, the PROC stage
  order, and the per-stage backend policy (preview vs output); drives PROC via SYS
  dispatch. The only layer that knows the whole pipeline shape.
- **IO / BRIDGE** — .sanmap import/export, SupCom/official-map import, sanpack
  reading; the platform seam. Loads/saves only — never simulates.
- **UI** — imgui-bypass rendering and the tabs; 100k+ entity preview. Owns no sim
  logic; sets params, trips dirty flags, composites/samples baked results.
- **SYS** — runtime primitives only: threading, allocation, GPU resources, the
  CPU/GPU dispatch *mechanism* (router), logging. Orchestration is NOT here — it is
  PIPELINE.

GPU/GL state never lives in the DATA layer (only SYS + the `.glsl` kernels). 
Dependencies flow downward only, no cycles: UI → PIPELINE → PROC → {DATA, PARAMS,
MATH} and SYS; IO → {DATA, PARAMS}; nothing depends upward.

Every DATA field has **exactly one writing stage**, and every PROC stage is a **pure,
re-runnable function** of its declared inputs — no stage read-modify-writes a field it
does not own (ARCH §3.4; a sim owning and evolving its own field is the one exception).

## 2. Naming law
Literal, fully-spelled, deterministic names — **no abbreviations**. The layer tag is a
**suffix** (`_MATH`, `_DATA`, `_PARAMS`, `_PROC`, `_PIPELINE`, `_IO`, `_UI`, `_SYS`;
`*Compute`/`.glsl` for GPU kernels), TGUE-style. A name states the **quantity**, not the
role. The full suffix system, the no-abbreviation rule + its exceptions (`tpId`,
extensions, `Cpu`/`Gpu`), CPU/GPU pairing, and file-size ceilings (soft 100 / hard 150
lines, functions ≤40 lines) are **resolved in ARCH §1–2**.

## 3. Optimization pillars
Maximum performance is law. Confirmed rules: prefer multiplying a precomputed
reciprocal over division inside loops; validate all input; report failures
clearly. Full numbered pillar set (SIMD saturation, cache/SoA layout,
branchless predication, loop inversion, Morton ordering, etc.): **(TBD)**.

## 4. CPU / GPU accuracy & dispatch standard
Every calculation declares an **accuracy class**:

- **Exact** — must reproduce the game's classified decision (e.g. slope →
  passability); any backend that is decision-exact is legal.
- **Accurate** — must be numerically correct within a stated tolerance; any
  backend within tolerance is legal; pick fastest.
- **Visual** — preview aesthetic only; lossy/stochastic allowed; fastest wins.

Backend choice is a dispatcher decision: explicit per-calculation override,
else the global CPU/GPU setting, else highest performance given data residency
and equivalence. **Preview** and **Output** are separate contexts
(idle-refinement: fast path during interaction, escalate to the accurate pass
on idle, fan to both). Exact field names (`DispatchPolicy`, `ComputeBackend`,
`GenerationContext`, `AccuracyClass`) and per-stage defaults are **resolved in ARCH §4**.
Roles: **CPU is the accuracy path; GPU is the speed path** — usable either as a
preview-only fast approximation or as the baked/exported output. A **Deterministic**
sub-mode of the CPU Exact path makes the *gameplay-authoritative* outputs
bit-identical across machines (competitive shared generation from settings+seed,
no file transfer) — CPU-only, portable transcendentals, ordered reductions. See
`DETERMINISM_SPEC`. In the Output context the guarantee closes over the whole
**Exact chain** — every stage feeding an Exact stage (ARCH §4.6).

## 5. Portability
Optimize maximally for the standalone target. Do not limit the program for
portability; the eventual UE-plugin port re-implements the swappable seams at
port time. Keep platform-specific touchpoints behind thin seams (IO/BRIDGE,
SYS); if a seam ever conflicts with peak performance, performance wins.

## 6. Input & asset safety (pre-alpha data is unreliable)
All external files (game blueprints, dds icons/textures, .san* assets, imported
maps) are validated before use: cap file size and image dimensions, sanity-check
format headers, and fall back to a safe placeholder on any failure — never load
an unverified or corrupt file into RAM or the UI. Mirror the game's own
validate-then-default-then-log pattern (see MODDING_SCRIPTING_SPEC). This is the
concrete form of "validate all input to avoid crashes." A `.sanmap`'s declared
schema version is external input too: an absent/old version is a loud, logged
fallback and a version newer than this build understands is a flat refusal, never
a silent best-effort (`IO_MIGRATION_SPEC`).

## 7. Work-order schema
Coders execute schema-valid work-orders only. Required fields: title; root
problem; target file(s); layer & accuracy class; backend policy; ARCH rules
invoked; solution + benchmark-backed performance estimate (with basis);
optional lossy alternative + accuracy-loss estimate; acceptance test; explicit
out-of-scope list.

## 8. Total tweakability (creative control)
Every value is exposable as a parameter — including physical and algorithmic
"constants" (erosion/deposition rates, thermal divisors, capacity multipliers,
inertia, thresholds, iteration counts). Ship sane defaults, but nothing is
hardcoded beyond a designer's reach: **any variable can be changed — even
constants — for interesting creative results.** (The current hardcoded GPU
constants — erosion `0.3`, thermal `/2.0` — are a direct violation to fix.)
