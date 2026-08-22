# Design Brief — Markers Tab UI

*For a dedicated design conversation with the SanGen UI Expert. Read `CLAUDE.md` first. This is
a DESIGN phase — no code gets written in this conversation. Output should be a work-order (or a
flagged PARAMS gap handed to the ARCH Expert first, if the type doesn't exist yet — see below).*

## Where things stand today
`MarkersTab_Placed_UI.h` is the only Markers UI that exists, and it is **read-only**: a
virtualized list of whatever the Placement stage resolved into `Data::PlacementInstances`. Its
own header comment is explicit about why: v1's editable manual markers (alias, position,
per-marker symmetry, spawn->army assignment, delete) are recipe content with **no `_PARAMS`
home in the tree today** — there is no `Params::ManualMarker` type, so no editor is drawn.

**Markers have no layer system today**, unlike Props/Decals. Confirmed by reading
`src/params/PropInstance_PARAMS.h`: Props and Decals got a real manual-layer authoring system —
`PropTransform::layerIndex` / `DecalTransform::layerIndex` plus `PropInstanceLayer`/
`DecalInstanceLayer` metadata arrays (ARCH_12_ManualPropDecalLayers.md §12, `ENTITY_AUTHORING_PARAMS_SPEC.md`, wired UI at
`src/ui/PropsTab_Manual_UI.h`/`.cpp`). Markers never got the equivalent — there is no
`MarkerInstanceLayer` or `MarkerTransform::layerIndex` anywhere in `src/params/`.

This matters for the design conversation because a companion effort
(`work_orders/BRIEF_OptimizedPreviewPipeline.md`, run separately) wants markers rendered as
independently-compositable layers in the preview for fast partial updates — that needs a real
per-layer data model to hang off of, and it doesn't exist yet.

## What to design
1. A real Markers tab, mirroring what Props/Decals already have where it makes sense:
   - Manual layer authoring (add/reorder/toggle-visibility layers), following the
     `PropsTab_Manual_UI.h`/`.cpp` precedent directly — read it as the reference implementation,
     don't design from scratch.
   - The existing placed-marker list (`MarkersTab_Placed_UI.h`) stays, unchanged unless there's a
     real reason to touch it.
   - Whatever manual marker placement/editing UI v1 had (alias, position, per-marker symmetry,
     spawn->army assignment, delete) that currently has nowhere to live.
2. **Flag, don't invent, the missing PARAMS type.** If the design needs `Params::MarkerInstanceLayer`
   / a `layerIndex` field on marker transforms, that's a new type in `Army_PARAMS.h`-adjacent /
   `ENTITY_AUTHORING_PARAMS_SPEC.md` territory — report the gap precisely (field names, shape,
   how it should parallel `PropInstanceLayer`/`DecalInstanceLayer`) rather than assuming it into
   existence. The ARCH Expert is the one who ratifies a new PARAMS type; a UI Expert consult
   reports the need, it doesn't create the type.

## Specs and files to read first
- `sangen_arch_pack/specs/ENTITY_AUTHORING_PARAMS_SPEC.md` — the Props/Decals manual-layer
  precedent this should parallel, and the existing named gap for markers if one is recorded.
- `sangen_arch_pack/specs/UI_FRAMEWORK_SPEC.md` — universal widget library, VirtualList,
  DraggableList — reuse existing widgets, don't invent new ones without cause.
- `sangen_arch_pack/specs/PLACEMENT_SCATTER_SPEC.md` — marker rules, symmetry, the
  `MarkersStack`/`GlobalMarkerSettings` procedural-rule side (distinct from this hand-authored
  side — don't conflate the two systems).
- `src/params/PropInstance_PARAMS.h`, `src/ui/PropsTab_Manual_UI.h`/`.cpp` — the direct precedent.
- `src/ui/MarkersTab_Placed_UI.h` — what exists today.
- `src/params/MarkerRule_PARAMS.h`, `src/params/GlobalMarkerSettings_PARAMS.h` — the procedural
  side, for contrast only.

## Who to consult
**SanGen UI Expert** — sole consult for this brief, per the human's explicit ask. Loop in the
ARCH Expert only if/when a new PARAMS type needs ratifying (see item 2 above) — not before.

## Output expected
A work-order ready for coder dispatch, OR (if a new PARAMS type is needed first) a precise gap
report to hand to the ARCH Expert as its own small ratification step before the UI work-order
gets written.

## Response style (carry forward)
Terse, ❓ for questions, ⚠️ for problems, no narration. See `work_orders/SESSION_HANDOFF_4.md`
§8 for the full house rule if more detail is needed.
