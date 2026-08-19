# Work-Order — Step 16: `Symmetry` global section — schema-v3 Correction 4 (minus `SymAlgorithm`)

*Constitution §7. Executor: SanGen Coder. Implements `SANMAP_FORMAT_SPEC.md` Correction 4 and
`ARCH.md` §13 (Radial N-fold symmetry), narrowed by an explicit ARCH Expert scoping ruling
obtained for this ticket (see "Ruled by this ticket" below) — the spec text has real ambiguity
about `SymAlgorithm` this ticket resolves by consult rather than guessing.*

## Root problem
`GlobalSymmetryMask` currently lives inside the legacy `mapGeneratorData` blob. `RadialSymmetryRepeatCount`
(the companion count for the new `SymmetryAxis::Radial` bit, ARCH §13) doesn't exist anywhere.
`SymmetryDetection` (`detectionTolerance`/`bSnapImperfectSymmetry`) has no aggregate home on
`MapRecipe` at all today (caller-owned, per `SymmetryTab_UI.h` SCOPE NOTE 2) and no IO round-trip.
Six more exotic-blend scalars (`SymSuperpositionBlend`/`SymmetryBlurRadius`/`CrossFadeWidth`/
`CylinderZScale`/`TorusMajorRadius`/`TorusMinorRadius`) don't exist in `src/` at all.

## Ruled by this ticket (ARCH Expert consult, resolving real spec ambiguity)
1. **`Params::SymAlgorithm` (the 6-value enum) is OUT OF SCOPE.** Defining a brand-new enum type
   is a bigger design act than the other fields (plain scalars/bools); the spec's own "whichever
   FUTURE work-order defines `Params::SymAlgorithm`... must default it to `Superposition`" phrasing
   only makes sense if no ticket has defined it yet, including this one. This ticket writes the
   OTHER 10 Correction-4 global-section fields (`GlobalSymmetryMask`, `RadialSymmetryRepeatCount`,
   `SnapImperfectSymmetry`, `SymmetryDetectionTolerance`, `SymSuperpositionBlend`,
   `SymmetryBlurRadius`, `CrossFadeWidth`, `CylinderZScale`, `TorusMajorRadius`,
   `TorusMinorRadius`) — zero PROC consumer for any of them, same "settings before a stage exists"
   posture already used for `StratumAppearance` and the `GeoLayer`/`Layer` symmetry override. Do
   NOT add a `SymAlgorithm` field or JSON key. Leave a code comment flagging the deferral
   explicitly so it isn't mistaken for an oversight of this ticket.
2. **`radialSymmetryRepeatCount` retrofit onto `GeoLayer`/`Layer` IS in scope** (not a separate
   ticket) — those two types already shipped `bSymmetryUseGlobal`/`symmetryMask` (a prior ticket,
   before this ratification existed) and ARCH §13 names them as one of the homes needing their own
   `N`. Add it alongside the other 6 homes below — 7 total, not 5.
3. **The `symmetryOrbitMaximum = 16` buffer-overflow risk stays dormant, no same-ticket fix
   needed.** Confirmed by direct read: `BuildSymmetryOrbit` (`Placement_Symmetry_PROC.h`) has
   exactly 4 explicit branches (MirrorX/MirrorZ/RotateHalfTurn/QuarterTurns) — not a generic
   walk-every-bit loop. Adding the `Radial` BIT with no orbit-generation branch for it means a mask
   with that bit set is simply ignored by the orbit builder today — dead data, no overflow. The
   actual N-way rotation generator (`AppendRadialTurns`, generalizing `AppendQuarterTurns` from a
   hardcoded 3 turns to a designer count) is explicitly separate, future PROC work, NOT this
   ticket. Add ONE test asserting `BuildSymmetryOrbit` does not yet branch on `Radial` (documents
   the dormancy so a future reviewer re-checks the buffer risk when that PROC ticket lands).
4. **Changing `MapRecipe::globalSymmetryMask`'s default from `None` to `RotateHalfTurn`
   (ARCH-ratified) breaks two confirmed tests — fix both in this ticket:**
   - `src/ui/SymmetryTab_UI_Test.cpp:72` asserts a fresh recipe's mask is `None` — update to assert
     `RotateHalfTurn` (with an updated label).
   - `src/proc/Placement_PROC_Test.cpp`'s `MakeRecipe()` fixture never sets `globalSymmetryMask`
     and has a spawn rule with `bSymmetryUseGlobal = true` — with the new default, symmetry clones
     would multiply `markers.Count()` past the test's hardcoded expectation of exactly 6. Add
     `recipe.globalSymmetryMask = Params::SymmetryAxis::None;` explicitly to `MakeRecipe()` — this
     test validates Poisson spacing/gates, not symmetry; pin it explicitly rather than reworking
     the count math.
   - **Audit required, not optional:** grep the full test suite for default-constructed
     `Params::MapRecipe`/`MapRecipe recipe;` that flow into `Placement_PROC`/`Placement_Rules_PROC`/
     `Placement_Accept_PROC` and assert an exact instance count or exact position. Pin
     `globalSymmetryMask = SymmetryAxis::None` on any such fixture that isn't specifically testing
     symmetry (same fix as `Placement_PROC_Test.cpp` above). `Placement_Gpu_PROC_Test.cpp`'s
     `MakeParityRecipe()` was flagged as needing this same audit — its assertions are loose
     (`Count() > 0`, CPU/GPU parity) and likely survive, but verify directly rather than assuming.
     `Placement_Symmetry_PROC_Test.cpp`'s `MakeSymmetricRecipe()` explicitly sets the mask from a
     parameter — confirmed unaffected, no action needed there.
   - **Third confirmed break (ARCH Expert catch, not left to the generic audit alone):**
     `src/pipeline/GenerationAssembler_TestScene_PIPELINE.h`'s shared `MakeRecipe()` — used by
     three `GenerationAssembler_*_PIPELINE_Test.cpp` binaries — has a `MarkerRule` with no
     `bSymmetryUseGlobal` override (defaults `true`), and
     `GenerationAssembler_Outputs_PIPELINE_Test.cpp:36`'s `CheckPlacement()` asserts
     `placements.markers.Count() == static_cast<std::size_t>(markerCount)` — an exact count,
     structurally identical to the `Placement_PROC_Test.cpp` break. Pin
     `recipe.globalSymmetryMask = Params::SymmetryAxis::None;` in this `MakeRecipe()` too. Its
     other, UI-side consumers use loose `Count() > 0` bounds and are confirmed safe.

## Target files
- `src/params/Symmetry_PARAMS.h` — add `constexpr int Radial = 1 << 4;` to the `SymmetryAxis`
  namespace. Add a new `Params::SymmetryBlend` struct (name chosen for this ticket — the spec pins
  field names/JSON keys, not a struct name) holding the 6 exotic-blend scalars: `float
  superpositionBlend`, `float blurRadius`, `float crossFadeWidth`, `float cylinderZScale`, `float
  torusMajorRadius`, `float torusMinorRadius`. Defaults are placeholders — pick sane values per
  Constitution §8 (not prescribed by the spec, same posture `STEP13_PlacementStacks_IO.md` took
  for `MarkerRule`'s 4 new fields).
- `src/params/MapRecipe_PARAMS.h` — add `int radialSymmetryRepeatCount = 3;` (flat sibling of the
  existing `globalSymmetryMask`, NOT nested in a new sub-struct — keeps every existing
  `recipe.globalSymmetryMask` call site in PROC/IO untouched, avoiding a large, risky rename
  across `Placement_Rules_PROC.cpp` and elsewhere). Change `globalSymmetryMask`'s default from
  `SymmetryAxis::None` to `SymmetryAxis::RotateHalfTurn` (ruling #4). Add `Params::
  SymmetryDetection symmetryDetection;` (giving it the aggregate home it's missing — retires
  `SymmetryTab_UI.h` SCOPE NOTE 2's "caller-owned" framing, see below). Add `Params::SymmetryBlend
  symmetryBlend;`.
- `src/params/MarkerRule_PARAMS.h`, `ScatterRule_PARAMS.h` (`PropRule`/`DecalRule`/`UnitRule`),
  `GeoLayer_PARAMS.h`, `Layer_PARAMS.h` — add `int radialSymmetryRepeatCount = 3;` as a flat
  sibling of each type's existing `symmetryMask` field (7 types total including `MapRecipe`).
- New: `src/io/MapExporter_Symmetry_IO.cpp` / `MapImporter_Symmetry_IO.cpp` — the new top-level
  `Symmetry` section (10 fields, see shape below).
- `src/io/MapExporter_Recipe_IO.cpp` — remove `generatorData["GlobalSymmetryMask"]` (relocation,
  not duplication — same discipline as every prior relocation ticket this session); add
  `document["Symmetry"] = BuildSymmetryJson(recipe);` at top level.
- `src/io/MapImporter_Recipe_IO.cpp` — remove the `GlobalSymmetryMask` read from wherever it
  currently sits (inside the `mapGeneratorData`-gated geometry reader).
- `src/io/MapExporter_MarkersStack_IO.cpp`/`PropsStack_IO.cpp`/`DecalsStack_IO.cpp`/
  `UnitsStack_IO.cpp`, and their `MapImporter_*` twins — add the `RadialSymmetryRepeatCount`
  read/write, sibling of the existing `SymmetryMask` key, on each rule type.
- `src/io/MapExporter_HeightmapStack_IO.cpp`/`MapImporter_HeightmapStack_IO.cpp` — add
  `RadialSymmetryRepeatCount` read/write on BOTH the `GeoLayer` and `Layer` blocks, sibling of
  their existing `SymmetryMask` key.
- `src/ui/SymmetryTab_UI.h`/`.cpp` — **no functional change**, but `SymmetryTab_UI.h`'s SCOPE NOTE
  2 ("`Params::SymmetryDetection` has no aggregate home yet... the caller owns the instance") is
  now false — `MapRecipe::symmetryDetection` exists. Update the comment; do NOT change
  `DrawSymmetryTab`'s signature or wire it to the new field — that's UI wiring, a separate,
  already-tracked follow-up (do not touch `SymmetryAxisOption::Radial`'s stale `QuarterTurns`
  stand-in mapping either — flagged for the UI Expert, explicitly not this ticket).
- `src/ui/SymmetryTab_UI_Test.cpp`, `src/proc/Placement_PROC_Test.cpp`,
  `src/pipeline/GenerationAssembler_TestScene_PIPELINE.h` — the three confirmed test fixes from
  ruling #4, plus the audit's findings.
- `src/proc/Placement_Symmetry_PROC_Test.cpp` — add ruling #3's dormancy test (asserts
  `BuildSymmetryOrbit` does not yet branch on `Radial`) — this file already exercises
  `BuildSymmetryOrbit` via `MakeSymmetricRecipe`/`PlacementStage`, the natural home for it.

## Layer & accuracy class
PARAMS (new bit, new struct, 7 new field additions) + IO/BRIDGE. Accuracy class: Exact.

## Backend policy
CPU only. No PROC change — `SymmetryAxis::Radial` and every new field in this ticket have zero
consumers (ruling #1/#3).

## ARCH rules invoked
- `SANMAP_FORMAT_SPEC.md` Correction 4, `ARCH.md` §13 — binding, as narrowed by "Ruled by this
  ticket" above.
- Constitution §8 — settings exist in PARAMS from the moment they're settable, no consumer
  required first (same precedent already established this session).
- ARCH §7.1 — `SymmetryDetection` getting an aggregate home is NOT a rival settings type; it's the
  same type gaining the one home it was always missing (its own header comment already frames
  `Params::MapRecipe::globalSymmetryMask` as "the ONE home of the mask; nothing here duplicates
  it" — this ticket doesn't touch that framing, `SymmetryDetection` is a separate concept).

## Solution — shape
```
Symmetry: {
    GlobalSymmetryMask         (int)    <- recipe.globalSymmetryMask        // relocated
    RadialSymmetryRepeatCount  (int)    <- recipe.radialSymmetryRepeatCount // NEW
    SnapImperfectSymmetry      (bool)   <- recipe.symmetryDetection.bSnapImperfectSymmetry
    SymmetryDetectionTolerance (float)  <- recipe.symmetryDetection.detectionTolerance
    SymSuperpositionBlend      (float)  <- recipe.symmetryBlend.superpositionBlend
    SymmetryBlurRadius         (float)  <- recipe.symmetryBlend.blurRadius
    CrossFadeWidth             (float)  <- recipe.symmetryBlend.crossFadeWidth
    CylinderZScale             (float)  <- recipe.symmetryBlend.cylinderZScale
    TorusMajorRadius           (float)  <- recipe.symmetryBlend.torusMajorRadius
    TorusMinorRadius           (float)  <- recipe.symmetryBlend.torusMinorRadius
}
```
Per-rule/per-layer: each `MarkersStack`/`PropsStack`/`DecalsStack`/`UnitsStack` entry, and each
`HeightmapStack` `GeoLayer`/`Layer` entry, gains `json["RadialSymmetryRepeatCount"] =
rule.radialSymmetryRepeatCount;` as a sibling of the already-live `SymmetryMask` key.

## Explicit out-of-scope
- **`Params::SymAlgorithm`** (ruling #1) — the enum type, its field, and its JSON key are all
  excluded. Defaulting it to `Superposition` is a requirement for whichever FUTURE ticket defines
  it, not this one.
- **The `AppendRadialTurns` PROC generator** — `SymmetryAxis::Radial` is a reserved bit with zero
  orbit-generation logic; building the actual N-way rotation math is separate, future PROC work.
- **The `symmetryOrbitMaximum` buffer widening** — stays dormant (ruling #3), not touched here.
- **UI wiring** — `DrawSymmetryTab` is not rewired to any new field; `SymmetryAxisOption::Radial`'s
  stale `QuarterTurns` stand-in is not fixed (flagged for the UI Expert separately).
- **Retroactive default-mask migration for old `.sanmap` files** — a document with no
  `Symmetry.GlobalSymmetryMask` key simply gets the new default (`RotateHalfTurn`) on import, same
  degrade-gracefully behavior every prior new-field addition in this project has had; no migration
  file needed (content-shape-only change, not a version bump).

## Acceptance test
A `Params::MapRecipe` with non-default `radialSymmetryRepeatCount`, `symmetryDetection`, and
`symmetryBlend` values, plus non-default `radialSymmetryRepeatCount` on at least one entry each of
`MarkerRule`/`PropRule`/`DecalRule`/`UnitRule`/`GeoLayer`/`Layer`, survives export→import exactly
through the new `Symmetry` section and the per-rule/per-layer sibling keys. Confirm (raw JSON text
check) `GlobalSymmetryMask` no longer appears under `document["mapGeneratorData"]` after this
ticket. Confirm the default-axis-change test fixes (ruling #4) all pass, including the full audit
of default-`MapRecipe` fixtures flowing into placement tests. Confirm the new
`BuildSymmetryOrbit`-does-not-branch-on-`Radial` test (ruling #3) passes. Full `SanGenV2` build
stays clean; every existing test continues to pass.
