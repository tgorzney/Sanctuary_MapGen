# STEP112 — Manually-placed Spawn markers use real per-army color

**Layer:** UI (render-tint resolution + parameter threading). **Domain:**
`MapCanvas_MarkerDrag_UI.h/.cpp`, `MapCanvas_UI.h`, `MapCanvas_MarkerDrag_UI_Test.cpp`.
**Sequence:** no dependency on other undone work-orders. Independent of STEP111 (icon-overlay RGB
— a different render path, `OverlayVisibleInstance`, different files), STEP113 (tab-gating), and
STEP114 (icon override) — no shared call sites with any of them; safe to implement in any order or
concurrently with them.

## Problem
Manually-placed markers render as plain dots via `DrawManualMarkerRoster`/`ManualMarkerTint` in
`src/ui/MapCanvas_MarkerDrag_UI.cpp:24-29,67-99` — a screen-space dot-renderer, structurally
separate from the icon-atlas overlay pipeline (`OverlayVisibleInstance`, STEP111's concern, not
this one; ARCH §14's "never baked/never conflated" split applies to layer identity, not to this
ticket's tint-source question).

`ManualMarkerTint` (lines 24-29) already correctly reads `Params::MarkerInstanceLayer::color`
(`src/params/MarkerInstance_PARAMS.h:23-42`, `color[4]` at line 25) for every layer, including
whatever layer a Spawn marker happens to sit on — confirmed by reading its full body: it indexes
`markerLayers[layerIndex].color` unconditionally, with no branch for the Spawn group at all.

This is wrong for the Spawn group specifically. `Params::MarkerInstanceGroup::name ==
Params::kSpawnMarkerGroupName` ("Spawn", `MarkerInstance_PARAMS.h:55-61,66`) is the reserved
commander-spawn roster; each of its `MarkerTransform` entries represents one player slot and
should render in that slot's real army color, `Params::Army::armyColor[4]`
(`src/params/Army_PARAMS.h:65`), not whatever arbitrary layer color the marker's `layerIndex`
happens to resolve to. Layer color is an authoring-convenience grouping tag; army color is the
actual per-player-slot identity the game itself uses.

The ratified lookup key already exists in this codebase: `Params::Army::name ==
Params::MarkerTransform::name`, byte-for-byte, **not** `MarkerTransform::alias`.
`ARCH_16_08_SpawnArmyShrink.md` §16.8 (lines 11-19) states this explicitly — "the relationship is
inferred purely by matching `Army::name` against a `MarkerTransform`'s **`name`** ... at
export/runtime" — and explicitly corrects an earlier draft that said "alias/name" as "wrong and
licenses a false negative." `Army::name` is the engine-identity field (`ARMY_XX`, machine-minted,
`Army_PARAMS.h:44-52`), and `MarkerTransform::name` is the folded-in `transforms[key]` dictionary
key that `MapExporter_Markers_IO.cpp` writes it under — for the Spawn group specifically this key
is the army's own name, per the same ARCH ruling. This precedent is already load-bearing
elsewhere in the codebase, not a novel invention: `src/io/MapImporter_ArmyIdentityNormalize_IO_Test.cpp:64-65`
already asserts `army.name == transform.name` for every Spawn transform after import
normalization (`for (const Params::Army& army : recipe.armies) if (army.name == transform.name)
bMatchesAnArmy = true;`), and the sibling render path `MapCanvas_ScenarioEditMode_DrawMarkers_UI.cpp:27-32`
(`ArmyTint`) already tints scenario-edit-mode Spawn candidates by real army color — via a
pre-resolved `armyIndex`, not a name match, but confirming the "Spawn markers get real army color,
not layer color" product rule is already established practice in a sibling render path this
ticket's manual-marker dot-renderer alone is missing.

The v1 reference (`gui/widgets/Widget_MapCanvas.cpp:346-363`) did this by parsing an army id out of
the marker's custom name (`marker.CustomName.find("Spawn_") == 0` then
`marker.CustomName.substr(6)`, `Widget_MapCanvas.cpp:350-355`) and looking it up in a
`std::map<std::string, Army>` by that parsed key. This ticket does **not** port that approach — no
`MapCanvas_MarkerDrag_UI.cpp` code today parses `MarkerTransform::name`/`alias` for a prefix, and
none should be added. Use the ratified `Army::name == MarkerTransform::name` byte-for-byte match
instead.

`DrawManualMarkerRoster` today has no access to `armies` at all — its signature
(`MapCanvas_MarkerDrag_UI.h:41-45`) takes `markers`/`markerLayers`/`dragState`/`composite`/`view`/
`regionOriginX,Y`/`drawList`, nothing else. Its one production caller,
`MapCanvas::DrawManualMarkerDragPass` (`MapCanvas_MarkerDrag_UI.cpp:135-141`), reads through
`manualMarkerDragMarkers`/`manualMarkerDragLayers` — but the class already separately holds
`manualMarkerDragRecipe` (`const Params::MapRecipe*`, `MapCanvas_UI.h:180`, set alongside the other
three pointers by `SetManualMarkerDragSource`, `MapCanvas_UI.h:97-105`), and
`Params::MapRecipe::armies` (`std::vector<Army>`, `src/params/MapRecipe_PARAMS.h:107`) is already
the live recipe's real army roster — confirmed the one call site,
`src/ui/Application_UI.cpp:97`, passes `&recipe` itself (`canvas.SetManualMarkerDragSource(&recipe.markers,
&recipe.markerLayers, &recipe.geometry, &recipe);`), not a detached copy. No new `MapCanvas` member
or new `Set...` call is needed — `manualMarkerDragRecipe->armies` is already reachable where the
fix belongs; it just isn't threaded through today.

## Fix

### 1. `DrawManualMarkerRoster` gains an `armies` parameter
`src/ui/MapCanvas_MarkerDrag_UI.h:41-45`, insert `armies` after `markerLayers`:
```cpp
void DrawManualMarkerRoster(const std::vector<Params::MarkerInstanceGroup>& markers,
                            const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            const std::vector<Params::Army>& armies,
                            const MarkerDragGestureState& dragState, const PreviewComposite& composite,
                            const MapCanvasView& view, float regionOriginX, float regionOriginY,
                            ImDrawList& drawList);
```
Add `#include "../params/Army_PARAMS.h"` to this header (current includes,
`MapCanvas_MarkerDrag_UI.h:9-13`: `<vector>`, `MarkerDragGesture_UI.h`, `MapCanvasView_UI.h`,
`../params/MarkerInstance_PARAMS.h`) — the signature now names `Params::Army` directly, so it must
not rely on a transitive include. No include change needed in the `.cpp`: `Params::Army` is already
reachable there via `#include "../params/MapRecipe_PARAMS.h"` (`MapCanvas_MarkerDrag_UI.cpp:8`),
which itself includes `Army_PARAMS.h` (`MapRecipe_PARAMS.h:10`).

### 2. New pure tint-resolution helper, `MapCanvas_MarkerDrag_UI.cpp`'s anonymous namespace
Beside `ManualMarkerTint` (current lines 24-29), add:
```cpp
// Resolves a Spawn-group transform's render tint to its matching army's real color — the ratified
// match rule, ARCH_16_08_SpawnArmyShrink.md §16.8: Army::name == MarkerTransform::name,
// byte-for-byte, NEVER MarkerTransform::alias. An orphaned Spawn slot (no army carries this name —
// already "a legal, unremarkable state," ARCH_16_08) falls back to `fallback`, the caller's own
// already-resolved layer-color tint — never a crash, never a hardcoded literal color.
ImU32 ManualSpawnArmyTint(const std::vector<Params::Army>& armies, const std::string& transformName,
                          ImU32 fallback) {
    for (const Params::Army& army : armies)
        if (army.name == transformName)
            return ImGui::ColorConvertFloat4ToU32(ImVec4(army.armyColor[0], army.armyColor[1],
                                                          army.armyColor[2], army.armyColor[3]));
    return fallback;
}
```
`ManualMarkerTint` itself is untouched — it stays the layer-color resolver for every non-Spawn
marker (and remains the Spawn group's own fallback source, per the comment above).

### 3. Call site — `DrawManualMarkerRoster`'s per-transform tint resolution
`MapCanvas_MarkerDrag_UI.cpp:76-89` (the per-group/per-transform loop). Add the `armies` parameter
to the function signature (matching §1) and replace the existing ternary (current lines 86-87):
```cpp
const ImU32 tint = (bThisGroupDragging && dragState.bSpawnCardinalityRefused)
                  ? refusedTint : ManualMarkerTint(markerLayers, transform.layerIndex);
```
with:
```cpp
ImU32 tint;
if (bThisGroupDragging && dragState.bSpawnCardinalityRefused) {
    tint = refusedTint;
} else if (IsSpawnMarkerGroup(group)) {
    tint = ManualSpawnArmyTint(armies, transform.name, ManualMarkerTint(markerLayers, transform.layerIndex));
} else {
    tint = ManualMarkerTint(markerLayers, transform.layerIndex);
}
```
`group` is already in scope (`const Params::MarkerInstanceGroup& group = markers[groupIndex];`,
current line 77). The Spawn-cardinality-refused red tint (existing behavior) keeps priority over
army color — a refused drag on the Spawn group still shows red, not army color; this is the same
branch order the existing code already used for the refused/non-refused split, just extended with
one more case rather than reordered.

`IsSpawnMarkerGroup(const Params::MarkerInstanceGroup&)` (`src/ui/MarkersTab_Manual_UI.h:106-108`,
`return group.name == kSpawnMarkerGroupName;`) is already reachable in this `.cpp` transitively:
`MapCanvas_MarkerDrag_UI.h:11` includes `MarkerDragGesture_UI.h`, which
(`MarkerDragGesture_UI.h:22`) includes `MarkersTab_Manual_UI.h`. No new include required, but this
is the one pure helper this ticket depends on for Spawn-group detection — do not re-derive
`group.name == "Spawn"` inline; use this existing function (same posture the file's own header
comment describes for `kSpawnMarkerGroupName`/`SelectedMarkerGroup`/`SelectedMarkerInstance`
reuse).

### 4. Threading — `MapCanvas::DrawManualMarkerDragPass`
`MapCanvas_MarkerDrag_UI.cpp:135-141`:
```cpp
void MapCanvas::DrawManualMarkerDragPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualMarkerDragMarkers == nullptr) return;
    static const std::vector<Params::MarkerInstanceLayer> kNoLayers;
    DrawManualMarkerRoster(*manualMarkerDragMarkers, manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers,
                          manualMarkerDragState, *composite, view, regionOriginX, regionOriginY,
                          *ImGui::GetWindowDrawList());
}
```
becomes (mirroring the existing `kNoLayers` null-safe fallback pattern this same function already
uses, extended for `armies` — `manualMarkerDragRecipe` may legitimately be null before
`SetManualMarkerDragSource` is first called, same as the other three pointers):
```cpp
void MapCanvas::DrawManualMarkerDragPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualMarkerDragMarkers == nullptr) return;
    static const std::vector<Params::MarkerInstanceLayer> kNoLayers;
    static const std::vector<Params::Army> kNoArmies;
    DrawManualMarkerRoster(*manualMarkerDragMarkers, manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->armies : kNoArmies,
                          manualMarkerDragState, *composite, view, regionOriginX, regionOriginY,
                          *ImGui::GetWindowDrawList());
}
```
No new `MapCanvas` class member is needed — `manualMarkerDragRecipe`
(`const Params::MapRecipe*`, `MapCanvas_UI.h:180`) already carries the live recipe's own `armies`
(`MapRecipe_PARAMS.h:107`), set by the existing `SetManualMarkerDragSource` call at
`Application_UI.cpp:97`, which is untouched by this ticket.

## Out of scope
- **The icon-overlay atlas pipeline** (`OverlayVisibleInstance` and everything STEP111 touches) —
  a structurally separate render path from this dot-renderer; not this ticket's concern.
- **Tab-gating (STEP113) and icon override (STEP114)** — separate, unrelated call sites.
- **`MapCanvas_ScenarioEditMode_DrawMarkers_UI.cpp`'s `ArmyTint`/`ResolveCandidateTint`**
  (lines 27-32, 90-100) — already resolves Spawn candidates to real army color correctly (via a
  pre-resolved `armyIndex`, a different data path); untouched, cited here only as existing
  precedent that this ticket's product rule is already established elsewhere.
- **Any change to `Params::MarkerInstanceLayer::color`'s meaning for non-Spawn markers.** Layer
  color remains exactly what it always was for every group except Spawn.
- **Re-deriving the army-name match rule with any parsing/prefix logic.** The v1
  `"Spawn_" + armyId` substring approach (`Widget_MapCanvas.cpp:350-355`) is explicitly not
  ported; the ratified match is a direct, whole-string `Army::name == MarkerTransform::name`
  comparison only.
- **`MarkerTransform::alias`.** Never read for this match, per `ARCH_16_08_SpawnArmyShrink.md`'s
  own explicit correction of an earlier draft that said "alias/name."
- **Changing `SetManualMarkerDragSource`'s signature or adding a new `MapCanvas` member.** The
  existing `manualMarkerDragRecipe` pointer already carries everything this ticket needs.

## Files touched
- `src/ui/MapCanvas_MarkerDrag_UI.h` — `DrawManualMarkerRoster` gains an `armies` parameter;
  `#include "../params/Army_PARAMS.h"`
- `src/ui/MapCanvas_MarkerDrag_UI.cpp` — new `ManualSpawnArmyTint` helper; `DrawManualMarkerRoster`'s
  per-transform tint resolution branches on `IsSpawnMarkerGroup(group)`;
  `MapCanvas::DrawManualMarkerDragPass` threads `manualMarkerDragRecipe->armies` (with a
  `kNoArmies` null-safe fallback) through
- `src/ui/MapCanvas_MarkerDrag_UI_Test.cpp` — all five existing `DrawManualMarkerRoster` call sites
  (lines 126, 138, 143, 159, 164) gain an `armies` argument; new checks (see Verify)

No change to `MapCanvas_UI.h`, `Application_UI.cpp`, `MarkerInstance_PARAMS.h`, or `Army_PARAMS.h`
— every field/pointer this ticket needs already exists.

## Verify
Acceptance bar: a Spawn-group transform whose name matches an army renders in that army's real
`armyColor`, not its layer's color; an orphaned Spawn slot (no matching army) falls back to the
existing layer-color behavior, not a crash or hardcoded color; a non-Spawn group whose transform
name happens to collide with an army name is unaffected (gated on group identity, not name
collision); the Spawn-cardinality-refused red tint still wins over army color. Verified via the
same live-headless-imgui-frame + `LastVertexColor(drawList)` vertex-color-inspection technique this
file's own `RunDrawRefusedTintChecks` (`MapCanvas_MarkerDrag_UI_Test.cpp:150-168`) already
establishes as this render function's working headless test seam — no new harness needed.

- **New unit test — Spawn transform matching an army renders that army's color**: build
  `markers = [{ name = Params::kSpawnMarkerGroupName, transforms = [MakeTransform("ARMY_01", 1.0f,
  1.0f)] }]`, `armies = [{ name = "ARMY_01", armyColor = {0.0f, 1.0f, 0.0f, 1.0f} }]`,
  `markerLayers = {}` (empty, so the layer-color fallback the army-color branch would otherwise use
  is the file's own neutral default, `IM_COL32(220, 220, 220, 255)`, `MapCanvas_MarkerDrag_UI.cpp:26`).
  Call `DrawManualMarkerRoster(markers, {}, armies, inactiveDragState, *fixture.composite,
  fixture.view, 0.0f, 0.0f, drawList)`; assert `LastVertexColor(drawList) ==
  ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 0.0f, 1.0f))`.
- **New unit test — orphaned Spawn slot (no matching army) falls back to layer color**: same Spawn
  group but the transform's name is `"ARMY_99"` (does not match any entry in `armies = [{ name =
  "ARMY_01", ... }]`), `markerLayers = {}`; assert `LastVertexColor(drawList) == IM_COL32(220, 220,
  220, 255)` — the exact same fallback `ManualMarkerTint` already returns for an out-of-range
  layerIndex (`MapCanvas_MarkerDrag_UI.cpp:26`), confirming the army-color branch degrades cleanly,
  never to a crash or an invented color.
- **New unit test — non-Spawn group with a name-colliding transform is unaffected**: `markers = [{
  name = "Alloys", transforms = [MakeTransform("ARMY_01", 1.0f, 1.0f)] }]` (group name deliberately
  NOT `kSpawnMarkerGroupName`), same `armies = [{ name = "ARMY_01", armyColor = {0,1,0,1} }]`,
  `markerLayers = {}`; assert `LastVertexColor(drawList) == IM_COL32(220, 220, 220, 255)`, NOT the
  green army color — proves the branch gates on `IsSpawnMarkerGroup(group)`, not on any transform
  name happening to match an army name.
- **New unit test — Spawn-cardinality-refused red tint still wins over army color**: Spawn group as
  in the first test, `armies = [{ name = "ARMY_01", armyColor = {0,1,0,1} }]`; drag state
  `bActive = true`, `groupIndex = 0`, `bSpawnCardinalityRefused = true`; assert
  `LastVertexColor(drawList) == IM_COL32(220, 60, 40, 255)` (the file's existing `refusedTint`
  constant, `MapCanvas_MarkerDrag_UI.cpp:73`), not the army green — guards the branch-priority
  ordering in §3 against a future accidental reorder.
- **Existing suite stays green with no behavior change to any assertion this ticket does not itself
  add**: `RunHitTestChecks`, `RunDrawAtRestAndSoftHideChecks`, `RunDrawRefusedTintChecks` — all five
  existing `DrawManualMarkerRoster` call sites (lines 126, 138, 143, 159, 164) pass an empty
  `std::vector<Params::Army>` (no army-color behavior exercised by those fixtures, since none use
  `kSpawnMarkerGroupName`), so their existing assertions are byte-identical.
