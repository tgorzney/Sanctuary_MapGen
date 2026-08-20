# Work-Order — Step 36: stop exporting the legacy `mapGeneratorData` blob

*Constitution §6/§8. Executor: SanGen Coder. Human directive: legacy blob export is safe to remove
now that `STEP30` gave every one of its live fields a real top-level home. This executes
`SANMAP_FORMAT_SPEC.md`'s already-ratified "Verified deletions," now unblocked.*

## Root problem
Confirmed by direct read of `MapExporter_MapGeneratorData_IO.cpp` and `MapExporter_Layers_IO.cpp`
(the two files producing the legacy `mapGeneratorData` block): every field it writes is now a pure
duplicate of a top-level home.
- `MapSize` — duplicate of `width`/`length`.
- `TerrainMaxHeight` — duplicate of `GeneralMapSettings.TerrainMaxHeight` (`STEP30`).
- `Stratums[]`'s 8 fields (`ImportedMaskMode`, `MaskRemapMinimum`/`Maximum`, `Enabled`,
  `TintRed`/`Green`/`Blue`, `TileCount`) — all duplicates of `stratumLayers[]`'s corresponding
  fields (`ImportedMaskMode`/`Enabled` via `STEP30`; the rest already duplicated since `STEP11`).
- `Water` block's 4 fields — all duplicates of the top-level `hasWater`/`waterLevel`/
  `deepWaterDepthMin`/`waterDepth` (`STEP27`, `STEP30`).

## Ruled by this ticket
**Remove the EXPORT of `mapGeneratorData` only. Keep the IMPORT side's gated legacy readers
(`ReadGeometryJson`/`ReadWaterJson`/`ReadStrataSettingsJson`) completely unchanged.** This is not
symmetric by design: old real files (confirmed to exist — `World_Domination.sanmap`,
`Pandemonium Isthmus.sanmap`) still carry this blob and are the only source SanGen has for their
data. The never-refuse import law (`STEP24`) depends on these readers staying alive to recover old
files. Only NEW exports from this build onward stop writing the now-redundant blob.

## Target files
- `src/io/MapExporter_Recipe_IO.cpp` — remove
  `document["mapGeneratorData"] = BuildMapGeneratorDataJson(recipe);` and the
  `#include "MapExporter_Recipe_IO.h"`-reachable declaration usage (keep the include if
  `BuildStratumLayersJson`'s declaration still needs it).
- `src/io/MapExporter_Recipe_IO.h` — remove `BuildMapGeneratorDataJson`'s declaration.
- Delete `src/io/MapExporter_MapGeneratorData_IO.cpp` entirely.
- Delete `src/io/MapExporter_Layers_IO.cpp` entirely (confirmed: holds ONLY
  `BuildStratumJson`/`BuildStrataSettingsJson`, nothing else — the file becomes fully empty once
  this content is removed).
- Any test exercising `BuildMapGeneratorDataJson`/`BuildStrataSettingsJson`/`BuildStratumJson`
  directly — remove or adapt (they no longer exist as export functions). Do NOT remove any
  IMPORT-side test coverage for `ReadGeometryJson`/`ReadWaterJson`/`ReadStrataSettingsJson` —
  those readers and their tests must keep passing, proving old-shaped files still import correctly.

## Explicit out-of-scope
- **Any change to import behavior.** A document that still HAS a `mapGeneratorData` block (any old
  file) must continue to import exactly as it does today.
- **`Sanmap_KnownTopLevelKeys_IO.cpp`'s allowlist** — `mapGeneratorData` stays in it (the import
  side still recognizes and partially consumes it) — no change needed there.

## Layer & accuracy class
IO only. Accuracy class: Exact.

## Acceptance test
1. A fresh, non-trivial recipe exported today produces NO `mapGeneratorData` key in the output
   JSON at all.
2. A SYNTHETIC old-shaped document (with a real `mapGeneratorData` block, no top-level
   `GeneralMapSettings`/`stratumLayers[].ImportedMaskMode`/etc.) still imports its
   `terrainMaxHeight`/water/stratum settings correctly from the legacy blob — the never-refuse law
   and backward-compat readers are completely unaffected by this ticket.
3. A full export → import → export round trip on a fresh (never-legacy) recipe stays stable — the
   second export also has no `mapGeneratorData` key, and every other field matches the first
   export exactly.
4. Full `SanGenV2` build stays clean; every existing test continues to pass.
