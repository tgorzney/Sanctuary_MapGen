# Work-Order M5-7 — application shell (RUN LAST, alone)

*Constitution §7. Milestone M5. **SEQUENTIAL — run last, single agent.** Depends on
M5-1…M5-6. Edits shared files (main, CMake) + the app entry. Executor: SanGen Coder (GL).
This is the "launch it and see terrain" milestone.*

## Root problem
Everything exists but nothing hosts it. Build the window + GL context + imgui runtime that
mounts the tabs and canvas and drives the pipeline — replacing the old `main.cpp` loop.

## Target files
- `src/ui/Application_UI.*` (window, GL context, imgui init, main loop) + a thin `main`.
- `CMakeLists.txt` — the final assembly: all `src/ui/*`, link the app target.
- End-to-end smoke test / launch check.

## Layer & accuracy
`UI` hosts; drives `PIPELINE`; GPU via `GpuResource_SYS`. Owns no sim logic.

## Solution
Create the window + GL context, initialize imgui, and run the frame loop: draw the tabs
(M5-6) and `MapCanvas` (M5-5); on parameter change, honor the two-tier dirty flags (M4-5) —
`bNeedsMapUpdate` → `GenerationPipeline::Run()`, `bNeedsPreviewRender` → composite only;
the canvas shows the resulting texture. Load a sanpack through the asset pipeline (M5-4) on
open. No hardcoded paths; cache dir user-selectable.

## Acceptance (the milestone)
The app launches, shows a window with tabs + canvas; generating from a default `MapRecipe`
produces visible terrain in the canvas; adjusting a layer regenerates and updates the view;
a gradient tweak recolors without a full regen; clicking selects an entity. Full CMake+MSVC
build + ctest green; app runs without crashing on open/generate/close.

## Out of scope
M6 (thickness stack, determinism gate, future sims, AI/host-client); polish/theming.
