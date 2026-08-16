# Work-Order M5-2 — list widgets (`VirtualList<T>`, `DraggableList<T>`)

*Constitution §7. Milestone M5. **BATCH 1 (parallel).** Own files. Executor: SanGen Coder.*

## Root problem
`UI_FRAMEWORK_SPEC`: 100k-entity lists must scroll cheaply, and the layer stacks need
reorder/enable/lock/delete. Two templated widgets, the reason 100k items don't tank the UI.

## Target files
- `src/ui/VirtualListWidget_UI.h`, `src/ui/DraggableListWidget_UI.h` (+ tests).

## Layer & accuracy
`UI`. Visual.

## Solution
- **`VirtualList<T>`** — `ImGuiListClipper` over a contiguous/SoA array: render only
  `DisplayStart..DisplayEnd` via a caller-supplied row-draw callback. O(visible), not O(n).
- **`DraggableList<T>`** — reorderable list with drag-drop payloads and per-row
  visibility / lock / delete affordances (the templated `RenderDraggableLayerList<T>`
  precedent), for the GeoLayer / layer stacks.
Both are header templates; no app state ownership; the caller supplies data + callbacks.

## Acceptance
VirtualList: with 100k items and a small viewport, only the visible rows invoke the draw
callback (assert the callback count ≈ visible rows, not n). DraggableList: a reorder
produces the expected new order; delete/enable/lock signals fire with the right index.
Builds clean.

## Out of scope
Core inputs (M5-1); gradient/icon widgets (M5-3).
