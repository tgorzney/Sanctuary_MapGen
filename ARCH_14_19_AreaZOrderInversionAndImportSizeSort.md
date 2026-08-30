[← ARCH index](ARCH.md) · [§14 ARCH_14_PreviewOverlayLayering](ARCH_14_PreviewOverlayLayering.md) · SanGen ARCH §14.19. **Only the ARCH Expert writes this file.**

### 14.19 Area Z-order convention INVERTED — array index 0 is now topmost, both on screen and in the Area Stack list; `recipe.areas` is kept continuously sorted ascending by size (amends §14.17 item 6, §21.8's hit-test and creation rulings)

Human-approved ruling, verified against the live code before being written down: `PreviewComposite_Cpu_UI.cpp`,
`PreviewComposite_Sampling_UI.glsl`, `MapCanvas_AreaDragDispatch_UI.cpp`, `AreasTab_List_UI.h`,
`AreasTab_UI.cpp`, `DraggableListWidget_RowLayout_UI.h`, `MapImporter_Areas_IO.cpp`,
`ScenarioScript_AreaImport_IO.cpp`, `MapExporter_Areas_IO.cpp`, `MapArea_PARAMS.h`.

**The human's ask, restated as the invariant it actually is:** not merely "sort at import" —
`recipe.areas` is kept **continuously** sorted ascending by size (`width * length`), smallest
first. Index 0 is simultaneously the top row `DraggableList<Params::MapArea>::Render` draws
(`DraggableListWidget_RowLayout_UI.h` indexes `areas[rowIndex]` 1:1, untouched by this ruling) and
the topmost rectangle in the composited Z-stack. One array, one order, two consumers that already
read it 1:1 — no new indirection anywhere.

**1. Why this is a standing invariant, not an import-only rule.** The immediate-mode border/handle
chrome (`MapCanvas_AreaDraw_UI.cpp`) already gives full editing feedback for the selected area on a
separate ImGui draw-list layer, drawn over the composited texture regardless of that area's fill
rank — so an area's position in the size-sorted stack never hides it *during* authoring. After
authoring, "a small carve-out renders on top of the large area it sits inside" is the desirable
default for every creation path, not a special case for import: a designer drawing a small
chokepoint zone inside `PlayableArea` wants to see its tint, not have it buried under whatever
background rectangle happens to occupy a higher array index. Scoping this rule to "import only"
would leave manual creation with the opposite (wrong) default. The rule therefore governs every
insertion point, uniformly, through one function (item 3).

**2. The Z rule flips: forward iteration, FIRST containing match wins, early exit.** Supersedes
§14.17 item 6's "forward iteration, LAST containing match wins" verbatim — ascending array index is
now Z-**descending** (index 0 = top), so the first hit scanning forward from index 0 already is the
topmost area; continuing to scan for a later, lower-priority match was always wasted work once the
convention states this, so early exit is a genuine (rough-estimate, unbenchmarked) small win on top
of being the correct rule, not a separate optimization decision.

CPU twin (`PreviewComposite_Cpu_UI.cpp`, `LayerColorAtPixel`'s `MapAreas` branch):
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
GPU twin (`PreviewComposite_Sampling_UI.glsl`, `mapAreaColorAtCell`):
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
**Parity is automatic, not merely intended:** both twins read the identical `mapAreaRectangles`
buffer, built once per `PrepareRun()` by the untouched `BuildMapAreaConfigurations` (which already
emits records in `recipe.areas` order 1:1, filtered only by visibility — nothing here changes it).
Applying the exact same loop shape to both twins in the same change, as ruled, guarantees
byte-identical output; the degenerate empty-list sentinel (`minimumX=1 > maximumX=-1`) still fails
its own test unconditionally regardless of scan direction, so the empty case is unaffected. A coder
must not "simplify" one twin's loop differently from the other (e.g. one using `break`+fallthrough,
the other an early `return`) in a way that changes how a would-be-continued scan is short-circuited
— the two must stay textually parallel, per this pack's standing CPU/GPU parity discipline.

Hit-test (`MapCanvas_AreaDragDispatch_UI.cpp`, `TryBeginAreaDrag` step 2 — supersedes its own
comment's "last match wins" citation of the old §21.8 convention):
```cpp
int hitIndex = -1;
for (int index = 0; index < static_cast<int>(areas.size()); ++index) {
    if (!IsAreaLocked(index)
        && IsWorldPointInsideArea(areas[static_cast<std::size_t>(index)], worldPoint.worldX, worldPoint.worldZ)) {
        hitIndex = index;
        break;   // ascending index is Z-descending: the first unlocked hit IS the topmost area.
    }
}
```
Click-to-select and what-you-see still can never disagree — the shared invariant §14.17 item 6
originally stated is preserved, only the direction of "wins" is inverted to match.

**3. One insertion function, used everywhere `recipe.areas` grows.** New in `MapArea_PARAMS.h`
(PARAMS, per §3.5 — a pure function whose signature carries a `Params::` type):
```cpp
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
Four call sites switch from an unconditional `push_back`/front-`insert` to this one function —
none of them change their own name-assignment or collision logic, only how the area lands in the
vector:
- **`MapImporter_Areas_IO.cpp::ReadAreasJson`** — `outRecipe.areas.push_back(area)` →
  `Params::InsertMapAreaSortedBySize(outRecipe.areas, area)`. The function already starts from
  `outRecipe.areas.clear()`, so this alone produces a fully size-sorted list on native `.sanmap`
  import — the human's literal ask.
- **`ScenarioScript_AreaImport_IO.cpp::ImportAreaRectanglesFromScenarioScriptFile`** —
  `recipe.areas.push_back(candidateArea)` → `Params::InsertMapAreaSortedBySize(recipe.areas,
  candidateArea)`, inside the existing non-colliding-only loop. Deliberately NOT a full re-sort of
  the whole list: this path is additive onto a live, possibly manually-reordered `recipe.areas`
  (its own header comment: "additive-never-destructive reconciliation"); a full re-sort would
  silently discard a designer's own prior drag-reorder of unrelated, untouched entries. Inserting
  each newly-accepted rectangle at its own size rank leaves every pre-existing entry's relative
  order untouched while still landing the new one correctly.
- **`AreasTab_UI.cpp::DrawAreasGlobals`** ("Add New Area") — `areas.push_back(area);
  state.selectedAreaIndex = static_cast<int>(areas.size()) - 1;` becomes
  `const std::size_t newIndex = InsertMapAreaSortedBySize(areas, area); state.selectedAreaIndex =
  static_cast<int>(newIndex);`. `ResolveAreaLocked(state.areaLocks, areas[newIndex].name, false)`
  reads the name back from the vector at `newIndex`, not from the local `area` copy (harmless either
  way here since the insert is a move of the same object, but matches the precedent
  `CreateAreaFromDrag` already sets for reading the post-`MakeNamesUnique` name).
- **`MapCanvas_AreaDragDispatch_UI.cpp::CreateAreaFromDrag`** —
  `manualAreaDrag.areas->push_back(area); … const int newIndex = size() - 1;` becomes `const
  std::size_t newIndex = Params::InsertMapAreaSortedBySize(*manualAreaDrag.areas, area);`, THEN
  `MakeNamesUnique(*manualAreaDrag.areas)` (unchanged position/order — confirmed by direct read of
  `UniqueNameList_UI.h`'s `MakeNamesUnique` that it only mutates `.name` in place and never
  reorders, so `newIndex` stays valid across that call), THEN
  `(*manualAreaDrag.areas)[newIndex].name` for the lock-table seed and
  `*manualAreaDrag.selectedAreaIndex = static_cast<int>(newIndex)` for selection.
- **`AreasTab_List_UI.h::EnsurePlayableArea`** — `areas.insert(areas.begin(), playableArea);`
  becomes `InsertMapAreaSortedBySize(areas, playableArea);`. In practice this still lands
  PlayableArea at (or near) the back, since it is sized to the whole map and is therefore almost
  always the single largest entry — but it is now correct BY THE SAME RULE as everything else,
  rather than a hardcoded "front" that this ruling would otherwise have had to hardcode to "back"
  as a special case. No behavior change for the ordinary case; a strictly more correct answer for
  the edge case of a manually-authored area exceeding the map's own footprint.

**A designer can still override the default by hand.** `DraggableList`'s existing Reorder signal
(`ApplyDraggableListSignal`, unchanged) lets a drag-and-drop reorder move any area to any position —
the size-sort above is the *default landing position on insertion*, not a permanent lock. This
mirrors the already-shipped precedent one file over: `ResolveAreaColor`'s palette assignment is
also only a default a designer's own swatch edit can override (`AreaColorTable_UI.h`).

**4. Nothing about the `.sanmap` schema, export, or any positional consumer changes.** Confirmed by
direct read, not assumed:
- `BuildAreasJson`/`ReadAreasJson` (`MapExporter_Areas_IO.cpp`/`MapImporter_Areas_IO.cpp`) round-trip
  `recipe.areas` through a JSON **object** keyed by `MapArea::name` — order-agnostic on the wire.
  Sorting the in-memory vector changes nothing about what gets written or how it is read back.
- `MapArea::name` is the only cross-reference the game engine (or anything else) resolves an area
  by — `MapArea_PARAMS.h`'s own comment: `GameUtils.GetArea(name)`, a load-bearing name lookup, never
  a positional one.
- A repo-wide search for any other reader of `recipe.areas[0]`/`.front()`/`.back()` found none —
  the three sites this ruling and §14.17/§21.8 already named (composite Z, hit-test Z, DraggableList
  display order) are the *only* consumers of the array's position. `EnsurePlayableArea`'s old
  front-insertion was therefore load-bearing for nothing beyond those same three sites; no Format
  Expert or Generator Expert re-check is required before this lands, though either should still be
  the one to confirm if a *future* ticket ever gives the game-side Lua a positional reason to care.
- No `SanGenVersion` bump, no new field, no migration — purely an in-memory ordering rule.

**5. Size is `width * length` (plain rectangle area), never a bounding diagonal or `max(width,
length)`** — the natural reading of "size" for an axis-aligned rectangle, and the only definition
`MapAreaSize` above needs.

**Dispatchable as one work-order**, all five call-site edits plus the two compositor twins plus the
hit-test loop landing together — none of the pieces is independently useful (a coder shipping only
the insertion-order change without the Z-loop flip would produce a list whose top row is NOT what
renders on top, the exact bug this ruling exists to fix; shipping only the Z-loop flip without the
insertion change would leave every already-imported map's areas in their pre-existing, unsorted
array order).
