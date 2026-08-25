# STEP108 — Prop/Decal manual layer lock (`bLocked`) on `PropInstanceLayer`/`DecalInstanceLayer`

**Layer:** PARAMS (fields), IO (wire), UI (gate wiring, controls). **Domain:**
`Params::PropInstanceLayer`, `Params::DecalInstanceLayer`, `PropsTab_Manual_UI.h/.cpp`,
`PropsTab_ManualDecals_UI.h/.cpp`. **Sequence:** no dependency on STEP106/STEP107 (disjoint files —
markers vs. props/decals); may land in either order or concurrently with them.

## Problem
`DESIGN_Assembly_R1.md`'s cross-layer multi-select (§2) needs to gate on "visible AND unlocked," but
found (its own §"Gating 'visible + unlocked'" section, lines 167-178) that **no manual entity layer
type carries a lock flag at all** — `PropInstanceLayer`/`DecalInstanceLayer`/`MarkerInstanceLayer`
(`PropInstance_PARAMS.h:30-31`) carry only `name`/`color`/`iconScale`/`layerId`. That design doc's
final open question (line 394-397) asked the human to rule whether to add `bLocked` now or defer;
**the human decided to extend it now**. STEP106 (a separate, already-ratified ticket) adds `bLocked`
to `MarkerInstanceLayer`. This ticket is the Props/Decals half.

Confirmed by reading `PropInstance_PARAMS.h` in full: `PropInstanceLayer` and `DecalInstanceLayer`
are **two parallel structs, not one shared type** (`PropInstance_PARAMS.h:30-31`) — identical shape,
separate definitions, exactly like `PropTransform`/`DecalTransform` above them. Every field addition
and every gate/wiring function below is therefore duplicated once per struct, mirroring how the
existing `ResolvePropInstanceLayerId`/`ResolveDecalInstanceLayerId` pair (same file, lines 37-44)
duplicates rather than templates.

**Confirmed divergence from STEP106 — read before implementing.** Markers have a full manual
per-instance authoring surface: live canvas drag (`MarkerDragGesture_UI.cpp`,
`MapCanvas_MarkerDrag_UI.cpp`), an instance roster with Position sliders and Add/Remove Instance
buttons (`MarkersTab_ManualInstance_UI.cpp`). **Props and Decals have none of this.** Confirmed by
grep: `PropTransform`/`DecalTransform`/`PropInstanceGroup`/`DecalInstanceGroup` appear in exactly two
UI files, `PropsTab_Manual_UI.cpp` and `PropsTab_ManualDecals_UI.cpp` (plus the read-only cull path
`MapCanvas_IconLayer_CullManual_UI.cpp`) — there is no prop/decal equivalent of
`MarkersTab_ManualInstance_UI.cpp` anywhere in `src/`, and `MapCanvas_MarkerDrag_UI.cpp` (grepped in
full) contains zero Prop/Decal references. In both `PropsTab_Manual_UI.cpp` and
`PropsTab_ManualDecals_UI.cpp`, the only per-instance-transform UI is `DrawTransformList` — and per
each header's own SCOPE NOTE 2 (`PropsTab_Manual_UI.h:23-29`, `PropsTab_ManualDecals_UI.h:14-22`) it
is **explicitly READ-ONLY**, previewing the resolved procedural `Data::PlacementInstances` buffer,
not `recipe.props`/`recipe.decals` — there is no slider, no Add Instance button, no Remove Instance
button, no drag gesture to gate. The only things a human can edit today for `recipe.props`/
`recipe.decals` at all are: (a) each manual LAYER's own `name`/`color`/`iconScale` (not gated by lock
in STEP106's marker precedent either — see its §4 vs. its silence on `DrawLayerRowSettings`'s Name/
Color/Icon-Scale fields), and (b) the layer LIST's own Add/Reorder/Delete operations, which STEP106
also leaves ungated (its `ApplyLayerListSignal`'s `ToggleLock` branch returns early; Delete/Reorder
fall through to the normal structural handling regardless of `bLocked`, same posture kept here).

**Conclusion, stated plainly so it is not "fixed" later as a bug**: this ticket adds the field, the
wire round-trip, the `[L]`/`[U]` `DraggableList` affordance (wiring the same dead `ToggleLock` signal
STEP106 found dead for markers — confirmed dead here too, same evidence: `DrawLayerList`'s
row-describe lambda never sets `row.bLocked`, `ApplyLayerListSignal` has no `ToggleLock` branch, in
BOTH `PropsTab_Manual_UI.cpp` and `PropsTab_ManualDecals_UI.cpp`), and a pure out-of-range-safe gate
function per struct — matching STEP106's shape exactly for all of that. But **there is no live
production call site to gate a position/add/remove edit behind**, because that editing capability
does not exist in the Props/Decals domain today. The gate functions are added now as the
Assembly-design's forward-looking extension point (its own recommended shape, line 177) and so the
lock icon reflects real, settable state — not because they refuse anything yet. Do not invent a
prop/decal drag gesture, position-slider roster, or Add/Remove-Instance UI to give the lock something
to gate; that is out of scope (see "Out of scope").

## Fix

### 1. New field — `PropInstance_PARAMS.h`
Add `bLocked` to both structs (`PropInstance_PARAMS.h:30-31`), next to `layerId`:
```cpp
struct PropInstanceLayer  { std::string name; float color[4] = {1.0f,1.0f,1.0f,1.0f}; float iconScale = 1.0f; int layerId = -1; bool bLocked = false; };
struct DecalInstanceLayer { std::string name; float color[4] = {1.0f,1.0f,1.0f,1.0f}; float iconScale = 1.0f; int layerId = -1; bool bLocked = false; };
```
Additive-only. No `SanGenVersion` bump (Constitution's additive-field rule — same posture STEP106 §1
uses for `MarkerInstanceLayer::bLocked`, and the same posture every prior `PropInstanceLayer`/
`DecalInstanceLayer` field addition on this line already used).

### 2. Wire representation — `MapExporter_Props_IO.cpp` / `MapImporter_Props_IO.cpp` /
`MapExporter_Decals_IO.cpp` / `MapImporter_Decals_IO.cpp`
Wire key, PascalCase to match each array's existing sibling keys (`"Name"`, `"Color"`,
`"IconScale"`, `"Id"`): `"Locked"`.

`BuildPropGroupsJson` (`MapExporter_Props_IO.cpp:60-72`) — add after the existing
`layerJson["Id"] = layer.layerId;` line (line 68), before `propGroups.push_back(layerJson);`:
```cpp
layerJson["Locked"] = layer.bLocked;
```
`BuildDecalGroupsJson` (`MapExporter_Decals_IO.cpp:56-68`) — identical addition after its own
`layerJson["Id"] = layer.layerId;` line (line 64), before `decalGroups.push_back(layerJson);`:
```cpp
layerJson["Locked"] = layer.bLocked;
```

`ReadPropGroupsJson` (`MapImporter_Props_IO.cpp:99-119`) — add after the existing
`ReadJsonInteger(layerJson, "Id", layer.layerId);` line (line 115), inside the same
`if (layerJson.is_object())` block:
```cpp
ReadJsonBoolean(layerJson, "Locked", layer.bLocked);
```
`ReadDecalGroupsJson` (`MapImporter_Decals_IO.cpp:91-111`) — identical addition after its own
`ReadJsonInteger(layerJson, "Id", layer.layerId);` line (line 107), inside the same
`if (layerJson.is_object())` block:
```cpp
ReadJsonBoolean(layerJson, "Locked", layer.bLocked);
```
Absent key (legacy `.sanmap` files saved before this ticket) keeps the struct's own default
(`false`) — no clamp, no range validation needed (Constitution §6's validate-then-default rule is
satisfied by the field default itself).

### 3. Enforcement gate — `IsPropInstanceLayerLocked` / `IsDecalInstanceLayerLocked`
One pure, out-of-range-safe function per struct (two functions, not one shared — the structs are not
shared, per the Problem section), mirroring `IsMarkerInstanceLayerLocked` (STEP106 §3) and this same
file's own `ResolvePropInstanceLayerId`/`ResolveDecalInstanceLayerId` out-of-range shape
(`PropInstance_PARAMS.h:37-44`).

`PropsTab_Manual_UI.h`, beside `SelectedManualPropLayer` (`PropsTab_Manual_UI.h:70-74`):
```cpp
// True when `layerIndex` names a layer with bLocked set. Out-of-range (Constitution §6) resolves
// to false — an invalid layerIndex must never itself become a reason to refuse an edit; same
// out-of-range-safe posture as SelectedManualPropLayer immediately above and
// IsMarkerInstanceLayerLocked (STEP106).
inline bool IsPropInstanceLayerLocked(const std::vector<Params::PropInstanceLayer>& propLayers,
                                      int layerIndex) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(propLayers.size())) return false;
    return propLayers[static_cast<std::size_t>(layerIndex)].bLocked;
}
```
`PropsTab_ManualDecals_UI.h`, beside `SelectedManualDecalLayer` (`PropsTab_ManualDecals_UI.h:62-66`):
```cpp
// Decal-typed mirror of IsPropInstanceLayerLocked.
inline bool IsDecalInstanceLayerLocked(const std::vector<Params::DecalInstanceLayer>& decalLayers,
                                       int layerIndex) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(decalLayers.size())) return false;
    return decalLayers[static_cast<std::size_t>(layerIndex)].bLocked;
}
```
No `#include` changes needed for either — both headers already include
`"../params/PropInstance_PARAMS.h"` (`PropsTab_Manual_UI.h:38`, `PropsTab_ManualDecals_UI.h:31`) and
already use `std::vector`/`<algorithm>` (both files' existing `NextPropLayerId`/`NextDecalLayerId`).

### 4. Call sites — lock enforcement
**None exist to gate today** — see the Problem section's "Conclusion." No drag gesture, no Position
sliders, no Add/Remove Instance buttons exist anywhere in the Props/Decals domain to wrap in
`IsPropInstanceLayerLocked`/`IsDecalInstanceLayerLocked` or `ImGui::BeginDisabled`/`EndDisabled`. The
only per-instance-transform UI, `DrawTransformList` in both `.cpp` files, is a read-only preview of
the procedural placement buffer and is not gated by anything (its own header's SCOPE NOTE 2 — not
touched by this ticket). Do not add a check here; the gate functions from §3 exist for the
Assembly-design consumer (a canvas multi-select gate this ticket does not itself build) and for §5's
`[L]`/`[U]` affordance below, not for a call site inside this ticket's own files.

### 5. Enforcement gate wiring — `DrawLayerList`/`ApplyLayerListSignal`, both files
Both `PropsTab_Manual_UI.cpp`'s and `PropsTab_ManualDecals_UI.cpp`'s `DrawLayerList` and
`ApplyLayerListSignal` already receive `ToggleLock` from the shared `DraggableList` widget (same
`[L]`/`[U]` per-row button, `DraggableListWidget_UI.h:113-114`) but do nothing with it — confirmed
dead in both files by reading them in full: `DrawLayerList`'s row-describe lambda only sets
`row.label` (never `row.bLocked`, so the widget always draws `[U]`), and `ApplyLayerListSignal` has
no `ToggleLock` branch. Wire it, in both files, mirroring STEP106 §5 exactly:

**`PropsTab_Manual_UI.cpp`** — `DrawLayerList` (lines 50-64), row-describe lambda (lines 54-58): add
`row.bLocked`, one new line:
```cpp
DraggableListSignal DrawLayerList(std::vector<Params::PropInstanceLayer>& propLayers,
                                  ManualPropLayersState& state, bool& bAnyNameCommitted) {
    return DraggableList<Params::PropInstanceLayer>::Render(
        "manualPropLayers", propLayers,
        [&](int rowIndex) {
            DraggableListRow row;
            row.label   = ManualPropLayerRowLabel(propLayers[static_cast<std::size_t>(rowIndex)]);
            row.bLocked = propLayers[static_cast<std::size_t>(rowIndex)].bLocked;   // NEW
            return row;
        },
        [&](int rowIndex) {
            Params::PropInstanceLayer& layer = propLayers[static_cast<std::size_t>(rowIndex)];
            if (DrawLayerRowSettings(layer, state)) bAnyNameCommitted = true;
        },
        state.selectedLayerIndex);
}
```
`ApplyLayerListSignal` (lines 72-91) — add a `ToggleLock` branch immediately after the existing
`kind == DraggableListSignalKind::Select` branch (lines 75-78), before the
`const bool bDeleting = ...` line (line 79):
```cpp
if (signal.kind == DraggableListSignalKind::ToggleLock) {
    if (signal.sourceRowIndex >= 0 && signal.sourceRowIndex < static_cast<int>(propLayers.size()))
        propLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bLocked =
            !propLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bLocked;
    return false;   // cosmetic-only: no structural move, same posture as Select
}
```
Insert this branch so it returns early like `Select` does, not falling through into the Delete/
Reorder handling below it — deleting or reordering a locked prop layer stays ALLOWED, unchanged
(Problem section: lock protects layer MEMBERS, not the layer-list container operations, same posture
STEP106 established for markers).

**`PropsTab_ManualDecals_UI.cpp`** — identical edit, decal-typed. `DrawLayerList` (lines 51-65), row-
describe lambda (lines 55-59):
```cpp
DraggableListSignal DrawLayerList(std::vector<Params::DecalInstanceLayer>& decalLayers,
                                  ManualDecalLayersState& state, bool& bAnyNameCommitted) {
    return DraggableList<Params::DecalInstanceLayer>::Render(
        "manualDecalLayers", decalLayers,
        [&](int rowIndex) {
            DraggableListRow row;
            row.label   = ManualDecalLayerRowLabel(decalLayers[static_cast<std::size_t>(rowIndex)]);
            row.bLocked = decalLayers[static_cast<std::size_t>(rowIndex)].bLocked;   // NEW
            return row;
        },
        [&](int rowIndex) {
            Params::DecalInstanceLayer& layer = decalLayers[static_cast<std::size_t>(rowIndex)];
            if (DrawLayerRowSettings(layer, state)) bAnyNameCommitted = true;
        },
        state.selectedLayerIndex);
}
```
`ApplyLayerListSignal` (lines 73-92) — add after its own `Select` branch (lines 76-79), before
`const bool bDeleting = ...` (line 80):
```cpp
if (signal.kind == DraggableListSignalKind::ToggleLock) {
    if (signal.sourceRowIndex >= 0 && signal.sourceRowIndex < static_cast<int>(decalLayers.size()))
        decalLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bLocked =
            !decalLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bLocked;
    return false;
}
```

### 6. UI convention — no `BeginDisabled`/`EndDisabled` needed in this ticket
STEP106's "wrap disabled controls in `ImGui::BeginDisabled()/EndDisabled()` when locked" applies to
the position-slider/Add/Remove call sites §4 found none of. `DrawLayerRowSettings`
(`PropsTab_Manual_UI.cpp:33-44`, `PropsTab_ManualDecals_UI.cpp:34-45` — the Name/Color/Icon-Scale
controls for the layer's OWN metadata) is **not** gated by its own lock, matching STEP106's identical
choice not to gate `DrawLayerRowBody`'s Name/Color/Icon-Scale fields on `MarkerInstanceLayer::bLocked`
either — a layer locking its members does not lock the layer's own cosmetic settings in either
domain. No `BeginDisabled`/`EndDisabled` call is added anywhere by this ticket.

## Out of scope
- **Any prop/decal per-instance editing UI** (canvas drag gesture, Position sliders, Add/Remove
  Instance buttons). None exists today (Problem section); this ticket does not invent one to give the
  lock something to gate. When/if such a feature is built later, it must consult
  `IsPropInstanceLayerLocked`/`IsDecalInstanceLayerLocked` at that time, the same way STEP106's marker
  call sites do today — but building that feature is not this ticket.
- **The Assembly-design's own cross-layer multi-select gating.** This ticket supplies the PARAMS
  field, the wire round-trip, and the pure gate functions the Assembly design's canvas selection logic
  will consult — it does not implement that selection logic itself. That is
  `work_orders/DESIGN_Assembly_R1.md`'s own future work-order.
- **Gating the manual layer LIST's own Add/Reorder/Delete operations on that layer's `bLocked`.**
  Unchanged, matching STEP106's identical choice for markers (§5 above).
- **Gating `DrawLayerRowSettings`'s Name/Color/Icon-Scale fields on the row's own lock.** Unchanged,
  matching STEP106's identical choice for `DrawLayerRowBody` (§6 above).
- **`MarkersTab_RuleLayers_UI.cpp`'s padlock-icon-means-"hidden" convention**, and any other tab's
  unrelated lock/hide iconography. Untouched, out of scope, same as STEP106.
- **Grid snap.** STEP106 §2 bundled grid snap into the marker ticket because it shared marker drag/
  reposition call sites. Props/decals have no drag/reposition call sites at all (Problem section), so
  there is nothing for a per-layer grid snap to bundle onto here — not built, not implied.
- **`PropsTab_Rules_UI.cpp`'s procedural `PropRule`/`DecalRule` stacks** (`DrawRuleList` in
  `PropsTab_UI.cpp`, `DrawDecalRuleStack` in `PropsTab_Decals_UI.cpp`) and their own `DraggableList`
  `ToggleVisibility` wiring. A completely separate data type (`Params::PropRule`/`Params::DecalRule`,
  procedural generation rules, not manual layers) with its own existing `bEnabled`/visibility handling
  already wired (`PropsTab_UI.cpp:73-77`) — untouched by this ticket.

## Files touched
- `src/params/PropInstance_PARAMS.h` — `bLocked` on `PropInstanceLayer` and on `DecalInstanceLayer`
- `src/io/MapExporter_Props_IO.cpp` — `BuildPropGroupsJson` writes `"Locked"`
- `src/io/MapImporter_Props_IO.cpp` — `ReadPropGroupsJson` reads `"Locked"`
- `src/io/MapExporter_Decals_IO.cpp` — `BuildDecalGroupsJson` writes `"Locked"`
- `src/io/MapImporter_Decals_IO.cpp` — `ReadDecalGroupsJson` reads `"Locked"`
- `src/ui/PropsTab_Manual_UI.h` — new `IsPropInstanceLayerLocked`
- `src/ui/PropsTab_Manual_UI.cpp` — `DrawLayerList` sets `row.bLocked`; `ApplyLayerListSignal`
  handles `ToggleLock`
- `src/ui/PropsTab_ManualDecals_UI.h` — new `IsDecalInstanceLayerLocked`
- `src/ui/PropsTab_ManualDecals_UI.cpp` — `DrawLayerList` sets `row.bLocked`; `ApplyLayerListSignal`
  handles `ToggleLock`

## Verify
Acceptance bar: both fields exist, round-trip through export/import (including legacy files with
neither key present), the `[L]`/`[U]` icon reflects real state and the button actually flips it, with
new/updated unit tests. No enforcement-refusal acceptance bar for either domain — §4 confirmed there
is no call site to refuse against.

- **New unit test — `PropGroups`/`DecalGroups` round-trip**, extend the existing live-document
  fixture in `src/io/MapImporter_PropsDecals_IO_Test.cpp`'s `BuildFixtureRecipe` (propLayer fixture:
  lines 39-45; decalLayer fixture: lines 68-75, both continuing past line 75) and its check block
  further down the same file (prop layer checks: lines 125-140; decal layer checks: lines 180-195):
  give `propLayer.bLocked = true` and `decalLayer.bLocked = true` in the fixture; add
  `Check(loadedLayer.bLocked == originalLayer.bLocked, "PropInstanceLayer::bLocked survives")` /
  the decal-typed equivalent to each check block.
- **New unit test — legacy default**: hand-construct a `PropGroups`/`DecalGroups` JSON array entry
  with no `"Locked"` key present, call `ReadPropGroupsJson`/`ReadDecalGroupsJson` directly, assert
  `bLocked == false` (struct default, untouched).
- **New unit test — `IsPropInstanceLayerLocked`/`IsDecalInstanceLayerLocked`**: empty vector and any
  index returns `false`; a vector of two layers with `[1].bLocked = true` returns `false` for index 0,
  `true` for index 1; an out-of-range index (negative and `>= size()`) against a non-empty vector
  returns `false`. Mirrors STEP106's `IsMarkerInstanceLayerLocked` test shape.
- **New/updated UI-logic test — `ToggleLock` flips `bLocked`**: in whatever existing headless test
  seam covers `ApplyLayerListSignal` today (check `PropsTab_UI_Test.cpp` and
  `PropsTab_ManualDecals_UI_Test.cpp` for one before assuming none exists — both were found by this
  ticket's own file search and were not read in full; the Coder must confirm which, if either, already
  exercises `ApplyLayerListSignal` and extend it, or add a minimal new case only if truly no seam
  exists), assert a `ToggleLock` signal against a valid `sourceRowIndex` flips that layer's `bLocked`
  and returns `false` (no structural move); an out-of-range `sourceRowIndex` leaves every layer's
  `bLocked` unchanged.
- **Existing suites stay green with no behavior change to any assertion this ticket does not itself
  add**: `MapExporter_IO_Test`, `MapImporter_IO_Test`, `MapImporter_PropsDecals_IO_Test` (all non-lock
  fixtures/checks byte-identical), `PropsTab_UI_Test`, `PropsTab_ManualDecals_UI_Test`.
