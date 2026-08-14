# FUTURE_SIM_TYPES_SPEC — fluvial, glacial & snow-melt passes (design, not yet built)

Forward-looking design. **None of these exist in the current code** — today's sim is
droplet hydraulic + thermal/talus + stochastic flow/accumulation (`SIM_ALGORITHMS_
SPEC`). This spec defines how new erosion passes plug into the *same* framework so
they are first-class, tweakable (§8), and CPU-accurate / GPU-fast (Constitution §4) —
not bolt-ons. Read alongside `SIM_ALGORITHMS_SPEC`, `LAYER_SYSTEM_SPEC` (per-material
physics, additive thickness, ErodeBeneath), and `DISPATCH_INTERFACE_SPEC`.

## The common contract every sim pass obeys
A sim pass is a kernel (`DISPATCH_INTERFACE_SPEC`) that reads/writes the shared
terrain state and reuses the existing machinery:
- **Inputs**: per-material stratum thickness columns, the flow + accumulation fields,
  per-material physics (`hardness/friction/cohesion/capacityMult/absorptionRate`),
  and a seed.
- **Outputs**: modified thickness columns (erosion removes, deposition adds — the
  additive-thickness volume model) and optionally a **new material/stratum** for its
  characteristic deposit.
- **Ordering**: slots into the dirty-hash dependency chain after blend, per the
  Separate/Unified GeoLayer toggle, and runs per-layer via ErodeBeneath where it
  should only cut into the material beneath.
- **Toggle + class**: each pass is independently enable/disable + accuracy-class
  selectable; Deterministic scope only for gameplay-authoritative shape
  (`DETERMINISM_SPEC`).

Each new pass therefore adds: a state field, a transport law, an erosion/deposition
rule, a material coupling, its parameter block, and (optionally) an output stratum.

## 1. Fluvial (channelized river incision)
Droplet hydraulic smooths slopes but does not carve coherent river valleys. Fluvial
uses the **accumulation field** (drainage area A) to drive channelized incision.
- **State**: reuse `AccumulationMap` (already computed) as drainage area; optional
  water-discharge field = precipitation × A.
- **Transport law**: stream-power incision `E = K · A^m · S^n` (S = local slope; K a
  tweakable erodibility scaled by material `hardness`; m,n exposed, typical m≈0.5,
  n≈1). High-A cells cut fastest → dendritic valley networks, canyons in hard rock.
- **Deposition**: where slope/discharge drop (valley floors, deltas), deposit
  **alluvium** — a dedicated soft stratum with low hardness/high erodibility, so
  downstream floodplains read differently from bedrock.
- **Meander**: reuse the CPU meander/divergence stochasticity that already exists in
  `Gen_Hydraulic` (and is missing on GPU) as the lateral-migration term.
- **Couples to**: snow-melt (discharge source), thermal (bank collapse into
  channels), stratum output (alluvium mask).
- **Params**: `K erodibility, m (area exp), n (slope exp), depositionRate,
  channelThreshold (min A to be a channel), meanderStrength, alluviumStratumIndex`.

## 2. Glacial (ice flow, U-valleys, moraines)
Water can't make cirques, U-shaped valleys, or moraine ridges. Glaciers do — slow
viscous ice that abrades and plucks.
- **State**: an **ice-thickness field**. Ice accumulates where elevation is above a
  tweakable **snowline** (a height threshold, optionally latitude/aspect-weighted)
  and where accumulation > ablation.
- **Transport law**: viscous gravity flow (shallow-ice approximation) — ice creeps
  down-gradient at a rate set by thickness and bed slope; much slower/heavier than
  water, so it broadens valleys instead of channelizing.
- **Erosion**: abrasion + plucking `E ∝ (ice velocity) · (ice thickness) / hardness`
  — carves the characteristic **U cross-section** and over-deepens cirque basins at
  accumulation heads.
- **Deposition**: transported debris drops at the glacier terminus/margins as
  **moraine/till** — a distinct blocky stratum (high friction, mixed grain); lateral
  and terminal moraine ridges emerge naturally.
- **Couples to**: snowline (shared with snow-melt), thermal (talus feeding the ice),
  stratum output (till mask).
- **Params**: `snowlineHeight, accumulationRate, ablationRate, iceViscosity,
  abrasionK, pluckingK, tillStratumIndex, minIceThickness`.

## 3. Snow-melt (seasonal water source & nivation)
Less a standalone carver than a **water source** that feeds fluvial/flow, plus its
own light sculpting.
- **State**: a snowpack field accumulating above the snowline; melt = f(temperature/
  elevation) releases water over the season.
- **Effect**: melt discharge is injected into the flow/accumulation field as extra
  precipitation concentrated near the snowline → stronger fluvial incision just below
  it (gullies, nivation hollows where snowpatches sit). Can drive a seasonal loop
  (accumulate → melt → erode) for repeated-pass realism.
- **Couples to**: fluvial (discharge), glacial (shared snowline/accumulation),
  precipitation param.
- **Params**: `snowlineHeight (shared), meltRate, seasonalCycles, gullyThreshold`.
- **Cheapest to ship first** — it can be a modifier on the existing precipitation/
  flow input before the full fluvial/glacial kernels land.

## Sequencing & interaction
Recommended pass order when combined: snow-melt (sets discharge) → fluvial (channels)
→ glacial (high-elevation carving) → thermal (relaxes all steep faces) → deposition
settles. All share the snowline and the accumulation field, so compute those once and
feed all three. Every pass is optional and independently weighted for creative,
non-physical results (§8) — a user can run glacial with no snowline for alien terrain.

## CPU / GPU / determinism
Same split as existing sims: CPU = accurate (full-precision, ordered, meander/DAG),
GPU = fast Visual preview, via `DISPATCH_INTERFACE_SPEC`. Fluvial and glacial are
iterative/feedback-sensitive (like erosion) → if they must be gameplay-authoritative
under shared-gen, they take the CPU Exact/Deterministic path with fixed-point
accumulation for the feedback state (`DETERMINISM_SPEC`), and must pass the same
cross-machine bit-exact gate. Decorative-only use stays on the fast path.

## Status
Design target for a later v2 milestone — after the core pipeline (noise/mask/
hydraulic/thermal/flow/placement) and the dispatch contract are rebuilt. Listed here
so the ARCH reserves the material/stratum slots (alluvium, till), the shared snowline/
discharge fields, and the pass-ordering hooks up front, rather than retrofitting.
