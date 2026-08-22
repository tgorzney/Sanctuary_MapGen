# STEP50 — CSR bucket index for procedural overlay sub-layers, keyed on `ruleIndex`

**Layer:** DATA (new index type) + PIPELINE (build wiring). **Domain:** `Data::PlacementInstances`,
`Pipeline::GenerationAssembler`. **Sequence:** Phase 1.3,
`work_orders/SEQUENCE_PreviewOverlayLayering.md`. §14.9. No dependency on Phase 1.1/1.2
(STEP47/STEP48) — this is DATA-adjacent infrastructure, independent of the coordinate-projection
and picking work those cover.

**Partial dependency on a separate, in-progress thread — corrected after this ticket's first
draft.** A different work-order thread (`ARCH_16_MarkerLayerSymmetry.md` §16, `STEP66_MarkerRuleLayer_PARAMS.md`) renames
`Params::MapRecipe::markerRules` → `markerRuleLayers` (`std::vector<MarkerRuleLayer>`, each owning
its own `std::vector<MarkerRule> rules` — a two-level nest, not a flat array). This ticket's
`props`/`units`/`decals` bucket indices are entirely unaffected and may land independent of that
thread. Only the **`markers`** bucket index's `bucketTotal` (originally `recipe.markerRules.size()`
below) is affected — corrected to sum rule counts across the new nested structure, under an
explicit, flagged assumption (see the code comment at that call site). **CONFIRMED —
`work_orders/STEP79_MarkerRuleLayerProcConsumer_PROC.md`'s "⭐ Downstream authority ruling" section
verifies `ruleIndex` stays a flat, running index over the layer-concatenated rule sequence, exactly
the assumption this ticket made, and states this ticket's `BuildRuleBucketIndex` (summing
`layer.rules.size()` across all layers, including disabled ones) is "correct as written." No code
change is needed here — the assumption flagged below is resolved, not open.**

## Problem
ARCH_14_09_RenderingPerformance.md §14.9's "Layer-id column" ruling forbids physically resorting `Data::PlacementInstances` by
layer, and instead mandates reusing the existing `ruleIndex`/`category` columns
(`PlacementInstance_DATA.h:46-47`) via a CSR (compressed sparse row) bucket index, "built once
(same lifecycle point as `Data::SpatialGrid`'s build, right after Placement, §8.3) — per-layer flat
index arrays, cached, rebuilt only when that layer's own sub-layer membership changes, not every
frame." No such index exists today: `src/` has zero matches for any bucket/rule index over
`PlacementResults`, and the future overlay draw pass (STEP53) will need, for a given procedural
sub-layer (one `recipe.markerRules[i]`/`propRules[i]`/`unitRules[i]`/`decalRules[i]`), the exact set
of SoA indices belonging to it in O(1) + O(bucket size) — not an O(N) scan of the whole collection
per draw call, per sub-layer, per frame.

**Confirmed wiring this index is built over** (read, not assumed):
- `Data::PlacementInstance::ruleIndex`/`category` (`PlacementInstance_DATA.h:46-47`) are written per
  instance at emit time: `instance.ruleIndex = configuration.ruleIndex; instance.category =
  configuration.category;` (`Placement_Emit_PROC.cpp:68-69`), then
  `CollectionFor(configuration.collectionIndex).Append(instance);` (`Placement_Emit_PROC.cpp:74`).
- `ruleIndex` is **per-family**, not a global running index: `AppendMarkerRules` assigns
  `static_cast<int>(index)` from its own loop over `recipe.markerRules`
  (`Placement_Rules_PROC.cpp:20,26` — **pre-STEP66 shape**; `STEP66` renames this to
  `recipe.markerRuleLayers`, nested, and its own consumer ticket, not yet drafted, owns updating
  `AppendMarkerRules` itself; not this ticket's file to touch); `AppendPropRules` likewise over `recipe.propRules`
  (`Placement_Rules_PROC.cpp:60,64`); `AppendUnitRules` over `recipe.unitRules`
  (`Placement_Rules_PROC.cpp:86,90`); `AppendDecalRules` over `recipe.decalRules`
  (`Placement_Rules_PROC.cpp:108,112`). So `ruleIndex` for a `results.props` instance means "index
  into `recipe.propRules`," not "index into some combined rule list" — each collection's bucket
  index must be sized and built independently, from that collection's own rule-array count.
- `CollectionFor` (`Placement_PROC.cpp:61-66`) confirms the 4-way split: `collectionIndex` 0/1/2/3 →
  `results.markers`/`props`/`units`/`decals`. `Placement_Kernel_PROC.h:52`'s
  `ScatterRuleConfiguration::collectionIndex` comment ("0 markers, 1 props, 2 units, 3 decals")
  matches. Confirms §14.13 item 4: **`Data::PlacementResults::decals` is the identical
  `Data::PlacementInstances` SoA type with the identical `ruleIndex`/`category` columns** — no
  special-case needed, decals get the same bucket index the other three domains do.
- **Why `category` gets no bucket index of its own.** §14.6 rules that `OverlayDomainKind_UI`'s
  Alloy/SpawnsArmies split "re-slices the existing `markers` buffer by its existing `category`
  column... without the DATA shape changing" — i.e. the split is a rule-level classification
  (`Params::MarkerRule::category` is fixed per rule, copied unchanged onto every instance that rule
  emits — confirmed `configuration.category = static_cast<int>(rule.category);`,
  `Placement_Rules_PROC.cpp:27`), not a second per-instance grouping that needs its own index. A
  future consumer wanting "every Alloy marker" already has the cheaper path: for each
  `recipe.markerRules[i]` whose `category == Spawn`, read that rule's own bucket from the
  `ruleIndex`-keyed index below — no second CSR, no drift risk between two indices that would have
  to agree.

## Fix — one reusable CSR type, built once per `PlacementResults` collection, right after Placement

### 1. `Data::RuleBucketIndex` — the CSR type, same two-pass counting-fill shape as `Data::SpatialGrid`
New file `src/data/RuleBucketIndex_DATA.h`, mirroring `SpatialGrid_DATA.h`'s own build (count per
bucket → prefix sum → scatter) but keyed on one integer column (`ruleIndex`) instead of a
world-position pair:

```cpp
// RuleBucketIndex_DATA.h — a computed index over one Data::PlacementInstances collection's
// ruleIndex column: which SoA indices belong to rule N, in O(1) + O(bucket size), without ever
// physically resorting the SoA (ARCH_14_09_RenderingPerformance.md §14.9's "Layer-id column" ruling). Flat CSR buckets of
// std::int32_t indices, mechanically identical build shape to Data::SpatialGrid (count -> prefix
// sum -> scatter) — see that file's own header for why this shape, not vector<vector<>>.
// Single writer (ARCH_03_ModuleBoundaries.md §3.4.1): Pipeline::GenerationAssembler, right after Placement.
#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>

namespace SanmapGen {
namespace Data {

class RuleBucketIndex {
public:
    int BucketCount() const { return bucketCount; }
    std::int32_t EntryCount() const { return static_cast<std::int32_t>(instanceIndex.size()); }
    bool IsEmpty() const { return instanceIndex.empty(); }

    // [begin, end) into InstanceIndexAt for one rule; out-of-range buckets and every bucket of an
    // empty/unbuilt index answer with an empty range (mirrors SpatialGrid::BucketBegin/End).
    std::int32_t BucketBegin(int bucket) const {
        return IsValidBucket(bucket) ? bucketStart[static_cast<std::size_t>(bucket)] : 0;
    }
    std::int32_t BucketEnd(int bucket) const {
        return IsValidBucket(bucket) ? bucketStart[static_cast<std::size_t>(bucket) + 1] : 0;
    }
    // The entry at a position inside a bucket range: an index into the Data::PlacementInstances
    // collection this index was built over.
    std::int32_t InstanceIndexAt(std::int32_t position) const {
        return instanceIndex[static_cast<std::size_t>(position)];
    }
    const std::int32_t* InstanceIndexData() const { return instanceIndex.data(); }

    // `key` is one collection's ruleIndex column (e.g. results.props.ruleIndex.data()); `count`
    // its length; `bucketTotal` the caller's OWN rule-array size (e.g. recipe.propRules.size()) —
    // NOT re-derived from max(key), so a trailing rule with zero accepted instances still gets an
    // addressable empty bucket. A key outside [0, bucketTotal) is DROPPED, not clamped: unlike
    // SpatialGrid's world-position clamp (any spatial answer is "close enough"), silently
    // reattributing an instance to the wrong rule's bucket would corrupt sub-layer membership
    // invisibly — dropping is the safer defensive default for an identity key. In practice this
    // never triggers: PROC always assigns ruleIndex within range (Placement_Rules_PROC.cpp).
    // Replaces the previous contents; it never accumulates.
    void Build(const int* key, std::int32_t count, int bucketTotal) {
        bucketCount = bucketTotal > 0 ? bucketTotal : 0;
        bucketStart.assign(static_cast<std::size_t>(bucketCount) + 1, 0);
        instanceIndex.clear();
        if (count <= 0 || key == nullptr || bucketCount <= 0) return;
        for (std::int32_t entry = 0; entry < count; ++entry)
            if (IsValidBucket(key[entry])) ++bucketStart[static_cast<std::size_t>(key[entry]) + 1];
        for (int bucket = 0; bucket < bucketCount; ++bucket)
            bucketStart[static_cast<std::size_t>(bucket) + 1] += bucketStart[static_cast<std::size_t>(bucket)];
        instanceIndex.resize(static_cast<std::size_t>(bucketStart.back()));
        std::vector<std::int32_t> writeCursor(bucketStart.begin(), bucketStart.end() - 1);
        for (std::int32_t entry = 0; entry < count; ++entry) {
            if (!IsValidBucket(key[entry])) continue;
            const int bucket = key[entry];
            instanceIndex[static_cast<std::size_t>(writeCursor[static_cast<std::size_t>(bucket)]++)] = entry;
        }
    }

    void Clear() {   // an empty index is valid and answers every query with an empty range
        bucketStart.assign(static_cast<std::size_t>(bucketCount) + 1, 0);
        instanceIndex.clear();
    }

private:
    bool IsValidBucket(int bucket) const { return bucket >= 0 && bucket < bucketCount; }
    std::vector<std::int32_t> bucketStart;      // size bucketCount + 1, monotonic
    std::vector<std::int32_t> instanceIndex;    // size = accepted entries
    int bucketCount = 0;
};

} // namespace Data
} // namespace SanmapGen
```

### 2. `Data::RuleBucketIndexSet` — the four-domain aggregate, mirrors `PlacementResults`' own shape
New file `src/data/RuleBucketIndexSet_DATA.h` (one primary type per file, ARCH_01_05_FileSizeCeilings.md §1.5 — kept separate
from `RuleBucketIndex_DATA.h` the same way `PlacementResults_DATA.h` is kept separate from
`PlacementInstances_DATA.h`):

```cpp
// RuleBucketIndexSet_DATA.h — one Data::RuleBucketIndex per Data::PlacementResults collection,
// same 4-way split (markers/props/units/decals), so a caller never has to guess which index goes
// with which collection. Layer: DATA (computed output over Data::PlacementResults).
#pragma once
#include "RuleBucketIndex_DATA.h"

namespace SanmapGen {
namespace Data {

struct RuleBucketIndexSet {
    RuleBucketIndex markers, props, units, decals;   // one per PlacementResults collection
    void Clear() { markers.Clear(); props.Clear(); units.Clear(); decals.Clear(); }
};

} // namespace Data
} // namespace SanmapGen
```

### 3. `Pipeline::GenerationAssembler` builds all four, in the same registered run as `BuildMarkerSpatialGrid`
Exact same lifecycle precedent `BuildMarkerSpatialGrid` already establishes
(`GenerationAssembler_Stages_PIPELINE.cpp:26-39`): PIPELINE is the single writer (§3.4.1), the build
runs inside the Placement stage's own registered closure, so a refresh that does not re-run
Placement cannot move it — no separate invalidation logic needed, the four indices are exactly as
stale-safe as `markerSpatialGrid` already is.

```cpp
// GenerationAssembler_Stages_PIPELINE.cpp — new function, placed right after BuildMarkerSpatialGrid()
// ARCH_14_09_RenderingPerformance.md §14.9: one Data::RuleBucketIndex per PlacementResults collection, keyed on that collection's
// own ruleIndex column, sized from that collection's own rule-array count (ruleIndex is per-family,
// not a global index — Placement_Rules_PROC.cpp). Same single-writer lifecycle as
// BuildMarkerSpatialGrid immediately above.
void GenerationAssembler::BuildRuleBucketIndex() {
    // CONFIRMED (STEP79 "⭐ Downstream authority ruling"): markers' bucketTotal sums rule counts
    // across the markerRuleLayers/rules nest (STEP66), matching ruleIndex's confirmed flat/global-
    // over-the-marker-family numbering — the same numbering STEP51's SeedMarkerDomains assumes,
    // kept consistent between the two tickets. STEP79 states this function is "correct as written";
    // no change needed.
    int markerRuleCount = 0;
    for (const Params::MarkerRuleLayer& layer : recipe.markerRuleLayers)
        markerRuleCount += static_cast<int>(layer.rules.size());
    ruleBucketIndex.markers.Build(placementResults.markers.ruleIndex.data(),
        static_cast<std::int32_t>(placementResults.markers.Count()),
        markerRuleCount);
    ruleBucketIndex.props.Build(placementResults.props.ruleIndex.data(),
        static_cast<std::int32_t>(placementResults.props.Count()),
        static_cast<int>(recipe.propRules.size()));
    ruleBucketIndex.units.Build(placementResults.units.ruleIndex.data(),
        static_cast<std::int32_t>(placementResults.units.Count()),
        static_cast<int>(recipe.unitRules.size()));
    ruleBucketIndex.decals.Build(placementResults.decals.ruleIndex.data(),
        static_cast<std::int32_t>(placementResults.decals.Count()),
        static_cast<int>(recipe.decalRules.size()));
}
```

Wire it into the existing Placement registration (`GenerationAssembler_Stages_PIPELINE.cpp:54-55`):
```cpp
AddStage("Placement", full, [this] { return placementStage.ComputeParameterHash(); },
         [this] { placementStage.Run(); BuildMarkerSpatialGrid(); BuildRuleBucketIndex(); });
```

`GenerationAssembler_PIPELINE.h` additions, same placement as the existing `MarkerSpatialGrid`
accessor/declarations (`GenerationAssembler_PIPELINE.h:67-73,109,119`):
```cpp
#include "../data/RuleBucketIndexSet_DATA.h"   // next to the existing SpatialGrid_DATA.h include (line 20)

// The procedural sub-layer index (ARCH_14_09_RenderingPerformance.md §14.9). Same lifecycle as MarkerSpatialGrid: PIPELINE is
// its single writer (§3.4.1), rebuilt inside the Placement stage's registered run, so a refresh
// that does not re-run Placement cannot move it. One Data::RuleBucketIndex per PlacementResults
// collection — see RuleBucketIndexSet_DATA.h.
const Data::RuleBucketIndexSet& RuleBucketIndex() const { return ruleBucketIndex; }
```
```cpp
void BuildRuleBucketIndex();          // GenerationAssembler_Stages_PIPELINE.cpp, private, next to BuildMarkerSpatialGrid()
Data::RuleBucketIndexSet ruleBucketIndex;   // private member, next to markerSpatialGrid
```

## Files touched
- `src/data/RuleBucketIndex_DATA.h` — new, the reusable CSR type
- `src/data/RuleBucketIndexSet_DATA.h` — new, the four-domain aggregate
- `src/pipeline/GenerationAssembler_PIPELINE.h` — new include (near line 20), new public accessor
  (near lines 67-73), new private method declaration (near line 109), new private member (near
  line 119)
- `src/pipeline/GenerationAssembler_Stages_PIPELINE.cpp` — new `BuildRuleBucketIndex()` function
  (after `BuildMarkerSpatialGrid()`, lines 26-39); `AddStage("Placement", ...)`'s closure (lines
  54-55) gains the new call
- `CMakeLists.txt` — new `add_sangen_test(RuleBucketIndex_DATA_Test src/data/RuleBucketIndex_DATA_Test.cpp)`
  in the `# DATA` block (alphabetically between `MapFields_DATA_Test` and `SpatialGrid_DATA_Test`,
  line 258)
- `src/data/RuleBucketIndex_DATA_Test.cpp` — new unit test (below)
- `src/pipeline/GenerationAssembler_Outputs_PIPELINE_Test.cpp` — new `CheckRuleBucketIndex` check,
  called from `RunOutputChecks` (below)

## Verify
This is new infrastructure with **no existing consumer yet** — the overlay draw pass (STEP53,
Phase 3.1) is what will read `RuleBucketIndex()`. So the acceptance test here is a focused unit
test of the bucket-index build/query itself, not an end-to-end rendering check:

- New `src/data/RuleBucketIndex_DATA_Test.cpp` (mirrors `SpatialGrid_DATA_Test.cpp`'s structure —
  same `check()`/`failures` harness):
  - Empty index (`bucketTotal == 0` and `count == 0`) answers every `BucketBegin`/`BucketEnd` with
    an empty range; `IsEmpty()`/`EntryCount() == 0`.
  - A known deterministic set of `ruleIndex` values across several buckets: prefix sum is
    monotonic, `bucketStart[bucketCount] == count`, every instance appears in exactly one bucket,
    and `InstanceIndexAt` for each entry in bucket N resolves (via the key array) back to
    `ruleIndex == N`.
  - A trailing rule with zero accepted instances (`bucketTotal` larger than `max(key) + 1`) still
    gets an addressable, empty, in-range bucket — proves buckets are sized from the caller's rule
    count, not re-derived from the data.
  - An out-of-range key (`key[i] >= bucketTotal` or negative) is dropped, not clamped and not
    crashing — `EntryCount()` reflects only the accepted entries.
  - Rebuild replaces, does not accumulate (build twice, assert `EntryCount()` unchanged).
  - `count <= 0`, a `nullptr` key column, and `Clear()` are all safe (same null-safety bar
    `SpatialGrid_DATA_Test.cpp` holds its own `Build`/`Clear` to).
- New `CheckRuleBucketIndex` in `GenerationAssembler_Outputs_PIPELINE_Test.cpp`, called from
  `RunOutputChecks` alongside the existing `CheckPlacement`: after a real `assembler.Run()`
  (`AssemblerTest::MakeRecipe` — 1 marker rule, 1 prop rule, 0 unit/decal rules), assert
  `assembler.RuleBucketIndex().markers`'s single bucket (`BucketBegin(0)`..`BucketEnd(0)`) contains
  every index of `assembler.Placements().markers` and none outside it (`markerCount == 4` from the
  test scene); same for `.props`'s single bucket against `placements.props`; assert `.units` and
  `.decals` are `IsEmpty()` (0 rules registered, exercises the `bucketTotal == 0` real-pipeline
  path, not just the unit test's synthetic one). This is the one integration check proving the
  PIPELINE wiring (not just the standalone type) actually fires at the right lifecycle point.
- Existing `GenerationAssembler_PIPELINE_Test.cpp`'s stage-order assertion
  (`expectedStageOrder`, still exactly 7 named stages) must stay green with zero edits — this
  work-order adds a private helper call inside the existing `"Placement"` stage's closure, it does
  not add a new pipeline stage.
