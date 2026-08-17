# Work-Order SPEC-4 — `.sanmap` schema v3: dissolve `mapGeneratorData` into versioned,
# top-level, format-sibling sections (DOCS)

*Constitution §7. Executor: **SanGen ARCH Expert** (targets live under
`sangen_arch_pack/`, which only the ARCH Expert may write). Authored by the Format
Expert domain, evaluated by the ARCH Expert for conformance, and ratified by the human
across a full design conversation. Status: evidence complete and every open fork ruled
on; corrections NOT yet applied.*

## Title
Replace the `.sanmap` format's `mapGeneratorData` blob (~40 keys, ~60% duplicate of the
format's own fields) with independently versioned, top-level, SanGen-owned sections that
sit as siblings of the format's own fields — plus one new PARAMS-shape ruling (Slope) and
several confirmed field additions the old blob was silently missing.

## Root problem
`mapGeneratorData` was audited field-by-field against a real official map
(`Pandemonium Isthmus.sanmap`) and the live v2 codebase. Findings:
- **~60% pure duplication.** `Atmosphere`, `Stratums`, `MapSize`, `TerrainMaxHeight`, and
  the generator `Water` sub-block all mirror the format's own top-level fields
  byte-for-byte (verified line-for-line against the real file).
- **v1 and v2 write two *different, mutually incompatible* dialects under the same key.**
  v1's `Stratums` shape (`hardness`/`friction`/`cohesion`/…) and v2's current
  `MapExporter_Layers_IO.cpp::BuildStratumJson` shape (`SlopeGateEnabled`/
  `MinimumSlopeDegrees`/…) both target `mapGeneratorData.Stratums` and cannot both be
  read back correctly by the same importer.
- **No independent version gate.** `MapGeneratorDataVersion`/`mapGeneratorDataVersion`
  (`MapExporter_IO.h:75`) is written but never read — `MapImporter_Recipe_IO.cpp` has no
  version branch at all. Every old save silently degrades to defaults.
- **App/machine-local facts are stored as map content.** `GamedataPath`,
  `GlobalEnvironmentPath`, and the GPU/backend toggle group are local-machine facts baked
  into shared map data.
- **Baked instances shipped instead of rules.** `MarkersList` stores resolved marker
  positions, duplicating the format's own `markers` dict, while the actual generator
  RULES that produced them (count/density/slope gates/priority) were never serialized at
  all — the real, load-bearing gap.
- **A live regression risk**: `TerrainMinHeight`, `WorldUnitsPerCell`, and
  `SimulationGrouping` are round-tripped by v2 *today* and were absent from the first
  draft of this schema — confirmed by re-reading the actual exporter/importer code, not
  assumed.

## Target files
- `sangen_arch_pack/specs/SANMAP_FORMAT_SPEC.md` — the primary rewrite (this work-order's
  main deliverable).
- `sangen_arch_pack/specs/MASKING_SPEC.md` — **required**, not optional. §1.7 is a
  ratified, binding contract this schema partially amends (see Correction 5).
- `sangen_arch_pack/specs/LAYER_SYSTEM_SPEC.md` — companion note confirming
  `HeightmapStack`'s IO shape maps onto the *existing* `GeoLayer`/`Layer`/`LayerStack`
  model; forward-reference marking where a future recursive-`GeoLayer` design would need
  to extend it.
- `sangen_arch_pack/specs/PLACEMENT_SCATTER_SPEC.md` — `MarkerRule`/`PropRule`/
  `DecalRule` gain a `name` field and a Group container; `MarkerRule` gains new
  per-layer density fields (Correction 7).
- `sangen_arch_pack/specs/PARAMS_PIPELINE_SPEC.md` — already stale (still written against
  `core/Parameters.h`, the v1 god object); refresh in the same pass since this work-order
  touches most of what it documents.
- `sangen_arch_pack/specs/DISPATCH_INTERFACE_SPEC.md` — N/A for this work-order: Correction
  9 removes Performance Settings from `.sanmap` entirely rather than adding dispatch-hint
  law. No amendment needed here after that ruling (see Correction 9).
- `ARCH.md` — the JSON top-level casing convention (Correction 0) as formal naming law.
- *(Outside `sangen_arch_pack/`, for the coder tier once ratified — NOT this work-order):*
  `src/io/*`, `src/params/*`.

## Layer & accuracy class
PARAMS (schema/field ownership) + IO/BRIDGE (serialization). Accuracy class **Exact** —
every promoted field must survive an export→import cycle bit-identical.

## Backend policy
N/A — no PROC calculation; IO loads/saves only (ARCH §3.1/§3.3, IO/BRIDGE scope).

## ARCH rules invoked
- ARCH §3.3/§5 (IO/BRIDGE: loads/saves only, never simulates)
- Constitution §2 (naming law) — Correction 0
- Constitution §6 (validate every external file; a version mismatch must not silently
  degrade) — Correction 1
- Constitution §8 (total tweakability) — Corrections 2, 7
- ARCH §7.1 ("no rival settings type" per concept) — the exact clause Correction 5 rules
  on

---

## Correction 0 — Naming convention (ratify as law)

The format already has a live, useful split: **camelCase top-level = game-native field;
PascalCase top-level = SanGen-owned section.** Ratify this explicitly in `ARCH.md`'s
naming law. Consequence: every new top-level key below is **single-token PascalCase, no
spaces** (`GeneralMapSettings`, not `"General Map Settings"`). Fields merged into an
*existing* format-native collection (e.g. `armies`) stay lowerCamelCase to match their
siblings — e.g. `armyColor`, not `"Army Color"`.

## Correction 1 — New version field

Add top-level **`SanGenVersion`** (int), replacing `MapGeneratorDataVersion`. Bump to
`3` (this is a breaking dialect change). Unlike today, **the importer must actually gate
on it** — an old/absent version must produce a loud, logged fallback, never a silent
default. This is the literal constant at `MapExporter_IO.h:75` to repoint, and the gap
named in `IO_PARITY_REPORT.md` Step 5.

## Correction 2 — `GeneralMapSettings`

`Seed`, `ScaleFeaturesToMapSize`, `GlobalGravity`, `TerrainMinHeight`,
`WorldUnitsPerCell`.
- `TerrainMinHeight`/`WorldUnitsPerCell` are live fields v2 already round-trips today
  (`MapExporter_Recipe_IO.cpp:38,41`) and were missing from the first draft — confirmed
  addition, not new design.
- `GlobalGravity` has **no durable PARAMS field today** (it's tab-local UI state,
  `HeightmapTab_UI.h:70`, explicitly marked unserialized). This is a genuine new-field
  addition for the coder work-order, not a relocation.

## Correction 3 — `HeightmapStack`

Replaces `GeoLayers`. Structural model **confirmed against the ratified
`LAYER_SYSTEM_SPEC.md`, not redesigned here**: a flat, ordered `LayerStack` of `GeoLayer`
(composition bands), each a flat, ordered stack of `Layer` (Material or Simulation type).
**Neither level nests or groups** — the spec explicitly considered and rejected a
grouping tier in favor of a flat "sim group" tag. `SimulationGrouping`
(`Params::LayerStack::simulationGrouping`) nests inside this key instead of floating as a
stray top-level sibling, which is where v2 currently writes it (`MapExporter_Recipe_IO.cpp:43`).

**Named gap, explicitly deferred:** the real map's per-layer `MinHeight`/`MaxHeight`/
`MinSlope`/`MaxSlope` height-and-slope gates (confirmed present in the live file at the
`GeoLayers.Layers[]` level) have **no equivalent field on v2's current `Layer` PARAMS at
all** — silently dropped in the v1→v2 port, not merely carried over. Since the internal
layer redesign is out of scope for this work-order, carry v2's current `Layer` field set
through unchanged and log this as a named gap for that future conversation.

**Symmetry override — new field, in scope now (see Correction 4):** `GeoLayer`/`Layer`
each gain a local `bSymmetryUseGlobal` + `symmetryMask` override, matching the pattern
already live on every placement rule type.

## Correction 4 — `Symmetry`

Global: `SymAlgorithm`, `SymSuperpositionBlend`, `SymmetryBlurRadius`, `CrossFadeWidth`,
`CylinderZScale`, `TorusMajorRadius`, `TorusMinorRadius`, `SnapImperfectSymmetry`,
`SymmetryDetectionTolerance`, `GlobalSymmetryMask`.

Per-rule/per-layer local override (`bSymmetryUseGlobal` + `symmetryMask`) is **confirmed
already live and tested** on `MarkerRule`/`PropRule`/`DecalRule`
(`PlacementRules_PARAMS_Test.cpp:16`) — pure relocation for those three. **New** for
`HeightmapStack`'s `GeoLayer`/`Layer` (Correction 3).

**Ruled, not deferred:** heightmap symmetry is wanted immediately, in full — both the
basic axis mechanism (`Params::SymmetryAxis` mirror/rotate) **and** the exotic
`SymAlgorithm` group (Fold/Blur/CrossFade/Superposition/Cylinder3D/Torus3D). No v2 code
implements a heightfield-symmetry PROC stage today (`IO_PARITY_REPORT.md` Decision #6).
**This work-order reserves and round-trips the settings and adds the new override field
to `GeoLayer`/`Layer` only.** Designing and building the actual heightfield-symmetry PROC
stage is real, near-term, out-of-scope work for a separate generator-expert/ARCH
work-order — flagged here so it is not lost, not resolved here.

## Correction 5 — `SlopeDefaults` (the load-bearing PARAMS-shape ruling)

**Ruling: keep per-stratum slope gates as the ground truth. Add a global-default layer on
top; do not replace per-stratum with a flat global.**

`MASKING_SPEC.md` §1.7 is a ratified, binding contract: mask settings
(`bSlopeGateEnabled`, `minimumSlopeDegrees`, `maximumSlopeDegrees`,
`slopeFeatherDegreesLow/High`, `bUseSmoothstep`, `bInvertSlopeGate`,
`slopeGateStrength`) are members of `Params::Stratum`, evaluated once per stratum by
design — that mechanism is the entire reason different strata can occupy different slope
bands (grass on shallow ground, rock on cliffs). A flat global window is not a
simplification of this, it deletes the feature: every stratum's gate would open and close
together.

**Resolution — mirrors the Symmetry pattern already proven above:** a new top-level
**`SlopeDefaults`** section (the seven fields above, as shared defaults) plus a new
`bSlopeUseGlobal` flag on `Params::Stratum` (default `true`). New strata inherit shared
defaults for free; a stratum that needs its own window flips one bool. The Mask stage's
existing config-flattening step resolves default-vs-override before running its
unchanged per-stratum kernel — no PROC change required, only a config source change.

**Requires a `MASKING_SPEC.md` §1.7 amendment** stating the default/override split
explicitly — this work-order names the amendment; the ARCH Expert authors it.

Do not confuse this section with the real map's `SlopeSettingsParams`, an unrelated
single-field physics-parity toggle (`bUseEngineParityMath`) — that field has no home
decided yet and is not part of this correction.

## Correction 6 — `Flow` and `Accumulation` (two sections, both reserved)

**Ruled: `Flow` and `Accumulation` are two separate, global, top-level sections — both
computed after heightmap layer calculation finishes, neither per-layer, neither part of
Erosion.**
- **`Flow`** — a literal flow-velocity map, produced by simulating rain/water movement
  over time given map variables.
- **`Accumulation`** — consumes `Flow`'s output to simulate where material naturally
  piles, fills crevices, spills over, and re-flows, without runaway mound-building.

**Confirmed NOT the same as Erosion's own settings.** `ErosionLayerSettings`
(`src/proc/Erosion_Settings_PROC.h`) is real, current, and already per-layer (inside
`HeightmapStack`'s Simulation-layer entries per `LAYER_SYSTEM_SPEC.md`) — it already
owns `slopeAdherence`, `bAccurateSimultaneousAccumulation`, `spilloverThreshold`, and the
rain-noise droplet-spawning fields. None of that moves; it stays exactly where it is.

**Confirmed NOT the same as v2's current `FlowAccumulation` stage**, either.
`FlowAccumulationConstants` (`src/proc/FlowAccumulation_Kernel_PROC.h`) is a real, single,
current stage (`cellWeight`, `flowNoiseImpact`, `depressionFillEpsilon`,
`flowMagnitudeScale`, iteration counts, `bFillDepressions`, `bNormalizeAccumulation`) —
drainage/routing for pathing, not the two-simulation velocity→accumulation model
described above, and its field names don't match v1's `FlowSettingsParams`
(`Precipitation`, `FlowVolumeMultiplier`, `StochasticVariance`) at all.

**This work-order reserves the two top-level keys (`Flow`, `Accumulation`) with field
lists marked TBD.** `FlowMapColor` (confirmed live, a preview tint) lands in `Flow`.
Designing the actual two-simulation model — and reviewing whether the existing sim math
(erosion included) is even currently correct, which the human has separately flagged as
suspect — is real PROC work for a future generator-expert/ARCH work-order, explicitly
**not** this one.

## Correction 7 — `MarkersStack`, `PropsStack`, `DecalsStack`, `UnitsStack`

Each a Group→Layer(rule) hierarchy, shape **pending** the deferred shared Group/Layer/
LayerType design (see Out-of-scope). For this work-order: each Stack's layers are, for
now, a flat array of the existing rule type, wrapped under the new top-level key:
- `MarkersStack` → `Params::MarkerRule` (`MarkerRule_PARAMS.h`) — confirmed field-complete
  for count/density/slope/height gates/priority/focus-gradient/symmetry.
- `PropsStack` → `Params::PropRule` (`ScatterRule_PARAMS.h`) — confirmed field-complete.
- `DecalsStack` → `Params::DecalRule` (`ScatterRule_PARAMS.h`) — confirmed field-complete.
- **`UnitsStack` (new, was a real gap)** → `Params::UnitRule` — a fourth, fully-wired v2
  rule type, already exported today (`MapExporter_Rules_IO.cpp:91-105,117,122`), absent
  from the first draft of this schema. Omitting it would regress what v2 ships *today*.

**Confirmed cardinality change, new fields required:** `HydroMultiplier`,
`ReclaimDensity`, `MexDensity`, `SpawnPointCount` move from global scalars (v1) to
per-layer fields on `Params::MarkerRule` — a genuine addition to that type, not a
relocation, needed for the coder work-order.

**`GlobalMarkerSettings`** sub-key inside `MarkersStack`: `GlobalIconAlloy`,
`GlobalIconPlasma`, `GlobalIconSpawn`, `MarkerColorAlloy/Plasma/Spawn`,
`MarkerScaleAlloy/Plasma/Spawn`. **Ruled: `Plasma` = Energy, a real planned resource
type, not the v1 invention flagged in `IO_PARITY_REPORT.md` Decision #5** — keep all
three Plasma-named fields.

## Correction 8 — `DetailNormal`

`DetailNormalMapSize` only. The future layered-heightmap-delta system (a stack of
heightmaps producing a delta normal map) is explicitly deferred — this correction
reserves the key and the one live field.

## Correction 9 — Remove app/machine-local settings from `.sanmap` entirely

- `GamedataPath`, `GlobalEnvironmentPath` — confirmed present (empty) in the real map,
  confirmed zero v2 code references. Move to a new, separate, global app-settings
  location (a user-chosen "SanGen folder"), which also holds the shared icon/thumbnail
  cache — one copy, not duplicated per map. **Not designed in this work-order.**
- **`PerformanceSettings`** (`UseGPUFlowMap`, `UseGPUMarkers`, `UseGPUTerrain`,
  `WYSIWYGBaking`, `GPUPreviewIterations`, `FastPreviewMode`) — **ruled OUT of `.sanmap`
  entirely, and explicitly no per-map override.** These describe the generating
  machine's hardware/backend, not the map; they belong solely in the global app-settings
  location above, read once at startup to seed `Sys::DispatchPolicy`. This supersedes the
  ARCH Expert's earlier "persist as a non-authoritative hint" recommendation — the
  simpler final ruling is no per-map storage of any kind, so no `DISPATCH_INTERFACE_SPEC.md`
  amendment is needed for this work-order.

## Correction 10 — `MarkersList` deleted

Baked marker positions duplicating the format's own `markers` dict. Deleted; superseded
entirely by `MarkersStack`'s rules (Correction 7), consistent with what v2 already does
today (rules, never baked instances).

## Correction 11 — Merges into existing format-native collections

- `armies[key]` gains **`armyColor`** (lowerCamelCase, not `"Army Color"` — Correction 0)
  and `alias`.
- `markers[key]` gains `alias`.
- The old global `Aliases` block is deleted once both land.

## Verified deletions (pure duplicates — delete outright)
`mapGeneratorData.Atmosphere`, `.Stratums` (both the v1 and the current-v2 dialects
squatting on that key), `.MapSize`, the global `.TerrainMaxHeight`, the entire generator
`.Water` sub-block.

---

## Solution + performance estimate
Documentation/schema edit; **no runtime performance impact.** Downstream implementation
(a follow-on coder work-order, not this one) touches `src/io/*` serialization and adds a
handful of new PARAMS fields (`bSlopeUseGlobal` + `SlopeGateDefaults`, `GeoLayer`/`Layer`
symmetry override, `MarkerRule` per-layer density fields, `Params::UnitRule`
Stack-wrapping, `armyColor`/`alias`) — all plain-data additions, negligible cost.

**Per-domain IO code organization** (one `MapExporter_<Domain>_IO.cpp`/
`MapImporter_<Domain>_IO.cpp` pair per top-level section, rather than the current
few-files-cover-many-domains shape) is an agreed convention for the coder work-order —
implementation detail, not a schema decision, noted here so it isn't lost.

## Lossy alternative
None applicable — every deletion above is a confirmed exact duplicate; every addition is
either a confirmed live field v2 already round-trips or an explicitly-ruled new concept.
Partial application would leave the importer straddling two incompatible dialects, which
is the exact defect this work-order exists to remove.

## Acceptance test
Extend `MapExporter_IO_Test`/`FilesTab_Roundtrip_UI_Test` (existing project convention)
with a field-count round-trip assertion: populate every field named in this work-order on
a `Params::MapRecipe`, export, re-import, assert byte-identical survival — explicitly
including `UnitsStack`, `GeneralMapSettings.TerrainMinHeight`/`WorldUnitsPerCell`,
`HeightmapStack.SimulationGrouping`, and the `GeoLayer`/`Layer` symmetry override, so this
work-order's own gap-fills don't silently regress again. `SanGenVersion` must be asserted
to actually gate import — a document with an old/absent version must produce a loud
failure or fallback, never a silent default.

## Out-of-scope (explicit)
- The internal `HeightmapStack` layer redesign (recursive `GeoLayer` grouping) — future
  conversation; `GeoLayer` currently cannot contain another `GeoLayer`.
- The shared Group/Layer/LayerType hierarchy for `MarkersStack`/`PropsStack`/
  `DecalsStack`/`UnitsStack` — future conversation. **Flagged finding:** `HeightmapStack`'s
  structure (flat, physics-ordered, exactly two layer types) is likely NOT the same DATA
  shape as the other four Stacks' (organizational, arbitrary grouping) — the future
  design conversation may need to converge on shared *editing conventions* rather than one
  shared *data model*.
- `DetailNormal`'s future layered-delta-heightmap system — key reserved only.
- Building the heightfield-symmetry PROC stage (both the axis mechanism and the full
  `SymAlgorithm` group) — real, ruled as wanted immediately, but PROC/pipeline design
  work for a separate generator-expert/ARCH work-order. This work-order only reserves and
  round-trips the settings plus the new `GeoLayer`/`Layer` override field.
- Designing and building the actual `Flow`/`Accumulation` two-simulation model, and the
  broader review of whether existing sim math (erosion included) is currently correct —
  explicitly flagged by the human as suspect and deferred to later, separate work.
- The global app-settings file/folder itself (`GamedataPath`/`GlobalEnvironmentPath`/
  `PerformanceSettings`/shared icon cache) — confirmed removed from `.sanmap`; its own
  design is a separate, smaller work-order.
- Per-domain IO source-file split — real, agreed convention, but implementation detail
  for the coder work-order once this schema is ratified.
- Any code change to `src/io/*` or `src/params/*` — this work-order updates the spec
  pack only.
