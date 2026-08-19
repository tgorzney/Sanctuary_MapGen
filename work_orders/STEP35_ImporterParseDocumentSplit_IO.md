# Work-Order — Step 35: relocate `ParseSanmapJsonText` out of `MapImporter_IO.cpp`

*Constitution §7. Executor: SanGen Coder. IO Architecture Expert consult. Mirrors `STEP29`/`STEP31`
on the export side, opposite direction.*

## Root problem
`src/io/MapImporter_IO.cpp` is 233 lines (150-line hard ceiling; already 174 before `STEP24`, has
grown since). Unlike the exporter (`MapExporter_IO.cpp` = disk-writing actions,
`MapExporter_Recipe_IO.cpp` = orchestration — two files), the importer welds both concerns into
one file: `ResolveSanmapDocumentPath`/`ReadDocumentText`/`LoadSanmap` (disk/path aspect) AND
`MapImporter::ParseSanmapJsonText` (the ~138-line in-memory JSON-assembly orchestrator) live
together. Every domain reader is ALREADY split into its own file
(`MapImporter_Areas_IO.cpp`/`MapImporter_Atmosphere_IO.cpp`/etc.) — there is no inline leaf
function left to relocate; the lever here is the orchestrator itself.

## Ruled by this ticket
**`MapImporter_IO.cpp` stays** — keeps `ResolveSanmapDocumentPath`, the private
`ReadDocumentText`, and `MapImporter::LoadSanmap` unchanged. Drops now-unneeded includes.
Est. **~85-95 lines**.

**New `src/io/MapImporter_ParseDocument_IO.cpp`** (no new `.h` — `MapImporter::ParseSanmapJsonText`
is already declared in `MapImporter_IO.h`; pure `.cpp`-only relocation). Holds the definition of
`MapImporter::ParseSanmapJsonText` plus 4 file-private helpers mirroring
`MapExporter_DocumentAssembly_IO.cpp`'s shipped shape (from `STEP31`) 1:1:
- `ParseDocumentEnvelopeJson` — `name`/`credits`, `hasWater`/`waterLevel`/`waterDepth`,
  `deepWaterDepthMin`, `height`, `width`.
- `ParseEntityDomainsJson` — `Areas`/`Armies`/`Markers`/`Chains`/`PropGroups`/`Props`/
  `DecalGroups`/`Decals`/`StratumLayers`/`StratumGenerationSettings`.
- `ParseStackDomainsJson` — `MarkersStack`/`GlobalMarkerSettings`/`PropsStack`/`DecalsStack`/
  `UnitsStack`.
- `ParseSimulationDomainsJson` — `Atmosphere`/`SlopeDefaults`/`GeneralMapSettings`/
  `HeightmapStack`/`Symmetry`/`Flow`/`Accumulation`/`DetailNormal`.
- `ParseSanmapJsonText` itself shrinks to: parse+validate try/catch, the migration-runner call
  (kept as its OWN explicit top-level line — do not fold it into a helper, its "LITERAL FIRST
  thing that touches the document" ordering law must stay visible at the orchestrator's own top
  level), the 4 helper calls, the gated `mapGeneratorData` tail
  (`ReadGeometryJson`/`ReadWaterJson`/`ReadStrataSettingsJson`) stays inline exactly like
  `BuildMapGeneratorDataJson`'s call stays inline in the exporter's own orchestrator.
- Est. total **~110-130 lines**.

**⚠️ Not a pure mechanical move — real reordering risk, treat with care.** Grouping into these 4
helpers to mirror the exporter's shape requires reordering two clusters relative to the file's
CURRENT physical order: `StratumLayers`/`StratumGenerationSettings` currently run AFTER
Atmosphere/SlopeDefaults/Flow/Accumulation/DetailNormal; `GeneralMapSettings`/`HeightmapStack`/
`Symmetry` currently run AFTER the stack-domain group. Every documented ordering comment in the
current file was checked against this reorder and none is violated (every group's own comment
only says "unconditional, before the `mapGeneratorData` gate," never "before/after sibling group
X") — but this is a real behavior-preserving reorder, not cut-paste, and must not be called done
on inspection alone. **Mandatory**: run the existing full round-trip test suite
(`MapExporter_IO_Test.cpp`/`MapImporter_IO_Test.cpp`) and confirm green BEFORE reporting this
ticket complete — if ANY test fails, do not silently "fix the test to match" — stop, and if the
reorder is genuinely unsafe, fall back to 5 helpers that preserve the CURRENT physical order
verbatim (same file-size outcome, just doesn't achieve literal 1:1 naming/grouping symmetry with
the exporter's helpers) rather than force the reorder through.

## Target files
- `src/io/MapImporter_IO.cpp` — trim to the 3 kept functions, drop unneeded includes.
- New `src/io/MapImporter_ParseDocument_IO.cpp` — `ParseSanmapJsonText` + the 4 helpers.
- No change to `src/io/MapImporter_IO.h` (declaration already correct, unchanged).

## Explicit out-of-scope
- `MapImporter_Recipe_IO.h`'s comment trim (a separate, sequenced ticket, `STEP36`) — do not touch
  that file in this ticket.
- Any behavior change beyond the documented-safe reorder above.

## Layer & accuracy class
IO only, refactor with a flagged reorder risk requiring real test verification, not just
inspection. Accuracy class: Exact.

## Acceptance test
1. Both files land under 150 lines.
2. The FULL existing round-trip test suite passes, not spot-checked — this is the test that
   actually proves the reorder is safe, given it wasn't provable by inspection alone.
3. An explicit before/after export→import→export byte-diff test for a non-trivial recipe with
   every domain populated (same technique as `STEP29`/`STEP31`) — confirms the reorder produces
   identical recovered data, not just "no crash."
4. Full `SanGenV2` build stays clean.
