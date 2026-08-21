# Work-Order Sequence — Preview Overlay Layering (ARCH.md §14)

Living planning document, not a work-order itself. Tracks what's ready to become a real
`STEP*_*.md` work-order vs. what's still blocked, and the dependency order between them. Update
this file as items move from planned -> drafted -> dispatched -> done, and as new items are
discovered. Source of truth for scope: `ARCH.md` §14 (ratifies
`work_orders/DESIGN_MarkerPreviewLayering_R2.md`).

## Status legend
- **READY** — no open design question blocks writing the real STEP file now.
- **BLOCKED** — needs a specific open item resolved first (named below).
- **DRAFTED** — STEP file exists in `work_orders/`, not yet dispatched to Coder.
- **DONE** — implemented and verified.

---

## Phase 0 — Independent, lands first
| # | Work order | Status | Notes |
|---|---|---|---|
| 0.1 | Gate the GPU color-texture readback to the CPU-fallback path only | **DRAFTED** | Already exists: [STEP46_SkipUnusedCompositeReadback_UI.md](STEP46_SkipUnusedCompositeReadback_UI.md). §14.10. No dependency on the rest of this sequence — can land any time. |

## Phase 1 — Core screen-space infrastructure (no overlay rendering yet)
| # | Work order | Status | Notes |
|---|---|---|---|
| 1.1 | World<->screen coordinate projection (`MapCanvasView::ProjectPreviewPixelToRegionLocal`/`PreviewPixelsPerRegionPixel`, `PreviewComposite::WorldToPreviewPixel`/`PreviewPixelToWorld`/`PixelsPerPreviewCell`) | **DRAFTED** | [STEP47_WorldScreenProjection_UI.md](STEP47_WorldScreenProjection_UI.md). Prerequisite for both picking (1.2) and icon placement (Phase 3). |
| 1.2 | Retire `EntityIdBuffer`/`PickEntity` marker-click path in `MapCanvas`; wire `Picking_UI::PickMarker`+`Data::SpatialGrid` into `ApplyClick` | **DRAFTED** | [STEP48_MigrateClickPickingToSpatialGrid_UI.md](STEP48_MigrateClickPickingToSpatialGrid_UI.md). Depends on 1.1. ⚠️ Flags one open call for ARCH before dispatch: does `MapCanvas` hold a `PreviewComposite*` directly, or does `Application` inject the derived numbers instead? Only removes `MapCanvas`'s *read* of `EntityIdBuffer` — the buffer/GPU pass/`PickEntity` retirement itself is a separate follow-up once this has zero remaining consumers. |
| 1.3 | CSR bucket-index build for procedural sub-layers: per-layer flat index arrays keyed on `ruleIndex`/`category`, built once after Placement alongside `Data::SpatialGrid` | **READY** | §14.9. Confirmed to apply identically to markers/props/units/**and procedural decals** (§14.13 item 4, closed). Do NOT physically resort `PlacementInstances`. |
| 1.4 | Retire `Data::EntityIdBuffer`'s write path: entity-id GPU/CPU pass in `PreviewComposite`, its GPU buffer allocation, and STEP46's now-orphaned entity-id readback line | **BLOCKED on 1.2 landing** | New item, found while drafting 1.2. Only safe once `MapCanvas` (1.2) is confirmed to be the buffer's last consumer and has actually stopped reading it. |

## Phase 2 — Overlay data model + icon asset plumbing (parallel with Phase 1)
| # | Work order | Status | Notes |
|---|---|---|---|
| 2.1 | `OverlayLayer_UI`/`OverlayDomainKind_UI`/`OverlaySubLayerRef_UI` data model + session-only settings struct | **READY** | §14.1, §14.2. UI-layer only, no DATA changes. `opacity: float`, not blend mode (item 5, closed). |
| 2.2 | Icon atlas pairing lookup: `templateIdentifier -> {thumbnailIconId, strategicIconId}` | **READY** | §14.3. New lookup alongside existing single-slot `IconAtlasManifest` — do not widen that struct. |
| 2.3 | Placeholder world-footprint-size table (`templateIdentifier -> baseFootprintWidth/Depth`), per-domain default | **READY (placeholder policy already decided)** | §14.3, §14.13 item 1. Real mesh-derived bounds explicitly out of scope/unscheduled — ship the placeholder now, do not block on real derivation. |

## Phase 3 — The screen-space icon draw pass (the core deliverable)
| # | Work order | Status | Notes |
|---|---|---|---|
| 3.1 | `MapCanvas_IconLayer_UI.cpp` — bulk-vertex-write overlay draw pass, atlas page bucketing, per-layer AABB+`SpatialGrid` culling | **READY, mandatory perf requirements are non-negotiable** | §14.9. **Must** use `ImDrawList::PrimReserve` + raw vertex/index writes — individual `AddImage()` calls per marker is an explicit non-starter (30-60ms risk at 600k markers). Depends on 1.1, 1.3, 2.1, 2.2. |
| 3.2 | Cross-layer visible-vertex budget + decimation (screen-cell clustering, then priority-cap fallback) | **READY to build, ships with a placeholder constant** | §14.9, §14.13 item 2. Placeholder ~400-500k instances — **must** ship as a named tweakable (Constitution §8), never a literal. Real value needs 3.3's benchmark. Bundle into 3.1 or keep separate — coder's call. |
| 3.3 | Microbenchmark: SIMD-transform / bulk-write / naive-`AddImage` timed separately at N ∈ {100k, 300k, 600k}, 0%-culled and ~5%-visible, real dev hardware | **READY, but sequenced after 3.1 exists** | §14.9, §14.13 item 2. This is what turns 3.2's placeholder into a ratified constant (Constitution §7/§12 basis-tag law). Needs a built binary — can't run earlier. |
| 3.4 | Two-mode LOD switch (thumbnail true-size vs. strategic constant-size) wired into 3.1's draw pass | **READY** | §14.3. Threshold-crossing during zoom needs no new invalidation (zoom already invalidates C2 unconditionally, §14.8). |

## Phase 4 — View toolbar (depends on Phase 3 existing)
| # | Work order | Status | Notes |
|---|---|---|---|
| 4.1 | "View" popup: two non-crossing `DraggableList` sections (Terrain / Overlays), opacity slider on overlay rows, blend-mode combo unchanged on terrain rows | **READY** | §14.7. Reuses existing `DraggableList` widget (`LayersTab`'s GeoLayers precedent) — no new widget. |
| 4.2 | Retire "Regenerate" from primary toolbar; collapse `MapCanvas::RequestRegeneration()`/`PreviewDriver::RequestMapUpdate()` to one call path | **READY** | §14.7. Keep exactly one debug/System-panel affordance calling `RequestMapUpdate()` directly for the one legitimate manual case (`PreviewDriver`'s own named exception: resize, recipe reload, new stratum art). |

## Phase 5 — Manual sub-layer support (Props/Decals authoring)
Design fully closed (§14.13 item 3, finalized). No asymmetry between Props and Decals — same
gap, same fix, both domains unblock together. Ready to draft as real STEP files.
| # | Work order | Status | Notes |
|---|---|---|---|
| 5.1 | Stable id on `PropInstanceLayer`/`DecalInstanceLayer`: new `layerId` field, derive-on-create (`1 + max(existing)`, or 0 if empty — no persisted counter), new `"Id"` JSON key in `PropGroups`/`DecalGroups`, legacy backfill on import (`layerId = array index` if `"Id"` absent). Existing `layerIndex` renumbering logic untouched. | **READY** | §14.13 item 3, ruling 1-2. |
| 5.2 | Wire manual props/decals into real PROC resolution (straight 1:1 copy-through, **no symmetry participation** — ruled) + `manualLayerId` correlation column on `Data::PlacementInstances` (mirrors `armyIndex`'s existing shape, sentinel -1 for procedural instances) | **READY, depends on 5.1** | §14.13 item 3, ruling 3. One work-order — (b) has nothing to populate without (a). |
| 5.3 | Wire manual Props/Decals sub-layers into the View toolbar's overlay sections | **BLOCKED on 5.1/5.2** | §14.1, §14.2. |
| — | Manual Alloy/SpawnsArmies sub-layers | **BLOCKED, outside this sequence** | §14.2 table: no `MarkerInstanceLayer` PARAMS type exists yet — tracked in `BRIEF_MarkersTabUI.md`, not this sequence. Procedural markers (via existing `category` split, §14.6) are unaffected and already covered by Phase 1-4. |
| — | Reclaim domain | **BLOCKED, outside this sequence** | No data or rule type exists yet. Slot reserved in `OverlayDomainKind_UI`, zero cost until it ships. Nothing to schedule. |

---

## What ships in a "v1" of this redesign without waiting on Phase 5
Procedural sub-layers for all six domains work end-to-end after Phases 0-4: Alloy, Spawns/Armies,
Units, Props, Reclaim (once it has data), Decals all draw via `recipe.*Rules[i]` today. Only
**manual, human-authored** Props/Decals layers (and markers, blocked elsewhere) need Phase 5.
Recommend treating Phases 0-4 as the first coder dispatch batch, Phase 5 as a fast-follow once
5.1/5.2's design lands.

## Open items this sequence does not resolve (tracked in `ARCH.md` §14.13)
1. Real footprint-size source (mesh-derived, not placeholder) — unscheduled, no work-order yet.
2. Cross-layer budget's real measured constant — becomes 3.3's output, not a separate unknown.
3. Manual sub-layer stable-id/correlation column — Phase 5, blocked pending Generator Expert design.
