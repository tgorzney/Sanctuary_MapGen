# Tab Rebuild — work orders + execution flow

*Companion to `TAB_REBUILD_PLAN.md` (the exact settings per tab). This file = the batches, the
order, and the conflict rules. Each WO below is a section; a coder reads this file and does its
assigned section. Settings detail is NOT repeated here — the coder follows `TAB_REBUILD_PLAN.md`.*

## Conflict rules (why the batching is safe)
1. **Sources are globbed.** CMake uses `file(GLOB_RECURSE … CONFIGURE_DEPENDS "src/**")`. New
   `.h/.cpp` files need NO CMake edit — they compile into `SanGenV2` on the next build. So a
   parallel agent verifies its code by building the `SanGenV2` lib (compile check).
2. **Tests are the only CMake edit.** Each `*_Test.cpp` is dormant until an `add_sangen_test`
   line is added. Parallel agents do NOT edit `CMakeLists.txt`. A single **integrate** WO adds
   all the test lines and runs `ctest`. Never two agents in `CMakeLists.txt` at once.
3. **No host edits in parallel.** Tabs create/extend ONLY their own `*Tab_UI.*` files. Wiring a
   tab into the panel switcher / left column is deferred to the host WO (E). One agent owns the
   `Application_*` files.
4. **One owner per PARAMS file.** A promoted setting is added to its owning tab's PARAMS file by
   that tab's WO only. Never edit another batch's PARAMS file.
5. Parallel WOs run as separate coder copies on the same tree — disjoint files, so no clash.

---

## EXECUTION FLOW
```
Batch 1  ── A1 ∥ A2                (parallel: shared widgets, disjoint files, no CMake)
Gate 2   ── A-int                  (single: register A tests, build + ctest)
Batch 3  ── B                      (single: Layer Editor + Heightmap tab; depends A)
Batch 4  ── C1 ∥ C2 ∥ C3 ∥ C4 ∥ D  (parallel: tabs + IO, disjoint files, no CMake/host)
Gate 5   ── CD-int                 (single: register all C+D tests, build + ctest)
Batch 6  ── E                      (single: host shell — left column, panel wiring, run)
```
After each single WO and each gate: paste the coder's report back; the next prompt may be
adjusted from it (e.g. if a WO surfaces a missing PARAMS field or a new shared widget).

---

## A1 — shared widgets, set 1  (parallel with A2; no CMake)
Build `src/ui/`: `Checkbox_UI`, `Combo_UI`, `SliderScalar_UI` (int+float, carries an RtToggle),
`TextInput_UI`. Each = `.h/.cpp/_Test.cpp`, own files. Match existing widget style
(`RangeSliderWidget_UI`). Verify by building `SanGenV2`. Do NOT edit CMakeLists.

## A2 — shared widgets, set 2  (parallel with A1; no CMake)
Build `src/ui/`: `ColorSwatch_UI` (picker only, `NoInputs` — no RGBA fields), `FilePathPicker_UI`
(button + short-path label; native dialog can stub to a path string for now), `Levels_UI`
(shadows/midtones/highlights + output black/white + input histogram), `Section_UI` (collapsing
header helper). Each own files. Build `SanGenV2` to verify. No CMakeLists edit.

## A-int — integrate widgets  (single; owns CMakeLists)
Add `add_sangen_test(...)` for all 8 new widget tests. Build `SanGenV2` + `SanGenV2App` + run
`ctest`. Green gate before B.

## B — Layer Editor + Heightmap tab  (single; depends A)
The core. Build `src/ui/LayerEditor_UI.*` per `TAB_REBUILD_PLAN.md § Layer Editor`: per-GeoLayer
group (`DraggableList`), per-NoiseLayer noise/levels/height-blend/density/symmetry, Soil Physics,
Hydraulic Erosion + Deposition sub-panels, Import RAW / Duplicate / Bake. Then `HeightmapTab_UI`
(seed, scale-to-size, map size, terrain max height, **new** terrain min height, global gravity,
hosts the GeoLayer stack). **Promote to settable** (your rule): add `TerrainMinHeight` to the
Geometry PARAMS, and the hidden erosion constants (BaseErosionRate, BaseDepositionRate,
MeanderStrength, DivergenceThreshold, ThermalIterations, ThermalRate) to the erosion PARAMS under
an "Advanced (constants)" section. Register its tests, build + ctest.

## C1 — terrain view tabs  (parallel; depends A)
`src/ui/`: `SymmetryTab_UI`, `SlopeTab_UI`, `FlowTab_UI`, `AccumulationTab_UI`. Symmetry also
promotes `SymmetryDetectionTolerance` + `SnapImperfectSymmetry` into the symmetry PARAMS (this WO
owns that file). Gradients via `GradientEditor`. Own files only; no CMake/host edits.

## C2 — material & mask tabs  (parallel; depends A + B)
`src/ui/`: `StratumsTab_UI` (per-stratum list: names, material/env combos, 3 file pickers, mask
mode 3-state, color swatches, remaps, tiling, soil), `DetailNormalTab_UI`, `TintTab_UI`,
`HolesTab_UI`, `SmoothnessTab_UI` (the last four = show-toggle + a Layer Editor stack from B).
Own files only.

## C3 — environment tabs  (parallel; depends A)
`src/ui/`: `WaterTab_UI` (levels/deep-water range sliders, gradient, shore & wind),
`AtmosphereTab_UI` (Sun, Skylight, Exposure/Skybox, Legacy/Background/Height/Linear Fog, Wind —
all sections per plan). Own files only.

## C4 — placement tabs  (parallel; depends A)
`src/ui/`: `MarkersTab_UI` (global scales + procedural rule list via `DraggableList` + placed
markers via `VirtualList`, icons via `IconGrid`), `ArmiesTab_UI` (armies + unit `IconGrid` modal),
`PropsTab_UI` (manual layers + procedural/decal Layer Editor stacks), `AreasTab_UI`. Own files.

## D — Files/Save + IO  (parallel with C; single agent; owns src/io)
`src/io/`: `FileDialog_IO` (native picker), `MapImporter_IO` (`.sanmap` → `MapRecipe` + baked
fields — this is the missing "Load sanmap"), `MapExporter_IO` (recipe+fields → `.sanmap`; plus
Heightmap RAW / Slope PNG / Flow PNG / Stratums TGA; Export All). `src/ui/FilesTab_UI` (Open
Sanmap, Import SupCom Lua, the exports, debug log). **No** `.json` generator file — sanmap is the
source of truth. Disjoint from C (different files); no CMake/host edits.

## CD-int — integrate tabs + IO  (single; owns CMakeLists)
After C1–C4 and D are all done: add `add_sangen_test(...)` for every new tab/IO test. Build all +
`ctest`. Green gate before E.

## E — host shell  (single; owns Application_*)
Rebuild the shell to the v1 layout: left column = vertical tab list under the three group headers
(TERRAIN & LAYERS / ENVIRONMENT / SYSTEM), with the `[O]/[ ]` preview-visibility toggles. Panel
switcher hosts every tab from A–D. Wire the Performance/System controls (GPU toggles, WYSIWYG,
determinism → `DispatchPolicy`). Preview-layer visibility toggles drive the composite. Full
`SanGenV2App` build + run: load a sanmap, generate, tweak, save. Final gate.
