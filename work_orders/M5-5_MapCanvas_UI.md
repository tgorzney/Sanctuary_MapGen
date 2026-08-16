# Work-Order M5-5 — `MapCanvas_UI` (the viewport)

*Constitution §7. Milestone M5. **BATCH 1 (parallel).** Own files + a small repoint of
`PreviewComposite_UI` (M4-3) to emit a GL texture. Depends on M4 preview/picking + M5-0b
texture primitive. Executor: SanGen Coder (GL).*

## Root problem
The generated map must show on screen and be clickable. Today `Widget_MapCanvas` is a
~720-line god-widget (draw + hit-test + geometry + spawning + army creation). v2: a lean
canvas that displays the composite texture, pans/zooms, and routes clicks to picking —
**no sim logic, no spawning** (that's Placement/PROC via PIPELINE).

## Target files
- `src/ui/MapCanvas_UI.h/.cpp`.
- `src/ui/PreviewComposite_UI.*` — small change: emit a real `GpuResource_SYS` **texture**
  (M5-0b) instead of the packed SSBO, so the canvas samples it directly.

## Layer & accuracy
`UI`. Visual. Samples the composite texture; reads back `EntityIdBuffer` for picking.

## Solution
Display the preview texture in an imgui image region with pan/zoom; on click, map cursor →
preview pixel and call `Picking_UI` (M4-4) to resolve the entity under it; surface the
selection. Purely presentation + input — it asks `PIPELINE` to regenerate, it never
simulates or spawns (ARCH §3.2).

## Acceptance
The composite texture renders in the canvas; pan/zoom map cursor→pixel correctly; a click
on a known entity resolves it via picking; a click on empty space resolves nothing; no sim
or spawn code in the canvas. Builds clean.

## Out of scope
Tabs (M5-6); the app window/loop (M5-7); entity editing gestures beyond selection.
