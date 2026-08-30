# STEP227 — Area Z-order inversion + continuous size-sort (ARCH §14.19)

## Summary
The human: "When importing Areas, they need to be ordered by size, with the smallest area on top
of list, and for preview rendering they need to be stacked in order of list, with the smallest
area (top of UI list) rendered on top of all other areas."

Ratified by the SanGen ARCH Expert as `ARCH_14_19_AreaZOrderInversionAndImportSizeSort.md`
(amending §14.17 item 6 and §21.8's hit-test/creation rulings) — **required reading, in full,
before touching any file**. This ticket transcribes that ruling verbatim; it is not a re-derivation.

**The core fact this ticket exists to fix:** today, `recipe.areas[0]` is drawn at the visual TOP of
the Area Stack UI list, but renders on the BOTTOM of the composited stack (both the CPU and GPU
compositors iterate forward and let the LAST containing match win) — confirmed by direct code
reading this session. The human wants the opposite: index 0 (top of list) should render on TOP.
The ARCH ruling generalizes this from "sort at import" to a standing invariant: `recipe.areas` is
kept continuously sorted ascending by size (`width * length`) through every insertion path, not
just the two import paths — because a freshly created small area should also default to rendering
on top of whatever it was carved out of, not just an imported one.

**This ticket ships as ONE unit — do not implement a subset.** Per the ARCH ruling's own closing
paragraph: shipping the Z-loop flip without the insertion-order change leaves every already-
imported map's areas in their old, unsorted order; shipping the insertion-order change without the
Z-loop flip produces a list whose top row is NOT what renders on top — the exact bug this ticket
exists to fix.

## Required reading
- `ARCH_14_19_AreaZOrderInversionAndImportSizeSort.md` (full file — the authoritative spec every
  code block below is transcribed from)
- `ARCH_14_17_MapAreaFieldLayer.md` (item 6, now marked amended/inverted)
- `ARCH_21_08_AreaCanvasGesture.md` (the new dated amendment section on the hit-test flip)
- `src/params/MapArea_PARAMS.h` (full file, 22 lines — where the new shared function is added)
- `src/ui/PreviewComposite_Cpu_UI.cpp:101-113` (`LayerColorAtPixel`'s `MapAreas` branch)
- `src/ui/PreviewComposite_Sampling_UI.glsl:138-147` (`mapAreaColorAtCell`)
- `src/ui/MapCanvas_AreaDragDispatch_UI.cpp` (full file — `TryBeginAreaDrag` step 2, and
  `CreateAreaFromDrag`, lines 142-172)
- `src/io/MapImporter_Areas_IO.cpp` (full file, `ReadAreasJson`)
- `src/io/ScenarioScript_AreaImport_IO.cpp:104-118` (the additive reconciliation loop)
- `src/ui/AreasTab_UI.cpp` (`DrawAreasGlobals`, "Add New Area")
- `src/ui/AreasTab_List_UI.h` (`EnsurePlayableArea`)

## 1. `src/params/MapArea_PARAMS.h` — the one shared insertion function

Add, after the `MapArea` struct:
```cpp
// ARCH §14.19 — plain rectangle area, never a bounding diagonal or max(width, length).
inline float MapAreaSize(const MapArea& area) { return area.width * area.length; }

// The ONE way any layer adds an area. Keeps `areas` continuously sorted ascending by size.
// Ties are stable: inserts before the first existing entry STRICTLY LARGER than `area`, so
// equal-size entries keep first-come-first-served order. Returns the index the area landed at.
inline std::size_t InsertMapAreaSortedBySize(std::vector<MapArea>& areas, MapArea area) {
    std::size_t insertAt = areas.size();
    for (std::size_t index = 0; index < areas.size(); ++index) {
        if (MapAreaSize(areas[index]) > MapAreaSize(area)) { insertAt = index; break; }
    }
    areas.insert(areas.begin() + static_cast<std::ptrdiff_t>(insertAt), std::move(area));
    return insertAt;
}
```
Add `#include <vector>` and `#include <cstddef>` to this header's include block (currently only
`<string>`).

## 2. `src/ui/PreviewComposite_Cpu_UI.cpp` — CPU compositor Z-loop flip

Replace the current `MapAreas` branch of `LayerColorAtPixel` (forward-iterate-all, last match
wins, unconditional overwrite) with:
```cpp
if (layerKind == PreviewLayerKind::MapAreas) {
    for (const PreviewMapAreaRectangle& rectangle : mapAreaRectangles) {
        if (sampleX < rectangle.minimumX || sampleX > rectangle.maximumX) continue;
        if (sampleY < rectangle.minimumZ || sampleY > rectangle.maximumZ) continue;
        PreviewColor result;
        result.red = rectangle.colorRed; result.green = rectangle.colorGreen;
        result.blue = rectangle.colorBlue; result.alpha = rectangle.colorAlpha;
        return result;
    }
    return PreviewColor();
}
```
ARCH §14.19 item 2 — early `return` on the first containing rectangle, since ascending array index
is now Z-descending (index 0 = top).

## 3. `src/ui/PreviewComposite_Sampling_UI.glsl` — GPU twin, textually parallel

Replace `mapAreaColorAtCell` with:
```glsl
vec4 mapAreaColorAtCell(float sampleX, float sampleY) {
    for (int index = 0; index < mapAreaRectangles.length(); ++index) {
        MapAreaRectangle area = mapAreaRectangles[index];
        if (sampleX < area.minimumX || sampleX > area.maximumX) continue;
        if (sampleY < area.minimumZ || sampleY > area.maximumZ) continue;
        return vec4(area.colorRed, area.colorGreen, area.colorBlue, area.colorAlpha);
    }
    return vec4(0.0);
}
```
**CPU/GPU parity is mandatory, not a suggestion**: both loops must stay textually parallel (same
early-exit shape) — do not have one twin use `break` and the other `return` in a way that changes
short-circuit behavior between them. `BuildMapAreaConfigurations` (unchanged by this ticket) already
emits `mapAreaRectangles` in `recipe.areas` order 1:1, so identical loop shape on both twins
guarantees byte-identical output.

## 4. `src/ui/MapCanvas_AreaDragDispatch_UI.cpp` — hit-test Z-loop flip

In `TryBeginAreaDrag`, step 2 (the "body hit-test over EVERY UNLOCKED area" block), replace the
current "forward iteration, last match wins" loop with:
```cpp
int hitIndex = -1;
for (int index = 0; index < static_cast<int>(areas.size()); ++index) {
    if (!IsAreaLocked(index)
        && IsWorldPointInsideArea(areas[static_cast<std::size_t>(index)], worldPoint.worldX, worldPoint.worldZ)) {
        hitIndex = index;
        break;   // ascending index is Z-descending: the first unlocked hit IS the topmost area.
    }
}
if (hitIndex < 0) return false;
```
Update the comment directly above this block (currently citing "later-in-vector is drawn topmost")
to cite ARCH §14.19 and state the new rule instead.

**`CreateAreaFromDrag`** (same file, lines 142-172) switches from `push_back` + `size()-1` to the
new insertion function:
```cpp
Params::MapArea area;
area.originX = std::min(pressWorld.worldX, releaseWorld.worldX);
area.originZ = std::min(pressWorld.worldZ, releaseWorld.worldZ);
area.width   = std::max(kAreaMinimumExtentWorldUnits, std::fabs(releaseWorld.worldX - pressWorld.worldX));
area.length  = std::max(kAreaMinimumExtentWorldUnits, std::fabs(releaseWorld.worldZ - pressWorld.worldZ));
area.name = NextAreaName(static_cast<int>(manualAreaDrag.areas->size()));
const std::size_t newIndex = Params::InsertMapAreaSortedBySize(*manualAreaDrag.areas, area);
MakeNamesUnique(*manualAreaDrag.areas);   // confirmed: mutates .name in place only, never reorders,
                                          // so newIndex stays valid across this call
if (manualAreaDrag.areaLocks != nullptr)
    ResolveAreaLocked(*manualAreaDrag.areaLocks,
                      (*manualAreaDrag.areas)[newIndex].name, /*bDefaultLocked=*/false);
if (manualAreaDrag.selectedAreaIndex != nullptr)
    *manualAreaDrag.selectedAreaIndex = static_cast<int>(newIndex);
```
Note `newIndex` is now `std::size_t` from the insertion function, cast to `int` only where an `int`
is required (`selectedAreaIndex`) — do not silently narrow it earlier than that.

## 5. `src/io/MapImporter_Areas_IO.cpp` — native `.sanmap` import

In `ReadAreasJson`, replace `outRecipe.areas.push_back(area);` with
`Params::InsertMapAreaSortedBySize(outRecipe.areas, area);`. `outRecipe.areas.clear()` at the top
of the function is unchanged, so this alone produces a fully size-sorted list on every native load.
Add `#include "../params/MapArea_PARAMS.h"` if not already reachable transitively through
`MapRecipe_PARAMS.h` (verify before assuming; add explicitly if the include is not already present
in the resolved chain).

## 6. `src/io/ScenarioScript_AreaImport_IO.cpp` — foreign scenario import

In the additive reconciliation loop, replace `recipe.areas.push_back(candidateArea);` with
`Params::InsertMapAreaSortedBySize(recipe.areas, candidateArea);`. Deliberately NOT a full re-sort
of the whole list — this path is additive onto a live, possibly manually-reordered `recipe.areas`;
inserting each newly-accepted rectangle at its own size rank leaves every pre-existing entry's
relative order untouched while still landing the new one correctly. No other line in this loop
changes (the collision-skip logic, `result.writtenNames`/`skippedCollisionNames` bookkeeping, are
untouched).

## 7. `src/ui/AreasTab_UI.cpp` — "Add New Area"

In `DrawAreasGlobals`, replace:
```cpp
areas.push_back(area);
state.selectedAreaIndex = static_cast<int>(areas.size()) - 1;
ResolveAreaLocked(state.areaLocks, area.name, /*bDefaultLocked=*/false);
```
with:
```cpp
const std::size_t newIndex = Params::InsertMapAreaSortedBySize(areas, area);
state.selectedAreaIndex = static_cast<int>(newIndex);
ResolveAreaLocked(state.areaLocks, areas[newIndex].name, /*bDefaultLocked=*/false);
```
(Reading the name back from `areas[newIndex]` rather than the local `area` copy matches the
precedent `CreateAreaFromDrag` already sets — harmless either way here since insertion moves the
same object, but keeps both call sites textually consistent.)

## 8. `src/ui/AreasTab_List_UI.h` — `EnsurePlayableArea`

Replace `areas.insert(areas.begin(), playableArea);` with
`InsertMapAreaSortedBySize(areas, playableArea);`. In practice this still lands PlayableArea at (or
near) the back of the list, since it is sized to the whole map and is therefore almost always the
single largest entry — this is a correctness fix for the edge case of a manually-authored area
exceeding the map's own footprint, not a behavior change for the ordinary case.

## ARCH rules invoked
- ARCH §14.19 (this ticket's entire authority) — supersedes §14.17 item 6 and the relevant part of
  §21.8, both already amended by the ARCH Expert.
- Constitution's CPU/GPU parity discipline — items 2/3 above must land together, textually parallel.
- ARCH §3.5 — `MapAreaSize`/`InsertMapAreaSortedBySize` are pure PARAMS-layer functions (their
  signature carries a `Params::` type), correctly homed in `MapArea_PARAMS.h`, not a UI or IO file.

## Explicit out-of-scope
- No `.sanmap` schema change, no `SanGenVersion` bump, no migration — this is a purely in-memory
  ordering rule; `BuildAreasJson`/`ReadAreasJson` already round-trip through a JSON object keyed by
  name, order-agnostic on the wire (confirmed by the ARCH ruling, not re-derived here).
- No change to `DraggableList<T>`, `ApplyAreaListSignal`'s Reorder handling, or any other
  `DraggableList` consumer (Armies/Decals/Props/Markers) — a designer's own manual drag-reorder
  after insertion is untouched and remains the override mechanism (mirrors `ResolveAreaColor`'s
  palette-default-with-override precedent).
- No change to `MapCanvas_AreaDraw_UI.cpp`'s border/handle chrome — the selected area's editing
  feedback already draws on a separate ImGui layer regardless of fill rank (ARCH §14.19 item 1),
  untouched by this ticket.
- No change to color assignment (`ResolveAreaColor`) — confirmed name-keyed, unaffected by
  reordering.

## Acceptance test
- A CPU compositor test (`PreviewComposite_MapAreas_UI_Test.cpp` or sibling): two overlapping
  areas, `recipe.areas = [small, large]` (small at index 0) — confirm a pixel inside both samples
  the SMALL area's color, not the large one's (the inverted rule). Confirm the reverse array order
  (`[large, small]`) samples the LARGE area's color at that same pixel — proving the rule is
  genuinely about array position, not some other tiebreak.
- A GPU parity test (wherever CPU/GPU parity is already exercised for MapAreas, e.g.
  `PreviewComposite_Gpu_UI_Test`/`Mask_Parity_PROC_Test`-style harness): the same two-area
  overlapping scenario produces byte-identical CPU and GPU output.
- A hit-test test (`AreaDragGesture_UI_Test.cpp` or `MapCanvas_UI_Test.cpp`, whichever exercises
  `TryBeginAreaDrag`): clicking inside the overlap of a small-at-index-0/large-at-index-1 pair
  selects the SMALL area, not the large one.
- `MapArea_PARAMS.h` gets its own new headless test (or an existing PARAMS test file, whichever
  this codebase's convention prefers) for `InsertMapAreaSortedBySize`: ascending insertion order is
  maintained across several inserts of varying size; a tie (equal size) inserts AFTER all existing
  equal-size entries (first-come-first-served, per the ARCH doc's own "before the first STRICTLY
  LARGER entry" wording); the returned index matches the area's actual landing position.
- `MapImporter_Areas_IO_Test`/`ScenarioScript_AreaImport_IO_Test`/`AreasTab_UI_Test` (Add New
  Area)/`MapCanvas_AreaDragDispatch_UI_Test` (CreateAreaFromDrag): each confirms its own insertion
  path now produces a size-ascending `recipe.areas`, using the new function rather than a raw
  `push_back`.
- `EnsurePlayableArea`'s own existing test: confirm PlayableArea (whole-map-sized) still lands
  correctly (now via size-sort, typically at/near the back) and the function's `bool` "did it just
  get created" return value is unchanged.
- Full existing test suite: zero regressions. Any existing test that asserted `recipe.areas[0] ==
  PlayableArea` (the OLD front-insertion behavior) needs updating to the new size-sorted
  expectation — adapt, don't weaken.

## Interpretation calls made
1. `ScenarioScript_AreaImport_IO.cpp`'s import stays a per-rectangle sorted INSERT, not a full
   re-sort of the whole list — per the ARCH ruling's own explicit reasoning (preserves a designer's
   prior manual reorder of unrelated, untouched entries). Do not "simplify" this to a `std::sort`
   call over the whole vector after the loop; that would silently discard manual reordering the
   ARCH ruling explicitly protects.
2. `newIndex`/`insertAt` are `std::size_t` throughout until the one point each call site needs an
   `int` (`selectedAreaIndex`) — narrowed only there, per the ARCH doc's own code block.
