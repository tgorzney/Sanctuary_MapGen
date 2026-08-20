# STEP45 — Rename the running app to v3 (exe target + window title only)

**Layer:** Build system (CMakeLists.txt) + UI string. **Domain:** shell identity.

## Problem
The human is now working on the `SanGen-v3` branch but the debugger's Startup Item dropdown
still shows `SanGenV2App.exe`, and the running app's window title still reads "Sanctuary Map
Generator - SanGen v2" — no visible way to confirm which build is actually running.

## Scope (deliberately narrow)
Rename only the two things the human actually sees:
1. The executable CMake target: `SanGenV2App` -> `SanGenV3App` (`CMakeLists.txt:223-233`, 4
   references — `add_executable`, `target_link_libraries`, the `FOLDER` property, and the
   `POST_BUILD` custom command's target name + `$<TARGET_FILE_DIR:...>` generator expression).
2. The window title: `Application_Settings_UI.h:19`, `"Sanctuary Map Generator - SanGen v2"` ->
   `"Sanctuary Map Generator - SanGen v3"`.

**Do NOT rename** the internal `SanGenV2` static library target, `SANGEN_V2_SOURCES`/
`SANGEN_V2_SHADERS`/`SANGEN_V2_SHADER_DIRECTORY` CMake variables, the `"SanGenV2 Tests"` folder
property, or anything else — those are internal build-graph plumbing the human never sees in
the dropdown or the running app, and renaming them is a much wider, purely-cosmetic churn
across ~25 references in `CMakeLists.txt` for zero visible benefit. `SanGenV3App` linking
against a library still internally named `SanGenV2` is fine and intentional.

## Files touched
- `CMakeLists.txt` — the `SanGenV2App` target only (lines 223, 224, 225, 231, 233)
- `src/ui/Application_Settings_UI.h` — `windowTitle` default string (line 19)

## Verify
Full solo rebuild (`cmake -S . -B build` — the target rename requires a fresh configure) +
`ctest -C Debug`, full suite green. Confirm no other file references the literal string
`SanGenV2App` (grep before finishing) — if any does (e.g. a launch script, `.claude/
launch.json`, a README), update it too and report it, since the work order's own file list may
not be exhaustive.
