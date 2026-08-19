# Work-Order — Step 17: `Flow`/`Accumulation` reserved sections — schema-v3 Correction 6

*Constitution §7. Executor: SanGen Coder. Implements `SANMAP_FORMAT_SPEC.md` Correction 6 —
explicitly "both reserved, field lists TBD." Deliberately minimal: two new SanGen-owned top-level
sections with no format precedent (confirmed zero matches for "Flow" anywhere in the real
`SanMap.cs` ground truth — this is a forward-looking reservation, not a relocation), no PROC
consumer, matching the "settings before a stage exists" posture used throughout this session.*

## Root problem
Neither `Flow` nor `Accumulation` exists in the `.sanmap` format or in `src/`. Correction 6
reserves both as future homes for a two-simulation velocity→accumulation model that doesn't exist
yet (confirmed distinct from both `ErosionLayerSettings` and the current `FlowAccumulation`
drainage/routing stage — neither of those moves or is touched by this ticket). The one concrete
field named by the spec, `FlowMapColor` (a preview tint), has no current PARAMS home either.

## Target files
New: `src/params/Flow_PARAMS.h` (`Params::Flow { float flowMapColor[4] = {0.2f, 0.4f, 1.0f, 1.0f};
}` — a sane placeholder default, a blue-ish tint; not prescribed by the spec).
`src/params/Accumulation_PARAMS.h` (`Params::Accumulation {}` — genuinely empty; the spec has no
field list at all for this section yet, "TBD" means TBD).
`src/io/MapExporter_FlowAccumulation_IO.cpp` / `MapImporter_FlowAccumulation_IO.cpp`.

Modified: `src/params/MapRecipe_PARAMS.h` — add `Flow flow; Accumulation accumulation;` as flat
siblings of `water`/`atmosphere`.

## Layer & accuracy class
PARAMS + IO/BRIDGE. Accuracy class: Exact for the one real field; N/A for the empty section.

## Backend policy
CPU only. No PROC consumer for either section — this ticket reserves the keys only.

## ARCH rules invoked
- `SANMAP_FORMAT_SPEC.md` Correction 6 — binding, implement verbatim (which, for `Accumulation`,
  means implementing literally nothing beyond the empty reserved key).
- Constitution §8 — `flowMapColor` becomes a real settable field rather than staying absent.

## Solution — shape
```
Flow: { FlowMapColor: {r,g,b,a} }   // {r,g,b,a} object, same convention as armyColor/atmosphere colors
Accumulation: {}                     // empty object, reserved
```
Both are top-level, unconditional, written/read before the `mapGeneratorData` gate on import —
same tier as every prior top-level-key ticket this session.

## Explicit out-of-scope
- **Any real Flow/Accumulation simulation** — the two-simulation velocity→accumulation model is
  explicitly deferred, real PROC/pipeline design work for a future generator-expert/ARCH ticket.
  This ticket writes/reads a color and an empty object, nothing else.
- **`ErosionLayerSettings`** and **the current `FlowAccumulation` drainage/routing stage**
  (`FlowAccumulationConstants`) — both confirmed distinct, both untouched.
- **`SlopeSettingsParams`'s `bUseEngineParityMath`** — unrelated field, no home decided, not this
  ticket's job.
- **UI wiring** — no tab reads from or writes to `recipe.flow`/`recipe.accumulation`.

## Acceptance test
A `Params::MapRecipe` with a non-default `flow.flowMapColor` survives export→import exactly.
`Accumulation` writes as an empty JSON object and reads back without error regardless of content
(forward-compatible: a future ticket adding real fields must not need this ticket's reader
rewritten just to tolerate unknown keys — confirm the reader ignores unrecognized keys rather than
warning/failing on them, matching this project's general degrade-gracefully IO posture). Full
`SanGenV2` build stays clean; every existing test continues to pass.
