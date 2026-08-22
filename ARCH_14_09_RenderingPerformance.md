[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.9. **Only the ARCH Expert writes this file.**

### 14.9 Rendering/performance — mandatory in the first work-order, not deferrable
- **Bulk vertex writes only.** "Batched icon quads" means one bulk `ImDrawList::PrimReserve` +
  raw vertex/index writes per layer, **not** N individual `ImDrawList::AddImage()` calls —
  per-call overhead at 600k markers could plausibly cost 30–60ms, larger than the entire frame
  budget, independent of the transform math. This is a stated work-order requirement, not left to
  a coder's default imgui usage.
- **Cross-layer visible-vertex budget with automatic decimation** (screen-cell clustering, then
  priority-cap fallback) — mandatory in the first work-order, not deferrable. Rough-estimate
  placeholder default ~400,000–500,000 instances before decimation kicks in (derived from a
  3ms-of-16ms frame-budget target) — **explicitly a placeholder pending a real microbenchmark**
  (SIMD-transform, bulk-write, and naive-`AddImage` timed separately at N ∈ {100k, 300k, 600k},
  both 0%-culled and ~5%-visible, on real dev hardware, before this number becomes a ratified
  constant per Constitution §7 basis-tag law), must ship as a named tweakable setting
  (Constitution §8), never a literal.
- **Atlas page bucketing is required.** Thumbnails for many distinct prop templates can legally
  scatter across many atlas pages (general bin-packed atlas, no same-page guarantee) — drawing in
  raw visit order risks draw-call count regressing toward O(pages touched) instead of O(layers).
  Fix: accumulate each visible instance's quad into a per-page bucket during vertex-gen, flush one
  draw command per non-empty bucket — bounds draw calls to O(pages touched this frame) regardless
  of visit order. Strategic-icon mode is naturally safe here (small fixed low-cardinality icon
  set) — put it on one dedicated always-resident page.
- Reuse the existing resident icon atlas (`Ui::IconAtlasManifest`) — already shared by
  Markers/Armies/Props pickers, already proven at 10k+ scale via `ImGuiListClipper`-style
  virtualization.
- Per-layer AABB early-out + per-layer `Data::SpatialGrid` (§8.3) for view-window culling. ⚠️ The
  grid gives **zero help** fully-zoomed-out (everything visible, every bucket queried) — that
  case is genuinely O(N); the cross-layer budget above is what bounds it, not the grid.
- **Layer-id column: do not physically resort `PlacementInstances` by layer.** Reuse the existing
  `ruleIndex`/`category` columns (`Data::PlacementInstances`, `PlacementResults_DATA.h`) via a CSR
  bucket index built once (same lifecycle point as `Data::SpatialGrid`'s build, right after
  Placement, §8.3) — per-layer flat index arrays, cached, rebuilt only when that layer's own
  sub-layer membership changes, not every frame. **Procedural Decals use this identical scheme,
  confirmed** (§14.13 item 4, closed): `Data::PlacementResults::decals` is the same
  `Data::PlacementInstances` SoA type with the same `ruleIndex`/`category` columns
  (`Placement_PROC.cpp:64` `CollectionFor(3)`, `Placement_Rules_PROC.cpp:104-138`
  `AppendDecalRules`, `Placement_Kernel_PROC.h:52` collection index 3) — no special-case needed.
  One of R2's own open items still bears directly on this scheme and is **not** resolved here —
  §14.13 item 3 (manual sub-layer stable-id; design closed, implementation still unscheduled —
  see its updated status below).
