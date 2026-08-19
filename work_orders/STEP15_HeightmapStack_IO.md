# Work-Order — Step 15: `HeightmapStack` — schema-v3 Correction 3

*Constitution §7. Executor: SanGen Coder. Implements `SANMAP_FORMAT_SPEC.md` Correction 3
verbatim. Replaces `GeoLayers` (currently nested in the legacy `mapGeneratorData` blob) with a new
top-level `HeightmapStack` section that also folds in `SimulationGrouping` (currently a stray
top-level `mapGeneratorData` key). Adds a real, ratified new field pair to `GeoLayer`/`Layer`
(a symmetry override) — but with NO PROC consumer, since no heightfield-symmetry stage exists yet
(Correction 4/ARCH territory, explicitly deferred) — this ticket is settings-only for that part.*

## Root problem
`GeoLayers`/`SimulationGrouping` currently live inside `mapGeneratorData`
(`BuildLayerStackJson`/`ReadLayerStackJson`, confirmed by direct read: `MapExporter_Recipe_IO.cpp`
writes `generatorData["GeoLayers"]`/`generatorData["SimulationGrouping"]` as two separate stray
top-level keys inside that blob). Correction 3 relocates both into one new top-level
`HeightmapStack` section, and gives `GeoLayer`/`Layer` a local symmetry override — matching the
pattern already live on every placement rule type (`MarkerRule`/`PropRule`/`DecalRule`/`UnitRule`,
including the DecalRule fix shipped earlier this session).

## Target files
New: `src/io/MapExporter_HeightmapStack_IO.cpp` / `MapImporter_HeightmapStack_IO.cpp`.

Modified:
- `src/params/GeoLayer_PARAMS.h` — add `bool bSymmetryUseGlobal = true; int symmetryMask = 0;`
  (same field names/defaults/position convention as `PropRule`/`UnitRule`/`DecalRule`).
- `src/params/Layer_PARAMS.h` — same two fields.
- `src/io/MapExporter_Layers_IO.cpp` — **remove** `BuildLayerStackJson` (relocated wholesale,
  content unchanged except the two new symmetry fields, into the new file). `BuildStratumJson`/
  `BuildStrataSettingsJson` in this same file are UNRELATED (Stratum content) — do not touch them.
- `src/io/MapImporter_Layers_IO.cpp` — **remove** `ReadLayerStackJson` (relocated wholesale, same
  reasoning). Leave nothing else in this file if it becomes empty after the move — check whether
  the file should be deleted entirely or kept as a stub matching the Stratum-only remainder (the
  file's own header comment currently frames it as "the layer stack and the per-stratum
  settings" — since it's the LAYERS file, not a Stratum file, deleting it once `BuildLayerStackJson`
  moves out is likely correct, but double check no other function still lives there before deleting).
- `src/io/MapExporter_Recipe_IO.cpp` — `BuildMapGeneratorDataJson`: remove
  `generatorData["GeoLayers"]`/`generatorData["SimulationGrouping"]`. `BuildSanmapJsonText`: add
  `document["HeightmapStack"] = BuildHeightmapStackJson(recipe.layerStack);` at top level.
- `src/io/MapImporter_Recipe_IO.cpp` — remove the `ReadLayerStackJson(generatorData, ...)` call
  from wherever it's currently invoked (inside the `mapGeneratorData`-gated block).
- `src/io/MapExporter_Recipe_IO.h`/`MapImporter_Recipe_IO.h`, `MapImporter_IO.cpp` — declare/wire
  the new builder/reader, top-level, unconditional, before the `mapGeneratorData` gate — same tier
  as every prior top-level-key ticket this session.

## Layer & accuracy class
PARAMS (2 new fields on 2 types, no new type) + IO/BRIDGE. Accuracy class: Exact.

## Backend policy
CPU only. **No PROC change of any kind** — the new symmetry fields have zero consumers; no
heightfield-symmetry stage exists in this codebase (confirmed, `SANMAP_FORMAT_SPEC.md`/
`IO_PARITY_REPORT.md` Decision #6: "No v2 code implements a heightfield-symmetry PROC stage
today"). This is the same "settings from the moment they are settable" posture already used for
`StratumAppearance` (Constitution §8) — reserve the field, wire no behavior.

## ARCH rules invoked
- `SANMAP_FORMAT_SPEC.md` Correction 3 — binding, implement verbatim.
- `LAYER_SYSTEM_SPEC.md` — confirms the flat `LayerStack` → `GeoLayer` → `Layer` model is unchanged
  by this ticket; no grouping tier is introduced.
- ARCH §1.1 (`b`-prefix), Constitution §8 (real settings, not hardcoded).

## Solution — shape
```
HeightmapStack: {
    SimulationGrouping: <int>,   // relocated verbatim from the stray top-level generatorData key
    GeoLayers: [ <same GeoLayer array shape BuildLayerStackJson already produces, plus the 2 new
                  symmetry fields per GeoLayer AND per Layer> ]
}
```
`GeoLayer`/`Layer`'s existing ~6/~25 fields keep their exact current JSON key spellings (verbatim
relocation, per spec — this is a container change, not a field-name change). New fields:
`json["SymmetryUseGlobal"] = geoLayer.bSymmetryUseGlobal; json["SymmetryMask"] =
geoLayer.symmetryMask;` on BOTH the `GeoLayer` and `Layer` builders/readers, same key names already
used by `MarkerRule`/`PropRule`/etc.'s symmetry override.

**Named gap, explicitly deferred (per the spec's own instruction — log it, do not fix it):** the
real map format's per-layer `MinHeight`/`MaxHeight`/`MinSlope`/`MaxSlope` height-and-slope gates
have no equivalent field on `Params::Layer` at all today — confirmed absent. This ticket does NOT
add them. Leave a code comment at the relevant read/write site noting the gap (matching the
pattern `StratumAppearance_PARAMS.h`'s own header comments already use for similar flagged gaps),
so a future reader doesn't assume the omission is an oversight of THIS ticket.

## Explicit out-of-scope
- **The heightfield-symmetry PROC stage itself** — real, ratified as wanted, but explicitly
  deferred PROC/pipeline design work for a separate generator-expert/ARCH ticket. This ticket
  reserves and round-trips the two new fields only.
- **`Correction 4`'s global `Symmetry` section** (`SymAlgorithm`, blur/crossfade/cylinder/torus
  settings, `SnapImperfectSymmetry`, `SymmetryDetectionTolerance`, `GlobalSymmetryMask` relocation)
  — separate ticket (tracked).
- **The per-layer height/slope gate gap** — flagged, not fixed, per the spec's own explicit
  instruction. Do not invent fields to close this gap.
- **Any internal `HeightmapStack` layer redesign** (recursive `GeoLayer` grouping) — explicitly
  out of scope for SPEC-4 itself; `GeoLayer` still cannot contain another `GeoLayer`.
- **`GlobalSymmetryMask`** — stays exactly where it is (`recipe.globalSymmetryMask`, currently
  written inside `mapGeneratorData`) — Correction 4's job, not this ticket's, even though it's the
  fallback value `bSymmetryUseGlobal == true` would eventually resolve against once a PROC stage
  exists. Do not relocate it here.

## Acceptance test
A `Params::LayerStack` with a non-default `simulationGrouping`, at least one `GeoLayer` with
non-default fields INCLUDING `bSymmetryUseGlobal = false` and a specific `symmetryMask`, and at
least one `Layer` inside it with the same non-default symmetry override, survives export→import
exactly through `HeightmapStack`. Confirm `"GeoLayers"`/`"SimulationGrouping"` no longer appear
under `document["mapGeneratorData"]` after this ticket (raw JSON text check, same style as prior
relocation tickets). Confirm every existing `Layer`/`GeoLayer` field (noise/blend/levels/etc.)
still round-trips exactly through the new location — this is the bulk of the content, a pure
relocation, and must not silently drop or corrupt any of the ~25 `Layer` fields or ~6 `GeoLayer`
fields in the move. Full `SanGenV2` build stays clean; every existing test continues to pass.
