# Work-Order — Step 18: `DetailNormal` — schema-v3 Correction 8

*Constitution §7. Executor: SanGen Coder. Implements `SANMAP_FORMAT_SPEC.md` Correction 8 —
reserves the top-level key and its one live field, `DetailNormalMapSize`. The future layered-
heightmap-delta system itself is explicitly deferred; this ticket is settings-only.*

## Root problem
`DetailNormalMapSize` has no PARAMS home today — confirmed by direct read of
`DetailNormalTab_UI.h`'s own SCOPE NOTE 2: "THE DETAIL-NORMAL SIZE has no PARAMS home in v2... It
is caller-owned tab state, the same standing as the Heightmap tab's global gravity." Not
format-native (zero matches in the real `SanMap.cs` ground truth) — a genuinely new, SanGen-owned
field, same class of addition as `GeneralMapSettings::globalGravity` (Step 14).

## Target files
New: `src/params/DetailNormal_PARAMS.h` (`Params::DetailNormal { int mapSize = 1024; }` — 1024
matches the v1 default already live in `DetailNormalTab_UI.h:45`, not invented).
`src/io/MapExporter_DetailNormal_IO.cpp` / `MapImporter_DetailNormal_IO.cpp`.

Modified: `src/params/MapRecipe_PARAMS.h` — add `DetailNormal detailNormal;` as a flat sibling of
`flow`/`accumulation`.

## Layer & accuracy class
PARAMS + IO/BRIDGE. Accuracy class: Exact.

## Backend policy
CPU only. No PROC consumer — the layered-heightmap-delta system doesn't exist; this reserves the
setting only, same posture as every other settings-before-a-consumer field this session.

## ARCH rules invoked
- `SANMAP_FORMAT_SPEC.md` Correction 8 — binding, implement verbatim.
- Constitution §8 — the field becomes real and settable rather than staying UI-only/unpersisted.

## Solution — shape
```
DetailNormal: { DetailNormalMapSize: <int> }
```
Top-level, unconditional, before the `mapGeneratorData` gate on import — same tier as every prior
top-level-key ticket this session.

## Explicit out-of-scope
- **The layered-heightmap-delta system itself** (a stack of heightmaps producing a delta normal
  map) — real, future PROC/generator design work, not this ticket.
- **UI wiring** — `DetailNormalTab_UI.h`'s `detailNormalSizeIndex` stays exactly as caller-owned
  tab state; not rewired to read from/write to `recipe.detailNormal.mapSize`. Same exclusion every
  PARAMS ticket this session has had.

## Acceptance test
A `Params::MapRecipe` with a non-default `detailNormal.mapSize` (e.g. `2048`) survives
export→import exactly. Full `SanGenV2` build stays clean; every existing test continues to pass.
