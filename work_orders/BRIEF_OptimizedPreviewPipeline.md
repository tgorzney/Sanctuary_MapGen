# Design Brief — Optimized Preview Pipeline

*For a dedicated design conversation. Read `CLAUDE.md` first. This is a DESIGN phase — no
code gets written in this conversation. Output should be a ratified spec update
(`sangen_arch_pack/`) plus a work-order sequence for a later coder session.*

## Status update since this brief was first written
`STEP44_PreviewWindowFitScaling_UI.md` shipped and is verified (94/94 tests green,
uncommitted, on branch `SanGen-v3`): the preview canvas now scales to fill the "Map Preview"
window in both directions instead of a hardcoded 760px square (`Application_Draw_UI.cpp:44-56`).
**That item is closed — don't re-litigate window-fit sizing in this conversation.** The two real
open problems this brief exists for are still fully open: (1) marker icons are not screen-space
(they scale with zoom because they're baked into the shared composite texture), and (2) there is
no per-layer compositable rendering (everything is one pass sequence into one texture, so
nothing can update independently or fast).

## This round's consult scope
The human wants to start with just the **SanGen UI Expert** and **SanGen UI Optimization
Expert** for this pass — not the full four-expert list below. Loop in the Compute Optimization
Expert and ARCH Expert later, once the UI-side design has a concrete shape to hand them (ARCH
ratification still gates anything becoming real law, per `CLAUDE.md`).

## The complaint that started this
The human ran the real app: marker icons scale with zoom instead of staying constant screen
size, and there's no way to update a marker layer without recompositing everything — nowhere
near "realtime." They asked why "what was supposed to be done, was not done" — whether it got
overridden or never wired up.

**Answer from this session's research, state it plainly at the start of the new conversation so
it doesn't get re-litigated**: this was never actually specified anywhere, as far as this
session could find. `sangen_arch_pack/specs/PREVIEW_COMPOSITING_SPEC.md`'s "v2 guidance" only
calls for two-tier dirty flags (full regen vs. GPU re-render) and decal compositing — it does
NOT describe per-marker-layer separate render targets, screen-space icon rendering, or partial
recomposite. So this isn't a dropped/broken implementation of an existing design — it's a real
architecture gap that needs a first design, not an investigation into what went wrong.

## Current architecture (confirmed by reading real code, not the spec's v1 narrative)
`PreviewComposite` (`src/ui/PreviewComposite_UI.h`) runs ONE GPU/CPU compute pass sequence per
composite: clear -> one pass per terrain/water/stratum field layer (samples baked
`Data::MapFields`) -> **overlay pass** (draws every marker/prop/army instance as a flat circular
"mark", `entityMarkRadiusPixels`, baked directly into the shared texture —
`PreviewComposite_Cpu_UI.cpp:124`, same in the GLSL twin `PreviewComposite_UI.glsl:76`) -> entity-id
pass. All of it writes into **one shared `GL_RGBA8` texture** (`CompositeTextureName::kCompositeImage`).
`MapCanvas` (`src/ui/MapCanvas_UI.h`/`MapCanvas_Draw_UI.cpp`) displays that single texture via
`ImGui::Image`, using texture-coordinate windowing for pan/zoom — zoom never re-composites, it
just samples a different window of the same baked raster. That's exactly why marker marks scale
with zoom: they were rasterized into the same texture terrain was, at bake time, in texture-pixel
space, and there is no separate screen-space draw pass for anything today.

There is also no per-marker-type layer concept at the render-target level: everything is one
pass sequence into one texture. Changing any single thing (one marker's color, one layer's
visibility) means the whole `PrepareRun()` + pass sequence reruns.

## What "proper, max-optimized" needs to mean here
The human wants an actual engineering answer, not a guess — bring in the right experts and use
real hardware-cost reasoning, not intuition:
1. **Screen-space marker icons** — decoupled from the baked texture's zoom/pan entirely. Sized
   in constant screen pixels regardless of preview zoom level.
2. **Markers on their own compositable layer(s)**, separate from the terrain/water/stratum base
   composite — so updating one marker layer/type doesn't require rerunning the terrain pass
   sequence, and vice versa.
3. **A real dirty-flag/update-scope model**: what triggers a full regen (CPU sim), what triggers
   a full recomposite (GPU, all layers), what triggers a single-layer recomposite (one marker
   type changed), what's a pure screen-space redraw (pan/zoom/icon-only change) with zero GPU
   recompute at all. Today only the coarse two-tier `bNeedsMapUpdate`/`bNeedsPreviewRender` split
   exists (`PREVIEW_COMPOSITING_SPEC.md` "Dirty flags" section) — this needs to become a real
   per-layer scope, not a binary.
4. **Real hardware-cost reasoning** for whichever design comes out of this — GPU-resident
   textures/framebuffers per layer vs. re-uploading, compositing cost of blending N layers per
   frame, when CPU vs GPU is the right backend for a given pass (this project already has a
   CPU/GPU dispatch seam, `DISPATCH_INTERFACE_SPEC.md` and `Sys::GpuResourceManager` —
   `src/proc/Placement_Kernel_PROC.h`, `PreviewComposite_GpuBuffers_UI.cpp` are real existing
   examples of the pattern to extend, not replace).

## Specs to read before designing
- `sangen_arch_pack/specs/PREVIEW_COMPOSITING_SPEC.md` (current architecture + its own "Known
  issues"/"v2 guidance" — now partially stale post-M4/M5, read the real code as ground truth
  over this doc's narrative)
- `sangen_arch_pack/specs/DISPATCH_INTERFACE_SPEC.md` (the CPU/GPU dispatch contract this design
  must conform to, not bypass)
- `sangen_arch_pack/specs/OPTIMIZATION_PILLARS.md` (the realized SoA/AoSoA/SIMD/tiling/GPU
  technique law — any new pass this design adds must follow these, not invent new ones)
- `sangen_arch_pack/specs/MATH_SIMD_SPEC.md` (hardware-math primitives available)
- `sangen_arch_pack/specs/UI_FRAMEWORK_SPEC.md` (imgui-bypass rendering, 100k-entity throughput
  patterns already established elsewhere in this codebase — `PLACEMENT_SCATTER_SPEC.md`'s prop
  SoA and `MarkersTab_Placed_UI.h`'s VirtualList are real precedent for "don't touch 100k things
  per frame")

## Who to consult (read-only design consults, in whatever order makes sense — this is a big
enough redesign that it likely needs all four)
- **SanGen Compute Optimization Expert** — hardware-cost reasoning, SIMD/GPU dispatch, whether
  per-layer framebuffers are the right primitive.
- **SanGen UI Optimization Expert** — this is literally their charter: "resident texture atlas,
  RT toggles, 100k+ entity throughput." The per-layer toggle-without-full-recomposite ask is
  exactly what "RT toggles" already names.
- **SanGen UI Expert** — WYSIWYG/preview design ownership, how this composes with `MapCanvas`'s
  existing pan/zoom/pick contract.
- **SanGen ARCH Expert** — must ratify any new architecture before it becomes real; is the one
  who'd amend `PREVIEW_COMPOSITING_SPEC.md`/`ARCH.md` if this design changes the shipped
  contract, and rules on any layer-boundary questions (this file already flags one: does a
  per-marker-layer render target live in UI, SYS, or a new seam?).

## Output expected from this conversation
A ratified spec update (not code) plus a work-order sequence sized for later coder dispatch —
follow this project's normal law: research -> design -> ARCH ratification -> work-order ->
coder -> verify (`CLAUDE.md`). Given the scope, expect multiple work-orders, not one.

## Response style (carry forward)
Terse, ❓ for questions, ⚠️ for problems, no narration. See `work_orders/SESSION_HANDOFF_4.md`
§8 for the full house rule if more detail is needed.
