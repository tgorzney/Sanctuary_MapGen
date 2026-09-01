# STEP243 — Round-trip the 7 new `MarkerLink` fields through `.sanmap`

**Layer:** IO. **Domain:** `src/io/MapExporter_MarkerLink_IO.cpp`,
`src/io/MapImporter_MarkerLink_IO.cpp`. **Sequence:** depends on STEP241 (landed) and STEP242
(bLocked, in flight) — needs the full, final `Params::MarkerLink` field set before wiring IO for
all of it at once.

Ratifies `ARCH_19_30_MarkerLinksWireShape.md` (original wire shape) extended to cover the 7 fields
STEP241/242 added: `bHidden`, `iconScale`, `bGridSnapEnabled`, `gridSnapSizeWorldUnits`,
`bSymmetryEnabled`, `symmetry`, `bLocked`.

## Session coordination

Check `ListAgents`/message peer sessions before touching EACH file. Confirm STEP242 has landed —
read the actual current `MarkerLink_PARAMS.h` field list first; do not assume this ticket's field
list is complete if a further correction landed after this ticket was written.

## Problem

STEP237 built the original `MarkerLinks` wire array (`Identifier`/`Name`/`ColorOverrideEnabled`/
`Color`). STEP241/242 added `bHidden`/`iconScale`/`bGridSnapEnabled`/`gridSnapSizeWorldUnits`/
`bSymmetryEnabled`/`symmetry`/`bLocked` to the PARAMS struct, but the exporter/importer were never
touched — those seven fields don't currently survive a save/load round-trip.

## Fix

Mirror the wire-key convention already used for the identical fields on `MarkerInstanceLayer`
(same field, same meaning, just promoted to also live on `MarkerLink`):
- `Hidden` (bool) — mirrors `MarkerGroups[].Hidden`.
- `IconScale` (float) — mirrors `MarkerGroups[].IconScale`.
- `GridSnapEnabled` (bool), `GridSnapSizeWorldUnits` (float) — mirrors the existing
  `MarkerGroups[]` grid-snap keys.
- `SymmetryEnabled` (bool), `Symmetry` (object) — mirrors the existing `MarkerGroups[]` symmetry
  wire shape for `Params::SymmetrySetting`.
- `Locked` (bool) — mirrors `MarkerGroups[].Locked`.

Add all seven to `BuildMarkerLinksJson` (exporter) and `ReadMarkerLinksJson` (importer). No
`SanGenVersion` bump (same additive precedent as the original four fields). Absent-on-import for
any of the seven → the struct's own default (matches every other field in this struct).

## Verify

- Extend `MapExporter_MarkerLink_IO_Test.cpp` / `MapImporter_MarkerLink_IO_Test.cpp` for full
  round-trip coverage of all 11 `MarkerLink` fields (4 original + 7 new), non-default values.
- A `.sanmap` written before this ticket (missing the 7 new keys) still imports cleanly, each new
  field taking its struct default.
- Existing `MapExporter_IO_Test`/`MapImporter_IO_Test` combined suites stay green.

## Out of scope

- Any further PARAMS/UI changes — this is IO-only, wiring what STEP241/242 already built.
