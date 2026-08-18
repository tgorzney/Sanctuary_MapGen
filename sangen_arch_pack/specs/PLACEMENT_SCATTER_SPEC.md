# PLACEMENT_SCATTER_SPEC — markers, props & unit scatter

Source: `core/params/Params_Geometry.h` (live `MarkerRule`/`PropRule`/`DecalRule`),
`core/Parameters.h` (`PropInstance`, `MarkerSpatialGrid`, armies/props),
`core/math/Sanmath_Spatial.h` (clearance scoring, JFA), `PreviewRenderer.cpp` (GPU
gate), `Widget_MapCanvas.cpp` (unit spawn). The `core/data/*` copies
(`MarkerType_*.h`, `TerrainType_Prop.h`) are a **dead parallel island** — not
compiled. Placement reads the mask/field outputs of `MASKING_SPEC` and feeds the
entities in `UNIT_PROP_MARKER_DATA_SPEC` + `PREVIEW_COMPOSITING_SPEC`.

**Staleness note (later addition, see "Current implementation status" at the end
of this file):** everything above and below this note describes the **dead `core/`
tree**, read at the spec's original authoring time. Much of the "Known issues to
fix in v2" list is **already resolved** in the current `src/proc/Placement_*_PROC`
family (`Placement_Accept_PROC.cpp`, `Placement_Symmetry_PROC.h`,
`Placement_Rules_PROC.cpp`, `Placement_Hash_PROC.cpp`) — a full pass reconciling
this spec's body against that current code is real work, out of scope for the
session that added this note. Treat the body below as **historical v1/dead-code
context**, and the final section as the current, authoritative addendum for
anything symmetry-related.

## Three scatter mechanisms today (to unify in v2)
1. **Procedural markers/resources (CPU, mostly declared-not-implemented).**
   `PlacementRules::{DetectSpawns,PlaceResources,PlaceProps}` score candidate spots
   by **radial clearance** — `Sanmath_Spatial.h`: `ScoreRadialClearance`
   (deterministic Bresenham perimeter) and `ScoreRadialClearance_Stochastic` (8
   random-angle samples); spacing/exclusion via `ComputeJFADistanceField`
   (Jump-Flood). Density here is a candidate count/threshold, not a spacing model.
2. **GPU preview scatter (live).** Per-rule `{MinSlope,MaxSlope,MinHeight,MaxHeight}`
   + `{Density,isMarker,isProp,ruleIndex}` flattened to SSBO 6 and evaluated as a
   **per-pixel density threshold gate** in the preview compute shader — no spacing,
   no count.
3. **Unit spawning (live, in the GUI widget).** `Widget_MapCanvas.cpp:250-291` — a
   **jittered/regular grid** (`rows×cols`, cell centers), bilinear height sample,
   writes `UnitTransform` straight into `params.Armies[...].Groups["INITIAL"]`.

These three disagree; v2 replaces them with **one scatter module** (a proper
density/spacing sampler — Poisson-disk/blue-noise, since none exists today) driving
markers, props, and units alike.

## Rules — `MarkerRule` (live: `Params_Geometry.h:172`)
Gates: `MinSlope/MaxSlope/MinHeight/MaxHeight`. Spacing/area: `ClearanceSpacing`
(min distance to other markers), `MapEdgePadding`, `AreaRadiusMin/AreaRadiusMax/
CheckMaxRadius`, `AreaHeightRange` (variance tolerance). Quantity/selection:
`UseDensity/Density/Count/UseAllPositions`, `RandomSelection`, `Priority`
(`MarkerPriority: Priority_LargestArea/SmallestArea/LeastVariance`). Spatial
weighting: `FocusGradient` (`Gradient_None/CenterFocus/EdgeFocus/Torus`) +
`FocusGradientRadius/Strength/Contrast`. Symmetry: `SymmetryUseGlobal/SymmetryMask`.
**No biome gate and no mask-texture gate** exist — filtering is height+slope+radial
clearance only. `PropRule`: `Density, MinSlope/MaxSlope/MinHeight/MaxHeight,
AvoidWater, NearCliffs`. `DecalRule`: same shape (but decals aren't previewed —
`PREVIEW_COMPOSITING_SPEC`).

## IO wrapping — `MarkersStack`/`PropsStack`/`DecalsStack`/`UnitsStack` (companion note, work-order SPEC-4)
`.sanmap` schema v3 (`SANMAP_FORMAT_SPEC` Correction 7) wraps each rule type above
under a new top-level section, for now as a **flat array** — the shared Group/Layer/
LayerType hierarchy for these four Stacks is deferred to a future conversation.
**Flagged finding, carried from `SPEC-4`:** `HeightmapStack`'s structure (flat,
physics-ordered, exactly two layer types, `LAYER_SYSTEM_SPEC`) is likely **not** the
same DATA shape as these four Stacks' (organizational, arbitrary grouping) — the
future design may need to converge on shared *editing conventions* rather than one
shared *data model*. Do not build the Group container against `HeightmapStack`'s
shape by assumption.
```
MarkersStack → Params::MarkerRule   (this page, "Rules — MarkerRule")
PropsStack   → Params::PropRule     (this page, "PropRule")
DecalsStack  → Params::DecalRule    (this page, "DecalRule")
UnitsStack   → Params::UnitRule     (NEW type — see below)
```
Each of `MarkerRule`/`PropRule`/`DecalRule` gains a **`name`** field (author-facing
identity, distinct from any format key) and, once the Group container design lands,
Group membership — reserved by this correction, not built.

**`Params::UnitRule` (new type)** — the fourth rule type, wrapped by `UnitsStack`.
Already exported today ahead of any schema (`MapExporter_Rules_IO.cpp:91-105,117,
122`); its absence from the first schema draft would have regressed what v2 ships
today. Shape follows the same rule-filter pattern as `MarkerRule`/`PropRule`/
`DecalRule` on this page (gates + spacing + selection + symmetry) — see the live
exporter for its current field set; this page does not re-derive it.

**New per-layer fields on `Params::MarkerRule`** (moved from v1 **global** scalars to
**per-layer** fields — a cardinality change, a genuine addition, not a relocation):
```
HydroMultiplier   ReclaimDensity   MexDensity   SpawnPointCount
```
v1 wrote all four as map-wide scalars and exposed **none** of them in any UI
(`IO_PARITY_REPORT.md` §5.B) — do not port the "unreachable global" shape forward;
they now live per-`MarkerRule`, alongside its other gates.

**`GlobalMarkerSettings`** — a sub-key inside `MarkersStack`, map-wide (not
per-rule): `GlobalIconAlloy`, `GlobalIconPlasma`, `GlobalIconSpawn`,
`MarkerColorAlloy`/`MarkerColorPlasma`/`MarkerColorSpawn`,
`MarkerScaleAlloy`/`MarkerScalePlasma`/`MarkerScaleSpawn`. `Plasma` = Energy, a real
planned resource type (ruled, `SANMAP_FORMAT_SPEC` Correction 7) — not the v1
invention flagged at `IO_PARITY_REPORT.md` Decision #5 (D-9); keep all three
Plasma-named fields.

**C++ shape (ARCH §11): `Params::GlobalMarkerSettings`**, `GlobalMarkerSettings_PARAMS.h`
— a new standalone file, sibling of `MarkerRule_PARAMS.h` (map-wide default, not a
member of `MarkerRule`, matching the global-vs-per-rule split above). Fields:
`iconNameAlloy`/`iconNamePlasma`/`iconNameSpawn` (atlas-manifest name keys,
`ASSET_LOADING_SPEC`), `colorAlloy`/`colorPlasma`/`colorSpawn` (`float[4]`),
`scaleAlloy`/`scalePlasma`/`scaleSpawn` — see `SANMAP_FORMAT_SPEC`'s
`GlobalMarkerSettings` paragraph and ARCH §11 for the full shape and naming
derivation. `MapRecipe_PARAMS.h` gains `GlobalMarkerSettings globalMarkerSettings;`
as a flat sibling of `markerRules` (shape only — wiring is a separate coder
work-order); the future `MarkersStack` Group/Layer wrapper may fold this inside it
later, not designed here.

## Transform & symmetry
`MarkerType_Transform.h` (orphan copy) shows the intended model: `Position[3]`,
`Rotation[4]` quaternion, `Scale[3]`, `Color[4]`, plus `SymmetryId`. `MarkerSymmetry`
is an axis **bit-flag** set: `Symmetry_None, Point, X, Z, XY, XZ, YZ, XYZ`. Clones
carry a shared `SymmetryId`; `IsHidden` markers still generate for clearance even
when their rule is off. **`Gen_Marker_Placement::CalculateMarkerSymmetryGroups` is
declared, undefined, and never called → symmetry alignment is non-functional
today.** v2 owns symmetry in the scatter/entity module, not the widget.

**Superseded by the current `src/params/Symmetry_PARAMS.h`** (see the addendum at
the end of this file): the live v2 axis set is `SymmetryAxis::{MirrorAcrossX,
MirrorAcrossZ, RotateHalfTurn, QuarterTurns}`, plus the newly-ratified `Radial`
(ARCH §13) — a different, smaller bit set than the `Symmetry_XY/XZ/YZ/XYZ`
combinations quoted above from the dead `MarkerType_Transform.h`, because v2's set
is OR-able (`XY` = `MirrorAcrossX | MirrorAcrossZ`, not its own bit) rather than
enumerating every combination as a separate value.

## Prop data model
Live SoA-intent struct `PropInstance` (`Parameters.h:105`): `X,Y,Z, TintColor
(packed), IconScale, LayerIndex, GroupIndex, TransformIndex`. It is documented as
SoA but implemented **AoS** (one interleaved vector), and stores **no `tpId`, no
rotation, no biome, no collision/reclaim flags** — it cannot round-trip full prop
state. `StaticPropsList` is deliberately kept out of the 32×32 `MarkerSpatialGrid`
(that grid is O(1) *hit-testing* for interactive markers, not scatter accel), so
100k props don't tank the UI. v2: real SoA (parallel arrays), add `tpId`/rotation/
biome/collision, since props with collision are gameplay-relevant (reclaim/pathing).

## Determinism (a hit-list item)
Weak and inconsistent: units and hydraulic use global non-seeded `rand()`;
`ScoreRadialClearance_Stochastic` is reproducible only via a **hardcoded default
seed 12345**, not plumbed from `GenerationParams`; no per-instance seed field
anywhere. For competitive shared-generation (`DETERMINISM_SPEC`) scatter must be a
pure function of `(seed, rule, position)` — a position-hashed RNG (the stochastic
scorer already shows the pattern: `seed ^ x*p1 ^ y*p2`), seed plumbed from params,
no `rand()`.

## CPU vs GPU
GPU (live): rule gates in the preview compute shader (inline source, not a file).
CPU (declared, mostly unimplemented): clearance scoring, JFA field, exclusion mask,
marker/prop placement (`#pragma omp parallel for`). Per Constitution §4 the
**authoritative placement is the CPU bake**; the GPU preview must sample the same
result, not re-filter (see the shadow-sim issue in `PREVIEW_COMPOSITING_SPEC`).

## Known issues to fix in v2
- **Core placement is unimplemented**: `Gen_Placement::Process` is called
  (`TerrainGenerator.cpp:70`) but has no body; `Gen_Marker_Procedural`,
  `Gen_Marker_Placement`, `PlacementRules` are included, never called, undefined →
  dead includes + unresolved-symbol risk.
- **God-widget / layer violation**: a GUI canvas (`Widget_MapCanvas.cpp`) performs
  gameplay spawning and writes armies directly. Placement is a PROC stage
  (Constitution §1); the widget only issues UI commands.
- **Non-deterministic `rand()`** in a widget and in generation — remove entirely.
- **Duplicate dead `core/data/` copies** (`MarkerRule` even differs: Density 0.5 vs
  0.02, Count 10 vs 4); empty `TerrainType_Prop.h` — delete (§2).
- **Mislabeled SoA** and **missing prop fields** (above).
- **Missing capability**: no Poisson/blue-noise, no scale-range/rotation-range/
  align-to-normal, no biome or mask-field-driven density — all needed for §8
  tweakability and natural scatter. Add as first-class tweakable fields.
- **Hardcoded constants**: stochastic seed 12345 + hash multipliers,
  `rand()%1000000` id space, `MaxHeight=128` default (the terrain's vertical extent in
  game units — should be **read from the map, not baked**; entity positions are
  absolute world/game units, and a Y above this floats above all terrain; see
  `SANMAP_FORMAT_SPEC` entity-position encoding), stratum loop bound 9.

**Two current-tree defects (ARCH §13, recorded not fixed — see the addendum at the
end of this file for full detail):**
- **Defect 1 — `DecalRule` has no `bSymmetryUseGlobal`/`symmetryMask` pair**
  (`src/params/ScatterRule_PARAMS.h`), and `AppendDecalRules`
  (`src/proc/Placement_Rules_PROC.cpp`) never calls `ResolveSymmetryMask` for
  decals — decals currently generate with **no symmetry at all**, not even the
  global default. A future coder work-order must add the missing PARAMS pair and
  wire it into `AppendDecalRules`, mirroring `AppendPropRules`.
- **Defect 2 — `Params::symmetryOrbitMaximum = 16`** backs a fixed-size stack array
  (`SymmetryOrbitPoint orbit[16]`, `src/proc/Placement_Accept_PROC.cpp:33`) that a
  designer-chosen `radialSymmetryRepeatCount` (ARCH §13) combined with mirrors can
  now exceed (e.g. 8-fold × MirrorX × MirrorZ → up to 32) — the buffer silently
  drops excess clones rather than erroring. A future Compute Optimization Expert or
  Generator Expert work-order must raise the cap and/or add a loud validated clamp
  on the designer-facing count (Constitution §6).

## v2 guidance
One scatter module: a seeded, position-hashed density/spacing sampler (Poisson-disk)
consuming height/slope/mask fields (`MASKING_SPEC`) with per-rule scale/rotation/
align ranges and biome+mask gates; symmetry owned here; real SoA prop/entity buffers
with full round-trip fields; CPU authoritative, GPU preview samples the same result
(`DISPATCH_INTERFACE_SPEC`); deterministic under the shared-gen mode
(`DETERMINISM_SPEC`).

---

## Current implementation status (later addition — symmetry, ARCH §13)

Everything above this line was written against the dead `core/` tree at the spec's
original authoring time (see the staleness note under the title). This addendum is
current against the live `src/` tree as of ARCH §13's ratification and takes
precedence over the body above wherever the two disagree on symmetry specifically.

**The scatter module described in "v2 guidance" above is substantially built.**
`src/proc/Placement_PROC.h` + `Placement_Rules_PROC.cpp` (rule → `ScatterRuleConfiguration`),
`Placement_Accept_PROC.cpp` (dart-throw acceptance: spacing + symmetry orbit + count
limit), `Placement_Symmetry_PROC.h` (`BuildSymmetryOrbit`, the pure position→orbit
function), and `Placement_Hash_PROC.cpp` (dirty-hash inputs) together implement CPU
placement for markers/props/units/decals with real symmetry, superseding this
spec's "Core placement is unimplemented" / "`CalculateMarkerSymmetryGroups`
... never called" findings above for the current tree specifically (those findings
remain historically accurate for the dead `core/` code they described).

`Params::SymmetryAxis` (`src/params/Symmetry_PARAMS.h`) is the live bit set:
`MirrorAcrossX (1<<0)`, `MirrorAcrossZ (1<<1)`, `RotateHalfTurn (1<<2)`,
`QuarterTurns (1<<3)`, and — ratified this session — `Radial (1<<4)`.
`BuildSymmetryOrbit` composes every set bit independently (confirmed by reading
`Placement_Symmetry_PROC.h`), so combinations including `Radial` with any mirror/
rotation bit already work structurally; `Radial`'s own N-way orbit generator (the
generalization of the existing `AppendQuarterTurns` helper from a hardcoded 3 turns
to a designer-chosen `radialSymmetryRepeatCount`) is new PROC work for a future
work-order, not built by this ratification. Full field/JSON-key detail:
`SANMAP_FORMAT_SPEC` Correction 4.

Defects 1 and 2 above are the two items this session records for follow-up; both
are real, current-tree, `src/` findings (not dead-code findings), confirmed by
direct code read (`ScatterRule_PARAMS.h`, `Placement_Rules_PROC.cpp`,
`Placement_Accept_PROC.cpp`).
