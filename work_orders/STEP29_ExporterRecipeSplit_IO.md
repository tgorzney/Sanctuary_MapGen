# Work-Order — Step 29: split `MapExporter_Recipe_IO.cpp` under the ARCH §1.5 size ceiling

*Constitution §7 (documented exception vs. actual split). Executor: SanGen Coder. Three read-only
consults (IO Architecture Expert for the mechanical plan, ARCH Expert for naming sign-off, Format
Expert for legacy-blob content status — content unchanged by this ticket, see out-of-scope).*

## Root problem
`src/io/MapExporter_Recipe_IO.cpp` is 178 lines, over ARCH §1.5's 150-line hard ceiling (soft 100).
No documented exception exists (Constitution §7 requires one for an over-ceiling file left as-is —
none was filed). `BuildSanmapJsonText` alone is a 98-line function, also over §1.5's 40-line
function cap. The bulk isn't logic — six of its call sites (`GeneralMapSettings`/`HeightmapStack`/
`Symmetry`/`Flow`/`Accumulation`/`DetailNormal`) each restate a near-identical 3-5 line "sibling of
`mapGeneratorData`, not nested in it" comment block.

## Ruled by this ticket (IO Architecture Expert plan, ARCH Expert naming sign-off)
1. **`BuildStratumLayersJson`** moves verbatim into new `src/io/MapExporter_StratumLayers_IO.cpp`
   (no new `.h` — declaration stays in `MapExporter_Recipe_IO.h`, unchanged). Mirrors the
   already-existing `MapImporter_StratumLayers_IO.cpp` 1:1 (same split, same reason, opposite
   direction) — ARCH Expert confirmed no objection.
2. **`BuildMapGeneratorDataJson` stays in `MapExporter_Recipe_IO.cpp`, content unchanged.** ARCH
   Expert's naming ruling: `MapImporter_Recipe_IO.cpp` is already this codebase's established,
   permanent home for `mapGeneratorData`'s remaining fields on the import side (never renamed
   "Legacy_IO" even after most content relocated away) — match that precedent on export, don't
   invent a new filename. **This ticket does NOT touch what `BuildMapGeneratorDataJson` writes** —
   see "Explicit out-of-scope."
3. **`BuildSanmapJsonText` splits into four new file-private helpers** (anonymous namespace, no
   header declarations — nothing outside this file calls them), each taking `(recipe, document&)`
   matching the existing `BuildAtmosphereJson(recipe, document&)` precedent:
   - `BuildDocumentEnvelopeJson` — the ~17 flat root scalars (`fileVersion` through
     `fadeStartDistance`).
   - `AppendEntityDomainsJson` — `stratumLayers`, `StratumGenerationSettings`, `areas`, `armies`,
     `markers`, `chains`, `decals`, `props`, `PropGroups`, `DecalGroups`.
   - `AppendStackDomainsJson` — `MarkersStack`, `PropsStack`, `DecalsStack`, `UnitsStack`,
     `GlobalMarkerSettings`.
   - `AppendSimulationDomainsJson` — `BuildAtmosphereJson`, `SlopeDefaults`, `GeneralMapSettings`,
     `HeightmapStack`, `Symmetry`, `Flow`, `Accumulation`, `DetailNormal`. Collapse the six
     near-duplicate "sibling of `mapGeneratorData`" comment blocks into ONE comment at the top of
     this function — keep each call's short Correction-number reference as a trailing note, don't
     lose that traceability, just stop repeating the full explanation six times.
   - `BuildSanmapJsonText` itself becomes: declare `document`, call the four helpers in order,
     `document["mapGeneratorData"] = BuildMapGeneratorDataJson(recipe)`, the existing ruling-6
     comment + `MergeUnknownImportKeys` call, indent/dump/return. ~16 lines.

Expected result: `MapExporter_Recipe_IO.cpp` ≈ 118-122 lines (under the hard ceiling, close to the
soft one); `MapExporter_StratumLayers_IO.cpp` ≈ 53 lines. Every function in both files lands under
the 40-line cap.

## Target files
- `src/io/MapExporter_Recipe_IO.cpp` — rewrite per ruling 3, `BuildMapGeneratorDataJson` unchanged.
- New `src/io/MapExporter_StratumLayers_IO.cpp` — `BuildStratumLayersJson`'s definition, moved
  verbatim (ruling 1).
- No change to `src/io/MapExporter_Recipe_IO.h` (declarations already correct).
- `CMakeLists.txt` (or wherever `src/io/*.cpp` is globbed/listed) — register the new file if the
  build isn't already glob-based.

## Explicit out-of-scope
- **`BuildMapGeneratorDataJson`'s content** — whether any of its four remaining fields
  (`MapSize`/`TerrainMaxHeight`/`Stratums`/`Water`) should be deleted (per one ARCH Expert
  consult's reading of `SANMAP_FORMAT_SPEC`'s "Verified deletions" list) or kept until they get a
  real top-level replacement home (per a separate Format Expert finding: `TerrainMaxHeight`'s full
  float precision, `Water.DeepWaterDepthMin`, and `Stratums[].ImportedMaskMode`/`.Enabled` have NO
  top-level equivalent today and would be silently lost on deletion) is an open, unreconciled
  conflict between two expert consults — not this ticket's to resolve. This ticket moves/splits
  code only; it changes zero JSON output. A separate future ticket settles the blob's actual fate.
- **`MapImporter::ParseSanmapJsonText`** (`MapImporter_IO.cpp:66-205`) — flagged by the IO
  Architecture Expert as having the identical, larger (~140-line) size-ceiling violation. Separate,
  future ticket — mirrored split treatment, not this one.
- **The `document["mapGeneratorData"] = BuildMapGeneratorDataJson(recipe)` call itself** — stays
  exactly where it is, unmoved by the helper split (it's part of `BuildSanmapJsonText`'s own
  remaining ~16 lines, not folded into any of the four new helpers, since it isn't one of the
  format-native top-level sections those helpers group).

## Layer & accuracy class
IO only, pure refactor — zero behavior change, zero JSON output change. Accuracy class: Exact.

## Backend policy
N/A.

## ARCH rules invoked
- ARCH §1.5 — 150-line hard file ceiling, 40-line function cap, `Type_Aspect_LAYER` split pattern.
- Constitution §7 — this ticket resolves the undocumented ceiling violation via an actual split,
  not a filed exception.

## Acceptance test
1. `MapExporter_StratumLayers_IO.cpp` produces byte-identical `stratumLayers` JSON output to
   before the move, for a non-trivial recipe (existing `MapExporter_IO_Test.cpp` coverage should
   already catch any regression — confirm it does, don't skip re-running it).
2. Every other top-level key `BuildSanmapJsonText` produces is byte-identical before/after the
   split — this is a pure refactor, so the simplest real acceptance test is: export the same
   non-trivial recipe before and after the change and diff the two JSON strings; they must be
   identical. Do this as an explicit test, not just "existing tests still pass" (existing tests may
   not exercise every field).
3. Both files pass ARCH §1.5's ceiling: `MapExporter_Recipe_IO.cpp` under 150 lines,
   `MapExporter_StratumLayers_IO.cpp` under 150 lines, every function in both under 40 lines.
4. Full `SanGenV2` build stays clean; every existing test continues to pass.
