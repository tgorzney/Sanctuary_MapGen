# STEP68 — `symmetryGroupIdentifier` field + a universal PIPELINE passthrough to the mirror-math

**Layer:** PARAMS, IO, PIPELINE. **Domain:** `MarkerTransform` (the field, marker-specific) + a new
thin `PIPELINE` wrapper around `Proc::BuildSymmetryOrbit`/`ResolveSymmetryMask`/
`ResolveRadialSymmetryRepeatCount` (the wrapper, **domain-agnostic by design** — see naming note
below; it takes only geometry/mask/position, nothing marker-specific, so Props/Decals/Units tabs
can call the same function later instead of each building their own copy).
**Sequence:** data plumbing only — this ticket does not make dragging work. It gives the future
drag-and-follow UI ticket (not yet drafted; needs `STEP49`, `STEP47`, `STEP48` landed first) a
field to write into and a legal call path to the mirror math. Independent of `STEP66`/`STEP67`
(manual markers vs. procedural rules are separate domains) and of `STEP60` (no dependency either
direction).

## Root problem
`ARCH_16_MarkerLayerSymmetry.md` §16 ratified the drag-and-follow design (`DESIGN_MarkerLayerSymmetry_R1.md`/`_R2.md`):
any member of a manually-placed symmetry group can be dragged, recomputing the others. That design
needs two things that don't exist yet:
1. A way to mark which `MarkerTransform` instances belong to the same mirrored group
   (`symmetryGroupIdentifier`, already ratified as a wire field, `SANMAP_FORMAT_SPEC.md`
   Correction 16 — not yet a real PARAMS field or IO round-trip).
2. A legal call path from UI code to `Proc::BuildSymmetryOrbit`. `ARCH_03_ModuleBoundaries.md` §3.1's dependency table
   gives `UI` no direct `PROC` access — only `UI → PIPELINE → PROC`. ARCH_16_03_ModuleBoundaryChain.md §16.3 ruled this should
   be solved with a new, narrowly-scoped **stateless query passthrough** in `PIPELINE` (not a
   relocation of the math itself, not a new dependency-table exception).

## Target files
- `src/params/MarkerInstance_PARAMS.h` — `symmetryGroupIdentifier` field on `MarkerTransform`.
- `src/io/MapExporter_Markers_IO.cpp` / `MapImporter_Markers_IO.cpp` — wire read/write.
- `src/pipeline/SymmetryOrbitQuery_PIPELINE.h`/`.cpp` (new, name proposed — see naming note below)
  — the thin forwarding wrapper. **Deliberately not marker-named** — see naming note.

## Layer & accuracy class
PARAMS + IO/BRIDGE + PIPELINE. Accuracy class: Exact (the wrapper calls the same exact,
deterministic function procedural placement already relies on).

## Backend policy
CPU only — synchronous, no `Dispatch_SYS` involvement. This is not a PROC stage and carries no GPU
twin obligation (it authors no new stage/kernel, only calls one that already exists and is already
CPU-only by design).

## ARCH rules invoked
- `ARCH_16_03_ModuleBoundaryChain.md` §16.3 — the passthrough ruling, binding: `PIPELINE` may host a stateless query
  function with no DAG participation (no dirty-hash, no `DispatchPolicy`, nothing PIPELINE "owns"
  across frames) alongside its stage-conductor responsibilities.
- `ARCH_16_05_MarkerTransformFields.md` §16.5 — no abbreviations; `symmetryGroupIdentifier`, not `symmetryGroupId`.
- `SANMAP_FORMAT_SPEC.md` Correction 16 — the ratified wire field, implemented verbatim here.
- §3.1 dependency table — `UI → PIPELINE → PROC` is legal, `UI → PROC` directly is not; this
  ticket's wrapper is what makes the future drag UI's call legal without a table exception.

## Solution — shape

### 1. New field

**⚠️ Correction 2026-08-22:** the snippet below originally showed `layerIndex` as if already
present, annotated "from STEP60." **STEP60 has not landed as of this ticket's dispatch** — the
current shipped `MarkerTransform` (`src/params/MarkerInstance_PARAMS.h`) has only 3 fields
(`name`, `transform`, `alias`). This ticket adds exactly ONE new field, as the 4th member, after
`alias`. Do not add `layerIndex` here — that is STEP60's field, on its own independent schedule;
if STEP60 has landed by the time this ticket is implemented, add `symmetryGroupIdentifier` after
whatever fields already exist, in whatever order results — the field's own meaning doesn't depend
on its position.

```cpp
// MarkerInstance_PARAMS.h — current shipped shape, before this ticket:
// struct MarkerTransform { std::string name; InstancedTransform transform; std::string alias; };
struct MarkerTransform {
    std::string name;
    InstancedTransform transform;
    std::string alias;
    int symmetryGroupIdentifier = 0;     // NEW — 0 = ungrouped, per Correction 16
};
```
Wire: direct field injection into `markers[type].transforms[name]`, lowerCamelCase, same merge
rule `alias` already uses (Correction 16, already ratified — no new format ruling needed here).
No range to validate on import — `0` is always legal, any positive value accepted as-is.

### 2. The PIPELINE wrapper
Wraps the existing pure functions unchanged — `Placement_Symmetry_PROC.h`,
`Placement_SymmetryOrbit_PROC.h`, `Placement_RuleBuild_PROC.h`'s `ResolveSymmetryMask`/
`ResolveRadialSymmetryRepeatCount` are **not modified** by this ticket. The wrapper handles two
things a UI caller can't do itself:
- **World ↔ cell coordinate conversion.** `BuildSymmetryOrbit` operates in heightfield cell space
  (`extent = vertexSize - 1`); `MarkerTransform.transform` stores absolute world units
  (`InstancedTransform_PARAMS.h`). Convert world → cell before calling (divide by
  `geometry.worldUnitsPerCell`), cell → world on the way back (multiply by the same), matching
  exactly how `Placement_Emit_PROC.cpp` already does this conversion for procedural placement —
  not a second, independently-derived scale.
- **`extent` without a baked heightfield.** Manual markers are edited before any generation may
  have run. Use `geometry.VertexSize() - 1` (PARAMS-only, no baked `Data::MapFields` needed) as
  `extent` — the identical value the baked-field path derives, just sourced from PARAMS since no
  bake is guaranteed to exist yet.

```cpp
// working name — see naming note below. Note: NOTHING in this signature is marker-specific —
// geometry, a mask, a world position, output points. Any future caller (Props/Decals/Units tabs
// wanting the same "mirror this position" query) uses this exact function, not a copy.
namespace SanmapGen { namespace Pipeline {

struct WorldSymmetryOrbitPoint { float worldPositionX = 0.0f; float worldPositionZ = 0.0f; };

// Synchronous, on-demand — explicitly NOT a Generation_PIPELINE/PreviewDriver_PIPELINE DAG stage.
int BuildWorldSymmetryOrbit(const Params::Geometry& geometry, int symmetryMask,
                            int radialSymmetryRepeatCount, float worldPositionX,
                            float worldPositionZ, WorldSymmetryOrbitPoint* outPoints,
                            int maximumPoints);

} }
```
Definition: convert `(worldPositionX, worldPositionZ)` to cell space; call
`Proc::BuildSymmetryOrbit` with `extent = geometry.VertexSize() - 1`, a default-constructed
`Proc::PlacementConstants{}.symmetryDuplicateEpsilon`, the caller's mask/position; convert every
returned point back to world units; return the same count. `Params::symmetryOrbitMaximum` is the
caller-supplied `maximumPoints` ceiling — same buffer the PROC caller already uses.

**Naming, corrected from an earlier draft**: this ticket originally proposed
`MarkerSymmetryQuery_PIPELINE.h`/`BuildMarkerSymmetryOrbit` — wrong, flagged by the human. The
wrapper's own parameters carry no marker concept at all (only `MarkerTransform`'s world→cell/
cell→world conversion needs generic `Params::Geometry`, which every domain already has). Renamed
to the domain-agnostic `SymmetryOrbitQuery_PIPELINE.h`/`BuildWorldSymmetryOrbit`/
`WorldSymmetryOrbitPoint` so a future Props/Decals/Units "place with mirroring" feature calls this
same function instead of an independent copy — the exact kind of duplicate-wrapper waste this
session already found once (two sessions had independently sketched near-identical PIPELINE
wrappers for the same math before this ticket existed).

**❓ Naming not fully finalized — flag for one quick ARCH confirmation before dispatch, not before
drafting.** The domain-agnostic direction above is settled; only the exact file name
(`SymmetryOrbitQuery_PIPELINE.h` vs. an alternative) is still a proposal. Why a new file rather
than an existing one: `Generation_PIPELINE.h` is the dirty-hash stage conductor,
`PreviewDriver_PIPELINE.h` is the two-tier dirty model, `GenerationAssembler_PIPELINE.h` assembles
the ordered PROC stage sequence — none of the three is "a place for a synchronous on-demand math
utility with no DAG participation"; bolting it onto any of them would misdescribe what that file
owns.

## Explicit out-of-scope
- **The actual drag-and-follow UI** — reading `symmetryGroupIdentifier`, matching group members at
  gesture-start, live-recomputing on drag, cardinality-change handling, cascade-delete/orphan
  rulings (`DESIGN_MarkerLayerSymmetry_R2.md` §1/§2). This ticket gives that future work a field
  and a call path; it implements none of the interaction itself.
- **Any canvas/picking change** — depends on `STEP47`/`STEP48` landing, untouched here.
- **`STEP49`'s manual-marker editor** — untouched; this ticket doesn't require it to exist yet
  (the field/wrapper are usable independent of any UI).
- **`Placement_Symmetry_PROC.h`/`Placement_SymmetryOrbit_PROC.h`/`Placement_Accept_PROC.cpp`** —
  unmodified; the wrapper calls them, it does not change them.
- **A "Place Symmetric" one-time-creation authoring tool** — a smaller, separate UX question,
  not scoped here.

## Solution — estimate
Field addition + IO wiring: mechanical, same shape as `layerIndex` (`STEP60`). Wrapper: pure
O(orbit size) work, bounded by `Params::symmetryOrbitMaximum`, sub-microsecond per call — no
benchmark needed per Constitution §7, same posture already used for comparable zero-benchmark
PARAMS+IO+thin-wrapper additions this session.

## Acceptance test
1. Wire round-trip: a `MarkerTransform` with non-zero `symmetryGroupIdentifier` survives export→
   import exactly, at the correct wire location (`markers[type].transforms[name]`, sibling of
   `alias` — and of `layerIndex` too, if STEP60 has landed by the time this is implemented;
   its presence or absence doesn't change this ticket's own field or its test).
2. Wrapper unit test (pure, no imgui/GL, no marker types involved — proving the function is
   genuinely domain-agnostic): `Params::Geometry` with `mapSize = 256`, `worldUnitsPerCell = 2.0f`;
   `BuildWorldSymmetryOrbit(geometry, MirrorAcrossX, 0, 40.0f, 100.0f, points,
   symmetryOrbitMaximum)` returns count 2, `points[0] == (40, 100)`, `points[1] == (472, 100)`
   (cell space: `40/2=20`, `extent=256`, mirrored `=236`, world `=236*2=472`), within float
   epsilon. Repeat with a combined mask for a 4-point orbit, and a position on the mirror line for
   the collapse-to-1 case.
3. Confirm `Placement_Symmetry_PROC.h`/`Placement_SymmetryOrbit_PROC.h`/`Placement_Accept_PROC.cpp`
   are byte-identical before/after (grep/diff, not just "no test failures").
4. Full `SanGenV2` build stays clean; every existing test continues to pass.
