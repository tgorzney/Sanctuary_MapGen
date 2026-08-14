# DETERMINISM_SPEC — optional cross-machine deterministic generation

## Purpose
An optional mode for competitive / shared generation: the host sends only the
**settings + seed** (tiny), and every player generates the **identical** map
locally — so the large heightmap/texture files never have to be transferred.

## Scope — gameplay-authoritative outputs only
Only what affects competitive fairness must match bit-for-bit across machines:
- Heightmap (including erosion — it shapes terrain/pathing).
- Marker / spawn / mex positions; playable area.
- **Collidable props and reclaim** (gameplay-relevant); purely decorative props
  are exempt.

**Exempt (may differ per player):** visual outputs — stratum mask textures, tint,
smoothness, decoration. These stay on the fast GPU path.

This maps onto the accuracy classes: **Deterministic = the Exact-class outputs,
made cross-machine reproducible.** Visual class is never in scope.

## How determinism is achieved
Realized as the **CPU Exact path + determinism discipline**:
- **CPU-only.** GPUs are not cross-machine deterministic (vendor/driver rounding,
  atomic ordering, FMA/warp reductions). The deterministic bake runs on CPU; GPU
  stays for fast non-competitive authoring.
- **Portable software transcendentals.** No `std::sin/cos/exp` or hardware
  intrinsics in the deterministic path (they differ in the last bits across
  libms/CPUs). Use SanGen's own minimax-polynomial trig/noise, identical on every
  machine. (Doubles as an optimization pillar.)
- **Controlled float:** disable fast-math / value-unsafe reassociation, pin FMA
  contraction (consistent everywhere), SSE/AVX not x87 (no 80-bit extended).
- **Deterministic reduction order:** no atomic float scatter; use ordered/tree
  reductions or integer accumulation. Fixed thread partitioning (chunk boundaries
  independent of core count/scheduling).

## Approach (approved): disciplined float first, fixed-point only where needed
Full fixed-point across the engine is overkill for a one-shot generator and lossy
for float-dense noise/erosion. Target **disciplined deterministic float** for the
whole pipeline, and use **integer / fixed-point accumulation only for the
feedback-sensitive erosion & flow state**, where iterative chaos (a droplet's path
depends on prior deposits) can amplify tiny float differences. This gets identical
gameplay heightmaps without the full fixed-point tax.

## Verification gate (mandatory before competitive use)
Deterministic mode is **experimental until proven**. It MUST pass a **cross-machine
bit-exact test** (e.g. Intel vs AMD, different GPUs present) on all gameplay
outputs from the same settings+seed. Erosion is the highest-risk pass — test it
hardest. Do not rely on it for competitive play until this gate passes; if
disciplined float can't hold parity on erosion, escalate that pass to fixed-point.

## Ties
- A `Deterministic` flag on the Exact class / CPU path; default authoring is fast
  GPU/float.
- Depends on the portable-transcendental and ordered-reduction optimization pillars.
