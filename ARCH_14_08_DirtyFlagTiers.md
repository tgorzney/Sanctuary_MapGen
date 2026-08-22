[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.8. **Only the ARCH Expert writes this file.**

### 14.8 Dirty-flag tiers — four, not two (extends `PREVIEW_COMPOSITING_SPEC`'s existing two-tier model)
| Tier | Trigger | Cost |
| --- | --- | --- |
| A — Full regen | Sim/recipe param changed | Unchanged (PROC) |
| B — Full recomposite | Terrain/water/stratum layer setting changed | Unchanged pass sequence. Cost is resolution-dependent, not one number — sub-ms-to-low-ms credible at the 512² default, plausibly several-to-10ms+ at the 8192² cap (256× the pixel work). **Rough-estimate; must be benchmarked at both, never shipped as one range** (Constitution §7 basis-tag law). |
| C — Screen-space redraw | Every overlay layer, every frame: pan/zoom/hover/visibility/opacity/reorder | Zero GPU recompute, per-layer culled, bounded by the §14.9 cross-layer budget |
| C2 — Interaction-scoped redraw (new) | Active drag/edit on a marker or group | Cache non-selected instances' generated vertex+draw-command bytes once at gesture-start (CPU bytes, not a GPU texture/FBO), replay via memcpy each frame, regenerate live only the selection. Invalidates on pan/zoom/selection-change/layer-setting-change mid-gesture. |

Reorder/opacity changes in the overlay View stack are O(layerCount), never O(instances) — opacity
is a per-vertex tint-alpha multiply, already covered by the C2 table's "layer-setting-change"
trigger above. **§14.13 item 5's resolution closes the open question this paragraph previously
flagged:** overlay layers carry `opacity`, not a per-layer blend-mode enum (§14.2), so every
overlay shares ImGui's one global blend equation and there is no divergent per-vertex
color-encoding (premultiplied vs. straight alpha) risk to confirm. The thumbnail-vs-strategic swap
still needs its own C2 invalidation check, independent of opacity. LOD threshold crossing during
zoom needs no new invalidation rule of its own: zoom already invalidates C2's cache
unconditionally. **ARCH §16.3's PIPELINE query passthrough directly serves this tier** — a
symmetry-group drag under §16 is exactly the C2 "active drag/edit" case, and the passthrough's
whole point is that it can be called every frame of that gesture with zero DAG/dirty-hash
involvement.

