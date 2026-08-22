# Design Brief — Markers Tab UI, Round 2 (supersedes `BRIEF_MarkersTabUI.md` in scope)

*For a dedicated design conversation with the SanGen UI Expert. Read `CLAUDE.md` first. DESIGN
phase only — no code. Output: a design doc (like `DESIGN_MarkerPreviewLayering_R2.md`) naming
every PARAMS/ARCH decision needed, plus whatever work-orders are already unblocked.*

## Why round 2
Round 1's consult (`BRIEF_MarkersTabUI.md`, output already written to
`work_orders/STEP49_ManualMarkersUI.md` + `work_orders/GAP_MarkerLayerAndSymmetry_PARAMS.md`)
scoped manual markers only and treated symmetry as blocked/deferred (Gap 2). The human has since
set a harder requirement that changes the shape of the whole problem — **design from scratch
using v1 as a starting reference, not a port target; improve on it where possible.**

## v1 reference behavior (confirmed by direct code read this round — not the earlier session's
approximation)
`gui/widgets/Widget_MapCanvas.cpp` + `gui/tabs/Tab_Markers.cpp` (also `core/params/Params_Geometry.h`
for the data shapes):
- **Canvas**: left-click a marker icon selects + drags it (screen UV → world coords, clamped to
  map bounds); right-click a marker opens a per-marker menu (Delete, which also removes its
  symmetric siblings); right-click empty canvas opens "Add Marker to Selected Layer," placing a
  marker (and its symmetric copies, computed inline) at the click position — gated on the
  selected `PlacedMarkerLayer.Type == LayerType::Manual`.
- **Tab**: `ProceduralMarkerLayer{Name, Enabled, Locked, Rules[]}` and `PlacedMarkerLayer{Name,
  Type, Enabled, Locked, MarkerKeys[]}` are **two separate struct types** — collapsible/draggable
  layer lists (`RenderDraggableLayerList<T>`), each layer's markers/rules further grouped by Type
  in a nested `CollapsingHeader`, each individual marker/rule also a `CollapsingHeader` exposing
  its full field set.
- **Symmetry (the part the human wants changed)**: `MarkerRule::SymmetryUseGlobal`/`SymmetryMask`
  is **per procedural rule**. `MarkerTransform::SymmetryUseGlobal`/`SymmetryMask` is **per manual
  marker instance**. Neither is per-layer. Editing a manual marker's symmetry (or any styling
  field) live-propagates to every other marker sharing its `SymmetryId`
  (`triggerSymmetryDeltaUpdate`, `Tab_Markers.cpp:264-294`) — this mirroring mechanic itself is
  good and worth keeping, just not the "where does the setting live" part.
- `LayerType` (`Terrain, Prop, Decal, Manual, Fixed`) is a **shared enum already reused across**
  GeoLayers/Props/Decals/PlacedMarkerLayer — v1 already had one shared layer-kind concept, just
  never unified procedural-rule-layers and placed-marker-layers into one type using it.

## The human's new requirement (binding for this round)
**Marker symmetry must be the same mechanism for procedural and manual markers, and it must be
settable at the LAYER level** — not per-rule (as `MarkerRule::symmetryMask` is today) and not
per-instance (as v1's `MarkerTransform::SymmetryMask` was). A layer's symmetry setting governs
every marker in it, whether that marker came from a procedural rule or manual placement.

This is a **real improvement over v1**, not a port — v1 never had per-layer symmetry for either
side. Design it properly.

## What this connects to / must not contradict
- **`SANMAP_FORMAT_SPEC` Correction 7**: `MarkersStack` today is a flat array of `MarkerRule` —
  the Group/Layer/LayerType hierarchy for it was explicitly deferred ("shape pending"). This
  brief is very likely the trigger to finally design that hierarchy for markers, at least enough
  to carry a symmetry setting. Don't assume the full deferred design must be built — scope to
  what this requirement actually needs.
- **`work_orders/GAP_MarkerLayerAndSymmetry_PARAMS.md` (round 1's output, read it first)**: Gap 1
  proposed `Params::MarkerInstanceLayer` (manual-only, Props/Decals-style: name/color/iconScale).
  Gap 2 flagged per-marker symmetry as blocked on "who consumes the mask" with three options (PROC
  bake-mutation, export-time expansion, preview-only). **The human's new requirement effectively
  answers part of Gap 2's question**: the consumer is a layer-level setting shared by both marker
  sources, not a per-instance one — but WHICH of the three consumption mechanisms actually expands
  a manual marker's mirrored copies into real exported data is still open and must be answered
  here, now that the setting's home (layer, not instance) is decided.
- **`DESIGN_MarkerPreviewLayering_R2.md`** (preview-rendering side, separate concern but same
  underlying entities): already modeled `OverlaySubLayerRef_UI{kind: Manual|ProceduralRule, index}`
  — a layer containing a *mix* of manual and procedural sub-layers, for rendering/Z-order purposes
  only (session-only UI state, not PARAMS). That design's shape is a strong hint this brief's
  answer should rhyme with it, but that one is explicitly UI-only/non-serialized — this brief's
  layer+symmetry concept, by contrast, **must be real recipe-serialized PARAMS**, since symmetry
  drives actual generation/placement, not just what's drawn. Don't conflate the two; do keep them
  compatible in naming/shape if possible so a later pass can relate them.
- **`Placement_Symmetry_PROC.h`'s `BuildSymmetryOrbit`**: the existing, working, pure
  `(symmetryMask, extent, x, y) -> orbit points` function already used by procedural generation.
  Whatever layer-level symmetry mechanism this design lands on should drive both procedural and
  manual markers **through this same function**, not a second parallel implementation (v1's inline
  `if (mask & Symmetry_X) ...` duplication in both `Tab_Markers.cpp` and `Widget_MapCanvas.cpp` is
  exactly the kind of duplication to NOT repeat in v2).

## What to design
1. **The unified layer concept**: does `MarkersStack` become one array of a new type that can
   hold either procedural rules or manual markers (or both) per layer, with the layer itself
   carrying `bSymmetryUseGlobal`/`symmetryMask`? Or two arrays (procedural layers, manual layers)
   that share a symmetry-bearing layer type without merging their content? Name the real option,
   don't assume — this is the central shape decision.
2. **Symmetry consumer mechanism for manual markers** (resolves Gap 2, now layer-scoped instead
   of instance-scoped): where does a manual marker's mirrored siblings actually get materialized
   into exportable data — bake-time PROC pass, export-time IO expansion, or something else. Must
   route through `BuildSymmetryOrbit`, not reimplement the mirror math.
3. **The full tab + canvas interaction**, using v1's proven interaction model as the floor:
   collapsible layers → collapsible type-groups → collapsible individual markers; canvas
   left-click select/drag, right-click add/delete, live symmetric-sibling propagation on edit.
   Improve where v1 was weak (e.g., v1 hardcoded `if/else if` per symmetry axis instead of
   composing bits, duplicated the mirror math in two files, had no per-layer setting at all).
4. **Flag, don't invent, any new PARAMS type or ARCH module-boundary ruling** — same posture as
   round 1. If this needs the ARCH Expert (very likely, given it touches `MarkersStack`'s deferred
   Group/Layer design), name exactly what needs ratifying.

## Specs and files to read first
- This file's "v1 reference behavior" and "what this connects to" sections above (already-done
  research, don't re-derive).
- `sangen_arch_pack/specs/PLACEMENT_SCATTER_SPEC.md`, `SANMAP_FORMAT_SPEC.md` Correction 7.
- `work_orders/GAP_MarkerLayerAndSymmetry_PARAMS.md`, `work_orders/STEP49_ManualMarkersUI.md`.
- `work_orders/DESIGN_MarkerPreviewLayering_R2.md` (the sub-layer mix precedent).
- `src/params/MarkerRule_PARAMS.h`, `src/params/MarkerInstance_PARAMS.h`,
  `src/proc/Placement_Symmetry_PROC.h`, `src/params/Symmetry_PARAMS.h`.
- `core/params/Params_Geometry.h` (v1 `MarkerRule`/`ProceduralMarkerLayer`/`PlacedMarkerLayer`),
  `gui/tabs/Tab_Markers.cpp`, `gui/widgets/Widget_MapCanvas.cpp` (v1 reference implementation).

## Who to consult
SanGen UI Expert first (interaction + first-pass data shape). Loop the ARCH Expert once the shape
question in item 1 above needs ratifying — likely immediately, given the scope. Loop the Generator
Expert for item 2 (symmetry consumer) if a new PROC pass is the answer.

## Response style (carry forward)
Terse, ❓ for questions, ⚠️ for problems, no narration.
