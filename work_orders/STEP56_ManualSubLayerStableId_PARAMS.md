# STEP56 — Manual sub-layer stable id (`layerId`) on `PropInstanceLayer`/`DecalInstanceLayer`

**Layer:** PARAMS (field), IO (wire), UI (derive-on-create). **Domain:** `PropInstanceLayer`,
`DecalInstanceLayer`, `PropGroups`/`DecalGroups` wire arrays. **Sequence:** Phase 5.1,
`work_orders/SEQUENCE_PreviewOverlayLayering.md` ("Stable id on `PropInstanceLayer`/
`DecalInstanceLayer`..."). No dependency on other undone work-orders. This is Work-Order A of
`ARCH_14_13_OpenItems.md` §14.13 item 3 (design closed by three rulings; this ticket schedules that design's
implementation, unmodified). Work-Order B (PROC resolution + `manualLayerId` correlation column,
Phase 5.2) depends on this ticket but is explicitly out of scope here.

## Problem
`PropInstanceLayer`/`DecalInstanceLayer` (`src/params/PropInstance_PARAMS.h:30-31`) carry only
`name`/`color[4]`/`iconScale` — no stable identity. The only backward reference from a placed
instance, `layerIndex` on `PropTransform`/`DecalTransform` (`PropInstance_PARAMS.h:19-20`), is a
plain vector position: renumbered on reorder (`RenumberPropLayerIndicesForReorder`/
`RenumberDecalLayerIndicesForReorder`) and clamped on delete (`ClampPropLayerIndicesForRemovedLayer`/
`ClampDecalLayerIndicesForRemovedLayer`, all four defined `src/ui/PropsTab_Manual_UI.h:98-133` and
its decal mirror `src/ui/PropsTab_ManualDecals_UI.h:88-119`) — never a stable id. `ARCH_14_13_OpenItems.md` §14.13
item 3 needs a correlation-safe id (Phase 5.2's `manualLayerId` column on
`Data::PlacementInstances`) that survives reorder/delete/save-load, which `layerIndex` cannot
provide by design.

## Fix

### 1. New field — `PropInstance_PARAMS.h`
Add `int layerId = -1;` to both structs, next to the existing fields (`src/params/PropInstance_PARAMS.h:30-31`):
```cpp
struct PropInstanceLayer  { std::string name; float color[4] = {1.0f,1.0f,1.0f,1.0f}; float iconScale = 1.0f; int layerId = -1; };
struct DecalInstanceLayer { std::string name; float color[4] = {1.0f,1.0f,1.0f,1.0f}; float iconScale = 1.0f; int layerId = -1; };
```
`-1` is the "unassigned" sentinel — a freshly default-constructed layer that skips derive-on-create
(should not happen via the UI's own Add-Layer button, but keeps the type's default safe on its own).

**Do not touch** `RenumberPropLayerIndicesForReorder`/`RenumberDecalLayerIndicesForReorder`/
`ClampPropLayerIndicesForRemovedLayer`/`ClampDecalLayerIndicesForRemovedLayer` — they keep
renumbering the existing positional `layerIndex` exactly as today. The two fields coexist, serving
different purposes (`layerIndex` = current authoring position/JSON linkage from transforms,
unchanged in shape and behavior; `layerId` = stable identity for correlation, ARCH_14_13_OpenItems.md §14.13 item 3
Work-Order A).

### 2. Derive-on-create — `PropsTab_Manual_UI.cpp` / `PropsTab_ManualDecals_UI.cpp`
Per Ruling 1 (`ARCH_14_13_OpenItems.md` §14.13 item 3): **not** a persisted counter field on `MapRecipe` —
`MapRecipe_PARAMS.h`'s own header states it "is exactly what `mapGeneratorData` serializes"; every
existing field is real recipe content, there is no session-only-scratch precedent to extend, and a
persisted counter is itself a second piece of state that can desync from the ids it is supposed to
stay ahead of (e.g. hand-edited JSON). Instead, derive-on-create with no stored counter: a newly
created layer's `layerId` = `1 + max(layerId across the current in-memory propLayers/decalLayers)`,
or `0` if empty. Self-healing across manual JSON edits; ids already present in a loaded file are
never renumbered (only `layerIndex` is); an O(layer count) scan per creation is free at this
cardinality (tens of layers, not thousands). Reusing an id number after its owning layer is deleted
is not a hazard — the existing clamp-on-delete path leaves nothing referencing the old id.

Add a small helper next to `NextPropLayerName`/`NextDecalLayerName`
(`src/ui/PropsTab_Manual_UI.h:90`, `src/ui/PropsTab_ManualDecals_UI.h:81`) and call it from
`DrawLayerListButtons` (`src/ui/PropsTab_Manual_UI.cpp:72-79`, `src/ui/PropsTab_ManualDecals_UI.cpp:73-80`):
```cpp
// PropsTab_Manual_UI.h — next to NextPropLayerName
inline int NextPropLayerId(const std::vector<Params::PropInstanceLayer>& propLayers) {
    int maximumId = -1;
    for (const Params::PropInstanceLayer& layer : propLayers) maximumId = std::max(maximumId, layer.layerId);
    return maximumId + 1;
}
```
```cpp
// PropsTab_Manual_UI.cpp — DrawLayerListButtons
Params::PropInstanceLayer layer;
layer.name = NextPropLayerName(static_cast<int>(propLayers.size()));
layer.layerId = NextPropLayerId(propLayers);
propLayers.push_back(layer);
```
Mirror both for `DecalInstanceLayer`/`NextDecalLayerId`/`PropsTab_ManualDecals_UI.cpp`'s
`DrawLayerListButtons`.

### 3. Wire representation — Ruling 2, new `"Id"` key
Lands in the four `PropGroups`/`DecalGroups` build/read functions, PascalCase to match that array's
existing sibling keys (confirmed present: `"Name"`, `"Color"`, `"IconScale"`) — a distinct JSON
context from the transform-level array's camelCase `"layerIndex"`, which stays untouched.

`BuildPropGroupsJson` (`src/io/MapExporter_Props_IO.cpp:59-70`):
```cpp
layerJson["Name"]  = layer.name;
layerJson["Color"] = { { "r", layer.color[0] }, { "g", layer.color[1] },
                       { "b", layer.color[2] }, { "a", layer.color[3] } };
layerJson["IconScale"] = layer.iconScale;
layerJson["Id"] = layer.layerId;
```
`BuildDecalGroupsJson` (`src/io/MapExporter_Decals_IO.cpp:56-67`) — identical addition.

`ReadPropGroupsJson` (`src/io/MapImporter_Props_IO.cpp:98-116`) — legacy backfill: an entry with no
`"Id"` key gets `layerId = <its index within the array>` (already unique — array positions are
already distinct — and safe going forward, since the derive-on-create scan in step 2 above scans
whatever ids exist post-backfill before minting the next one):
```cpp
for (const nlohmann::json& layerJson : document["PropGroups"]) {
    Params::PropInstanceLayer layer;
    layer.layerId = static_cast<int>(outRecipe.propLayers.size());   // legacy-backfill default
    if (layerJson.is_object()) {
        ReadJsonText(layerJson, "Name", layer.name);
        // ...existing Color/IconScale reads, unchanged...
        ReadJsonInteger(layerJson, "Id", layer.layerId);
    }
    outRecipe.propLayers.push_back(layer);
}
```
`ReadDecalGroupsJson` (`src/io/MapImporter_Decals_IO.cpp:91-109`) — identical addition, mirrored
onto `outRecipe.decalLayers`.

## Out of scope
- **Zero rendering/overlay consumer.** No compositor, toolbar, or draw pass reads `layerId` yet —
  that is Phase 5.3 (View toolbar wiring) and later. This ticket only makes the field exist,
  derive, and round-trip.
- **`Data::PlacementInstances::manualLayerId` correlation column and manual props/decals PROC
  resolution** — Work-Order B / Phase 5.2, a separate work-order, depends on this one.
- Any change to `layerIndex`, `RenumberPropLayerIndicesForReorder`,
  `RenumberDecalLayerIndicesForReorder`, `ClampPropLayerIndicesForRemovedLayer`, or
  `ClampDecalLayerIndicesForRemovedLayer` — all four stay byte-for-byte unmodified.
- Symmetry participation for manual props/decals (Ruling 3) — unaffected by this ticket either way.

## Files touched
- `src/params/PropInstance_PARAMS.h` — `layerId` field on both layer structs
- `src/ui/PropsTab_Manual_UI.h` — `NextPropLayerId()` helper
- `src/ui/PropsTab_Manual_UI.cpp` — `DrawLayerListButtons` assigns `layerId` on create
- `src/ui/PropsTab_ManualDecals_UI.h` — `NextDecalLayerId()` helper
- `src/ui/PropsTab_ManualDecals_UI.cpp` — `DrawLayerListButtons` assigns `layerId` on create
- `src/io/MapExporter_Props_IO.cpp` — `BuildPropGroupsJson` writes `"Id"`
- `src/io/MapImporter_Props_IO.cpp` — `ReadPropGroupsJson` reads `"Id"`, legacy-backfills by index
- `src/io/MapExporter_Decals_IO.cpp` — `BuildDecalGroupsJson` writes `"Id"`
- `src/io/MapImporter_Decals_IO.cpp` — `ReadDecalGroupsJson` reads `"Id"`, legacy-backfills by index

## Verify
Acceptance bar (per the sequence entry): the new field exists, derive-on-create works, and it
round-trips through export/import including the legacy-backfill path, with new/updated unit tests.
No rendering acceptance bar — none exists yet to test against.

- **New unit test — derive-on-create**, alongside `src/ui/PropsTab_UI_Test.cpp`'s existing
  `RenumberPropLayerIndicesForReorder`/`ClampPropLayerIndicesForRemovedLayer` coverage: an empty
  `propLayers` vector yields `NextPropLayerId == 0`; a vector containing ids `{0, 2}` yields `3`
  (max-plus-one, not count-based); mirror for `NextDecalLayerId`.
- **Updated round-trip test — `src/io/MapImporter_PropsDecals_IO_Test.cpp`**
  (`RunPropsDecalsRoundTripTests`, the pure-builder/-reader deep coverage for `PropGroups`/
  `DecalGroups`): give `BuildFixtureRecipe`'s prop/decal layer a non-default `layerId` (e.g. `7`)
  and assert it survives `BuildPropGroupsJson`/`ReadPropGroupsJson` (and the Decal pair) unchanged.
- **New unit test — legacy backfill**, same file: hand-construct a `PropGroups`/`DecalGroups` JSON
  array with two entries and no `"Id"` key on either, call `ReadPropGroupsJson`/`ReadDecalGroupsJson`
  directly, and assert `layerId == 0` for the first entry and `layerId == 1` for the second (backfill
  by array index).
- **Updated live-document round-trip — `src/io/MapImporter_IO_Test.cpp`** (`FillFixturePropsAndDecals`
  / `CheckPropsAndDecals`, the `BuildSanmapJsonText`/`ParseSanmapJsonText` live-document path): set a
  non-default `layerId` on the fixture's prop/decal layer and assert it survives in `CheckPropsAndDecals`.
- Existing suites (`MapExporter_IO_Test`, `MapImporter_IO_Test`, `MapImporter_PropsDecals_IO_Test`,
  `PropsTab_UI_Test`) stay green with no behavior change to any assertion this ticket does not
  itself add — `layerIndex` renumber/clamp coverage must be byte-identical before/after.
