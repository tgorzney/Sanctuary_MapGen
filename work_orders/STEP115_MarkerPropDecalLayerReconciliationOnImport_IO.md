# STEP115 — Synthesize marker/prop/decal layers on import when none exist

**Layer:** IO, SYS. **Domain:** `MapImporter_MarkerLayerReconcile_IO.cpp` (new),
`MapImporter_Props_IO.cpp`, `MapImporter_Decals_IO.cpp`, `MapImporter_ParseDocument_IO.cpp`,
`MapImporter_Recipe_IO.h`, `PathStem_SYS.h` (new). **Sequence:** no dependency on other undone
work-orders.

## ⚠️ ARCH signoff corrections applied to this revision
1. **`BlueprintPathStem` promoted to a shared SYS primitive, not duplicated a 3rd/4th time.** ARCH
   ruled: two existing UI-layer copies already do this identical algorithm
   (`TemplateIdentifierFromBlueprintPath`, `MapCanvas_IconLayer_CullManual_UI.cpp:47-53`;
   `FileStemOfEntryName`, `Application_Assets_UI.cpp:23-30`) — this ticket would add a 3rd and 4th
   copy simultaneously, crossing this codebase's own stated "promote at the third occurrence"
   threshold (`MapImporter_Markers_IO.cpp`'s own header comment). New shared primitive:
   `Sys::FileStemFromPath`, `src/sys/PathStem_SYS.h`, header-only inline, mirroring
   `LuaTableValue_SYS.h`'s existing small-primitive shape. Both `IO` and `UI` may depend on `SYS`
   (§3.1) — no boundary violation. This ticket's own two new call sites (Props/Decals reconciliation)
   consume it; migrating the two existing UI copies onto it is a recommended, low-risk follow-up,
   NOT required as part of this ticket (those files are outside this ticket's scope/file list).
2. **`ReconcileMarkerLayers` split into its own new file, not added to `MapImporter_Markers_IO.cpp`.**
   ARCH found `MapImporter_Markers_IO.cpp` is ALREADY 160 lines — already over the §1.5 hard-150
   ceiling with no documented exception — and this ticket's original ~28-line addition would push it
   further with no acknowledgment. New file: `src/io/MapImporter_MarkerLayerReconcile_IO.cpp`,
   matching this codebase's own precedent for exactly this situation (`MapImporter_ParseDocument_IO.cpp`
   was split out of `MapImporter_IO.cpp` under STEP35 when it grew too large). Its declaration still
   lives in `MapImporter_Recipe_IO.h` exactly as originally planned — only the `.cpp` location
   changes. `MapImporter_Props_IO.cpp` (123 lines) and `MapImporter_Decals_IO.cpp` (115 lines) stay
   under the ceiling once correction #1 (promoting the stem helper out) is applied — no split needed
   for those two, `ReconcilePropLayers`/`ReconcileDecalLayers` stay in their existing domain files as
   originally planned.

## Problem
Importing any real, non-SanGen-authored `.sanmap` leaves `recipe.markerLayers`/`recipe.propLayers`/
`recipe.decalLayers` empty, even though the corresponding instance arrays (`recipe.markers`/
`recipe.props`/`recipe.decals`) import fine and render — because `MarkerGroups`/`PropGroups`/
`DecalGroups` (the layer-metadata wire sections) are SanGen-invented and never present in a real
file. `ReadMarkerGroupsJson` (`src/io/MapImporter_Markers_IO.cpp:116-117`), `ReadPropGroupsJson`
(`src/io/MapImporter_Props_IO.cpp:99-100`), and `ReadDecalGroupsJson`
(`src/io/MapImporter_Decals_IO.cpp:91-92`) all guard on `document.contains("<Key>") &&
document["<Key>"].is_array()` and return immediately if absent — zero default-layer synthesis. Every
imported instance's `layerIndex` (`MarkerTransform::layerIndex` default `0`,
`src/params/MarkerInstance_PARAMS.h:48`; `PropTransform::layerIndex`/`DecalTransform::layerIndex`
default `0`, `src/params/PropInstance_PARAMS.h:19-20`) silently sits at 0, pointing at a nonexistent
layer. This makes the entire per-layer editing UI (lock, grid-snap, per-layer color, symmetry, Fix
Symmetry — several tickets shipped this session, e.g. STEP106) structurally unreachable for any real
map, since it's all nested inside a layer row's expanded body and there are zero layer rows.

## Already-ratified decisions this session (implement exactly, do not re-litigate)
1. Not a migration — plain import-time normalization on already-parsed `Params::MapRecipe` structs,
   nothing to do with `SanGenVersion`. No `<Domain>_Migrate_V<N>_IO` unit.
2. Three separate functions, one per domain — not one shared function. `ReconcileMarkerLayers` lives
   in its own new file, `MapImporter_MarkerLayerReconcile_IO.cpp` (see the ARCH signoff correction
   above — `MapImporter_Markers_IO.cpp` is already over the file-size ceiling); `ReconcilePropLayers`/
   `ReconcileDecalLayers` live in their existing domain files (`MapImporter_Props_IO.cpp`/
   `MapImporter_Decals_IO.cpp`) as originally planned. This mirrors the standing precedent already in
   this exact triple of files:
   `ReadMarkerGroupsJson`/`ReadPropGroupsJson`/`ReadDecalGroupsJson` are near-identical (all three
   read the same `Name`/`{r,g,b,a} Color`/`IconScale`/`Id` shape, `ReadPropGroupsJson`'s own header
   comment at `MapImporter_Props_IO.cpp:97-98` calling it "the same shape as `ReadArmyColorJson`'s
   `{r,g,b,a}` read... reused verbatim") yet live in three separate files, never merged — the
   established posture for this domain triple. `MapExporter_Markers_IO.cpp:1-4`'s own header
   independently states the same law for `markers`/`chains`: "Own file (not shared with Chains):
   `markers`/`chains` are independent top-level format keys with no shared JSON parent (IO
   Architecture Expert ruling, applied directly from `STEP2_ArmiesAreas_IO`'s `armies`/`areas`
   precedent — `STEP3_MarkersChains_IO`)."
3. Call site: a distinct step in `ParseEntityDomainsJson` (`MapImporter_ParseDocument_IO.cpp`), run
   after both that domain's `Read*GroupsJson` and `Read*Json` — see the real ordering below.
4. Logging via `result.Warn(...)`.
5. Synthesized layer fields are struct defaults, verbatim, no distinguishing value. White-as-"unset"
   is the established convention (`MarkerInstanceLayer::color`, `PropInstanceLayer::color`,
   `DecalInstanceLayer::color` all default `{1,1,1,1}`) — do not invent a distinguishing color. Any
   type-based color resolution (e.g. Alloy-yellow for a non-Spawn marker type) is a separate,
   not-yet-drafted UI-domain ticket; this ticket must not preempt it.
6. `layerId` sequential, identical existing convention.
7. Layer NAME: Markers use `group.name` verbatim; Props/Decals use a basename/stem of
   `group.blueprintPath`.
8. Granularity: one synthesized layer per source GROUP entry (`MarkerInstanceGroup`/
   `PropInstanceGroup`/`DecalInstanceGroup`), never deduplicated by name/blueprintPath — two Prop
   entries sharing the same `blueprintPath` get two separate synthesized layers.

## Fix

### 1. Real call-site ordering — confirmed against `MapImporter_ParseDocument_IO.cpp:70-88`
`ParseEntityDomainsJson`'s real current body:
```cpp
void ParseEntityDomainsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                            MapImportResult& result) {
    ReadAreasJson(document, outRecipe);
    ReadArmiesJson(document, outRecipe);
    ReadMarkerGroupsJson(document, outRecipe);
    ReadMarkersJson(document, outRecipe, result);
    ReadChainsJson(document, outRecipe);
    ReadPropGroupsJson(document, outRecipe);
    ReadPropsJson(document, outRecipe, result);
    ReadDecalGroupsJson(document, outRecipe);
    ReadDecalsJson(document, outRecipe, result);
    ReadStratumLayersJson(document, outRecipe, result);
    ReadStratumGenerationSettingsJson(document, outRecipe, result);
    ReadScenariosJson(document, outRecipe, result);
    NormalizeArmyIdentities(outRecipe, result);   // STEP76 §4b — must stay LAST
}
```
The dispatched proposed ordering (`ReadMarkerGroupsJson`→`ReadMarkersJson`→`ReconcileMarkerLayers`→
`ReadChainsJson`→`ReadPropGroupsJson`→`ReadPropsJson`→`ReconcilePropLayers`→`ReadDecalGroupsJson`→
`ReadDecalsJson`→`ReconcileDecalLayers`) is confirmed correct against this real body — insert exactly
there, three new lines, nothing else in this function reordered:
```cpp
    ReadMarkerGroupsJson(document, outRecipe);
    ReadMarkersJson(document, outRecipe, result);
    ReconcileMarkerLayers(outRecipe, result);              // STEP115
    ReadChainsJson(document, outRecipe);
    ReadPropGroupsJson(document, outRecipe);
    ReadPropsJson(document, outRecipe, result);
    ReconcilePropLayers(outRecipe, result);                // STEP115
    ReadDecalGroupsJson(document, outRecipe);
    ReadDecalsJson(document, outRecipe, result);
    ReconcileDecalLayers(outRecipe, result);                // STEP115
    ReadStratumLayersJson(document, outRecipe, result);
    ...
```
**Ordering relative to `NormalizeArmyIdentities` is safe**: confirmed by reading
`MapImporter_ArmyIdentityNormalize_IO.h:19-24` — it rewrites `Army::name`/`displayName` and every
`markers["Spawn"].transforms[*].name` (the per-INSTANCE name, e.g. `"Player1"` → canonical), never
the owning `MarkerInstanceGroup::name` (`"Spawn"` itself, unaffected). `ReconcileMarkerLayers` reads
only `group.name`, so running it well before `NormalizeArmyIdentities` (which must stay last per the
existing STEP76 §4b comment, `MapImporter_ParseDocument_IO.cpp:65-69`) cannot desync it.

### 2. New shared primitive — `src/sys/PathStem_SYS.h`

New header-only file, mirroring `LuaTableValue_SYS.h`'s existing small-primitive shape:
```cpp
#pragma once
#include <string>

namespace SanmapGen::Sys {

// "Props/Rock/Rock01.santp" -> "Rock01". Strips up to the last '/' or '\\' and the trailing
// extension. Promoted here (STEP115) because this exact algorithm was independently written twice
// already (TemplateIdentifierFromBlueprintPath, MapCanvas_IconLayer_CullManual_UI.cpp:47-53;
// FileStemOfEntryName, Application_Assets_UI.cpp:23-30) and this ticket needed it a third/fourth
// time — crossing this codebase's own "promote at the third occurrence" threshold.
inline std::string FileStemFromPath(const std::string& path) {
    const std::size_t lastSeparator = path.find_last_of("/\\");
    const std::size_t stemBegin = lastSeparator == std::string::npos ? 0 : lastSeparator + 1;
    const std::size_t lastDot = path.find_last_of('.');
    const std::size_t stemEnd =
        (lastDot == std::string::npos || lastDot < stemBegin) ? path.size() : lastDot;
    return path.substr(stemBegin, stemEnd - stemBegin);
}

}  // namespace SanmapGen::Sys
```
Migrating the two existing UI-layer copies (`TemplateIdentifierFromBlueprintPath`/
`FileStemOfEntryName`) onto this primitive is a recommended, low-risk follow-up — NOT part of this
ticket's scope/file list; leave both existing UI functions exactly as they are.

### 3. New functions — exact bodies

**New file `src/io/MapImporter_MarkerLayerReconcile_IO.cpp`** (split out per ARCH signoff —
`MapImporter_Markers_IO.cpp` is already 160 lines, already over the §1.5 hard-150 ceiling with no
documented exception; do not add to it):
```cpp
#include "MapImporter_Recipe_IO.h"

namespace SanmapGen::Io {

// STEP115: a real, non-SanGen-authored `.sanmap` never carries `MarkerGroups` (SanGen-invented, no
// format precedent, see MapImporter_Markers_IO.cpp's own header comment) — every transform's
// layerIndex silently defaults to 0, pointing at a layer that does not exist, and the Manual Marker
// Layers tab has nothing to show. Synthesizes one MarkerInstanceLayer per `outRecipe.markers` GROUP
// entry (the marker TYPE, e.g. "Spawn"/"Alloys"), only when markerLayers is empty AND at least one
// marker group exists. A file that already carries MarkerGroups (even a short/partial one covering
// fewer groups than exist) is left exactly as read — PARTIAL coverage is explicitly out of scope for
// this ticket (see STEP115's Out of Scope section), this guard fires on EMPTY only. Every synthesized
// layer is struct-default (white, unlocked, no grid snap, default symmetry) — no distinguishing
// color invented (white-as-"unset" is this data model's existing convention).
void ReconcileMarkerLayers(Params::MapRecipe& outRecipe, MapImportResult& result) {
    if (!outRecipe.markerLayers.empty() || outRecipe.markers.empty()) return;
    for (Params::MarkerInstanceGroup& group : outRecipe.markers) {
        Params::MarkerInstanceLayer layer;
        layer.name = group.name;
        const int newLayerIndex = static_cast<int>(outRecipe.markerLayers.size());
        layer.layerId = newLayerIndex;   // same sequential convention as ReadMarkerGroupsJson's own
                                          // legacy-backfill default (MapImporter_Markers_IO.cpp:121).
        outRecipe.markerLayers.push_back(layer);
        for (Params::MarkerTransform& transform : group.transforms)
            transform.layerIndex = newLayerIndex;
    }
    result.Warn("No MarkerGroups section present; synthesized "
               + std::to_string(outRecipe.markerLayers.size())
               + " marker layer(s) from the existing marker type(s) so the Manual Marker Layers tab"
                 " has something to show.");
}

}  // namespace SanmapGen::Io
```
One aggregate `Warn` per import, not per synthesized layer: this is a single whole-document
structural fact ("no `MarkerGroups` section"), not an independent per-instance correctness event like
`ClampMarkerLayerIndex`'s per-transform warn (`MapImporter_Markers_IO.cpp:87-97`) — flooding the log
once per marker type on a map with many types would bury the signal. Confirm the real namespace/
include shape used by sibling IO `.cpp` files (e.g. `MapImporter_ParseDocument_IO.cpp`'s own top)
before finalizing this new file's header — match it exactly, the sketch above is illustrative.

**`MapImporter_Props_IO.cpp`** — add `#include "../sys/PathStem_SYS.h"` and the exported function
after `ReadPropGroupsJson` (currently the file's last function, lines 99-120), before
`} // namespace Io` (line 122):
```cpp
// STEP115: mirrors ReconcileMarkerLayers (MapImporter_MarkerLayerReconcile_IO.cpp) — same problem
// (`PropGroups` is SanGen-invented, never present on a real file), own file per the
// per-domain-file-split law. One PropInstanceLayer synthesized per `outRecipe.props` GROUP entry —
// `props` is a plain ORDERED ARRAY (this file's own header comment, line 7), so two entries sharing
// the same blueprintPath legitimately get two separate layers, never collapsed by name.
void ReconcilePropLayers(Params::MapRecipe& outRecipe, MapImportResult& result) {
    if (!outRecipe.propLayers.empty() || outRecipe.props.empty()) return;
    for (Params::PropInstanceGroup& group : outRecipe.props) {
        Params::PropInstanceLayer layer;
        layer.name = Sys::FileStemFromPath(group.blueprintPath);
        const int newLayerIndex = static_cast<int>(outRecipe.propLayers.size());
        layer.layerId = newLayerIndex;
        outRecipe.propLayers.push_back(layer);
        for (Params::PropTransform& transform : group.transforms)
            transform.layerIndex = newLayerIndex;
    }
    result.Warn("No PropGroups section present; synthesized "
               + std::to_string(outRecipe.propLayers.size())
               + " prop layer(s) from the existing prop blueprint group(s) so the Manual Prop Layers"
                 " tooling has something to show.");
}
```

**`MapImporter_Decals_IO.cpp`** — add `#include "../sys/PathStem_SYS.h"` plus:
```cpp
void ReconcileDecalLayers(Params::MapRecipe& outRecipe, MapImportResult& result) {
    if (!outRecipe.decalLayers.empty() || outRecipe.decals.empty()) return;
    for (Params::DecalInstanceGroup& group : outRecipe.decals) {
        Params::DecalInstanceLayer layer;
        layer.name = Sys::FileStemFromPath(group.blueprintPath);
        const int newLayerIndex = static_cast<int>(outRecipe.decalLayers.size());
        layer.layerId = newLayerIndex;
        outRecipe.decalLayers.push_back(layer);
        for (Params::DecalTransform& transform : group.transforms)
            transform.layerIndex = newLayerIndex;
    }
    result.Warn("No DecalGroups section present; synthesized "
               + std::to_string(outRecipe.decalLayers.size())
               + " decal layer(s) from the existing decal blueprint group(s) so the Manual Decal"
                 " Layers tooling has something to show.");
}
```

### 4. New declarations — **`MapImporter_Recipe_IO.h`, not `MapImporter_IO.h`**
Correction against the dispatched instructions: `Read*GroupsJson`/`Read*Json` are declared in
`src/io/MapImporter_Recipe_IO.h` (confirmed by grep — lines 62-63, 70-73), not in
`MapImporter_IO.h` (which declares only the `MapImporter` class and its statics, no free-function
per-domain readers). Add the three new declarations there, beside each domain's existing pair:
```cpp
void ReadMarkerGroupsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadMarkersJson(const nlohmann::json& document, Params::MapRecipe& outRecipe, MapImportResult& result);
void ReconcileMarkerLayers(Params::MapRecipe& outRecipe, MapImportResult& result);   // NEW — STEP115
void ReadChainsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);

void ReadPropGroupsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadPropsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe, MapImportResult& result);
void ReconcilePropLayers(Params::MapRecipe& outRecipe, MapImportResult& result);     // NEW — STEP115
void ReadDecalGroupsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
void ReadDecalsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe, MapImportResult& result);
void ReconcileDecalLayers(Params::MapRecipe& outRecipe, MapImportResult& result);    // NEW — STEP115
```
i.e. insert at real lines 63/71/73 respectively (`MapImporter_Recipe_IO.h:54-73`).

## Out of scope
- **The manual-marker-type-default-color gap** (ruling #5) — synthesized layers stay white; no
  type-based color resolution (e.g. Alloy-yellow) added here. Separate, not-yet-drafted UI ticket.
- **Any change to `Read*GroupsJson`/`Read*Json` themselves.** Confirmed untouched — this is a new,
  purely-additive downstream step; every existing behavior/warning/clamp in
  `ReadMarkerGroupsJson`/`ReadMarkersJson`/`ClampMarkerLayerIndex` and its Prop/Decal siblings is
  unaffected.
- **Any change to `MapExporter_Markers_IO.cpp`/`MapExporter_Props_IO.cpp`/`MapExporter_Decals_IO.cpp`.**
  Confirmed unnecessary by reading `MapExporter_Markers_IO.cpp:43-83`: `BuildMarkersJson` walks
  `recipe.markers` and `BuildMarkerGroupsJson` walks `recipe.markerLayers` unconditionally, with no
  special-casing — a synthesized layer that now exists in the recipe round-trips out through the
  existing exporter with zero exporter-side change. (Props/Decals exporters share the identical
  unconditional-array-walk shape per their own file headers; not re-read line-by-line since the
  pattern is already established and this ticket makes no PARAMS shape change for them to react to.)
- **Partial coverage — layers present but not covering every group.** The confirmed bug and every
  ratified rule above is scoped to the EMPTY-vector case (`if (!outRecipe.<domain>Layers.empty() ||
  ...) return;`). A file with e.g. one `MarkerGroups` entry but three marker types in `markers` is
  left exactly as read by this ticket — the existing `Clamp*LayerIndex` functions already handle the
  resulting out-of-range `layerIndex` values safely (loud warn + clamp to 0, not a crash), so nothing
  is unsafe about deferring this. Explicitly deferred to a future ticket if it proves to matter in
  practice — do not silently fold it into this ticket's guard condition.
- **Migrating the two existing UI-layer stem-helper copies onto the new `Sys::FileStemFromPath`
  primitive.** Recommended follow-up, not this ticket's scope/file list — `MapCanvas_IconLayer_CullManual_UI.cpp`
  and `Application_Assets_UI.cpp` are left exactly as they are.
- **`MapImporter_PropsDecals_IO_Test.cpp`'s existing clamp/backfill test coverage.** Untouched; those
  tests exercise `Read*GroupsJson`/`Read*Json` directly with populated `*GroupsJson` fixtures, never
  triggering the new empty-vector guard.

## Files touched
- `src/sys/PathStem_SYS.h` — **new file**, `Sys::FileStemFromPath` (promoted shared primitive, per
  ARCH signoff correction #1)
- `src/io/MapImporter_MarkerLayerReconcile_IO.cpp` — **new file**, `ReconcileMarkerLayers` (split out
  of `MapImporter_Markers_IO.cpp` per ARCH signoff correction #2 — that existing file is untouched by
  this ticket)
- `src/io/MapImporter_Props_IO.cpp` — new `#include "../sys/PathStem_SYS.h"`, new `ReconcilePropLayers`
- `src/io/MapImporter_Decals_IO.cpp` — new `#include "../sys/PathStem_SYS.h"`, new `ReconcileDecalLayers`
- `src/io/MapImporter_Recipe_IO.h` — three new declarations, beside each domain's existing
  `Read*GroupsJson`/`Read*Json` pair
- `src/io/MapImporter_ParseDocument_IO.cpp` — `ParseEntityDomainsJson` gains three new call-site
  lines, no reordering of any existing call
- `src/io/MapImporter_IO_Test.cpp` — new marker-domain reconciliation tests (see Verify)
- `src/io/MapImporter_PropsDecals_IO_Test.cpp` — new prop/decal-domain reconciliation tests (see
  Verify)

## Verify
Acceptance bar: a fixture with `markers`/`props`/`decals` populated and no `*Groups` keys at all
synthesizes exactly one layer per group entry, correctly named, sequentially `layerId`'d, and every
transform's `layerIndex` repointed at its real synthesized layer; a fixture WITH `*Groups` present is
provably untouched (no double-synthesis); existing suites stay green.

- **New test — `MapImporter_IO_Test.cpp`, marker synthesis on empty `MarkerGroups`**: build a raw
  `nlohmann::json` document with a `"markers"` object containing two type-groups (e.g. `"Spawn"` with
  one transform, `"Alloys"` with two transforms) and explicitly NO `"MarkerGroups"` key. Call
  `Io::ReadMarkerGroupsJson` (no-op, confirms it doesn't fabricate the key), `Io::ReadMarkersJson`,
  then `Io::ReconcileMarkerLayers`. Assert: `loaded.markerLayers.size() == 2`; `markerLayers[0].name
  == "Spawn"` (or whichever group iterated first — assert against the actual key-iteration order of
  `ReadNameKeyedObject`, `MapImporter_Markers_IO.cpp:24-34`, not an assumed alphabetical order);
  `markerLayers[0].layerId == 0`, `markerLayers[1].layerId == 1`; every transform in the `"Spawn"`
  group has `layerIndex == 0`, every transform in `"Alloys"` has `layerIndex == 1`; every synthesized
  layer's `color`/`iconScale`/`bLocked`/`bGridSnapEnabled`/`gridSnapSizeWorldUnits` equal
  `MarkerInstanceLayer`'s own struct defaults; `result.warningCount == 1` (one aggregate Warn, not
  one per layer).
- **New test — `MapImporter_IO_Test.cpp`, marker synthesis is a no-op when `MarkerGroups` present**:
  reuse `FillFixtureMarkersAndChains`'s existing fixture (already populates one `MarkerInstanceLayer`
  and one marker group, `MapImporter_IO_Test.cpp:1199-1229`) — after
  `Io::ReadMarkerGroupsJson`/`Io::ReadMarkersJson` populate `loaded.markerLayers` (size 1) and
  `loaded.markers`, call `Io::ReconcileMarkerLayers(loaded, result)` and assert `loaded.markerLayers`
  is still size 1, byte-identical to before the call (`layerId`, `name`, all fields unchanged), and
  `result.warningCount` is unchanged (no new warning fired).
- **New test — `MapImporter_IO_Test.cpp`, partial coverage is explicitly a no-op (documents the
  deferred-scope decision)**: build `loaded.markerLayers` with exactly ONE entry but `loaded.markers`
  with TWO groups (three transforms total, one group's transforms pointing at the real layer 0, the
  other group's transforms left at their default `layerIndex = 0` too — no synthesis has run for
  them). Call `Io::ReconcileMarkerLayers(loaded, result)`; assert `loaded.markerLayers.size()` is
  STILL 1 (untouched — the guard is `markerLayers.empty()`, not "every group covered") and
  `result.warningCount` unchanged. Comment the test explicitly: "STEP115 scopes reconciliation to the
  fully-empty case only; partial coverage is deferred, this test pins that behavior so it isn't
  silently changed by a future edit."
- **New test — `MapImporter_PropsDecals_IO_Test.cpp`, prop/decal synthesis on empty
  `PropGroups`/`DecalGroups`**: build a fixture with two `PropInstanceGroup` entries sharing the SAME
  `blueprintPath` (e.g. both `"Props/Rock/Rock01.santp"`, one with 1 transform, one with 2) and no
  `propLayers`; call `Io::ReconcilePropLayers`. Assert `loaded.propLayers.size() == 2` (NOT
  deduplicated by blueprintPath — pins ruling #8), both layers named `"Rock01"`, `layerId`s `0`/`1`
  sequential, first group's transform(s) have `layerIndex == 0`, second group's have `layerIndex ==
  1`. Repeat the identical shape for `DecalInstanceGroup`/`Io::ReconcileDecalLayers`. Also assert the
  `BlueprintPathStem` extraction itself against a path with no directory separator and against a path
  with no extension (both edge cases the existing UI-layer twin algorithm doesn't have a dedicated
  test for either, per a repo-wide grep — worth covering once, here).
- **New test — `MapImporter_PropsDecals_IO_Test.cpp`, prop/decal synthesis is a no-op when Groups
  present**: reuse `BuildFixtureRecipe`'s existing fixture (already populates one `PropInstanceLayer`
  and one `DecalInstanceLayer`) and assert `ReconcilePropLayers`/`ReconcileDecalLayers` leave
  `propLayers`/`decalLayers` at size 1, unchanged.
- **Existing suites stay green, unchanged assertions**: `MapExporter_IO_Test`, `MapImporter_IO_Test`
  (`RunRoundTripTests`'s existing marker/props/decals fixtures already populate their `*Layers`
  vectors before the new Reconcile calls run, so the empty-vector guard never fires for them — every
  existing assertion in `CheckMarkersAndChains`/`CheckPropsAndDecals` stays byte-identical), and
  `MapImporter_PropsDecals_IO_Test`'s three existing `Run*` functions (clamp/backfill/legacy-default
  coverage) — none of them populate zero `*Groups` while providing non-empty instances, so none
  trigger the new synthesis path; confirmed by re-reading `BuildFixtureRecipe` (always pushes exactly
  one layer per domain before any transform).
