# Work-Order — Step 27: read the top-level `hasWater`/`waterLevel`/`waterDepth` mirrors on import

*Constitution §6. Executor: SanGen Coder. Independent of `STEP24`/`STEP26` — does not touch the
migration runner or Unknown-Import bag, does not need further ARCH ratification, can be dispatched
immediately. Found by a round-trip-fidelity audit (Prompt C, "Global/recipe-level domain parity").*

## Root problem
`.sanmap` export writes water settings TWICE, in two different locations:
1. Top-level document-root mirrors: `document["hasWater"]`, `document["waterLevel"]`,
   `document["waterDepth"]` (`MapExporter_Recipe_IO.cpp:96-98`).
2. The legacy `mapGeneratorData.Water` block: `Enabled`/`WaterLevelMax`/`DeepWaterDepthMax` (plus
   `DeepWaterDepthMin`, which has no top-level mirror at all) (`MapExporter_Recipe_IO.cpp:66-69`).

The importer (`ReadWaterJson`, `MapImporter_Recipe_IO.cpp:50-57`) reads ONLY the legacy blob's
nested keys, and is called ONLY when `mapGeneratorData` is present
(`MapImporter_IO.cpp:177-183`, gated). The top-level `hasWater`/`waterLevel`/`waterDepth` keys are
never read by anything in `src/io/`.

**Confirmed real-world impact:** official/SupCom demo maps never carry a `mapGeneratorData` block
at all (confirmed by the Format Expert's earlier direct read of real shipped files this session).
Opening one of those maps today silently loses ALL water settings — `bEnabled`,
`waterLevelMaximum`, `deepWaterDepthMaximum` all stay at their `Params::Water` struct defaults,
with no warning, even though the file's own top-level `hasWater`/`waterLevel`/`waterDepth` keys are
sitting right there, unread. SanGen's own round-trip tests never caught this because SanGen always
emits the legacy blob on export too, masking the gap for every self-authored file.

## Solution — shape
Add three unconditional top-level reads to `ParseSanmapJsonText`
(`src/io/MapImporter_IO.cpp`), same tier as every other top-level reader this session
(`ReadAreasJson`, `ReadAtmosphereJson`, `name`/`credits` from `STEP25`, etc.) — NOT gated behind
`mapGeneratorData`:
```cpp
ReadJsonBoolean(document, "hasWater", outRecipe.water.bEnabled);
ReadJsonFloat(document, "waterLevel", outRecipe.water.waterLevelMaximum);
ReadJsonFloat(document, "waterDepth", outRecipe.water.deepWaterDepthMaximum);
```
The existing gated `ReadWaterJson(generatorData, outRecipe)` call (`MapImporter_IO.cpp:183`) stays
exactly as-is and runs AFTER these three unconditional reads (confirm call order in the actual
target file — the gated call must run second so a SanGen-authored file's more-precise legacy-blob
values, which include `DeepWaterDepthMin` with no top-level equivalent at all, still take final
precedence over the top-level mirrors for the three overlapping fields when both are present; this
matches the existing precedent at `MapImporter_IO.cpp:96` where `mapGeneratorData` already
"overrides" the top-level value for `terrainMaxHeight`). This closes the official-map gap (nothing
else to read there) while leaving SanGen's own more-precise round-trip unaffected (the blob still
wins when present).

## Target files
- `src/io/MapImporter_IO.cpp` (`ParseSanmapJsonText`) — add the three unconditional top-level
  reads, positioned before the `mapGeneratorData` gate, in the same tier as `width`/`length`/
  `height`/`name`/`credits`.
- `src/io/MapImporter_IO_Test.cpp` — extend `BuildPopulatedRecipe`/the round-trip test to cover a
  water-enabled recipe if not already covered; add the acceptance-test cases below.

## Explicit out-of-scope
- **`water.deepWaterDepthMinimum`** — has no top-level mirror at all (only the legacy blob), so
  there is nothing new to read for it; stays exactly as it is today (legacy-blob-only, fragile if
  the blob is ever deleted per `SANMAP_FORMAT_SPEC`'s "Verified deletions" list — a separate,
  future ticket needs to give it a real top-level home before that deletion can safely happen,
  flagged by the same audit that found this ticket's gap, not this ticket's job).
- **`geometry.terrainMaxHeight` precision loss** — a related, separate finding from the same audit
  (top-level `height` is a lossy int; full float precision only survives via the doomed legacy
  blob) — different field, different fix, separate ticket.
- **The 5 adjacent Stratum fields** flagged by the same audit as sharing this exact gating pattern
  (`importedMaskMode`, `maskRemapMin/Max`, `bEnabled`, `tintRGB`, `tileCount`) — not audited in
  depth, not this ticket's scope.
- **No exporter change** — the exporter already writes both locations correctly; this is purely an
  importer completeness fix.

## Layer & accuracy class
IO only (1 new unconditional reader triplet). Accuracy class: Exact.

## Backend policy
N/A — no compute.

## ARCH rules invoked
- Constitution §6 — a version mismatch / missing block must not silently degrade; this closes a
  case where a real, always-present top-level field was being silently ignored.

## Acceptance test
1. A document with `hasWater`/`waterLevel`/`waterDepth` at the top level and NO
   `mapGeneratorData` block at all (simulating a real official/SupCom map) imports with
   `recipe.water.bEnabled`/`waterLevelMaximum`/`deepWaterDepthMaximum` correctly populated from
   those top-level keys.
2. A document with BOTH the top-level mirrors AND a `mapGeneratorData.Water` block present, where
   the two disagree, imports with the legacy blob's values winning (confirms call-order
   precedence, matching the existing `terrainMaxHeight` precedent).
3. A full SanGen-authored export → import round-trip (both locations present, always in
   agreement, as the exporter already guarantees) is unaffected — confirm no regression via the
   existing round-trip test.
4. Full `SanGenV2` build stays clean; every existing test continues to pass, including
   `MapImporter_IO_Test.cpp` and `MapExporter_IO_Test.cpp`.
