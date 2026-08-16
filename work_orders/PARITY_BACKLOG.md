# v2 Parity Backlog — old app (`gui/`+`core/`) vs v2 shell (`src/`)

*What the original SanGen exposed that the M0–M5 rebuild does not yet. Ranked by
"makes the tool usable," not by effort. P0 items are why the app feels broken.*

## P0 — the tool can't ingest or produce maps without these

**PB-1 · Load a `.sanmap`** (the button that does nothing today)
Two missing pieces, not one:
- Native file picker. Old = `core/FileDialog.cpp` (Windows `IFileDialog`). v2 shell has
  the *button* but no picker wired. → `src/io/FileDialog_IO.*` (or GLFW-native).
- The importer. Old = `core/MapImporter.cpp` (~49 KB): parse a `.sanmap` and rebuild the
  editable state. v2 has `SanpackReader_IO` (asset *pack* for icons) but **no map
  importer** — nothing reconstructs a `MapRecipe` from a saved map. → `src/io/MapImporter_IO.*`
  (sanmap → `Params::MapRecipe` + baked fields).

**PB-2 · Save / Export a `.sanmap`**
Old = the "Files / Save" tab (`RenderSaveExportTab`) + `core/export/`. v2 has **no save or
export path at all** — you can generate but not keep the result. → `src/io/MapExporter_IO.*`
(recipe + baked `MapFields` → `.sanmap` + export metadata).

## P1 — core tuning surface that's missing

**PB-3 · Debug view overlays on the canvas**
Old could switch the preview to any field: Heightmap, Slope, Flow, Accumulation, Stratums,
Detail-Normal, Tint, Holes, Smoothness, Symmetry. v2 canvas shows the composite only. These
are the primary way you tune generation. → a view-mode selector on `MapCanvas_UI` that
samples the chosen `MapFields` layer.

**PB-4 · Materials / Stratums editing**
Old `MaterialTabs.cpp` (32 KB) + Stratums tab edited per-stratum material settings. v2 has
`recipe.strata` + defaults but needs the editing UI confirmed. → verify it lives in
`LayersTab_UI`; if not, a Materials tab.

**PB-5 · Erosion / Thermal controls** *(M5 known gap #5)*
Stages run with hardcoded defaults (`ConfigureDefaultStages`); no UI to tune droplet count,
iterations, etc. → Erosion + Thermal sections (Layers tab or their own).

## P2 — whole domains not ported

**PB-6 · Atmosphere tab** — `gui/tabs/Tab_Atmosphere.cpp`, not in v2.
**PB-7 · Armies tab** — `gui/tabs/Tab_Armies.cpp` (army placement), not in v2.
**PB-8 · Areas tab** — `gui/tabs/Tab_Areas.cpp` + `Widget_AreaEditor`, not in v2.

## P3 — polish / known M5 gaps

**PB-9** · Determinism toggle is a no-op (wire `DispatchPolicy.bDeterministic`).
**PB-10** · Async asset loading + progress popup (old preloads icons; v2 loads inline).
**PB-11** · Cache-dir / sanpack-path settings home (System tab).
**PB-12** · icon → `tpId` bridge polish.

## Recommended order
PB-1 → PB-2 (round-trip: load, edit, save) → PB-3 (see what you're tuning) →
PB-5/PB-4 → PB-6/7/8 → P3 cleanup.

## Notes
- `SupComImporter.cpp` (import SupCom maps) exists in old core; port only if still wanted.
- P0 is one sequential batch (shared IO + shell wiring). P1/P2 tabs are disjoint files →
  parallel coders, one tab each.
