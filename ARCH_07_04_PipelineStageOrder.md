[← ARCH index](ARCH.md) · [§7 ARCH_07_M3Resolutions](ARCH_07_M3Resolutions.md) · SanGen ARCH §7.4. **Only the ARCH Expert writes this file.**

### 7.4 Pipeline stage order (binding on M3-8)
```
NoiseBlend → Erosion → Thermal → FlowAccumulation → Mask → Placement → Bake
```
This **supersedes** the order M3-8 currently registers (`NoiseBlend → Mask → Erosion → …`)
and the informal order quoted in earlier drafts of §3.3 / §6.

Why Mask moves after the sims:
- **The gate must be evaluated on the final slope.** Gating on the pre-erosion heightfield
  gates against terrain that no longer exists — the visible strata would not follow the
  eroded landform. Semantic requirement, independent of §7.2.
- **The proportions the gate multiplies must be the post-sim proportions.** Erosion and
  Thermal move material between strata; the visible surface must reflect where the material
  ended up.
- **It removes the ordering hazard entirely.** With Mask last among the field stages, no
  renormalizing stage runs after it, so the gate cannot be undone even by a future sim
  inserted into the chain.

Placement of Mask relative to `FlowAccumulation`: `FlowAccumulation` reads the heightfield
and writes only `flow` / `accumulation` — it does not modify height, so Mask could legally
precede it. Mask is nevertheless placed **after** it, at zero cost, so that `accumulation`
is available as a gate input (e.g. "no grass in the river channel") when §8 tweakability
adds it. Mask must come **before** Placement (§7.2.6) and Bake.

Any later insertion of a sim stage (`FUTURE_SIM_TYPES_SPEC`) goes **before** Mask, in the
sim block. This is a standing rule, not a one-off.

