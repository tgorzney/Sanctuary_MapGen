# SanGen Constitution — Tier-1 Law (always loaded)

The non-negotiable law every SanGen agent and coder obeys. Kept deliberately
short. Authored and ratified with the human by the SanGen ARCH Expert. Items
marked **(TBD)** are settled during the ARCH Expert's setup conversation, after
it has read the code.

## 1. The layers
SanGen is divided by technical layer, not by feature:

- **MATH / SIMD** — pure, stateless math (noise, vector, gradient, PRNG).
- **DATA / SOA** — struct-of-arrays map data (heightfield, layers, markers,
  props, armies/units, areas, water, atmosphere).
- **PROC** — applied processors (terrain synthesis, erosion, masking, placement).
- **IO / BRIDGE** — .sanmap import/export, SupCom/official-map import; the
  platform seam.
- **UI** — imgui-bypass rendering and the tabs; 100k+ entity preview.
- **SYS** — threading, allocation, orchestration, CPU/GPU dispatch.

GPU/GL state never lives in the DATA layer.

## 2. Naming law
Literal, procedural, deterministic names. Observed precedents to formalize:
`Sanmath_*` (MATH), `Gen_*` (PROC), `Tab_*` / `Widget_*` (UI), `*Compute` +
`.glsl` (GPU), `Params_*` (data/config). Full suffix system and file-size
ceilings: **(TBD)**.

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
on idle, fan to both). Exact field names and per-calculation defaults: **(TBD)**.

## 5. Portability
Optimize maximally for the standalone target. Do not limit the program for
portability; the eventual UE-plugin port re-implements the swappable seams at
port time. Keep platform-specific touchpoints behind thin seams (IO/BRIDGE,
SYS); if a seam ever conflicts with peak performance, performance wins.

## 6. Work-order schema
Coders execute schema-valid work-orders only. Required fields: title; root
problem; target file(s); layer & accuracy class; backend policy; ARCH rules
invoked; solution + benchmark-backed performance estimate (with basis);
optional lossy alternative + accuracy-loss estimate; acceptance test; explicit
out-of-scope list.
