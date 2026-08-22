[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.11. **Only the ARCH Expert writes this file.**

### 14.11 Determinism
Presentation-only — same Visual-class exemption `OPTIMIZATION_PILLARS.md` pillar 15 already
grants GPU-resident preview compositing (Constitution §4, `DETERMINISM_SPEC`). **Binding
guardrail:** any future screen-space decimation/clustering may only affect what is *drawn* — it
must never mutate or discard `PlacementInstances`, and must never feed back into export/bake. A
"helpful" LOD optimization silently becoming a second, non-deterministic placement decision is the
exact failure mode this sentence forbids.

