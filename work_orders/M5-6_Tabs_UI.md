# Work-Order M5-6 — parameter tabs (`*Tab_UI`)

*Constitution §7. Milestone M5. **BATCH 2 (after the widget batch M5-1/2/3).** Own files;
tabs are independent of each other (internally parallelizable). Executor: SanGen Coder.*

## Root problem
The user edits the `MapRecipe` (PARAMS) through tabs, each composed from the shared widget
library — no hand-rolled imgui per tab.

## Target files
- `src/ui/TerrainTab_UI.*`, `LayersTab_UI.*`, `MarkersTab_UI.*`, `PropsTab_UI.*`,
  `WaterTab_UI.*`, `SystemTab_UI.*` (one file each; can be split across parallel coders).

## Layer & accuracy
`UI`. Each tab reads/writes its slice of the `MapRecipe` and trips the correct dirty flag
(via the RT-toggle / dirty model), never touching PROC directly.

## Solution
Compose `RangeSlider`/`Dial`/`RtToggle` (M5-1), `VirtualList`/`DraggableList` (M5-2),
`GradientEditor`/`IconGrid` (M5-3) to edit each PARAMS domain:
- **Terrain** — `Geometry` (mapSize/seed/worldUnitsPerCell/terrainMaxHeight).
- **Layers** — the GeoLayer/Layer stack via `DraggableList` (reorder/enable/lock) + per-
  layer noise/blend controls; the Separate/Unified toggle.
- **Markers / Props** — rule lists (`VirtualList`) + gates via IconGrid pickers.
- **Water** — the `Water` settings. **System** — cache dir, dispatch/backend, determinism
  toggle.
Each control sets `bNeedsMapUpdate` vs `bNeedsPreviewRender` per the M4-5 dirty model
(derived, not hand-mapped).

## Acceptance
Each tab edits its PARAMS and the change is reflected in the `MapRecipe`; a visual-only
control trips only `bNeedsPreviewRender`; a sim control trips `bNeedsMapUpdate`; tabs use
only shared widgets. Builds clean.

## Out of scope
The window/main loop that hosts the tabs (M5-7).
