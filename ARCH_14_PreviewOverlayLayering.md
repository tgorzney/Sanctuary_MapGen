[← ARCH index](ARCH.md) · SanGen ARCH §14. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 14. Preview overlay layering — six-domain screen-space compositor (ARCH ruling, ratifies `work_orders/DESIGN_MarkerPreviewLayering_R2.md`)

Closes and widens the hit-list #4 gap `PREVIEW_COMPOSITING_SPEC` already recorded ("Decals never
composited"): today `Props`/`Units`/`Decals`, all resolved in `Data::PlacementResults`, **never
reach the canvas at all** — a materially bigger gap than an earlier, superseded round
(`work_orders/DESIGN_MarkerPreviewLayering_R1.md`, historical only, do not consult as current)
assumed under a markers-only framing. This ruling covers **six** dynamic overlay domains — Alloy,
Spawns/Armies, Units, Props, Reclaim (not in-game yet; slot reserved), Decals — kept open to
adding more without a code-shape change.


---

### Subsections of §14

| § | File | Ruling |
|---|---|---|
| §14.1 | [ARCH_14_01_ModuleBoundaryDataVsParams.md](ARCH_14_01_ModuleBoundaryDataVsParams.md) | Module boundary and the DATA-vs-PARAMS split |
| §14.2 | [ARCH_14_02_DataModel.md](ARCH_14_02_DataModel.md) | Data model (binding shape) |
| §14.3 | [ARCH_14_03_IconRenderingLod.md](ARCH_14_03_IconRenderingLod.md) | Icon rendering — two-mode LOD, not constant-screen-size-only |
| §14.4 | [ARCH_14_04_NestedUnitGroupAddressing.md](ARCH_14_04_NestedUnitGroupAddressing.md) | Nested `UnitGroup` addressing is flat |
| §14.5 | [ARCH_14_05_ViewStackState.md](ARCH_14_05_ViewStackState.md) | View-stack state — split by field, not one blanket policy |
| §14.6 | [ARCH_14_06_OverlayDomainKindCoexistence.md](ARCH_14_06_OverlayDomainKindCoexistence.md) | `OverlayDomainKind_UI` vs `MarkerCategory`/`PlacementResults` — sits alongside, changes neither |
| §14.7 | [ARCH_14_07_ViewToolbar.md](ARCH_14_07_ViewToolbar.md) | View toolbar — replaces "Regenerate," one popup / two non-crossing sections |
| §14.8 | [ARCH_14_08_DirtyFlagTiers.md](ARCH_14_08_DirtyFlagTiers.md) | Dirty-flag tiers — four, not two (extends `PREVIEW_COMPOSITING_SPEC`'s existing two-tier model) |
| §14.9 | [ARCH_14_09_RenderingPerformance.md](ARCH_14_09_RenderingPerformance.md) | Rendering/performance — mandatory in the first work-order, not deferrable |
| §14.10 | [ARCH_14_10_GpuColorReadbackBug.md](ARCH_14_10_GpuColorReadbackBug.md) | GPU color-texture readback bug (recorded, separate narrow fix, lands first) |
| §14.11 | [ARCH_14_11_Determinism.md](ARCH_14_11_Determinism.md) | Determinism |
| §14.12 | [ARCH_14_12_Naming.md](ARCH_14_12_Naming.md) | Naming |
| §14.13 | [ARCH_14_13_OpenItems.md](ARCH_14_13_OpenItems.md) | Open items — status as of this ratification (closed items marked) |
