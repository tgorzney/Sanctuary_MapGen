# STEP223 — "Center in Map" header button, plus a rename-retargeting gap STEP222 left open

## Summary
The human: "I need a button to center in map center (button should be in the area header to left
of the [o] button...)." Sequenced after STEP222 (which made `[o]` real) because the human's own
request anchors this button's position relative to `[o]`.

**Bundled fix, discovered by the STEP222 implementation and explicitly flagged rather than fixed
unauthorized (Constitution §6 — a confirmed defect gets reported and repaired, not silently routed
around):** `DrawAreaSettings`'s existing rename-retargeting block (`AreasTab_UI.cpp`, inside the
`if (bCommitted && area.name != nameBeforeEdit)` branch) retargets `areaColors` and
`state.areaLocks` when an area is renamed, but was never extended to the new
`AreaVisibilityEntry` table STEP222 added — so renaming a hidden area silently resets it back to
default-visible on the next resolve. This is the exact same bug class STEP21/STEP212 already had
to fix twice (once for color, once for lock); this ticket fixes it a third time, for visibility,
while it is still small and while this file is already open for the Center button below.

## Required reading
- `src/ui/AreasTab_UI.cpp` (current, post-STEP222 — full file, 228 lines)
- `src/ui/AreasTab_UI.h` (current, post-STEP222 — full file, 118 lines)
- `src/ui/AreasTab_List_UI.h` (full file) — `SetAreaToMapSize` (the function this ticket's new
  `CenterAreaInMap` sits beside, same "pure list-lifecycle rule, return whether it moved" shape),
  `ResolvedAreaMapSize`.
- `src/ui/DraggableListWidget_RowLayout_UI.h:88-147` (`DraggableList<T>::Render`, both overloads)
  — the 3-callback + `headerExtraWidthPixels` overload this ticket switches `DrawAreaList` to, and
  `RenderCollapsibleRow`'s own header-extra placement (`headerExtraWidthPixels > 0.0f` branch,
  same file lines 43-47) — confirms a header-extra control draws immediately to the LEFT of the
  `[o]`/`[U]`/`X` affordance strip, exactly where the human asked for "Center."
- `src/ui/MarkersTab_ManualLayers_UI.cpp:80-114` — the one existing production consumer of the
  3-callback overload (STEP123's Color-Override checkbox+swatch), as a shipped precedent for the
  call shape.

## 1. `src/ui/AreasTab_List_UI.h` — new pure function, beside `SetAreaToMapSize`

```cpp
// STEP223 — centers the area's own geometric rectangle on the map's center, preserving its
// width/length exactly (never a resize). No lock gate: the tab's own sliders and "Set to Map
// Size" already ignore lock entirely (lock only gates the CANVAS gesture — see
// AreaLockTable_UI.h's own header) — Center follows the same precedent, unconditionally available
// on any row including PlayableArea. Reports whether the rectangle moved, so a press that changes
// nothing (an area already centered) costs no recomposite.
inline bool CenterAreaInMap(Params::MapArea& area, int mapSize) {
    const float half = static_cast<float>(ResolvedAreaMapSize(mapSize)) * 0.5f;
    const float newOriginX = half - area.width  * 0.5f;
    const float newOriginZ = half - area.length * 0.5f;
    const bool bMoved = area.originX != newOriginX || area.originZ != newOriginZ;
    area.originX = newOriginX;
    area.originZ = newOriginZ;
    return bMoved;
}
```

## 2. `src/ui/AreasTab_UI.h` — one new width constant

Beside the existing `kAreaScalarCompactTrackWidthPixels`/`kAreaScalarCompactFieldWidthPixels`
(Constitution §8 — no literal at the draw site):
```cpp
// STEP223 — the "Center" header button's fixed reserved width (DraggableList's header-extra slot).
inline constexpr float kAreaCenterButtonWidthPixels = 54.0f;
```

## 3. `src/ui/AreasTab_UI.cpp` — the rename-retarget fix, then the Center button

**Rename-retarget fix.** `DrawAreaSettings` gains a fourth parameter and one more retarget loop.
Current signature and comment (lines 24-25, 32-35):
```cpp
bool DrawAreaSettings(Params::MapArea& area, AreasTabState& state, int mapSize,
                      std::vector<AreaColorEntry>& areaColors) {
```
becomes:
```cpp
bool DrawAreaSettings(Params::MapArea& area, AreasTabState& state, int mapSize,
                      std::vector<AreaColorEntry>& areaColors,
                      std::vector<AreaVisibilityEntry>& areaVisibility) {
```
and the rename block (currently lines 42-47):
```cpp
        if (bCommitted && area.name != nameBeforeEdit) {
            for (AreaColorEntry& entry : areaColors)
                if (entry.name == nameBeforeEdit) { entry.name = area.name; break; }
            for (AreaLockEntry& entry : state.areaLocks)
                if (entry.name == nameBeforeEdit) { entry.name = area.name; break; }
        }
```
becomes:
```cpp
        if (bCommitted && area.name != nameBeforeEdit) {
            for (AreaColorEntry& entry : areaColors)
                if (entry.name == nameBeforeEdit) { entry.name = area.name; break; }
            for (AreaLockEntry& entry : state.areaLocks)
                if (entry.name == nameBeforeEdit) { entry.name = area.name; break; }
            // STEP223 — the same repair extended to STEP222's visibility table: without this, a
            // rename silently resets a hidden area back to default-visible on the next resolve.
            for (AreaVisibilityEntry& entry : areaVisibility)
                if (entry.name == nameBeforeEdit) { entry.name = area.name; break; }
        }
```
Update the comment two lines above (currently "if the name commits to something new, the color
AND lock entries keyed on the OLD name must both be retargeted...") to say "the color, lock, AND
visibility entries."

`DrawAreaList`'s inner `drawRowBody` lambda (currently `bAreasMoved = DrawAreaSettings(area, state,
mapSize, areaColors) || bAreasMoved;`) passes the new parameter through:
```cpp
bAreasMoved = DrawAreaSettings(area, state, mapSize, areaColors, areaVisibility) || bAreasMoved;
```

**The Center button.** `DrawAreaList`'s `DraggableList<Params::MapArea>::Render` call switches
from the 2-callback overload to the 3-callback + `headerExtraWidthPixels` overload, adding a third
lambda that draws a "Center" button in the header-extra slot (which, per
`RenderCollapsibleRow`/`RenderFlatRow`'s existing layout math, draws immediately to the LEFT of the
`[o]`/`[U]`/`X` affordance strip — exactly where the human asked):
```cpp
DraggableListSignal DrawAreaList(std::vector<Params::MapArea>& areas, AreasTabState& state,
                                 int mapSize, std::vector<AreaColorEntry>& areaColors,
                                 std::vector<AreaVisibilityEntry>& areaVisibility, bool& bAreasMoved) {
    return DraggableList<Params::MapArea>::Render(
        "areas", areas,
        [&](int rowIndex) {
            const Params::MapArea& area = areas[static_cast<std::size_t>(rowIndex)];
            DraggableListRow row;
            row.label    = AreaRowLabel(area);
            row.bLocked  = *ResolveAreaLocked(state.areaLocks, area.name);
            row.bVisible = *ResolveAreaVisible(areaVisibility, area.name);
            return row;
        },
        [&](int rowIndex) {
            Params::MapArea& area = areas[static_cast<std::size_t>(rowIndex)];
            bAreasMoved = DrawAreaSettings(area, state, mapSize, areaColors, areaVisibility) || bAreasMoved;
        },
        // STEP223 — the header-extra slot: a fixed-width "Center" button drawn on the row's own
        // header line, to the LEFT of the [o]/[U]/X strip (DraggableListWidget_RowLayout_UI.h's
        // existing headerExtraWidthPixels/drawRowHeaderExtra mechanism, STEP123).
        [&](int rowIndex) {
            Params::MapArea& area = areas[static_cast<std::size_t>(rowIndex)];
            if (ImGui::SmallButton("Center"))
                bAreasMoved = CenterAreaInMap(area, mapSize) || bAreasMoved;
        },
        kAreaCenterButtonWidthPixels,
        state.selectedAreaIndex);
}
```
No lock gate on the Center button (see the ruling in `AreasTab_List_UI.h`'s new comment above) —
it is unconditionally clickable on every row, including `PlayableArea`.

## ARCH rules invoked
- ARCH §3.2 / Constitution §6 — `DrawRowAffordances`/`DraggableList` own no app state; `Center`'s
  effect is a plain mutation of the caller's own `Params::MapArea`, reported back via the existing
  `bAreasMoved`/`bCommitted` convention every other control in this file already uses.
- Constitution §8 — the new button width is a named constant, not a literal at the draw site.
- Constitution §6 — the rename-retarget fix repairs a confirmed defect (a rename silently
  reverting visibility) rather than leaving it to surface as a support report later.

## Explicit out-of-scope
- No change to `DraggableListWidget_RowLayout_UI.h`/`DraggableListWidget_Types_UI.h`/
  `DraggableListWidget_RowAffordances_UI.h` — the header-extra mechanism this ticket uses already
  exists and ships unmodified (STEP123).
- No change to any OTHER `DraggableList` consumer (Armies/Decals/Props/Markers) —
  `headerExtraWidthPixels` defaults to `0.0f` for every call site that does not opt in, which is
  every one of them; this ticket touches none of their files.
- No lock gate on Center, matching "Set to Map Size"'s own existing precedent exactly (both ignore
  lock; lock only gates the canvas gesture).
- No canvas-side (`MapCanvas_*`) change — Center is a tab-panel-only action.

## Acceptance test
- `AreasTab_UI_Test.cpp`: `CenterAreaInMap` (pure, headless) — an off-center area with known
  width/length centers correctly for several `mapSize` values, including an odd map size and an
  area whose width/length exceeds the map size (still centers, by design — no clamping beyond what
  `AreaOriginSliderRange`'s own slack already allows elsewhere in this file). A press on an
  already-centered area reports no movement (`bMoved == false`).
- A click-driven acceptance check (extending STEP222's own headless-imgui click-sweep test, if one
  exists, or a new one alongside it): clicking the row's "Center" button actually recenters the
  live `Params::MapArea` in `recipe.areas`, and the button renders to the LEFT of the `[o]` icon on
  screen (position assertion, not just the behavior).
- A rename-then-toggle regression check for the bundled fix: hide an area, rename it, confirm
  `ResolveAreaVisible` for the NEW name still reports hidden (not silently reset to visible) —
  mirrors whatever existing test already covers the color/lock rename-retarget behavior.
- Full existing test suite: zero regressions; every existing call site of `DrawAreaSettings`/
  `DrawAreaList` in test code needs updating for the new `areaVisibility` parameter on
  `DrawAreaSettings`, not weakening.

## Interpretation calls made
1. `kAreaCenterButtonWidthPixels = 54.0f` is a starting value sized for `ImGui::SmallButton("Center")`'s
   own frame padding, matching the sizing approach STEP221's ruling already used for its own new
   constants. If a coder finds it clips or leaves excess gap at the shipped default panel width,
   adjust only this constant, not the widget.
2. The rename-retarget fix is bundled into this ticket rather than split into its own STEP number,
   because it is a one-loop addition directly adjacent to code this ticket already opens
   (`DrawAreaSettings`'s signature is already changing to add nothing new here — wait, it's NOT
   changing for Center; it only changes for the rename fix) — call this out explicitly in your own
   report as a bundled fix, per this session's established practice (STEP221 did the same for its
   own adjacent `colorOptions` seeding bug).
