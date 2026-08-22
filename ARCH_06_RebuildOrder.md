[← ARCH index](ARCH.md) · SanGen ARCH §6. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 6. v2 rebuild order (dependency-ordered milestones)

Bottom-up along §3.1; each milestone independently testable.

- **M0 — Foundation** (no deps): the real `MATH` library (portable SIMD abstraction,
  minimax transcendentals with declared accuracy classes, 2D/3D Morton + block-linear,
  spatial) + `SYS` primitives (`ArenaAllocator_SYS`, `ThreadPool_SYS`, `Log_SYS`,
  `GpuResource_SYS`, `Dispatch_SYS` router). `MATH_SIMD_SPEC`, `DISPATCH_INTERFACE_SPEC`.
- **M1 — Data model** (hit-list #1): define `*_DATA` (computed) + `*_PARAMS` (settings)
  replacing `GenerationParams`; delete dead `core/data/*` + `GenParams_*`;
  `.sanmap` schema v3 round-trip through `IO` (`SANMAP_FORMAT_SPEC`, `IO_MIGRATION_SPEC`).
- **M2 — Dispatch + PIPELINE skeleton** (hit-list #3): `DispatchPolicy`, `Dispatch_SYS`
  resolution, `Generation_PIPELINE` (DAG + dirty-hash). **Vertical slice on one stage
  (noise), both backends**, to prove the whole spine before fanning out.
- **M3 — PROC stages**, in pipeline order (§7.4): Noise/Blend → Erosion → Thermal →
  Flow/Accumulation → Mask → Placement → Bake. **Each stage built as a complete CPU +
  GPU pair and parity-checked together — a stage is "done" only when both backends
  produce in-class-equivalent results.** Not all-CPU-then-GPU; finish the pair, then the
  next stage.
- **M4 — Preview / WYSIWYG** (hit-list #4): `PreviewComposite_UI` samples the bake
  (shadow-sim deleted), `Picking_UI`, two-tier dirty flags derived from the DAG.
- **M5 — UI**: universal imgui-bypass widget library, tabs, `MapCanvas_UI`, and the
  asset pipeline (sanpack ingest → atlas → disk cache; `ASSET_LOADING_SPEC`).
- **M6 — Advanced / optional**: determinism mode + cross-machine bit-exact gate
  (`DETERMINISM_SPEC`); the persistent thickness stack + true surface-exposure
  derivation (§7.5); then future sim types (fluvial/glacial/snow-melt,
  `FUTURE_SIM_TYPES_SPEC`); then AI-analyzability validation + host/client
  (`AI_HOSTCLIENT_SPEC`).

### 6.1 Definition of done (per PROC stage)
A stage is complete only when: CPU **and** GPU implemented and parity-verified within the
stage's accuracy class; wired into `PIPELINE` + `Dispatch_SYS` (no rival toggle); all its
constants exposed as `PARAMS` (§8); its declared inputs/outputs satisfy the §3.4 purity
and single-writer rules; files within the §1.5 ceilings; and its work-order acceptance
test (Constitution §7) passes.
