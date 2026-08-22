# STEP60 — Manual marker layer (`MarkerInstanceLayer`) + stable `layerId`, from day one

**⚠️ Amended post-draft**: `DESIGN_MarkerLayerSymmetry_R1.md`/`_R2.md` were ratified this session
(`ARCH_16_MarkerLayerSymmetry.md` §16, `SANMAP_FORMAT_SPEC.md` Correction 16). `MarkerInstanceLayer`
gains a `symmetry` field as part of that ratification — folded into this ticket below (§1/§3), since
this is the type's one authoring home and the field is small, ratified, and directly parallel to
the rest of this ticket's shape. `STEP61_ManualMarkerSymmetryAuthoring_UI.md` (the separate, smaller
"Place Symmetric" ticket this file's original text deferred to) is **retired** — it explicitly
named the larger ratified design as its own supersession condition, and that condition is now
met. The drag-follow/`symmetryGroupIdentifier` mechanism itself is still separate, unratified-
into-a-ticket UI work and stays out of scope here (see §1's field comment).

**⚠️ Recovered 2026-08-21**: this file was deleted from disk by another session during a
cross-track ownership transfer; this is a byte-for-byte reconstruction from this session's own
conversation transcript (the original full read plus all 5 amendment edits applied on top), not a
re-derivation from `ARCH_16_01_NewParamsShapes.md` alone — nothing below is guessed.

Real difference from the Props/Decals precedent, stated up front: Props/Decals already had
`layerIndex` live (third session, ARCH_12_ManualPropDecalLayers.md §12) and are only now getting `layerId` retrofitted
(`work_orders/STEP56_ManualSubLayerStableId_PARAMS.md`, itself still unimplemented at the time of
this ticket — verify `src/params/PropInstance_PARAMS.h` before assuming it's landed). Markers have
**neither** field today — `MarkerInstanceLayer` does not exist and `MarkerTransform` has no
`layerIndex` at all (`src/params/MarkerInstance_PARAMS.h`, current contents below). This ticket
introduces both at once, with `layerId` present from creation — no retrofit generation gap to repeat
for a third domain later.

## Problem
`src/params/MarkerInstance_PARAMS.h` today:
```cpp
struct MarkerTransform {
    std::string name;                  // folded-in inner dict key — instance name (e.g. "Mex 0")
    InstancedTransform transform;
    std::string alias;
};

struct MarkerInstanceGroup {
    std::string name;                        // folded-in outer dict key — marker TYPE name
    bool bResource = false;
    std::vector<MarkerTransform> transforms;
};
```
No manual-layer metadata type, no per-instance layer reference. `MapRecipe` (`src/params/MapRecipe_PARAMS.h:101,105-108`)
carries `markers` (the entity roster) alongside `propLayers`/`decalLayers` (Props/Decals' manual-layer
metadata arrays) but has no marker equivalent. Confirmed no PROC consumer exists for `recipe.markers`
either way (no `MarkerInstanceGroup` reference anywhere under `src/proc/`) — this ticket, like
Work-Order A for Props/Decals, is pure PARAMS+IO plumbing with **zero rendering/overlay consumer**.

## Fix

### 0. ⚠️ Correction 2026-08-22 — missing prerequisite type, added here

This ticket's §1 uses `Params::SymmetrySetting`, citing `ARCH_16_01_NewParamsShapes.md` §16.1 as
if the struct already exists. **It does not — confirmed absent anywhere in `src/` (zero matches).**
The ratifying ARCH section defines it as design law but no ticket had actually created the type
until now. Add it first, in `src/params/Symmetry_PARAMS.h`, alongside the existing
`SymmetryDetection`/`SymmetryBlend` types:

```cpp
// Symmetry_PARAMS.h — new
struct SymmetrySetting {
    bool bSymmetryUseGlobal = true;
    int  symmetryMask       = 0;
    int  radialSymmetryRepeatCount = 3;
};
```

Then `#include "Symmetry_PARAMS.h"` in `MarkerInstance_PARAMS.h` before using it below. (This same
struct is independently specified, verbatim identical, in `STEP66_MarkerRuleLayer_PARAMS.md` §Solution
— whichever of STEP60/STEP66 lands first defines it; the other must check for its existence before
re-adding it, exactly the same "first ticket to land wins" rule already used elsewhere in this
backlog for shared primitives, e.g. `ReadTextFileBytes` between STEP71/STEP72.)

### 1. New type + new field — `MarkerInstance_PARAMS.h`
```cpp
struct MarkerInstanceLayer {
    std::string name;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float iconScale = 1.0f;
    int   layerId = -1;   // stable id, present from day one — see Ruling 1 below (§2)
    Params::SymmetrySetting symmetry;   // NEW — ARCH_16_01_NewParamsShapes.md §16.1, SANMAP_FORMAT_SPEC Correction 16.
                                         // The layer's own mirror-mask setting; what a future
                                         // "place with symmetry" tool would resolve against. No
                                         // consumer/writer UI exists yet (that's separate,
                                         // unratified-as-a-ticket work) — this ticket only gives
                                         // the field its PARAMS+IO home, same posture as
                                         // `layerIndex` before STEP49's tab existed.
};

struct MarkerTransform {
    std::string name;
    InstancedTransform transform;
    std::string alias;
    int layerIndex = 0;   // NEW — indexes recipe.markerLayers, plain vector position (Ruling below)
};
```
`-1` is `MarkerInstanceLayer::layerId`'s "unassigned" sentinel, identical convention to
`PropInstanceLayer::layerId`/`DecalInstanceLayer::layerId` (STEP56 §1) — a freshly default-constructed
layer that skips derive-on-create keeps a safe, visibly-invalid default on its own.

**Asymmetry vs. Props/Decals, load-bearing, confirmed by reading the real marker IO code — state
this explicitly, do not silently inherit the Props/Decals framing:** `MarkerTransform::name` and
`MarkerInstanceGroup::name` are **not** cosmetic labels the way `PropInstanceLayer::name`/
`DecalInstanceLayer::name` are. Confirmed against `src/io/MapExporter_Markers_IO.cpp`/
`MapImporter_Markers_IO.cpp`: the `.sanmap` `markers` block is a two-level name-keyed dictionary,
`markers[group.name].transforms[transform.name] = {...}` — `ReadNameKeyedObject`'s file-local helper
(`MapImporter_Markers_IO.cpp:17-28`) literally assigns `item.name = name` from the JSON object key
both levels. Both names are real dictionary keys already requiring uniqueness independent of this
ticket (a collision silently drops/overwrites an entry, not a new failure mode this ticket
introduces). `MarkerInstanceLayer::name` (the NEW type this ticket adds) is a different, ordinary
cosmetic label — it lives in the new `MarkerGroups` plain JSON array (§3 below), not the dictionary,
same posture as `PropInstanceLayer::name`.

### 2. `MapRecipe` gains `markerLayers` — `MapRecipe_PARAMS.h`
Add next to `propLayers`/`decalLayers` (`MapRecipe_PARAMS.h:107-108`):
```cpp
std::vector<MarkerInstanceLayer> markerLayers;
```
Unlike `propLayers`/`decalLayers`, whose neighboring comment (`MapRecipe_PARAMS.h:103-104`) is stale
("NOT yet live-wired" — false, confirmed live-wired), `markerLayers` ships already wired end-to-end
by this ticket (§3) — give it its own accurate comment, don't reuse or extend the stale one, and
don't fix the stale comment here either (out of scope, flagged below).

**Ruling 1 — counter placement: NOT a persisted counter field on `MapRecipe`, identical to Work-Order
A's Ruling 1.** `layerId` for a newly-created layer = `1 + max(layerId across the current in-memory
markerLayers)`, or `0` if empty. No stored counter — self-healing across hand-edited JSON, ids already
present in a loaded file are never renumbered (only `layerIndex` is, and only once a consumer exists to
renumber it — see out-of-scope note below), O(layer count) scan per creation is free at this
cardinality. Reusing an id after its owning layer is deleted is not a hazard for the same reason
Work-Order A's Ruling 1 already established.

**Derive-on-create helper — no host UI file exists yet.** `work_orders/GAP_MarkerLayerAndSymmetry_PARAMS.md`'s
own "Deferred UI work" section lists `MarkersTab_ManualLayers_UI.h/.cpp` (the Manual Marker Layers
tab, Phase 2) as **not yet built** — confirmed absent from `src/ui/`. Rather than invent that tab's
scope here (out of scope, see below) or leave the derive-on-create rule unimplemented and untestable,
add one small, pure, imgui-free header now, ready for that future tab to include:
```cpp
// src/ui/MarkerLayerId_UI.h — new, single-purpose, mirrors PropsTab_Manual_UI.h's NextPropLayerId
// (STEP56 §2) one function early, since no MarkersTab_ManualLayers_UI.h host exists yet to hold it
// alongside a NextMarkerLayerName sibling. The future Phase 2 tab includes this header instead of
// duplicating the function — do not re-derive it inline there.
#pragma once
#include <algorithm>
#include <vector>
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

inline int NextMarkerLayerId(const std::vector<Params::MarkerInstanceLayer>& markerLayers) {
    int maximumId = -1;
    for (const Params::MarkerInstanceLayer& layer : markerLayers) maximumId = std::max(maximumId, layer.layerId);
    return maximumId + 1;
}

} // namespace Ui
} // namespace SanmapGen
```

### 3. Wire representation — new `MarkerGroups` array (parallel to `PropGroups`/`DecalGroups`)
No `MarkerGroups` key exists today (confirmed: absent from `Sanmap_KnownTopLevelKeys_IO.cpp`'s known-key
list and from both `MapExporter_Markers_IO.cpp`/`MapImporter_Markers_IO.cpp`) — add it fresh, PascalCase,
identical shape to `PropGroups`/`DecalGroups` (`"Name"`/`"Color"`/`"IconScale"`) plus the new `"Id"`
key Ruling 2 below adds directly (no separate follow-up ticket needed, unlike Props/Decals where
`"Id"` is a later retrofit onto an already-shipped array).

`BuildMarkerGroupsJson` — new function, `src/io/MapExporter_Markers_IO.cpp`, mirroring
`BuildPropGroupsJson` (`MapExporter_Props_IO.cpp:59-70`):
```cpp
nlohmann::ordered_json BuildMarkerGroupsJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json markerGroups = nlohmann::ordered_json::array();
    for (const Params::MarkerInstanceLayer& layer : recipe.markerLayers) {
        nlohmann::ordered_json layerJson;
        layerJson["Name"]  = layer.name;
        layerJson["Color"] = { { "r", layer.color[0] }, { "g", layer.color[1] },
                               { "b", layer.color[2] }, { "a", layer.color[3] } };
        layerJson["IconScale"] = layer.iconScale;
        layerJson["Id"] = layer.layerId;
        // NEW — Correction 16's SymmetrySetting triplet, flattened as sibling keys, same three
        // spellings already live at the per-rule tier (Correction 4) and the MarkersStack tier
        // (Correction 15) — reused verbatim, not renamed.
        layerJson["SymmetryUseGlobal"] = layer.symmetry.bSymmetryUseGlobal;
        layerJson["SymmetryMask"] = layer.symmetry.symmetryMask;
        layerJson["RadialSymmetryRepeatCount"] = layer.symmetry.radialSymmetryRepeatCount;
        markerGroups.push_back(layerJson);
    }
    return markerGroups;
}
```
Declare in `src/io/MapExporter_Recipe_IO.h` next to `BuildMarkersJson` (line 59). Wire into
`AppendEntityDomainsJson` (`MapExporter_DocumentAssembly_IO.cpp:50-67`):
`document["MarkerGroups"] = BuildMarkerGroupsJson(recipe);` — add next to
`document["markers"] = BuildMarkersJson(recipe);` (line 61).

`ReadMarkerGroupsJson` — new function, `src/io/MapImporter_Markers_IO.cpp`, mirroring
`ReadPropGroupsJson` (`MapImporter_Props_IO.cpp:98-116`), **legacy-backfill by array index** exactly as
Ruling 2 specifies:
```cpp
void ReadMarkerGroupsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("MarkerGroups") || !document["MarkerGroups"].is_array()) return;
    outRecipe.markerLayers.clear();
    for (const nlohmann::json& layerJson : document["MarkerGroups"]) {
        Params::MarkerInstanceLayer layer;
        layer.layerId = static_cast<int>(outRecipe.markerLayers.size());   // legacy-backfill default
        if (layerJson.is_object()) {
            ReadJsonText(layerJson, "Name", layer.name);
            if (layerJson.contains("Color") && layerJson["Color"].is_object()) {
                const nlohmann::json& color = layerJson["Color"];
                ReadJsonFloat(color, "r", layer.color[0]);
                ReadJsonFloat(color, "g", layer.color[1]);
                ReadJsonFloat(color, "b", layer.color[2]);
                ReadJsonFloat(color, "a", layer.color[3]);
            }
            ReadJsonFloat(layerJson, "IconScale", layer.iconScale);
            ReadJsonInteger(layerJson, "Id", layer.layerId);
            // NEW — Correction 16. No range to validate (free integers, same tolerance as the
            // per-rule/MarkersStack tiers) — absent keys keep SymmetrySetting's own defaults.
            ReadJsonBoolean(layerJson, "SymmetryUseGlobal", layer.symmetry.bSymmetryUseGlobal);
            ReadJsonInteger(layerJson, "SymmetryMask", layer.symmetry.symmetryMask);
            ReadJsonInteger(layerJson, "RadialSymmetryRepeatCount", layer.symmetry.radialSymmetryRepeatCount);
        }
        outRecipe.markerLayers.push_back(layer);
    }
}
```
Declare in `src/io/MapImporter_Recipe_IO.h` next to `ReadMarkersJson` (line 59).

**Ordering requirement — new for this ticket, does not exist in the current code.** `ReadMarkerGroupsJson`
MUST run before `ReadMarkersJson` in `ParseEntityDomainsJson` (`MapImporter_ParseDocument_IO.cpp:61-73`),
identical reasoning to `ReadPropGroupsJson`/`ReadPropsJson`'s existing ordering comment
(`MapImporter_Props_IO.cpp:9-11`): the `layerIndex` clamp (§4 below) validates against
`outRecipe.markerLayers.size()`, which `ReadMarkerGroupsJson` populates. Insert
`ReadMarkerGroupsJson(document, outRecipe);` immediately before the existing
`ReadMarkersJson(document, outRecipe);` line.

**Also add `"MarkerGroups"` to `Sanmap_KnownTopLevelKeys_IO.cpp`'s known-key list**
(`src/io/Sanmap_KnownTopLevelKeys_IO.cpp:29-30`), next to `"PropGroups"`/`"DecalGroups"` — otherwise a
saved file's own new top-level key gets flagged as unknown on the next round-trip.

### 4. `layerIndex` clamp-to-0 on out-of-range — new for markers, requires one signature change
Constitution §6's validate-then-default-then-log convention, applied identically to
`ClampPropLayerIndex` (`MapImporter_Props_IO.cpp:57-66`):
```cpp
void ClampMarkerLayerIndex(Params::MarkerTransform& markerTransform, std::size_t markerLayerCount,
                           MapImportResult& result) {
    if (markerTransform.layerIndex >= 0
        && static_cast<std::size_t>(markerTransform.layerIndex) < markerLayerCount)
        return;
    result.Warn("Marker transform layerIndex " + std::to_string(markerTransform.layerIndex)
               + " is out of range against " + std::to_string(markerLayerCount)
               + " MarkerGroups entries; clamped to 0.");
    markerTransform.layerIndex = 0;
}
```
Call from `ReadMarkerInstanceGroupJson`'s per-transform loop (`MapImporter_Markers_IO.cpp:65-72`) —
add `layerIndex` to `ReadMarkerTransformJson`'s flattened reads (siblings of `alias`, same
flattened-on-the-wire shape `PropTransform`/`DecalTransform` already use, per that file's own header
comment) and clamp each transform immediately after reading it, mirroring
`ReadPropInstanceGroupJson`'s loop shape (`MapImporter_Props_IO.cpp:68-79`).

**Signature change required, does not exist today:** `ReadMarkersJson`
(`MapImporter_Markers_IO.cpp:76-85`, declared `MapImporter_Recipe_IO.h:59`) currently takes no
`MapImportResult&` — `void ReadMarkersJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);`.
Change to `void ReadMarkersJson(const nlohmann::json& document, Params::MapRecipe& outRecipe, MapImportResult& result);`,
threading `markerLayerCount = outRecipe.markerLayers.size()` down to `ReadMarkerInstanceGroupJson`
exactly as `ReadPropsJson` already threads `propLayerCount`/`result` to `ReadPropInstanceGroupJson`.
Update the one call site, `ParseEntityDomainsJson` (`MapImporter_ParseDocument_IO.cpp:65`):
`ReadMarkersJson(document, outRecipe, result);` (that function already has `result` in scope, `MapImportResult& result`
is its own parameter — `MapImporter_ParseDocument_IO.cpp:61-62`).

## Out of scope
- **The Manual Marker Layers UI tab** (`MarkersTab_ManualLayers_UI.h/.cpp`, `GAP_MarkerLayerAndSymmetry_PARAMS.md`'s
  Phase 2) — a full `DraggableList<MarkerInstanceLayer>` block with add/delete/reorder, "Use Group
  Color" toggle, and the reorder/delete repair functions for `layerIndex`
  (the `RenumberPropLayerIndicesForReorder`/`ClampPropLayerIndicesForRemovedLayer` equivalents,
  `PropsTab_Manual_UI.h:98-133`) — none of that exists for markers yet and none of it is built here.
  This ticket only adds `NextMarkerLayerId()` (§2) so that future tab has a tested primitive to call,
  exactly as `NextPropLayerId` exists for `PropsTab_Manual_UI.cpp` to call (STEP56 §2). Until that tab
  ships, `layerIndex` values in `recipe.markers` can only change via hand-edited JSON or a future
  importer/other tool — the import-time clamp (§4) is what keeps that safe in the meantime.
- **A per-instance Layer picker on STEP49's manual-marker roster editor** — same Phase-2 dependency.
- **Any rendering/overlay consumer.** Same posture as Work-Order A: no compositor, toolbar, or draw
  pass reads `markerLayers`/`layerIndex` yet.
- **`Data::PlacementInstances` correlation column / PROC resolution for manual markers** — `recipe.markers`
  has zero PROC consumer today (confirmed, Problem section) and stays that way; this ticket does not
  add one, matching Work-Order B's explicit non-bundling for Props/Decals.
- **`symmetryGroupIdentifier` on `MarkerTransform`, the drag-follow/gesture-matching mechanism, or
  any UI to author/consume `MarkerInstanceLayer::symmetry`.** `DESIGN_MarkerLayerSymmetry_R1.md`/
  `R2.md` are now ratified as *design* (`ARCH_16_MarkerLayerSymmetry.md` §16), but no coder ticket
  exists yet for the drag/UI half of that work — this ticket adds only the layer's own `symmetry`
  field (§1/§3), its ratified PARAMS+IO home, with no consumer.
- **Fixing `MapRecipe_PARAMS.h:103-104`'s stale "NOT yet live-wired" comment** on `props`/`decals` —
  real, but pre-existing and unrelated to markers; not this ticket's file to touch beyond adding
  `markerLayers`' own accurate comment.
- **`MarkerRule`/`MarkerRuleLayer`** (the *procedural* marker-rule tier) — untouched; this ticket is
  entirely the manual/hand-placed side (`recipe.markers`, `MarkerInstanceGroup`/`MarkerTransform`),
  same domain split as `STEP66_MarkerRuleLayer_PARAMS.md`'s (unratified draft) full scope.

## Files touched
- `src/params/MarkerInstance_PARAMS.h` — new `MarkerInstanceLayer` struct; `layerIndex` field on `MarkerTransform`
- `src/params/MapRecipe_PARAMS.h` — `std::vector<MarkerInstanceLayer> markerLayers;`
- `src/ui/MarkerLayerId_UI.h` — new file, `NextMarkerLayerId()`
- `src/io/MapExporter_Markers_IO.cpp` — new `BuildMarkerGroupsJson`
- `src/io/MapExporter_Recipe_IO.h` — `BuildMarkerGroupsJson` declaration
- `src/io/MapExporter_DocumentAssembly_IO.cpp` — `document["MarkerGroups"] = BuildMarkerGroupsJson(recipe);`
- `src/io/MapImporter_Markers_IO.cpp` — new `ReadMarkerGroupsJson`; `ReadMarkerTransformJson` reads
  `layerIndex`; new `ClampMarkerLayerIndex`; `ReadMarkersJson` signature gains `MapImportResult& result`
  and threads `markerLayerCount`
- `src/io/MapImporter_Recipe_IO.h` — `ReadMarkerGroupsJson` declaration; `ReadMarkersJson` signature update
- `src/io/MapImporter_ParseDocument_IO.cpp` — `ReadMarkerGroupsJson` call added before `ReadMarkersJson`;
  `ReadMarkersJson` call updated to pass `result`
- `src/io/Sanmap_KnownTopLevelKeys_IO.cpp` — `"MarkerGroups"` added to the known-key list

## Verify
Acceptance bar: the new fields/type exist, derive-on-create works, and everything round-trips through
export/import including the legacy-backfill path and the new `layerIndex` clamp, with new/updated unit
tests. No rendering acceptance bar — none exists yet to test against.

- **New unit test — derive-on-create**, new `src/ui/MarkerLayerId_UI_Test.cpp` (or folded into an
  existing small UI-test binary if one already aggregates single-function headers like this — check
  before creating a new test executable target): an empty `markerLayers` vector yields
  `NextMarkerLayerId == 0`; a vector containing ids `{0, 2}` yields `3` (max-plus-one, not
  count-based) — identical cases to STEP56's `NextPropLayerId` coverage.
- **New unit test — `MarkerGroups` round-trip**, `src/io/MapImporter_IO_Test.cpp` (extend
  `FillFixtureMarkersAndChains`/`CheckMarkersAndChains`, `MapImporter_IO_Test.cpp:484-530,951-974`,
  the live `BuildSanmapJsonText`/`ParseSanmapJsonText` path — there is no dedicated
  `MapImporter_MarkersChains_IO_Test.cpp` deep-builder file the way Props/Decals has
  `MapImporter_PropsDecals_IO_Test.cpp`; extending the existing live-document fixture is the correct,
  proportionate target for this domain, not standing up a new test file): give the fixture one
  `MarkerInstanceLayer` with a non-default `layerId` (e.g. `7`) and set the fixture marker transform's
  `layerIndex` to `0` (in range — this fixture feeds `RunRoundTripTests`'s "no warning" assertion,
  same constraint `CheckPropsAndDecals`'s own comment states, `MapImporter_IO_Test.cpp:534-536`);
  assert `recipe.markerLayers.size() == 1`, the layer's `name`/`color`/`iconScale`/`layerId` all
  survive, and the transform's `layerIndex` survives.
- **New unit test — legacy backfill**, same file or a small new one alongside it: hand-construct a
  `MarkerGroups` JSON array with two entries and no `"Id"` key on either, call `ReadMarkerGroupsJson`
  directly, assert `layerId == 0` for the first entry and `layerId == 1` for the second.
- **New unit test — `layerIndex` out-of-range clamp**: hand-construct a `markers` JSON entry with
  `layerIndex` set out of range (e.g. `5` against zero or one `MarkerGroups` entries), call
  `ReadMarkersJson` directly with an empty `outRecipe.markerLayers`, assert the resulting
  `MarkerTransform::layerIndex == 0` and that `result` carries a warning — mirrors
  `MapImporter_Props_IO.cpp`'s own `ClampPropLayerIndex` coverage (locate its existing test and mirror
  its exact assertion shape).
- **Existing suites stay green with no behavior change to any assertion this ticket does not itself
  add**: `MapExporter_IO_Test`, `MapImporter_IO_Test` (all non-marker fixtures/checks byte-identical),
  `Sanmap_KnownTopLevelKeys_IO`'s own coverage (if any exists — confirm), and anything exercising
  `ReadMarkersJson`'s old two-argument call shape (must be updated at its one call site, not left
  broken).
