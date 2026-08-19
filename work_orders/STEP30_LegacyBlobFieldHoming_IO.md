# Work-Order — Step 30: give the 4 legacy-blob-only fields real top-level homes

*Constitution §6/§8. Executor: SanGen Coder. Human directive: do NOT delete the legacy
`mapGeneratorData` blob until export coverage is 100% — this ticket is that closure. Mirrors the
already-shipped `STEP25`/`STEP27` pattern exactly; no new expert consult needed.*

## Root problem
4 fields exist ONLY inside the legacy `mapGeneratorData` blob today (no top-level mirror), all
confirmed real and actively used, none deprecated:
1. `geometry.terrainMaxHeight` (full float precision) — the top-level `height` mirror is a lossy
   `int` (`MapExporter_Recipe_IO.cpp`, `BuildDocumentEnvelopeJson`). `terrainMaxHeight` is the
   value every generation stage scales the heightfield by (confirmed: `Mask_Slope_PROC.h`,
   `Thermal_Kernel_PROC.h`, `Placement_RuleBuild_PROC.h`, etc.).
2. `Stratum::importedMaskMode` — a real 3-state enum (`Disabled`/`ProceduralStart`/
   `StaticOverride`) consumed by `Mask_Prepare_PROC.cpp`/`Mask_PROC.cpp` + a GLSL twin
   (`Mask_Merge_PROC.glsl`), wired to a live UI toggle (`StratumsTab_Material_UI.cpp`).
3. `Stratum::bEnabled` — whether a stratum slot participates in generation at all.
4. `Water::deepWaterDepthMinimum` — drives the deep-water preview color gradient
   (`PreviewComposite_UI.cpp`) and is a live Water-tab range-slider min value
   (`WaterTab_UI.h:78,110,118,120`). Not an official `.sanmap` field (confirmed against the
   ground-truth `SanMap.cs`), but real and designer-editable in this codebase today.

## Solution — shape
**1. `terrainMaxHeight` → `GeneralMapSettings`, mirroring `terrainMinHeight`'s already-shipped
home exactly** (`MapExporter_GeneralMapSettings_IO.cpp`/`MapImporter_GeneralMapSettings_IO.cpp`
already write/read `TerrainMinHeight`). Add the sibling key `TerrainMaxHeight` (PascalCase,
matching convention) to the same object, full float precision. The existing lossy top-level
`height` (int) mirror stays untouched (official-format-required, engine reads it) — this just adds
the precise value alongside it, same relationship `waterLevel`(lossy-ish top-level)/`WaterLevelMax`
(blob) had before STEP27, now resolved the correct way (a real, precise top-level home, not a
second copy in a doomed blob).

**2 & 3. `Stratum::importedMaskMode`/`bEnabled` → new sibling fields on each `stratumLayers[]`
array entry**, matching Correction 13's own framing ("`stratumLayers[9]` is the single source of
truth" for stratum data) and the exact pattern `radialSymmetryRepeatCount` used to join an
existing array-of-objects as a new per-entry key (`STEP23`/`STEP16`). New keys:
`"ImportedMaskMode"` (int, enum ordinal, `ReadJsonEnumeration` with `valueCount = 3`), `"Enabled"`
(bool). Written/read in `MapExporter_StratumLayers_IO.cpp`/`MapImporter_StratumLayers_IO.cpp`
(the files STEP29 already split out) alongside the existing per-layer fields
(`albedo`/`normal`/`mask`/`tileSize`/etc.).

**4. `Water::deepWaterDepthMinimum` → new top-level sibling of the STEP27-shipped
`hasWater`/`waterLevel`/`waterDepth` trio.** JSON key `"deepWaterDepthMin"` (camelCase, matching
the sibling trio's casing exactly — the official format's own `Water` region uses camelCase for
`hasWater`/`waterLevel`/`waterDepth`, unlike the PascalCase used inside SanGen-owned sections like
`GeneralMapSettings`). Read/written in `MapExporter_Recipe_IO.cpp`
(`BuildDocumentEnvelopeJson`)/`MapImporter_IO.cpp`, same tier as the STEP27 trio, unconditional,
before the `mapGeneratorData` gate. The gated legacy `ReadWaterJson` call still wins on overlap for
a SanGen-authored file with both present (matching the STEP27 precedent exactly).

## Target files
- `src/io/MapExporter_GeneralMapSettings_IO.cpp`/`MapImporter_GeneralMapSettings_IO.cpp` — add
  `TerrainMaxHeight` read/write, sibling of `TerrainMinHeight`.
- `src/io/MapExporter_StratumLayers_IO.cpp`/`MapImporter_StratumLayers_IO.cpp` — add
  `ImportedMaskMode`/`Enabled` read/write per stratum-layer entry.
- `src/io/MapExporter_Recipe_IO.cpp` (`BuildDocumentEnvelopeJson`)/`src/io/MapImporter_IO.cpp` —
  add `deepWaterDepthMin` unconditional top-level read/write, sibling of the STEP27 trio.
- Test files for all of the above — extend existing round-trip coverage
  (`MapExporter_IO_Test.cpp`, `MapImporter_IO_Test.cpp`) to assert these 4 fields survive
  export → import with non-default values.

## Explicit out-of-scope
- **Deleting anything from the legacy blob** — not this ticket. Once this ships, export coverage
  of the blob's remaining 4 live fields is 100% (the other 2, `MapSize`/pure-duplicate `Stratums`
  sub-fields already covered by `stratumLayers[]`, were already confirmed pure duplicates, safe to
  eventually delete — but the human's own instruction is "THEN we will see about deleting it,"
  a separate future decision, not automatic once this ticket ships).
- **`terrainMinHeight`'s dormancy** — still not consumed by any generation stage; this ticket does
  not change that, only ensures its sibling `terrainMaxHeight` has equally complete IO coverage.
  Binding it into actual generation math is separate, future PROC work (already flagged in its own
  PARAMS comment as needing "a stage work-order").
- **Any generation/PROC behavior change** — this is IO-layer completeness only.

## Layer & accuracy class
IO only (PARAMS unchanged — all 4 fields already exist as struct members; this is purely giving
them JSON read/write coverage they're missing). Accuracy class: Exact.

## Acceptance test
1. A recipe with non-default `terrainMaxHeight` (a non-round float, e.g. `142.375`) round-trips at
   full precision through `GeneralMapSettings`.
2. A recipe with a non-default `importedMaskMode`/`bEnabled` on at least one stratum round-trips
   through its `stratumLayers[]` entry.
3. A recipe with non-default `deepWaterDepthMinimum` round-trips through the new top-level key.
4. A document with BOTH the new top-level/array homes AND the legacy blob present, disagreeing,
   imports with the legacy blob's values winning for all 4 (matching STEP27's precedent — the
   blob stays authoritative until it's actually deleted).
5. Full `SanGenV2` build stays clean; every existing test continues to pass.
