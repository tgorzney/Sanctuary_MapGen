# Work-Order — Step 57: manual props/decals PROC resolution + `manualLayerId` correlation column

*Constitution §7. Executor: SanGen Coder. Transcribes `ARCH_14_13_OpenItems.md` §14.13 item 3's "WORK-ORDER B"
verbatim into a real work-order — design is CLOSED by that item's three rulings; nothing here is a
new architectural decision. Phase 5.2 in `work_orders/SEQUENCE_PreviewOverlayLayering.md`.*

## Correction — `ruleIndex` sentinel, found by `STEP83_ReclaimFilterWiring_UI.md` §7 (this session)
`STEP83_ReclaimFilterWiring_UI.md` §7, drafted this session, found a real bug in this ticket's
original text: `MakeManualInstance` left `instance.ruleIndex` at `Data::PlacementInstance`'s struct
default (`0`), and `STEP50_ProceduralSubLayerCsrBucketIndex_UI.md`'s CSR bucket build treats `0` as
a live, in-range procedural rule index whenever `recipe.propRules`/`recipe.decalRules` is
non-empty — it has no way to tell "genuinely rule 0's instance" apart from "manual instance that
inherited the default." Left uncorrected, every manually-placed prop/decal silently lands in
procedural rule 0's overlay bucket once both tickets are implemented, and gets double-drawn/
mis-partitioned by any per-rule filter (including STEP83's own `bReclaimable` routing). Fixed below:
manual instances now explicitly set `ruleIndex = -1` (the same "not applicable" sentinel `armyIndex`
already uses), which STEP50's own `IsValidBucket` (`bucket >= 0 && bucket < bucketCount`) already
excludes with zero change required on STEP50's side. This ticket's earlier out-of-scope claim that
it was "unrelated" to STEP50 was **wrong** and is corrected in place below — the two tickets share
the `ruleIndex` column directly.

## Dependency — blocked until STEP56 lands
This ticket depends on **STEP56** (Phase 5.1 / `ARCH_14_13_OpenItems.md` §14.13 item 3 "WORK-ORDER A" — the
`layerId` stable-id field on `Params::PropInstanceLayer`/`DecalInstanceLayer`) landing first. Part
(b2) below reads `layerId`, which does not exist on `PropInstanceLayer`/`DecalInstanceLayer` until
STEP56 adds it (`ARCH_14_13_OpenItems.md` §14.13 item 3: "Add `int layerId = -1;` to `PropInstanceLayer`/
`DecalInstanceLayer`, assigned once at creation"). Confirmed today: `PropInstanceLayer`/
`DecalInstanceLayer` (`src/params/PropInstance_PARAMS.h:30-31`) carry only `name`/`color[4]`/
`iconScale` — no `layerId` field exists yet. Do not start this ticket before STEP56 is verified
landed.

## Root problem
Confirmed live by direct code read (this ticket, and independently by `ARCH_14_13_OpenItems.md` §14.13 item 3's
own correction note, which withdrew an earlier false claim that props/decals were not
live-wired into IO at all):
- `recipe.props`/`recipe.decals` (`Params::MapRecipe::props`/`::decals`,
  `src/params/MapRecipe_PARAMS.h:105-106`) round-trip correctly through import/export today —
  `MapExporter_DocumentAssembly_IO.cpp:63-64` (`document["decals"] = BuildDecalsJson(recipe);
  document["props"] = BuildPropsJson(recipe);`), `MapImporter_ParseDocument_IO.cpp:67,69`
  (`ReadPropGroupsJson(document, outRecipe); ... ReadDecalGroupsJson(document, outRecipe);`),
  tested by `MapExporter_IO_Test.cpp:83-88` and `MapImporter_IO_Test.cpp:978,1180-1183`
  (`RunRoundTripTests` calling `CheckPropsAndDecals`).
- But **zero PROC-side resolution step exists for manual authoring at all.** `src/proc/` has no
  reference to `PropInstanceGroup`/`DecalInstanceGroup`/`recipe.props`/`recipe.decals` (confirmed:
  grepping `src/proc` for `blueprintPath`, `recipe.props`, `recipe.decals` returns nothing).
  `Placement_PROC.cpp:61-66` (`PlacementStage::CollectionFor`) shows the four SoA collections
  (`results.markers`/`results.props`/`results.units`/`results.decals`) exist and are filled — but
  `results.props`/`results.decals` are filled **exclusively** by the procedural `ScatterRule` path
  (`Placement_Rules_PROC.cpp`'s `AppendPropRules`/`AppendDecalRules` -> `Placement_Accept_PROC.cpp`
  -> `Placement_Emit_PROC.cpp:74`'s `CollectionFor(configuration.collectionIndex).Append(instance)`).
  A designer's hand-placed prop/decal is saved and reloaded faithfully, but never actually appears
  anywhere Placement's output is consumed (preview, export-time instance data, the future overlay
  draw pass) — the round-trip is real, the resolution is not.
- `Data::PlacementInstances` (`src/data/PlacementInstances_DATA.h`) has no correlation column back
  to a manual layer at all — only `armyIndex` exists for units.

## Ruled by this ticket (transcribing `ARCH_14_13_OpenItems.md` §14.13 item 3 "WORK-ORDER B", verbatim scope)
One work-order, not split — part (b2) has nothing to populate without part (b1) existing first.

### (b1) — wire `recipe.props`/`recipe.decals` into a real PROC resolution step
Append into `results.props`/`results.decals` — the same `Data::PlacementInstances` SoA the
procedural scatter path already fills (`Data::PlacementResults`, `src/data/PlacementResults_DATA.h`).

**Ruling 3 — no symmetry participation; straight 1:1 copy-through. State this explicitly so the
coder does not second-guess it later:**
`PropTransform`/`DecalTransform` (`src/params/PropInstance_PARAMS.h:19-20`) carry only
`InstancedTransform transform; int layerIndex;` — confirmed by direct read, no
`bSymmetryUseGlobal`/`symmetryMask` fields, unlike every procedural rule type (`PropRule`/
`DecalRule`/`MarkerRule`/`ScatterRule`, which all carry that pair). This is not an oversight to
fix. It follows directly from the framing every hand-placed type in this codebase already shares
(`MapRecipe_PARAMS.h:96-98`'s own comment on `armies`/`areas`/`markers`/`chains`, extended to
props/decals by the ARCH_12_ManualPropDecalLayers.md §12 ruling): *"round-trip fidelity... their entire purpose; no PROC stage
computes or reinterprets them."* Direct precedent, confirmed by grep of `src/proc`: `recipe.armies`
and `recipe.markers` (the hand-placed `MarkerInstanceGroup` kind, distinct from `MarkerRule`) never
appear anywhere under `src/proc/` — searching `src/proc` for `armies`/`markers` turns up only
`results.markers`/local scatter-result variables named `markers`, never `recipe.armies` or
`recipe.markers`. The existing hand-placed-entity family already never runs through
`BuildSymmetryOrbit`/`ResolveSymmetryMask` (both only appear in `Placement_Symmetry_PROC.h`,
`Placement_SymmetryOrbit_PROC.h`, `Placement_Accept_PROC.cpp`, `Placement_Rules_PROC.cpp`,
`Placement_RuleBuild_PROC.h`, and their own test file — never touching `recipe.armies`/
`recipe.markers`/`recipe.props`/`recipe.decals`). **Ruling: this resolution step is a straight
1:1 copy-through, no symmetry-orbit expansion.** An author who wants a mirrored prop places the
mirrored copy manually, exactly as they already must for armies and markers today.

**What "straight 1:1 copy-through" means for every `Data::PlacementInstance` field, grounded in
the same "no PROC stage computes or reinterprets them" sentence above (this governs the whole
step, not just symmetry) — do not compute or infer any of the following, only copy or default:**
- `positionX/Y/Z`, `rotationX/Y/Z/W`, `scaleX/Y/Z` — copied verbatim from
  `transform.transform` (`Params::InstancedTransform`, `src/params/InstancedTransform_PARAMS.h`).
  Note `positionY` (elevation) is copied as-authored, **not** resampled from the terrain
  heightfield the way `Placement_Emit_PROC.cpp:62-63` does for procedural instances — the manual
  transform already carries an authored elevation; resampling it would be exactly the
  "reinterpretation" Ruling 3's precedent forbids.
- `templateIdentifier` — left at `Data::TemplateIdentifier`'s own default (zeroed,
  `PlacementInstance_DATA.h:17`). Manual authoring has no tpId concept: `PropInstanceGroup`/
  `DecalInstanceGroup` key by `blueprintPath` (`std::string`, a full asset path e.g.
  `"Props/Rock/Rock01.santp"` — confirmed in `MapImporter_IO_Test.cpp:999,1021`), a fundamentally
  different shape from the fixed 7-character-plus-terminator `char[8]` tpId scheme
  `Data::TemplateIdentifier` exists for (`PlacementInstance_DATA.h:13-15`). Truncating/hashing a
  full path string into that 8-byte field would invent a lossy mapping nothing in `ARCH.md`
  specifies. Confirmed safe to leave zeroed: grepping every `.templateIdentifier` read site in
  `src/` shows nothing reads `results.props[...]`/`results.decals[...]`'s `templateIdentifier`
  column today — the only SoA `templateIdentifier` column ever read back is `results.markers`'s
  (`MarkersTab_Placed_UI.cpp:25`). `blueprintPath` itself is untouched and stays the manual props/
  decals' real identity, exactly as already preserved on `recipe.props`/`recipe.decals`.
- `category`, `symmetryIdentifier`, `biomeStratumIndex`, `armyIndex`, `bCollidable` — all left at
  `Data::PlacementInstance`'s own struct defaults (`PlacementInstance_DATA.h:46-51`: `category = 0`,
  `symmetryIdentifier = 0`, `biomeStratumIndex = 0`, `armyIndex = -1`, `bCollidable = false`). None
  of these are computable from a `PropTransform`/`DecalTransform` without inventing a value (there
  is no biome sample, no symmetry group, no army, no collision flag on the manual transform types)
  — leaving them at their existing defaults is the literal meaning of "not a symmetry-orbit
  expansion... no PROC stage computes or reinterprets them," applied consistently to every field,
  not just the symmetry ones.
- `ruleIndex` — **the one explicit exception to "leave at struct default," found by
  `STEP83_ReclaimFilterWiring_UI.md` §7 (this session).** `Data::PlacementInstance`'s own struct
  default is `ruleIndex = 0` (`PlacementInstance_DATA.h:46`), but unlike `armyIndex`'s `-1`, `0` is
  **not** a neutral "no rule" value — it is a live, in-range procedural rule index whenever
  `recipe.propRules`/`recipe.decalRules` is non-empty.
  `STEP50_ProceduralSubLayerCsrBucketIndex_UI.md`'s CSR bucket build (`Data::RuleBucketIndex::Build`,
  `STEP50:118-134`) keys directly off this column and drops only keys **outside**
  `[0, bucketTotal)` (`IsValidBucket`, `STEP50:142`: `bucket >= 0 && bucket < bucketCount`) — it
  cannot distinguish "genuinely rule 0's instance" from "manual instance that inherited the struct
  default." Left at `0`, every manually-authored instance would silently land in procedural rule
  0's CSR bucket and get double-drawn/mis-partitioned by any per-rule consumer (including STEP83's
  own `bReclaimable` seed-time routing). **Ruling: manual instances set `instance.ruleIndex = -1;`
  explicitly** — the same "not applicable" sentinel `armyIndex` already uses. `IsValidBucket`'s own
  `bucket >= 0` check already excludes `-1`, so this requires zero change on STEP50's side —
  confirmed by direct read of `RuleBucketIndex_DATA.h:142` above, not just asserted.
  **Flag explicitly, do not fix here:** manual props ship non-collidable
  (`bCollidable = false`) because `PropTransform` carries no collision flag — unlike procedural
  `PropRule`, which sets `ScatterSelectionFlag::Collidable` from `rule.transform.bCollidable`
  (`Placement_Rules_PROC.cpp:51` region / `PlacementTest`'s own fixture at
  `Placement_PROC_Test.cpp:51`). This may matter for gameplay (`AI_HOSTCLIENT_SPEC`'s collidable-
  props-are-gameplay framing, `PlacementInstance_DATA.h:51`'s own comment) but adding a collision
  flag to `PropTransform` is a PARAMS-layer schema change `ARCH_14_13_OpenItems.md` §14.13 item 3 does not
  authorize — out of scope for this ticket; note it as a follow-up gap, do not invent the field.

**Where the new step plugs in.** New file `src/proc/Placement_Manual_PROC.cpp`, mirroring the
existing `Placement_Rules_PROC.cpp`/`Placement_Emit_PROC.cpp` naming convention (one small file per
distinct concern within the `Placement_*_PROC` family, all `#include "Placement_PROC.h"`, same
`namespace SanmapGen::Proc`). Add one new private method to `PlacementStage`
(`src/proc/Placement_PROC.h`, alongside `BuildRuleConfigurations()` etc.,
`Placement_PROC.h:62-63`):
```cpp
void ResolveManualPropsAndDecals();   // Placement_Manual_PROC.cpp — hand-authored props/decals,
                                       // straight 1:1 copy-through, no symmetry (ARCH_14_13_OpenItems.md §14.13 item 3)
```
Call it from `PlacementStage::RunScatter` (`Placement_PROC.cpp:40-58`), immediately after the
existing early-return guard, before `BuildRuleConfigurations()`:
```cpp
void PlacementStage::RunScatter(bool bUseGpuGate) {
    results.Clear();
    evaluatedCandidateCount = 0;
    acceptedCandidateCount  = 0;
    nextSymmetryIdentifier  = 1;
    bGpuGateUsed            = false;
    bObstacleFieldBuilt     = false;
    bGpuFieldsUploaded      = false;
    if (!recipe.IsValid() || !mapFields.IsSized()) return;

    ResolveManualPropsAndDecals();   // NEW — no dependency on rule configs or derived fields

    BuildRuleConfigurations();
    BuildDerivedFields();
    ...
}
```
Placed after the guard (not before) so manual resolution keeps the same "stage requires a valid
recipe and sized fields" contract every other resolved output already honours — it does not
change what "the stage produced nothing" means. It runs before the procedural rule loop because it
has zero dependency on `ruleConfigurations`/derived fields (`obstacleDistanceField`,
`gateWeightField`); ordering relative to the procedural loop has no observable effect since the two
paths write disjoint field values.

Shape of `Placement_Manual_PROC.cpp` (illustrative — coder's call on exact helper decomposition,
must stay under the §1.5 ceilings):
```cpp
#include "Placement_PROC.h"

namespace SanmapGen {
namespace Proc {
namespace {

int ResolveManualLayerId(int layerIndex, const std::vector<Params::PropInstanceLayer>& layers) {
    if (layerIndex < 0 || static_cast<std::size_t>(layerIndex) >= layers.size()) return -1;
    return layers[layerIndex].layerId;
}
int ResolveManualLayerId(int layerIndex, const std::vector<Params::DecalInstanceLayer>& layers) {
    if (layerIndex < 0 || static_cast<std::size_t>(layerIndex) >= layers.size()) return -1;
    return layers[layerIndex].layerId;
}

Data::PlacementInstance MakeManualInstance(const Params::InstancedTransform& transform) {
    Data::PlacementInstance instance;   // every other field stays at its own default (see ruling)
    instance.positionX = transform.positionX; instance.positionY = transform.positionY;
    instance.positionZ = transform.positionZ;
    instance.rotationX = transform.rotationX; instance.rotationY = transform.rotationY;
    instance.rotationZ = transform.rotationZ; instance.rotationW = transform.rotationW;
    instance.scaleX = transform.scaleX; instance.scaleY = transform.scaleY; instance.scaleZ = transform.scaleZ;
    instance.ruleIndex = -1;   // NOT the struct default (0) — 0 is a live procedural rule index;
                               // -1 is the sentinel STEP50's CSR bucket build excludes (STEP83 §7)
    return instance;
}

} // namespace

void PlacementStage::ResolveManualPropsAndDecals() {
    for (const Params::PropInstanceGroup& group : recipe.props) {
        for (const Params::PropTransform& transform : group.transforms) {
            Data::PlacementInstance instance = MakeManualInstance(transform.transform);
            instance.manualLayerId = ResolveManualLayerId(transform.layerIndex, recipe.propLayers);
            results.props.Append(instance);
        }
    }
    for (const Params::DecalInstanceGroup& group : recipe.decals) {
        for (const Params::DecalTransform& transform : group.transforms) {
            Data::PlacementInstance instance = MakeManualInstance(transform.transform);
            instance.manualLayerId = ResolveManualLayerId(transform.layerIndex, recipe.decalLayers);
            results.decals.Append(instance);
        }
    }
}

} // namespace Proc
} // namespace SanmapGen
```
`ResolveManualLayerId`'s bounds check is required defensive coding, not new architecture: a
hand-edited `.sanmap` can carry a `layerIndex` outside `recipe.propLayers`/`decalLayers`'s current
range (the same class of risk `ClampPropLayerIndicesForRemovedLayer`,
`PropsTab_Manual_UI.cpp:43-67`, already exists to guard against on the authoring side) — an
out-of-range `layerIndex` here resolves to the same `-1` "N/A" sentinel `armyIndex` already uses
for "not applicable," not a crash.

### (b2) — `manualLayerId` correlation column on `Data::PlacementInstances`
Mirrors the existing `armyIndex` column's exact shape. Confirmed by direct read of
`src/data/PlacementInstances_DATA.h`:
- Declaration: `std::vector<int> armyIndex;` (line 27).
- `Clear()`: `armyIndex.clear();` (line 38).
- `Reserve()`: `armyIndex.reserve(instanceCount);` (line 49).
- `Append()`: `armyIndex.push_back(instance.armyIndex);` (line 69).
- `Get()`: `instance.armyIndex = armyIndex[index];` (line 86).

Add `manualLayerId` as a sibling column at each of those five sites, same pattern:
```cpp
// declaration, next to armyIndex
std::vector<int>  manualLayerId;   // -1 for procedurally-scattered; STEP56's layerId for manual
```
```cpp
// Clear()
manualLayerId.clear();
```
```cpp
// Reserve()
manualLayerId.reserve(instanceCount);
```
```cpp
// Append()
manualLayerId.push_back(instance.manualLayerId);
```
```cpp
// Get()
instance.manualLayerId = manualLayerId[index];
```
And on the record view, `src/data/PlacementInstance_DATA.h` (confirmed current shape,
`PlacementInstance_DATA.h:34-52`: `armyIndex` declared at line 50 as
`int armyIndex = -1; // units only; -1 for markers/props/decals`), add a sibling field
immediately after it:
```cpp
int  manualLayerId     = -1; // manual props/decals only; -1 for procedurally-scattered instances
```
Default `-1` for every procedurally-scattered instance (the field's own default, never touched by
`Placement_Emit_PROC.cpp`'s `EmitInstance` — confirmed: `EmitInstance` never sets `armyIndex`
either except via `configuration.armyIndex`, so `manualLayerId` correctly stays at its `-1` default
for every procedural instance with zero code change to `Placement_Emit_PROC.cpp`); populated with
STEP56's `layerId` (not the renumbered `layerIndex`) for manually-authored instances, via (b1)'s
`ResolveManualLayerId`.

## Target files
- New `src/proc/Placement_Manual_PROC.cpp` — `PlacementStage::ResolveManualPropsAndDecals()` (b1).
- `src/proc/Placement_PROC.h` — declare `ResolveManualPropsAndDecals()` private method (b1).
- `src/proc/Placement_PROC.cpp` — call it from `RunScatter()` (b1).
- `src/data/PlacementInstance_DATA.h` — add `manualLayerId` field (b2).
- `src/data/PlacementInstances_DATA.h` — add `manualLayerId` column, threaded through `Clear`/
  `Reserve`/`Append`/`Get` (b2).
- `src/proc/Placement_PROC_Test.cpp` (or a new sibling test file if the coder judges the existing
  one would exceed the §1.5 ceiling with this addition) — new acceptance coverage, see below.

## Explicit out-of-scope
- **Symmetry-orbit expansion for manual props/decals** — ruled out entirely by Ruling 3. Do not add
  `bSymmetryUseGlobal`/`symmetryMask` to `PropTransform`/`DecalTransform`.
- **A collision flag on `PropTransform`** — flagged above as a real gap (manual props ship
  non-collidable) but out of scope; would require a PARAMS/IO schema change `ARCH_14_13_OpenItems.md` §14.13 item 3
  does not authorize.
- **Resampling `positionY` from the terrain heightfield for manual instances** — explicitly ruled
  out; the authored elevation is copied verbatim, matching every other hand-placed type's
  round-trip-fidelity framing.
- **Any mapping from `blueprintPath` into `Data::TemplateIdentifier`** — left zeroed; inventing a
  truncation/hash scheme is new architecture, not this ticket's to decide, and nothing downstream
  reads it for props/decals today (confirmed by grep, see (b1) above).
- **Phase 5.3** (wiring manual Props/Decals sub-layers into the View toolbar's overlay sections,
  keyed off `manualLayerId`) — separate, blocked-on-this ticket per
  `SEQUENCE_PreviewOverlayLayering.md`. This ticket only makes the correlation column exist and
  get populated; nothing yet reads it for UI overlay purposes.
- **Phase 1.3's CSR bucket-index build** (`STEP50_ProceduralSubLayerCsrBucketIndex_UI.md`, per-layer
  flat index arrays keyed on `ruleIndex`/`category` for procedural sub-layers). **Correction (found
  by `STEP83_ReclaimFilterWiring_UI.md` §7, this session): this ticket is NOT unrelated to STEP50.**
  This section previously claimed "unrelated column, unrelated mechanism" — that claim was wrong.
  The two tickets share the `Data::PlacementInstance::ruleIndex` column directly: STEP50's bucket
  build reads `results.props.ruleIndex`/`results.decals.ruleIndex` and cannot distinguish a manual
  instance from a genuine procedural rule-0 instance by inspection alone. This ticket resolves the
  collision entirely on its own side (see the `ruleIndex = -1` ruling above); as a result **STEP50
  itself requires zero changes** — its existing `IsValidBucket` range check already excludes `-1`.
  What remains genuinely out of scope for *this* ticket is only building or modifying the CSR index
  itself (STEP50's own file) and wiring a future manual-sub-layer index keyed off `manualLayerId`
  (Phase 5.3, not yet drafted).
- **STEP56 itself** (the `layerId` field, its derive-on-create rule, its `"Id"` JSON wire key) —
  a separate, prerequisite ticket. This ticket only *consumes* `layerId` once it exists.

## Layer & accuracy class
PROC (new resolution step, `results.props`/`results.decals` population) + DATA (new SoA/record
column). Accuracy class: Exact — this is a lossless copy of already-round-tripped data, not a
computed field; there is no preview/output accuracy split to reason about (nothing here samples a
field or sizes an approximation).

## Backend policy
CPU only, unconditionally — matches `Placement_PROC.h`'s existing header comment ("Cpu is
authoritative... the stage never picks a backend") and STEP16/STEP23's established precedent that
CPU-only, non-GPU-representable per-instance data (`ruleTemplateIdentifiers`,
`ruleRadialSymmetryRepeatCounts`) never gets mirrored into `ScatterRuleConfiguration` or its GLSL
twin. `ResolveManualPropsAndDecals()` never touches `ScatterRuleConfiguration`,
`Placement_PROC.glsl`, or `BuildGateFieldGpu`/`BuildGateFieldCpu` — it runs identically regardless
of `bUseGpuGate`, called once per `RunScatter` before the backend-dependent per-rule loop even
starts. No GPU counterpart is needed or authorized: this is not a compute kernel, it is a straight
data copy with no per-cell field sampling, and §6.1's "CPU and GPU implemented and parity-checked"
requirement does not apply to a stage substep that performs no computation at all — there is
nothing for a GPU twin to accelerate or to diverge from.

## ARCH rules invoked
- `ARCH_14_13_OpenItems.md` §14.13 item 3 — binding; this ticket is item 3's "WORK-ORDER B" transcribed verbatim,
  including Ruling 3's no-symmetry-participation ruling and its stated precedent
  (`recipe.armies`/`recipe.markers` never appearing in `src/proc/`).
- `ARCH_12_ManualPropDecalLayers.md` §12 / `MapRecipe_PARAMS.h:96-98`'s comment — "round-trip fidelity... their entire
  purpose; no PROC stage computes or reinterprets them," the framing this ticket's field-by-field
  defaulting decisions are grounded in.
- Constitution §1.5 — size ceilings; new file kept small and single-purpose rather than folded into
  an existing `Placement_*_PROC.cpp` file.
- Constitution §3 — downward-only deps; the new file adds no new dependency direction (PROC already
  depends on PARAMS/DATA).
- `STEP83_ReclaimFilterWiring_UI.md` §7 — found the `ruleIndex` default-`0`/STEP50 CSR-bucket-0
  collision this session, and is the source of the `ruleIndex = -1` fix and the correction to this
  ticket's out-of-scope section (both above). Not itself a binding ARCH rule, but the citation of
  record for why this ticket's text changed.

## Acceptance test
In `Placement_PROC_Test.cpp` (or a new sibling test file — coder's call if size ceilings force a
split, matching the `Placement_Symmetry_PROC_Test.cpp` precedent of one test file per concern):
1. **New coverage — manual props/decals actually resolve.** Build a `Params::MapRecipe` with at
   least one `PropInstanceLayer`/`DecalInstanceLayer` (`layerId` set, per STEP56) in
   `propLayers`/`decalLayers`, and at least one `PropInstanceGroup`/`DecalInstanceGroup` in
   `props`/`decals` whose `PropTransform`/`DecalTransform::layerIndex` references that layer with a
   known, non-default `InstancedTransform` (distinct position/rotation/scale). Run
   `PlacementStage::Run()` (or `RunOnCpu()` directly) and assert:
   - `results.props.Count()`/`results.decals.Count()` include the manually-authored instance(s) —
     this is the exact gap the root problem names: **today this assertion fails**, since no PROC
     resolution step exists.
   - The resolved `positionX/Y/Z`/`rotationX/Y/Z/W`/`scaleX/Y/Z` match the authored transform
     exactly (copy-through, no resampling/mutation).
   - `results.props.manualLayerId[...]`/`results.decals.manualLayerId[...]` equals the referenced
     layer's `layerId` (not its `layerIndex` array position — construct the fixture so the two
     differ, e.g. by having more than one layer, to make this assertion load-bearing).
   - `results.props.ruleIndex[...]`/`results.decals.ruleIndex[...]` equals `-1` for every
     manually-authored instance, not `0` (the field's own struct default). Build the fixture with at
     least one `PropRule`/`DecalRule` present (`recipe.propRules`/`decalRules` non-empty) so this
     assertion is load-bearing — the bug this catches (`ruleIndex` left at `0`) is otherwise
     indistinguishable from correct behavior whenever no procedural rules exist. This is the exact
     regression `STEP83_ReclaimFilterWiring_UI.md` §7 found: left at `0`, a manual instance silently
     aliases into `STEP50`'s procedural-rule-0 CSR bucket.
2. **Sentinel default on procedural instances.** The existing procedural fixture (`MakeRecipe` /
   `Placement_PROC_Test.cpp`'s spawn+prop rules) continues to produce `manualLayerId == -1` for
   every instance in `results.markers`/`results.props`/`results.units`/`results.decals` — confirms
   the new column defaults correctly and `EmitInstance` needed no changes.
3. **Out-of-range `layerIndex` resolves to `-1`, not a crash or out-of-bounds read.** A
   `PropTransform`/`DecalTransform` whose `layerIndex` has no matching entry in
   `recipe.propLayers`/`decalLayers` (e.g. an empty `propLayers` array, or `layerIndex` past the
   array's end) still resolves — `manualLayerId == -1`, count still includes the instance
   (position/rotation/scale still copy through; only the correlation id degrades to the sentinel).
4. **No symmetry participation, directly tested.** A manual prop/decal placed with
   `recipe.globalSymmetryMask` set to a non-`None` value (e.g. `MirrorAcrossX`) still produces
   exactly one resolved instance, not two — confirming `ResolveManualPropsAndDecals()` never
   consults `globalSymmetryMask`/`ResolveSymmetryMask`/`BuildSymmetryOrbit` at all, the literal
   content of Ruling 3.
5. **Existing tests stay green, unedited (this is the correctness bar for (b2)'s SoA change):**
   `Placement_PROC_Test.cpp`'s existing checks, `Placement_Symmetry_PROC_Test.cpp`,
   `Placement_Gpu_PROC_Test.cpp` (CPU/GPU parity on `results.markers`, unaffected). Confirm
   `Placement_Gpu_PROC_Test.cpp` still passes with the new column present — the GPU gate path never
   emits a manual instance, so parity on `results.markers`/`results.props` is unaffected by this
   ticket, but the SoA's new column must not break `Get()`/`Append()`'s existing shape assumptions
   anywhere they're used (e.g. `PlacementChecksum`, which reads specific columns by name and is
   unaffected since it does not touch `manualLayerId`).
6. **`MapExporter_IO_Test.cpp`/`MapImporter_IO_Test.cpp` round-trip tests stay green with zero
   edits** — this ticket touches only `src/proc`/`src/data`, never `src/io`; per `ARCH_14_13_OpenItems.md` §14.13
   item 3's own correction note, the import/export round-trip for `recipe.props`/`recipe.decals`
   already works today and is out of this ticket's blast radius entirely.

Full `SanGenV2` build stays clean.
