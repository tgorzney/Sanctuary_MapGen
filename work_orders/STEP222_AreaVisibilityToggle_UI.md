# STEP222 — Real per-area visibility, wired to the "[o]" icon

## Summary
The human: "I need a button to center in map center (button should be in the area header to left
of the [o] button (which does nothing? and I believe should hide that area in preview?)." This
ticket is the "[o]" half only — the Center button is STEP223, sequenced after this ticket because
both touch `DrawAreaList`'s `DraggableList::Render` call and the human's own "Center in Map"
request explicitly anchors it relative to "[o]", so "[o]" must exist and be real first.

Confirmed by direct code reading this session: the per-area header row's `[o]`/`[-]` "visibility"
icon (`DraggableListWidget_RowAffordances_UI.h:34-37`, `DrawVisibilityIcon`) is a STUB for Areas
today. `Params::MapArea` has no visibility bit; `DrawAreaList`'s `describeRow` lambda
(`AreasTab_UI.cpp:104-117`) never sets `DraggableListRow::bVisible` (it defaults `true`, so the
icon always shows `[o]` and can never show `[-]`); and `ApplyAreaListSignal`
(`AreasTab_UI.cpp:147`) explicitly discards the `ToggleVisibility` signal: `if (signal.kind ==
DraggableListSignalKind::ToggleVisibility) return false;` with the comment "an area owns no
visibility bit, so ToggleVisibility is still ignored (ARCH §4)." The human's own report — "which
does nothing" — is exactly correct.

Ruled by the SanGen UI Expert (this session): the human's own phrasing, "hide that area in
preview," is presentation state — matching `AreaColorTable_UI.h`'s existing category exactly (its
own header comment: "these are PRESENTATION settings: they do not serialize into
`mapGeneratorData`") — NOT format data, so no new `Params::MapArea` field. **Ownership must mirror
`areaColors`, not `areaLocks`**: unlike lock (which only gates the canvas gesture and therefore has
"NO composite-side reader at all," per `AreaLockTable_UI.h`'s own header, and stays owned by
`AreasTabState`), visibility must gate `PreviewComposite::BuildMapAreaConfigurations` — it has
exactly the composite-side reader that is `areaColors`'s own reason for living in
`PreviewCompositeSettings` rather than tab state. New areas are visible by default with no
"created vs. pre-existing" distinction (unlike lock, which treats those two cases differently) —
every first-touch resolve defaults to visible, no exceptions, no second creation-time override
needed anywhere.

## Required reading
- `src/ui/AreaLockTable_UI.h` (full file, 58 lines) — the exact shape to mirror, simplified (no
  `bDefaultLocked` parameter needed here).
- `src/ui/AreaColorTable_UI.h` (full file) — the ownership precedent (`PreviewCompositeSettings`
  owns it, not tab state) and the "ONE funnel every area reaches" pattern `ResolveAreaColor` uses.
- `src/ui/AreasTab_UI.h` (current, post-STEP221 — full file, 116 lines)
- `src/ui/AreasTab_UI.cpp` (current, post-STEP221 — full file, 209 lines)
- `src/ui/PreviewComposite_Settings_UI.h` (full file, 108 lines)
- `src/ui/PreviewComposite_Prepare_UI.cpp:129-157` (`BuildMapAreaConfigurations`)
- `src/ui/DraggableListWidget_RowAffordances_UI.h:34-37` (`DrawVisibilityIcon` — already correct,
  reads `row.bVisible`, needs no change)
- `src/ui/Application_PanelEnvironment_UI.cpp:56-58` (the `DrawAreasTab` call site)

## 1. New file `src/ui/AreaVisibilityTable_UI.h`

Mirrors `AreaLockTable_UI.h`'s exact shape, simpler (no default-locked distinction):
```cpp
// AreaVisibilityTable_UI.h — the UI-only per-area VISIBILITY, and nothing else. Layer: UI.
// Mirrors AreaColorTable_UI.h's ownership (NOT AreaLockTable_UI.h's): unlike lock, visibility has
// a real composite-side reader (PreviewComposite::BuildMapAreaConfigurations skips a hidden area's
// fill entirely), so its single owner is PreviewCompositeSettings::areaVisibility — the same
// category areaColors already occupies — never AreasTabState. STEP222.
//
// Every area defaults VISIBLE on first resolve, with no "just created vs. pre-existing"
// distinction (unlike AreaLockEntry's bDefaultLocked) — a freshly created area is exactly as
// visible as any other first-touch area, so there is no second creation-time override anywhere.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Ui {

struct AreaVisibilityEntry {
    std::string name;
    bool        bVisible = true;
};

// Finds the visibility entry for `areaName`, or appends one (default visible) on first touch —
// the same linear-scan-then-lazy-append idiom `ResolveAreaColor`/`ResolveAreaLocked` already use.
inline bool* ResolveAreaVisible(std::vector<AreaVisibilityEntry>& areaVisibility,
                                const std::string& areaName) {
    for (AreaVisibilityEntry& entry : areaVisibility)
        if (entry.name == areaName) return &entry.bVisible;
    AreaVisibilityEntry entry;
    entry.name = areaName;
    areaVisibility.push_back(entry);
    return &areaVisibility.back().bVisible;
}

} // namespace Ui
} // namespace SanmapGen
```

## 2. `src/ui/PreviewComposite_Settings_UI.h`

Add the include beside the existing `AreaColorTable_UI.h` one (line 11):
```cpp
#include "AreaColorTable_UI.h"
#include "AreaVisibilityTable_UI.h"
```
Add the field to `PreviewCompositeSettings`, immediately after `areaColors` (current line 103):
```cpp
    std::vector<AreaColorEntry> areaColors;

    // STEP222 — the single owner of the per-area presentation VISIBILITY, mirroring areaColors'
    // ownership exactly (composite-side reader: BuildMapAreaConfigurations skips a hidden area's
    // fill). Session-only, never serialized, same category as areaColors/gradientRamps/clearColor.
    std::vector<AreaVisibilityEntry> areaVisibility;
```

## 3. `src/ui/AreasTab_UI.h` — thread the new table through `DrawAreasTab`

Change the declaration (current lines 111-112) to add the new parameter, after `areaColors`:
```cpp
void DrawAreasTab(Params::MapRecipe& recipe, AreasTabState& state,
                  Pipeline::PreviewDriver* previewDriver, std::vector<AreaColorEntry>& areaColors,
                  std::vector<AreaVisibilityEntry>& areaVisibility);
```
Add `#include "AreaVisibilityTable_UI.h"` to this header's include block (beside the existing
`AreasTab_List_UI.h`/`Section_UI.h`/`SliderScalar_UI.h` — note `AreaVisibilityTable_UI.h` depends
on nothing but `<string>`/`<vector>`, so this stays a lightweight addition, matching
`AreaLockTable_UI.h`'s own footprint discipline that this header's `AreasTab_List_UI.h` companion
already documents).

## 4. `src/ui/AreasTab_UI.cpp` — wire describeRow, ApplyAreaListSignal, BuildMapAreaConfigurations caller

**`DrawAreaList`** (current lines 100-123): add an `areaVisibility` parameter and set
`row.bVisible` in `describeRow`:
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
            // STEP222 — activates the previously-stubbed [o]/[-] icon: a real per-area bit now,
            // not the DraggableListRow default.
            row.bVisible = *ResolveAreaVisible(areaVisibility, area.name);
            return row;
        },
        [&](int rowIndex) {
            Params::MapArea& area = areas[static_cast<std::size_t>(rowIndex)];
            bAreasMoved = DrawAreaSettings(area, state, mapSize, areaColors) || bAreasMoved;
        },
        state.selectedAreaIndex);
}
```

**`ApplyAreaListSignal`** (current lines 132-155): add an `areaVisibility` parameter; replace the
current `if (signal.kind == DraggableListSignalKind::ToggleVisibility) return false;` (line 147)
with:
```cpp
bool ApplyAreaListSignal(std::vector<Params::MapArea>& areas, AreasTabState& state,
                         std::vector<AreaVisibilityEntry>& areaVisibility,
                         const DraggableListSignal& signal) {
    const int rowIndex = signal.sourceRowIndex;
    const bool bRowValid = rowIndex >= 0 && rowIndex < static_cast<int>(areas.size());
    if (signal.kind == DraggableListSignalKind::Select) {
        if (bRowValid) state.selectedAreaIndex = rowIndex;
        return false;
    }
    if (signal.kind == DraggableListSignalKind::ToggleLock) {
        if (bRowValid) {
            bool* const bLocked = ResolveAreaLocked(state.areaLocks, areas[static_cast<std::size_t>(rowIndex)].name);
            *bLocked = !*bLocked;
        }
        return false;
    }
    // STEP222 — no longer a stub: UNLIKE lock, visibility DOES have a composite-side reader
    // (BuildMapAreaConfigurations), so this must return true to trip NotifyPlacementChange's
    // recomposite — a hidden/shown area actually changes what the composite draws.
    if (signal.kind == DraggableListSignalKind::ToggleVisibility) {
        if (bRowValid) {
            bool* const bVisible = ResolveAreaVisible(areaVisibility, areas[static_cast<std::size_t>(rowIndex)].name);
            *bVisible = !*bVisible;
        }
        return true;
    }
    if (signal.kind == DraggableListSignalKind::Delete
        && (!bRowValid || !IsAreaRemovable(areas[static_cast<std::size_t>(rowIndex)])))
        return false;
    if (!ApplyDraggableListSignal(areas, signal)) return false;
    state.selectedAreaIndex = ResolvedAreaSelection(state.selectedAreaIndex,
                                                   static_cast<int>(areas.size()));
    return true;
}
```
Update the comment directly above this function (currently "AFFORDANCE SCOPE: an area owns no
visibility bit, so ToggleVisibility is still ignored (ARCH §4)." at line 125) to reflect the new
behavior — replace it with something like: "STEP222 — ToggleVisibility is no longer ignored: it
flips the per-area AreaVisibilityEntry the row's own [o]/[-] icon now displays, and (unlike
ToggleLock) trips the composite recompose since a hidden area actually changes what
BuildMapAreaConfigurations draws."

**`DrawAreasTab`** (current lines 189-205): thread the new parameter through both the signature
and the two call sites inside it:
```cpp
void DrawAreasTab(Params::MapRecipe& recipe, AreasTabState& state,
                  Pipeline::PreviewDriver* previewDriver, std::vector<AreaColorEntry>& areaColors,
                  std::vector<AreaVisibilityEntry>& areaVisibility) {
    ImGui::PushID("areasTab");
    const int mapSize = recipe.geometry.mapSize;
    bool bAreasMoved = EnsurePlayableArea(recipe.areas, mapSize);
    bAreasMoved = DrawAreasGlobals(recipe.areas, state) || bAreasMoved;
    if (DrawSectionBegin("Area Stack", state.areaSection)) {
        const DraggableListSignal signal =
            DrawAreaList(recipe.areas, state, mapSize, areaColors, areaVisibility, bAreasMoved);
        if (signal.bHasSignal())
            bAreasMoved = ApplyAreaListSignal(recipe.areas, state, areaVisibility, signal) || bAreasMoved;
        DrawSectionEnd();
    }
    if (bAreasMoved) MakeNamesUnique(recipe.areas);
    NotifyPlacementChange(bAreasMoved, previewDriver);
    ImGui::PopID();
}
```

## 5. `src/ui/PreviewComposite_Prepare_UI.cpp` — gate the composited fill

In `BuildMapAreaConfigurations` (current lines 134-157), skip a hidden area before it is flattened
into a rectangle — add one line at the top of the loop body:
```cpp
void PreviewComposite::BuildMapAreaConfigurations() {
    mapAreaRectangles.clear();
    const float cellsPerWorldUnit = ReciprocalOrZero(settings.worldUnitsPerCell);
    for (int index = 0; index < static_cast<int>(areas.size()); ++index) {
        const Params::MapArea& area = areas[static_cast<std::size_t>(index)];
        // STEP222 — a hidden area contributes nothing to the composite, exactly like a deleted
        // one; the existing empty-list sentinel fallback below already covers "every area hidden."
        if (!*ResolveAreaVisible(settings.areaVisibility, area.name)) continue;
        PreviewMapAreaRectangle record;
        record.minimumX = area.originX * cellsPerWorldUnit;
        record.minimumZ = area.originZ * cellsPerWorldUnit;
        record.maximumX = (area.originX + area.width) * cellsPerWorldUnit;
        record.maximumZ = (area.originZ + area.length) * cellsPerWorldUnit;
        float* const color = ResolveAreaColor(settings.areaColors, area.name);
        record.colorRed   = color[0];
        record.colorGreen = color[1];
        record.colorBlue  = color[2];
        record.colorAlpha = color[3];
        mapAreaRectangles.push_back(record);
    }
    if (mapAreaRectangles.empty()) {
        PreviewMapAreaRectangle sentinel;
        sentinel.minimumX = 1.0f;
        sentinel.maximumX = -1.0f;
        mapAreaRectangles.push_back(sentinel);
    }
}
```
Add `#include "AreaVisibilityTable_UI.h"` to this file's include block if `PreviewComposite_Settings_UI.h`
does not already transitively make `ResolveAreaVisible` visible here (it will, via the settings
header's own new include from step 2 above — verify, do not assume, before deciding whether a
direct include is also needed here for clarity/§1.5 hygiene).

## 6. `src/ui/Application_PanelEnvironment_UI.cpp` — the one call site

Line 57 currently reads:
```cpp
DrawAreasTab(recipe, tabState.areas, &previewDriver, composite.Settings().areaColors);
```
Change to:
```cpp
DrawAreasTab(recipe, tabState.areas, &previewDriver, composite.Settings().areaColors,
            composite.Settings().areaVisibility);
```

## ARCH rules invoked
- ARCH §14.17 item 9's own precedent (PreviewCompositeSettings as the single owner of per-area
  presentation state with a composite-side reader) — extended here from color to visibility.
- Constitution §6 — an index (`rowIndex`) is validated (`bRowValid`) before use, never trusted.
- ARCH §3.2 — the widget (`DraggableList`) owns no app state; `AreaVisibilityTable_UI.h` is the
  caller-owned side table the row's `bVisible` field is populated FROM, not a widget-internal flag.

## Explicit out-of-scope
- No change to `Params::MapArea`, `MapExporter_Areas_IO.cpp`, or `MapImporter_Areas_IO.cpp` —
  visibility is session-only presentation state, never serialized, per the UI Expert's ruling.
- No exemption for `PlayableArea` from hiding — symmetric treatment with every other area, since
  hiding is presentation-only and never touches the bake (unlike its color, which IS pinned/
  disabled — that pinning is untouched by this ticket).
- No change to `MapCanvas`'s own outline/drag-handle overlay for a selected/hovered area
  (`MapCanvas_ManualDragSources_UI.h`) — the UI Expert's ruling explicitly scopes this ticket to
  the COMPOSITED FILL only (`BuildMapAreaConfigurations`), leaving the canvas outline/handle
  affordance visible even when an area is hidden, mirroring lock's own existing precedent (locked
  ≠ invisible; interactability and visibility stay orthogonal). If the human wants the canvas
  outline hidden too, that is separate follow-up.
- No "Center in Map" button — that is STEP223, sequenced after this ticket.

## Acceptance test
- A new or extended `AreasTab_UI_Test.cpp` (or `PreviewComposite_Prepare_UI_Test.cpp`, whichever
  already exercises `DrawAreaList`/`ApplyAreaListSignal`/`BuildMapAreaConfigurations`) case:
  clicking the `[o]` icon on a row flips that area's `AreaVisibilityEntry::bVisible` (verify via
  `ResolveAreaVisible` on the same table afterward) and `ApplyAreaListSignal` returns `true` for a
  `ToggleVisibility` signal (unlike `ToggleLock`, which returns `false`).
- A composite-side test: a hidden area contributes NO rectangle to
  `PreviewComposite::mapAreaRectangles` after `BuildMapAreaConfigurations()` runs; re-showing it
  restores its rectangle. Hiding every area still leaves exactly the one degenerate sentinel
  rectangle (never a zero-length buffer).
- Full existing test suite: zero regressions — every existing `DrawAreaList`/`ApplyAreaListSignal`/
  `DrawAreasTab`/`BuildMapAreaConfigurations` call site in test code needs updating for the new
  parameter, not weakening.

## Interpretation calls made
1. `ApplyAreaListSignal` returns `true` (trips a recompose) for `ToggleVisibility`, unlike
   `ToggleLock`'s `false` — because visibility has a real composite-side reader and lock does not,
   per the UI Expert's ruling. This is a deliberate asymmetry between two otherwise similar-looking
   toggle signals, not an oversight — do not "fix" it to match `ToggleLock`'s `false`.
2. The canvas outline/drag-handle affordance for a hidden-but-selected area is left untouched
   (still visible/interactable) — a UX nuance the UI Expert flagged as open but not blocking; if
   this reads wrong once built, it is a fast follow-up, not a reason to block this ticket.
