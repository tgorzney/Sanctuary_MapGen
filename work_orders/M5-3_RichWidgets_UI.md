# Work-Order M5-3 — rich widgets (`GradientEditor`, `IconGrid`)

*Constitution §7. Milestone M5. **BATCH 1 (parallel).** Own files. Executor: SanGen Coder.*

## Root problem
`UI_FRAMEWORK_SPEC` + `ASSET_LOADING_SPEC`: color ramps need an editor, and unit/prop
pickers need a thumbnail grid that samples the resident atlas (never files).

## Target files
- `src/ui/GradientEditorWidget_UI.h/.cpp`, `src/ui/IconGridWidget_UI.h/.cpp` (+ tests).

## Layer & accuracy
`UI`. Visual.

## Solution
- **GradientEditor** — add/move/delete/recolor stops on a `Params::GradientSettings`
  (M4-0a), with the smooth/linear toggle; edits feed the M4-2 LUT bake. Returns "changed?".
- **IconGrid** — a scrollable grid of atlas-thumbnail buttons: each cell samples the
  **resident texture atlas** by UV (the asset pipeline, M5-4), never a file; virtualized
  (uses `VirtualList` semantics) so thousands of icons scroll with zero per-item I/O.
  Emits the selected id.

## Acceptance
GradientEditor: add/move/delete a stop mutates the settings correctly; the resulting LUT
(via M4-2) reflects the edit. IconGrid: given a mock atlas manifest, cells map to the
right UV rects and selection returns the right id; only visible cells are drawn. Builds
clean.

## Out of scope
The atlas build itself (M5-4); tabs (M5-6).
