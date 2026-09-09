---
name: project-import-layers-empty-bug
description: "FIXED (STEP115, ReconcilePropLayers/equivalent) — was: importing a real non-SanGen .sanmap left marker/prop/decal layer lists empty, breaking per-layer editing UI"
metadata: 
  node_type: memory
  type: project
  originSessionId: 1020d8fc-2688-405c-b760-425353e898e9
  modified: 2026-09-01T01:04:33.068Z
---

**FIXED as of STEP115** — confirmed live in code (`ReconcilePropLayers`/equivalent present and
wired in `MapImporter_ParseDocument_IO.cpp`, `MapImporter_Props_IO.cpp`, `MapImporter_Decals_IO.cpp`,
`MapImporter_Recipe_IO.h`, plus test coverage in `MapImporter_PropsDecals_IO_Test.cpp`). No longer a
live concern — do not gate marker/prop/decal layer-scoped feature testing on this bug anymore.
Reported fixed by a peer session's direct code read 2026-08-31, spot-verified by grep this session
(function exists and is wired into the import path, full semantics not re-traced).

Original bug report, kept for context:

Confirmed 2026-08-25: opening any real `.sanmap` file not previously round-tripped through SanGen (i.e. any game-shipped map, or one from another tool) left `recipe.markerLayers`/`recipe.propLayers`/`recipe.decalLayers` completely empty, even though the corresponding instance arrays (`recipe.markers`/`recipe.props`/`recipe.decals`) imported and rendered fine.

**Root cause:** `MarkerGroups`/`PropGroups`/`DecalGroups` (the layer-metadata wire sections) are SanGen-invented — never present in a file the game or any other tool produced. `ReadMarkerGroupsJson`/`ReadPropGroupsJson`/`ReadDecalGroupsJson` (`src/io/MapImporter_{Markers,Props,Decals}_IO.cpp`) all return immediately if their section is absent, with zero default-layer synthesis. Every imported instance's `layerIndex` silently defaults to 0, pointing at a nonexistent layer.

**Why this matters:** the entire per-layer editing UI (lock, grid-snap, per-layer color override, symmetry settings, the Fix Symmetry button) is nested inside a layer row's expanded body. With zero layers, there's nothing to expand — markers render on the preview canvas, but the layer list UI shows empty, and every layer-scoped feature is structurally unreachable until the user manually clicks "Add Layer." This affected the user's real-world testing of several features shipped in the same session (STEP106-114) that all assumed layers exist.

**How to apply:** before shipping or testing ANY marker/prop/decal layer-scoped feature, confirm the test map either (a) was previously saved by SanGen after layers were manually created, or (b) has had the (planned) import-time layer-synthesis reconciliation applied. A "94/94 tests pass" or "code correctly wired" verification is not sufficient — see [[feedback_no_manual_testing]] and [[project_realworld_verification_gap]] for the broader pattern of automated-test blind spots on this project. The planned fix: synthesize one default layer per distinct instance-group name when the layers array is empty but the instance array isn't, logged loudly, applied uniformly across Markers/Props/Decals since all three share the identical gap.
