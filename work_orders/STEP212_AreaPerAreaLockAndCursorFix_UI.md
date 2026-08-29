# STEP212 — Areas: per-area lock (replacing the global lock) + cursor-lies-about-lock-state fix

**Layer:** UI. **Domain:** `MapCanvas` (the canvas gesture surface), `AreasTabState`/a new minimal
side-table header (the tab's UI-only presentation-state tables), `recipe.areas` is read but never
schema-changed. **Executor:** SanGen Coder. Authored by the SanGen UI Expert, following up on this
expert's own direct read-based diagnosis of two real defects: (1) Areas ship with a single global
`AreasTabState::bAreasLocked` gating every area uniformly, with no way for one area to be locked while
another is not, and no way for a freshly authored area to start immediately draggable without the human
hunting down a separate checkbox first; (2) `MapCanvas_AreaDraw_UI.cpp`'s hover-cursor feedback hit-tests
handles and shows a resize cursor purely from proximity, never checking any lock at all — so a locked
area's handles show an interactive-looking resize cursor even though a click on them is silently refused,
which is the exact "cursor lies" report this ticket closes.

**This is a full re-authoring against the tree AFTER STEP211 landed** (confirmed: all 165 tests passing,
`AreaColorEntry`/`ResolveAreaColor` now live in `src/ui/AreaColorTable_UI.h`, owned by
`PreviewCompositeSettings::areaColors`; `DrawAreasTab` takes an extra `std::vector<AreaColorEntry>&`;
`MapCanvas::SetManualAreaDragSource` takes five parameters including `mapAreaSuppressedIndex`; `MapCanvas`
has `SetAreaCompositeRefreshCallback`/`SetMapAreaSuppression`; `MapCanvas_AreaDraw_UI.cpp` only fills the
currently drag-suppressed area, borders only when [MapAreas layer enabled] AND [suppressed] AND
[selected]). A prior draft of this ticket predates all of that and is stale — every file cited below was
read fresh, directly against the live tree, while drafting this version, including a fresh full read of
`ARCH_21_08_AreaCanvasGesture.md` (original ruling + its 2026-08-29 amendment) and
`ARCH_14_17_MapAreaFieldLayer.md`. **This ticket touches NONE of STEP211's field-layer/GPU-composite/
drag-suppression machinery** — only how the per-area LOCK gates access to gestures already wired.

## Summary

Two independent fixes, both scoped to files STEP210/STEP211 already shipped:

**Fix 1 — per-area lock.** Retires the single tab-wide `AreasTabState::bAreasLocked` bool in favor of a
real per-area lock, using the EXACT same UI-only, name-keyed side-table pattern `AreaColorEntry`/
`ResolveAreaColor` establish (STEP21 ruling #4's precedent — presentation state with no `_PARAMS` home
lives in a small side table keyed by `MapArea::name`, never vector position). **Ownership call:** unlike
color, lock has **zero composite involvement** — the GPU-composited fill (`PreviewLayerKind::MapAreas`,
ARCH §14.17) is driven entirely by `areaColors` + the field layer's own enable/opacity; it never asks
whether an area is locked. Lock only gates whether `MapCanvas`'s own gesture code accepts a click/drag for
a given area — a pure UI/canvas-side concern. So the new table is **not** added to
`PreviewCompositeSettings`; it stays where `bAreasLocked` already lived, `AreasTabState::areaLocks`
(`AreasTab_UI.h`), with `MapCanvas` holding a plain injected pointer to that same vector — the identical
ownership shape the retired global bool already had, just widened from one bit to a name-keyed table. The
new type gets its **own minimal header**, `src/ui/AreaLockTable_UI.h`, mirroring `AreaColorTable_UI.h`'s
exact shape (`<string>`/`<vector>` only) — not inlined into `AreasTab_List_UI.h` (which STEP211 narrowed
to "the pure lifecycle rules for the list," not a side-table catch-all) and not merged into
`AreaColorTable_UI.h` itself (a different owner, a different reason for existing). This also lets
`MapCanvas_ManualDragSources_UI.h` depend on it exactly as minimally as it already depends on
`AreaColorTable_UI.h`, without pulling in `AreasTab_List_UI.h`'s heavier
`ColorSwatch_UI.h`/`RtToggleWidget_UI.h`/`UniqueNameList_UI.h`/`MapArea_PARAMS.h` chain.

A pre-existing area (including the engine-required `PlayableArea`) defaults LOCKED on first resolve; a
freshly created area — via the tab's own "Add New Area" button OR the canvas's own `CreateAreaFromDrag` —
is inserted UNLOCKED explicitly, at creation time, before the table's own lazy default would otherwise
apply. The Area Stack list's own `[U]/[L]` row icon (already drawn by the shared `DraggableList` row
affordance strip, `DraggableListWidget_RowAffordances_UI.h`, previously wired to a hardcoded
`!IsAreaRemovable(area)` stand-in and a no-op `ToggleLock` handler) becomes the real, working per-area lock
toggle — no new UI control is added; an existing, already-drawn, previously-inert one is wired up. The
tab-wide "Lock Areas" checkbox is REMOVED (not repurposed as a bulk action — see "Interpretation calls
made," item 2). Every canvas-side gate that used to read `manualAreaDrag.bAreasLocked` now asks a new
`MapCanvas::IsAreaLocked(int areaIndex)` helper about the SPECIFIC area in question, at the point the
gesture code first knows which area that is — reused identically by both `TryBeginAreaDrag` (Fix 1) and
`DrawAreaOverlayPass`'s cursor section (Fix 2), never two independently-maintained lock checks.

**Fix 2 — cursor lies about lock state.** `MapCanvas_AreaDraw_UI.cpp`'s hover-cursor section (the tail of
the STEP211-rewritten `DrawAreaOverlayPass`, which already deals with the suppressed-index fill/border
logic) is gated on the selected area's own (now per-area) lock via `IsAreaLocked`: while locked, no
`ImGui::SetMouseCursor` override at all (falls through to imgui's own default arrow) — no new "locked"
cursor glyph is introduced (see "Interpretation calls made," item 3). The suppressed-index fill, the
enabled/suppressed/selected-gated border, and the selected-area handle circles are all **byte-identical**
to the STEP211-shipped version — only the cursor-shape affordance gains a lock check.

## Required reading

`ARCH_21_08_AreaCanvasGesture.md` in full, **including its 2026-08-29 amendment** (the original gesture/
dispatch/create-by-drag rulings are unchanged by that amendment; the draw-pass ruling is superseded by
§14.17 and this ticket must not resurrect the pre-amendment "fill every area every frame" text). Its own
closing ruling on the retired global lock — *"Locked gates the whole surface, uniformly, including
selection-by-click... Selecting a different area while locked is still possible through the Area Stack
list... unaffected — no real capability is lost, only the canvas-side shortcut"* — is the direct precedent
this ticket's per-area re-application of the SAME uniform-refusal posture follows (Interpretation call 1).
`ARCH_14_17_MapAreaFieldLayer.md` in full — item 9 is the direct precedent for extracting a presentation
side-table into its own minimal header when a different owner needs a narrower dependency footprint; item
11's suppression/refresh-callback mechanism and item 12's border rule are both **untouched** by this ticket
and must not be re-derived or second-guessed while reading `MapCanvas_AreaDraw_UI.cpp`/
`MapCanvas_AreaDragDispatch_UI.cpp` fresh.

**Note for the ARCH Expert (not acted on by this ticket):** `ARCH_21_08`'s correction 1 sentence *"there
is no per-area lock (only the tab-wide `AreasTabState::bAreasLocked`...)"*, and every
`bAreasLocked`/`areasLocked` literal in that file's own pre-amendment code blocks, become stale text once
this ticket ships — flagged for a future ARCH pass, not blocking this ticket.

---

## 1. New file: `src/ui/AreaLockTable_UI.h`

```cpp
// AreaLockTable_UI.h — the UI-only per-area LOCK, and nothing else. Layer: UI.
// Mirrors AreaColorTable_UI.h's exact shape (STEP21 ruling #4's precedent extended to a second
// presentation-state concern, STEP212): a small side table keyed by MapArea::name, depending on
// nothing but <string>/<vector>. UNLIKE AreaColorTable_UI.h, this table has NO composite-side
// reader at all — lock never affects what the GPU composite draws (that is color/enabled-layer's
// job alone, ARCH_14_17_MapAreaFieldLayer.md), it only gates whether MapCanvas's own gesture code
// accepts a click/drag for a given area. Its single owner therefore stays `AreasTabState::areaLocks`
// (AreasTab_UI.h) — never `PreviewCompositeSettings` — with MapCanvas holding a plain pointer to
// that same vector (SetManualAreaDragSource), the identical ownership shape
// `AreasTabState::bAreasLocked` (the single global bool this table replaces) already used.
//
// Given its own minimal header regardless (rather than living inline in AreasTab_List_UI.h, which
// remains this domain's "pure lifecycle rules" file — PlayableArea, unique names, Set to Map Size —
// not a catch-all for every presentation-state side table) so `MapCanvas_ManualDragSources_UI.h` can
// depend on it exactly as minimally as it already depends on `AreaColorTable_UI.h`, without pulling
// in `AreasTab_List_UI.h`'s own heavier `ColorSwatch_UI.h`/`RtToggleWidget_UI.h`/
// `UniqueNameList_UI.h`/`MapArea_PARAMS.h` chain — the same footprint discipline that file's own
// existing include list already practices.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Ui {

// A UI-only per-area LOCK, keyed by area NAME — the exact same side-table pattern as
// `AreaColorEntry` (AreaColorTable_UI.h). Areas default LOCKED (matches the retired global
// `AreasTabState::bAreasLocked`'s own default), EXCEPT a freshly created one (via the tab's own
// "Add New Area" button or the canvas's own `CreateAreaFromDrag`), which is inserted UNLOCKED
// explicitly at creation time, before this table's own lazy default would otherwise apply.
struct AreaLockEntry {
    std::string name;
    bool        bLocked = true;
};

// Finds the lock entry for `areaName`, or appends one on first touch using `bDefaultLocked` — the
// same linear-scan-then-lazy-append idiom `ResolveAreaColor` already uses. `bDefaultLocked` is
// `true` for every ordinary lazy resolve (a pre-existing area encountered for the first time — an
// imported project, or the engine-required PlayableArea, which is never "just created" by the user
// in practice); the two creation call sites (AreasTab_UI.cpp's "Add New Area",
// MapCanvas_AreaDragDispatch_UI.cpp's CreateAreaFromDrag) pass `false` explicitly, inserting their
// own entry UNLOCKED before this resolver's own default would otherwise apply. Once an entry
// already exists, a LATER resolve's own `bDefaultLocked` argument is irrelevant — the existing
// value always wins, never silently re-defaulted.
inline bool* ResolveAreaLocked(std::vector<AreaLockEntry>& areaLocks, const std::string& areaName,
                               bool bDefaultLocked = true) {
    for (AreaLockEntry& entry : areaLocks)
        if (entry.name == areaName) return &entry.bLocked;
    AreaLockEntry entry;
    entry.name    = areaName;
    entry.bLocked = bDefaultLocked;
    areaLocks.push_back(entry);
    return &areaLocks.back().bLocked;
}

} // namespace Ui
} // namespace SanmapGen
```

---

## 2. Modified: `src/ui/AreasTab_List_UI.h`

Add one new `#include` (alphabetically before `ColorSwatch_UI.h`) and one sentence to the existing
STEP211 header-comment paragraph. Everything else in this file — `IsPlayableArea`/`IsAreaRemovable`,
`AreaRowLabel`, `ResolvedAreaMapSize`, `SetAreaToMapSize`, `NextAreaName`, `EnsurePlayableArea`, the
`static_assert` — is unmodified.

```cpp
// ARCH_14_17_MapAreaFieldLayer.md §14.17 item 9 — `AreaColorEntry`/`ResolveAreaColor` moved OUT of
// this file into the new minimal `AreaColorTable_UI.h`; this file includes it and re-exports both
// names, so every existing call site (`AreasTab_UI.cpp`, `MapCanvas_AreaDraw_UI.cpp`,
// `AreasTab_UI_Test.cpp`) keeps compiling unchanged against `AreasTab_List_UI.h`. The color table's
// single OWNER is now `PreviewCompositeSettings::areaColors` (see that header) — not
// `AreasTabState`, which no longer carries a color field of its own.
//
// STEP212 — `AreaLockEntry`/`ResolveAreaLocked` (the per-area lock, replacing the retired global
// `AreasTabState::bAreasLocked`) live in the equally minimal sibling `AreaLockTable_UI.h`, included
// and re-exported here for the identical reason: every existing `#include "AreasTab_List_UI.h"`
// call site keeps compiling with zero new includes needed. UNLIKE the color table, the lock table's
// owner stays tab-side (`AreasTabState::areaLocks`) — it has no composite-side reader at all.
#pragma once
#include <string>
#include <vector>
#include "AreaColorTable_UI.h"
#include "AreaLockTable_UI.h"
#include "ColorSwatch_UI.h"
#include "RtToggleWidget_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/MapArea_PARAMS.h"

namespace SanmapGen {
namespace Ui {

static_assert(kAreaColorChannelCount == kColorSwatchChannelCount,
             "AreaColorEntry's channel count must match the swatch widget's own, or DrawColorSwatch "
             "would read/write past the array ResolveAreaColor hands it.");
```

(the rest of the file — `kPlayableAreaName` through `EnsurePlayableArea` — is byte-identical to today's
tree; no further edit.)

---

## 3. Modified: `src/ui/AreasTab_UI.h`

Replace the `AreasTabState` struct (current lines 33-49) with:

```cpp
struct AreasTabState {
    SectionState       globalSection;
    SectionState       areaSection;
    ColorSwatchOptions colorOptions = ColorSwatchOptions();
    int  selectedAreaIndex = -1;
    // STEP212 — replaces the retired global `bool bAreasLocked = true;`: one lock bit PER AREA, the
    // same UI-only, name-keyed side-table shape `PreviewCompositeSettings::areaColors` uses for
    // color (AreaLockTable_UI.h's AreaLockEntry/ResolveAreaLocked) — but owned HERE, tab-side, not
    // by the composite, because lock never affects what gets drawn in the GPU-composited fill, only
    // whether the canvas gesture accepts input (see AreaLockTable_UI.h's own header comment). A
    // pre-existing area defaults LOCKED on first resolve; a freshly created one (Add New Area below,
    // or the canvas's own CreateAreaFromDrag) is inserted UNLOCKED explicitly, before this table's
    // own lazy default would otherwise apply.
    std::vector<AreaLockEntry> areaLocks;

    // ONE shared toggle set for the currently-selected area's detail section — not per-row: only
    // the selected area's settings ever draw, the same posture ArmiesTabState uses for its own
    // single-selection editor over a real PARAMS vector (STEP20/STEP21).
    RealtimeToggle originXToggle;
    RealtimeToggle originZToggle;
    RealtimeToggle widthToggle;
    RealtimeToggle lengthToggle;
    RealtimeToggle colorToggle;
};
```

(Only the `bool bAreasLocked = true;` field is removed and `std::vector<AreaLockEntry> areaLocks;`
added; every other member and every other function/comment in this file — `AreasTabColorSwatchOptions`,
`AreaExtentSliderRange`, `AreaOriginSliderRange`, `SelectedArea`, `ResolvedAreaSelection`,
`DrawAreasTab`'s own declaration and its unchanged `std::vector<AreaColorEntry>& areaColors` parameter —
is unmodified. `AreaLockEntry` is already visible here transitively through this file's existing
`#include "AreasTab_List_UI.h"` — no new include needed.)

---

## 4. Modified: `src/ui/AreasTab_UI.cpp`

Full file (every function that touches lock is rewritten; the `#include` list drops `Checkbox_UI.h`,
now unused — it had exactly one call site, the retired "Lock Areas" checkbox):

```cpp
// AreasTab_UI.cpp — the imgui composition of the areas tab. Layer: UI.
// Shared widgets only: DraggableList for the ordered area stack, TextInput for the name,
// SliderScalar for the four rectangle scalars, ColorSwatch (alpha bar) for the overlay tint, and
// Section for the two blocks. No ImGui::SliderFloat / DragFloat / ColorEdit4 in this file — v1
// called all three here. STEP212 retires the tab-wide "Lock Areas" checkbox (Checkbox_UI is no
// longer used here) in favor of a real PER-AREA lock, driven by the DraggableList row's own
// [U]/[L] icon (AreaLockTable_UI.h's AreaLockEntry/ResolveAreaLocked).
#include "AreaColorTable_UI.h"
#include "AreasTab_UI.h"
#include "DraggableListWidget_UI.h"
#include "PlacementRuleSections_UI.h"
#include "TextInput_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// One area's own settings, drawn inline inside its own expanded row body (STEP110), never once at
// the bottom for whatever was "selected". Every scalar is whole-cell; color has no `_PARAMS` home
// (STEP21 ruling #4) and is resolved from the UI-only side table, keyed by name.
bool DrawAreaSettings(Params::MapArea& area, AreasTabState& state, int mapSize,
                      std::vector<AreaColorEntry>& areaColors) {
    const ScalarSliderRange originRange = AreaOriginSliderRange(mapSize);
    const ScalarSliderRange extentRange = AreaExtentSliderRange(mapSize);
    bool bCommitted = false;
    if (IsPlayableArea(area)) {
        ImGui::TextDisabled("PlayableArea is required by the engine: it cannot be renamed or removed.");
    } else {
        // Captured BEFORE the edit: if the name commits to something new, the color AND lock entries
        // keyed on the OLD name must both be retargeted, or a rename silently reverts the area's
        // color to default and its lock to LOCKED next frame (STEP21 ruling #5 for color; STEP212
        // extends the same repair to the new per-area lock table for the identical reason).
        const std::string nameBeforeEdit = area.name;
        TextInputRules nameRules;
        nameRules.maximumLength = 48;
        nameRules.bAllowEmpty   = false;
        nameRules.fallbackText  = "Area";
        bCommitted = DrawTextInput("Name", area.name, nameRules).bCommitted;
        if (bCommitted && area.name != nameBeforeEdit) {
            for (AreaColorEntry& entry : areaColors)
                if (entry.name == nameBeforeEdit) { entry.name = area.name; break; }
            for (AreaLockEntry& entry : state.areaLocks)
                if (entry.name == nameBeforeEdit) { entry.name = area.name; break; }
        }
    }
    bCommitted = DrawSliderScalar("X Position", area.originX, originRange, state.originXToggle,
                                  WidgetStyle(), "%.0f").bCommitted || bCommitted;
    bCommitted = DrawSliderScalar("Z Position", area.originZ, originRange, state.originZToggle,
                                  WidgetStyle(), "%.0f").bCommitted || bCommitted;
    bCommitted = DrawSliderScalar("Width", area.width, extentRange, state.widthToggle,
                                  WidgetStyle(), "%.0f").bCommitted || bCommitted;
    bCommitted = DrawSliderScalar("Length", area.length, extentRange, state.lengthToggle,
                                  WidgetStyle(), "%.0f").bCommitted || bCommitted;
    // ARCH §14.17 item 10 — PlayableArea is always Green and non-editable: re-pin its color before
    // drawing (the swatch below is the only OTHER path that could ever set a PlayableArea color) and
    // disable the control so a designer cannot pick a different one.
    float* const color = ResolveAreaColor(areaColors, area.name);
    if (IsPlayableArea(area)) {
        color[0] = kDefaultAreaColor[0]; color[1] = kDefaultAreaColor[1];
        color[2] = kDefaultAreaColor[2]; color[3] = kDefaultAreaColor[3];
        ImGui::BeginDisabled();
        DrawColorSwatch("Color", color, state.colorOptions, state.colorToggle);
        ImGui::EndDisabled();
    } else {
        bCommitted = DrawColorSwatch("Color", color, state.colorOptions,
                                     state.colorToggle).bCommitted || bCommitted;
    }
    if (ImGui::Button("Set to Map Size")) bCommitted = SetAreaToMapSize(area, mapSize) || bCommitted;
    return bCommitted;
}

// The area stack. STRUCTURAL edits (reorder/delete) still MUTATE NOTHING while drawing: the signal
// is applied by the caller after the list closes (the v1 erase-while-iterating defect). STEP110:
// each row's own settings now draw inside that row's own body, gated on the row's own expand state
// — never on `state.selectedAreaIndex` — so an expanded row can never show another row's settings.
DraggableListSignal DrawAreaList(std::vector<Params::MapArea>& areas, AreasTabState& state,
                                 int mapSize, std::vector<AreaColorEntry>& areaColors, bool& bAreasMoved) {
    return DraggableList<Params::MapArea>::Render(
        "areas", areas,
        [&](int rowIndex) {
            const Params::MapArea& area = areas[static_cast<std::size_t>(rowIndex)];
            DraggableListRow row;
            row.label   = AreaRowLabel(area);
            // STEP212 — the row's [U]/[L] icon now shows and drives the REAL per-area lock (the
            // canvas's own MapCanvas::IsAreaLocked reads this exact same table). Replaces the old
            // `!IsAreaRemovable(area)` stand-in, which conflated "cannot be deleted" with "cannot be
            // dragged" — two different concepts that happened to coincide for PlayableArea alone.
            // PlayableArea's DELETE protection is untouched (still IsAreaRemovable-driven, in
            // ApplyAreaListSignal below); only its LOCK display/toggle now comes from this table,
            // defaulting locked on first resolve exactly like any other pre-existing area.
            row.bLocked = *ResolveAreaLocked(state.areaLocks, area.name);
            return row;
        },
        [&](int rowIndex) {
            Params::MapArea& area = areas[static_cast<std::size_t>(rowIndex)];
            bAreasMoved = DrawAreaSettings(area, state, mapSize, areaColors) || bAreasMoved;
        },
        state.selectedAreaIndex);
}

// AFFORDANCE SCOPE: an area owns no visibility bit, so ToggleVisibility is still ignored (ARCH §4).
// STEP212 — ToggleLock is NO LONGER ignored: it flips the per-area lock entry the row's own [U]/[L]
// icon now displays (DrawAreaList above). Delete is refused on the engine-required PlayableArea,
// exactly as v1 refused it. Reports whether the list actually moved (a lock flip is presentation
// state — it does not move the recipe and has no composite-side reader at all, so it is deliberately
// NOT folded into the `true` this function can return for other signals; it trips no recompose and
// needs no PreviewDriver notify).
bool ApplyAreaListSignal(std::vector<Params::MapArea>& areas, AreasTabState& state,
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
    if (signal.kind == DraggableListSignalKind::ToggleVisibility) return false;
    if (signal.kind == DraggableListSignalKind::Delete
        && (!bRowValid || !IsAreaRemovable(areas[static_cast<std::size_t>(rowIndex)])))
        return false;
    if (!ApplyDraggableListSignal(areas, signal)) return false;
    state.selectedAreaIndex = ResolvedAreaSelection(state.selectedAreaIndex,
                                                   static_cast<int>(areas.size()));
    return true;
}

// Add New Area. STEP212 — the tab-wide "Lock Areas" checkbox this function used to also draw is
// RETIRED: superseded, not replaced 1:1, by the DraggableList row's own per-area [U]/[L] icon, which
// is the correct place to lock/unlock now that the concept is per-area (see this ticket's own
// "Interpretation calls made" for why no bulk "Lock All" control replaces it).
bool DrawAreasGlobals(std::vector<Params::MapArea>& areas, AreasTabState& state) {
    if (!DrawSectionBegin("Areas", state.globalSection)) return false;
    ImGui::TextWrapped("Areas are named rectangles exported beside the terrain. The map canvas can "
                       "drag and resize an area unless it is individually locked (the Area Stack "
                       "list's own lock icon, below).");
    bool bAreasMoved = false;
    if (ImGui::Button("Add New Area")) {
        Params::MapArea area;
        area.name   = NextAreaName(static_cast<int>(areas.size()));
        // Params::MapArea's own defaults are 0/0 (correct for "absent from an import degrades to
        // nothing" — Constitution §6), but a freshly authored row needs to be visible and usable
        // (STEP21 ruling #7) — mirrors the name already being explicitly set above.
        area.width  = 100.0f;
        area.length = 100.0f;
        areas.push_back(area);
        state.selectedAreaIndex = static_cast<int>(areas.size()) - 1;
        // STEP212 — the human's own explicit rule: a freshly created area starts UNLOCKED. Inserted
        // here, eagerly, using the name just assigned above — see "Interpretation calls made," item
        // 7, for the narrow, accepted collision-rename exposure this shares with `areaColors`.
        ResolveAreaLocked(state.areaLocks, area.name, /*bDefaultLocked=*/false);
        bAreasMoved = true;
    }
    DrawSectionEnd();
    return bAreasMoved;
}

} // namespace

void DrawAreasTab(Params::MapRecipe& recipe, AreasTabState& state,
                  Pipeline::PreviewDriver* previewDriver, std::vector<AreaColorEntry>& areaColors) {
    ImGui::PushID("areasTab");
    const int mapSize = recipe.geometry.mapSize;
    bool bAreasMoved = EnsurePlayableArea(recipe.areas, mapSize);
    bAreasMoved = DrawAreasGlobals(recipe.areas, state) || bAreasMoved;
    if (DrawSectionBegin("Area Stack", state.areaSection)) {
        const DraggableListSignal signal = DrawAreaList(recipe.areas, state, mapSize, areaColors, bAreasMoved);
        if (signal.bHasSignal()) bAreasMoved = ApplyAreaListSignal(recipe.areas, state, signal) || bAreasMoved;
        DrawSectionEnd();
    }
    // The export keys areas by NAME, so the duplicate repair runs on the frames a name settled —
    // not every frame, which would rename a row mid-typing.
    if (bAreasMoved) MakeNamesUnique(recipe.areas);
    NotifyPlacementChange(bAreasMoved, previewDriver);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
```

(`DrawAreasTab`'s signature — including its STEP211 `std::vector<AreaColorEntry>& areaColors` parameter
and its `Application_PanelEnvironment_UI.cpp` call site passing `composite.Settings().areaColors` — is
completely unchanged by this ticket; lock lives inside `AreasTabState` itself, needing no new parameter.)

---

## 5. Modified: `src/ui/MapCanvas_ManualDragSources_UI.h`

Add one new `#include` and replace the `ManualAreaDragSources_UI` struct (current lines 37-53):

```cpp
#include <vector>
#include "AreaColorTable_UI.h"         // AreaColorEntry — ARCH §14.17 item 9's retarget
#include "AreaDragGesture_UI.h"
#include "AreaLockTable_UI.h"          // AreaLockEntry — STEP212's new per-area lock side table
#include "InstanceDragGesture_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MapArea_PARAMS.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/PropInstance_PARAMS.h"
```

```cpp
// ARCH §21.8 — the Area canvas gesture's own injected-pointer bundle. `recipe.areas` is a flat
// vector with no group/transform/lock shape at all (§21.8 correction 1), so this does NOT mirror
// ManualPropDragSources_UI/ManualDecalDragSources_UI's own `InstanceDragGestureState` — it carries
// the standalone AreaDragGestureState instead.
struct ManualAreaDragSources_UI {
    std::vector<Params::MapArea>* areas             = nullptr;   // mutable: canvas creates/moves/resizes
    std::vector<AreaColorEntry>*  areaColors         = nullptr;   // mutable: ResolveAreaColor lazily
                                                                    // appends a default entry for a
                                                                    // freshly canvas-created area
    // STEP212 — replaces the retired `const bool* bAreasLocked`: one lock bit PER AREA, the exact
    // same UI-only name-keyed side-table shape as `areaColors` above (AreaLockTable_UI.h's own
    // AreaLockEntry/ResolveAreaLocked, mirroring AreaColorTable_UI.h's AreaColorEntry/
    // ResolveAreaColor). Mutable (not read-only like the field it replaces) because
    // ResolveAreaLocked lazily appends a default-LOCKED entry on first touch, exactly as
    // ResolveAreaColor already does for areaColors, AND because CreateAreaFromDrag must insert a
    // freshly created area's own entry as UNLOCKED (STEP212 Fix 1) — the canvas now legitimately
    // writes into this table, unlike the plain bool it replaces. Unlike areaColors, this table has
    // NO composite-side reader at all (lock never affects what the GPU composite draws — only
    // whether the canvas gesture accepts input) — its single owner stays `AreasTabState::areaLocks`,
    // never `PreviewCompositeSettings`.
    std::vector<AreaLockEntry>*   areaLocks          = nullptr;
    int*                          selectedAreaIndex  = nullptr;   // mutable: auto-select-on-touch/deselect
    // ARCH §14.17 item 11 — mutable: the canvas sets/clears this to omit the dragged area from the
    // composite input for the duration of a gesture. Points at
    // `PreviewCompositeSettings::mapAreaSuppressedIndex` — one source of truth, never a second copy.
    // STEP212 — untouched; this field's own plumbing is STEP211 territory.
    int*                          mapAreaSuppressedIndex = nullptr;
    AreaDragGestureState           state;
};
```

(`ManualPropDragSources_UI`/`ManualDecalDragSources_UI` and this file's own top-of-file comment are
unmodified.)

---

## 6. Modified: `src/ui/MapCanvas_UI.h`

**Setter** — replace `SetManualAreaDragSource` (current lines 155-168) with:

```cpp
    // ARCH §21.8 / STEP212 — mirrors SetManualPropDragSource's shape minus Geometry/
    // globalSymmetryRecipe (Areas carry no symmetry/layer concept of their own, §21.8 correction
    // 1/3). `areas`/`areaColors`/`areaLocks`/`selectedAreaIndex` are all mutable: STEP212 replaces
    // the retired, read-only, tab-wide `const bool* areasLocked` with a per-area lock TABLE the
    // canvas now legitimately writes into too (CreateAreaFromDrag inserts a freshly created area's
    // own entry as unlocked; every ordinary lazy resolve elsewhere still defaults locked) — exactly
    // ResolveAreaColor's own already-established mutable-pointer shape for `areaColors` above.
    // ARCH §14.17 item 11 — a fifth parameter, `mapAreaSuppressedIndex`, lets the canvas set/clear the
    // composite's transient drag-suppression slot WITHOUT reaching through the canvas's own `const
    // PreviewComposite* composite` (deliberately const — the canvas never composites, see below).
    // STEP212 leaves this fifth parameter and its own plumbing completely untouched.
    void SetManualAreaDragSource(std::vector<Params::MapArea>* areas, std::vector<AreaColorEntry>* areaColors,
                                  std::vector<AreaLockEntry>* areaLocks, int* selectedAreaIndex,
                                  int* mapAreaSuppressedIndex) {
        manualAreaDrag.areas = areas; manualAreaDrag.areaColors = areaColors;
        manualAreaDrag.areaLocks = areaLocks; manualAreaDrag.selectedAreaIndex = selectedAreaIndex;
        manualAreaDrag.mapAreaSuppressedIndex = mapAreaSuppressedIndex;
    }
```

**Private method declarations** — insert `IsAreaLocked` immediately after `AreaGestureEligible()`'s
existing declaration (current line 340):

```cpp
    bool AreaGestureEligible() const;                                          // MapCanvas_AreaDragDispatch_UI.cpp
    // STEP212 — the per-area lock query AreaGestureEligible() no longer performs itself (lock is now
    // per-area, keyed by name, not a single tab-wide bool) — every canvas-side gate that used to read
    // the old `manualAreaDrag.bAreasLocked` (both TryBeginAreaDrag and DrawAreaOverlayPass's
    // cursor-shape section) now calls this instead, at the point it knows WHICH area. Missing
    // sources or an out-of-range index answer locked (Constitution §6 — refuse, never silently
    // permit), mirroring AreaGestureEligible's own existing null-refuses posture in the same file.
    bool IsAreaLocked(int areaIndex) const;                                    // MapCanvas_AreaDragDispatch_UI.cpp
    bool TryBeginAreaDrag(float regionLocalX, float regionLocalY);             // ditto
```

No other line in this file changes — `ManualAreaDragSources_UI manualAreaDrag;`/`bool bAreaDragActive`/
`std::function<void()> areaCompositeRefreshCallback;` (current lines 427-434) and every other
member/method are unmodified. No new `#include` is needed — `AreaLockEntry` is already visible
transitively through this file's existing `#include "MapCanvas_ManualDragSources_UI.h"` (§5 above).

---

## 7. Modified: `src/ui/MapCanvas_AreaDragDispatch_UI.cpp`

Full file:

```cpp
// MapCanvas_AreaDragDispatch_UI.cpp — MapCanvas::AreaGestureEligible/IsAreaLocked/TryBeginAreaDrag/
// ContinueAreaDrag/EndAreaDrag/CreateAreaFromDrag (ARCH §21.8), plus SetMapAreaSuppression
// (ARCH §14.17 item 11's exactly-two-recomposites-per-gesture rule). STEP212 adds the per-area lock
// query (IsAreaLocked) and gates TryBeginAreaDrag's two hit-test steps on it — the suppression/
// recomposite mechanism below is STEP211 territory and is otherwise byte-identical. Standalone
// sibling of MapCanvas_ManualDragDispatch_UI.cpp's 3-way Markers/Props/Decals dispatcher — Areas has
// no group/transform/lock shape to fit into that dispatcher's switch (§21.8 correction 1/5).
#include "MapCanvas_UI.h"
#include "AreasTab_List_UI.h"       // NextAreaName, MakeNamesUnique, ResolveAreaLocked
#include "PreviewComposite_UI.h"
#include <algorithm>
#include <cmath>

namespace SanmapGen {
namespace Ui {

// STEP212 — the Areas-panel-active gate ONLY. The lock check this function used to also perform
// (`!*manualAreaDrag.bAreasLocked`) is retired: lock is now per-area, so it cannot be answered until
// a specific area is known — every call site below asks IsAreaLocked(index) once it has one.
bool MapCanvas::AreaGestureEligible() const {
    return activePanelSource != nullptr && *activePanelSource == ApplicationPanel::Areas;
}

// STEP212 — missing sources or an out-of-range index refuse (answer locked), never silently permit
// (Constitution §6 — the same "null/false-safe refuses" posture AreaGestureEligible's own panel
// gate already uses). Declared `const`: it mutates only the POINTEE of `manualAreaDrag.areaLocks`
// (a lazy append, exactly `ResolveAreaColor`'s own already-established precedent elsewhere in this
// class, e.g. this file's own SetMapAreaSuppression neighbor and DrawAreaOverlayPass), never a
// member of `*this`. Reused verbatim by DrawAreaOverlayPass's cursor-shape section
// (MapCanvas_AreaDraw_UI.cpp) — one lock query, not two independently-maintained checks.
bool MapCanvas::IsAreaLocked(int areaIndex) const {
    if (manualAreaDrag.areas == nullptr || manualAreaDrag.areaLocks == nullptr) return true;
    if (areaIndex < 0 || areaIndex >= static_cast<int>(manualAreaDrag.areas->size())) return true;
    const Params::MapArea& area = (*manualAreaDrag.areas)[static_cast<std::size_t>(areaIndex)];
    return *ResolveAreaLocked(*manualAreaDrag.areaLocks, area.name);
}

// ARCH §14.17 item 11 — the ONE place the "did the suppressed index actually change" condition is
// evaluated, shared by every call site below (an index, never a `bEnabled` toggle: flipping
// `fieldLayers[i].bEnabled` for a drag's duration would clobber the user's own View-popup/left-column
// enable state, which this dedicated slot cannot collide with). STEP212 — untouched.
void MapCanvas::SetMapAreaSuppression(int areaIndex) {
    if (manualAreaDrag.mapAreaSuppressedIndex == nullptr) return;
    if (*manualAreaDrag.mapAreaSuppressedIndex == areaIndex) return;
    *manualAreaDrag.mapAreaSuppressedIndex = areaIndex;
    if (areaCompositeRefreshCallback) areaCompositeRefreshCallback();
}

bool MapCanvas::TryBeginAreaDrag(float regionLocalX, float regionLocalY) {
    bAreaDragActive = false;
    if (!AreaGestureEligible()) return false;
    if (manualAreaDrag.areas == nullptr || composite == nullptr) return false;
    std::vector<Params::MapArea>& areas = *manualAreaDrag.areas;

    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));

    // Step 1 — a selection exists AND is unlocked: hit-test THAT one area's own 8 handles + body
    // first. STEP212: the old single upfront AreaGestureEligible() lock check is replaced by this
    // per-area IsAreaLocked() test, now that the lock is per-name, not a tab-wide bool.
    if (manualAreaDrag.selectedAreaIndex != nullptr) {
        const int selectedIndex = *manualAreaDrag.selectedAreaIndex;
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(areas.size())
            && !IsAreaLocked(selectedIndex)) {
            const AreaHandle_UI handle = HitTestAreaHandles(areas[static_cast<std::size_t>(selectedIndex)],
                                                             *composite, view, regionLocalX, regionLocalY);
            if (handle != AreaHandle_UI::None) {
                bAreaDragActive = BeginAreaDragGesture(manualAreaDrag.state, areas, selectedIndex, handle,
                                                       worldPoint.worldX, worldPoint.worldZ);
                // ARCH §14.17 item 11 — the FIRST of exactly two recomposites this gesture will cost.
                if (bAreaDragActive) SetMapAreaSuppression(selectedIndex);
                return bAreaDragActive;
            }
        }
    }

    // Step 2 — a miss on the selected area's own handles/body (or it was locked and so never
    // tested): body hit-test over EVERY UNLOCKED area, forward iteration, last match wins
    // (later-in-vector is drawn topmost, Widget_AreaEditor.cpp's own "reverse Z-order" comment,
    // Widget_AreaEditor.cpp:50). STEP212 interpretation call 1: a LOCKED area is excluded from this
    // scan entirely — it stays fully inert to canvas hit-testing, including re-selection, exactly
    // mirroring ARCH_21_08's own pre-STEP212 ruling ("Locked gates the whole surface, uniformly,
    // including selection-by-click... Selecting a different area while locked is still possible
    // through the Area Stack list... unaffected"), just applied per-area instead of tab-wide. A
    // locked area remains selectable only through the Area Stack list's own Select signal
    // (AreasTab_UI.cpp's ApplyAreaListSignal), untouched by this canvas-side gate.
    int hitIndex = -1;
    for (int index = 0; index < static_cast<int>(areas.size()); ++index)
        if (!IsAreaLocked(index)
            && IsWorldPointInsideArea(areas[static_cast<std::size_t>(index)], worldPoint.worldX, worldPoint.worldZ))
            hitIndex = index;
    if (hitIndex < 0) return false;   // total miss (or every candidate locked) — release resolves click/create

    if (manualAreaDrag.selectedAreaIndex != nullptr) *manualAreaDrag.selectedAreaIndex = hitIndex;
    bAreaDragActive = BeginAreaDragGesture(manualAreaDrag.state, areas, hitIndex, AreaHandle_UI::Center,
                                           worldPoint.worldX, worldPoint.worldZ);
    if (bAreaDragActive) SetMapAreaSuppression(hitIndex);
    return bAreaDragActive;
}

void MapCanvas::ContinueAreaDrag(float regionLocalX, float regionLocalY, bool bShiftHeld, bool bCtrlHeld) {
    if (!bAreaDragActive || manualAreaDrag.areas == nullptr || composite == nullptr) return;
    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));
    // ARCH §21.8 correction 4 / §14.17 item 11 — writes recipe.areas LIVE every frame and requests
    // ZERO recomposites: the live visual is the bespoke immediate-mode pass in
    // MapCanvas_AreaDraw_UI.cpp, not a GPU recompose. STEP212 — untouched.
    UpdateAreaDragGesture(manualAreaDrag.state, *manualAreaDrag.areas, worldPoint.worldX, worldPoint.worldZ,
                          bShiftHeld, bCtrlHeld);
}

void MapCanvas::EndAreaDrag() {
    EndAreaDragGesture(manualAreaDrag.state);
    bAreaDragActive = false;
    // ARCH §14.17 item 11 — the SECOND of exactly two recomposites: the area rejoins the composite
    // input now that the gesture is over. STEP212 — untouched.
    SetMapAreaSuppression(-1);
}

void MapCanvas::CreateAreaFromDrag(float pressRegionLocalX, float pressRegionLocalY,
                                   float releaseRegionLocalX, float releaseRegionLocalY) {
    if (manualAreaDrag.areas == nullptr || composite == nullptr) return;
    const PreviewPixelCoordinate pressPixel = view.ResolvePreviewPixel(pressRegionLocalX, pressRegionLocalY);
    const PreviewPixelCoordinate releasePixel = view.ResolvePreviewPixel(releaseRegionLocalX, releaseRegionLocalY);
    const PreviewComposite::PreviewWorldPoint pressWorld = composite->PreviewPixelToWorld(
        static_cast<float>(pressPixel.pixelX), static_cast<float>(pressPixel.pixelY));
    const PreviewComposite::PreviewWorldPoint releaseWorld = composite->PreviewPixelToWorld(
        static_cast<float>(releasePixel.pixelX), static_cast<float>(releasePixel.pixelY));

    Params::MapArea area;
    area.originX = std::min(pressWorld.worldX, releaseWorld.worldX);
    area.originZ = std::min(pressWorld.worldZ, releaseWorld.worldZ);
    area.width   = std::max(kAreaMinimumExtentWorldUnits, std::fabs(releaseWorld.worldX - pressWorld.worldX));
    area.length  = std::max(kAreaMinimumExtentWorldUnits, std::fabs(releaseWorld.worldZ - pressWorld.worldZ));
    area.name = NextAreaName(static_cast<int>(manualAreaDrag.areas->size()));   // AreasTab_List_UI.h,
                                                                                 // the SAME helper "Add New Area" uses
    manualAreaDrag.areas->push_back(area);
    MakeNamesUnique(*manualAreaDrag.areas);   // called HERE, not left for DrawAreasTab's end-of-frame
                                               // call — see ARCH §21.8's own "Create-by-drag" section
    const int newIndex = static_cast<int>(manualAreaDrag.areas->size()) - 1;
    // STEP212 — the human's own explicit rule: a freshly created area starts UNLOCKED. Reads the
    // area's FINAL (post-MakeNamesUnique) name back out of the vector rather than reusing the local
    // `area` copy's own name — the local copy predates whatever rename a collision would have
    // applied, so using it here could silently key the lock entry to a name nothing in `areas` uses.
    if (manualAreaDrag.areaLocks != nullptr)
        ResolveAreaLocked(*manualAreaDrag.areaLocks,
                          (*manualAreaDrag.areas)[static_cast<std::size_t>(newIndex)].name,
                          /*bDefaultLocked=*/false);
    if (manualAreaDrag.selectedAreaIndex != nullptr)
        *manualAreaDrag.selectedAreaIndex = newIndex;
    // ARCH §14.17 item 11 — a brand-new area must appear: one recomposite, no suppression change.
    // STEP212 — untouched.
    if (areaCompositeRefreshCallback) areaCompositeRefreshCallback();
}

} // namespace Ui
} // namespace SanmapGen
```

---

## 8. Modified: `src/ui/MapCanvas_AreaDraw_UI.cpp`

Full file (Fix 2 — the cursor-shape section gains a lock check via the `IsAreaLocked` member method
declared in §6/implemented in §7; the fill/border/handle-circle logic — the entire STEP211 suppressed-
index rewrite — is byte-identical to today's shipped version):

```cpp
// MapCanvas_AreaDraw_UI.cpp — MapCanvas::DrawAreaOverlayPass. Fill/border/handles per
// ARCH_21_08_AreaCanvasGesture.md's 2026-08-29 amendment / ARCH_14_17_MapAreaFieldLayer.md §14.17
// item 12 (STEP211, unchanged by this ticket): the FILL is the composite's own job in the steady
// state (PreviewLayerKind::MapAreas) — this pass draws the fill ONLY for the one area currently
// suppressed from the composite (the one being dragged/resized/moved). The BORDER draws only when
// the MapAreas layer is enabled AND this is the suppressed area AND it is selected. The 8 handles
// (selected-area only) are unchanged. STEP212 — the hover-only cursor-shape feedback now gates on
// the SELECTED area's own per-area lock (MapCanvas::IsAreaLocked, MapCanvas_AreaDragDispatch_UI.cpp):
// while locked, no cursor override at all — the pre-STEP212 version hit-tested handles and set a
// resize cursor purely from proximity, regardless of lock, which is the exact "cursor lies" bug this
// ticket fixes (a locked area's handles showed a resize cursor even though TryBeginAreaDrag silently
// refused the click).
#include "MapCanvas_UI.h"
#include "AreasTab_List_UI.h"       // ResolveAreaColor
#include "PreviewComposite_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

// §14.17 item 12's own preferred path: read (a) through the canvas's existing `const
// PreviewComposite*` — it needs no new plumbing at all, since `Settings()` already has a const
// overload. A plain linear scan (not `PreviewFieldLayerOfKind`, which has no const overload) since
// this is the one place a const settings reference is available.
bool IsMapAreasLayerEnabled(const PreviewCompositeSettings& settings) {
    for (const PreviewFieldLayer& layer : settings.fieldLayers)
        if (layer.kind == PreviewLayerKind::MapAreas) return layer.bEnabled;
    return false;
}

} // namespace

void MapCanvas::DrawAreaOverlayPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualAreaDrag.areas == nullptr) return;
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const std::vector<Params::MapArea>& areas = *manualAreaDrag.areas;
    const int selectedIndex = manualAreaDrag.selectedAreaIndex != nullptr ? *manualAreaDrag.selectedAreaIndex : -1;
    const int suppressedIndex = manualAreaDrag.mapAreaSuppressedIndex != nullptr
                              ? *manualAreaDrag.mapAreaSuppressedIndex : -1;

    auto ToScreen = [&](float worldX, float worldZ) {
        const PreviewComposite::PreviewPixelPoint pixel = composite->WorldToPreviewPixel(worldX, worldZ);
        const RegionLocalPoint local = view.ProjectPreviewPixelToRegionLocal(pixel.pixelX, pixel.pixelY);
        return ImVec2(regionOriginX + local.regionLocalX, regionOriginY + local.regionLocalY);
    };

    // ARCH §14.17 item 12 — fill + (conditional) border, ONLY for the suppressed area. Unchanged.
    if (suppressedIndex >= 0 && suppressedIndex < static_cast<int>(areas.size())) {
        const Params::MapArea& area = areas[static_cast<std::size_t>(suppressedIndex)];
        const ImVec2 nwScreen = ToScreen(area.originX, area.originZ);
        const ImVec2 seScreen = ToScreen(area.originX + area.width, area.originZ + area.length);

        const float* const color = manualAreaDrag.areaColors != nullptr
            ? ResolveAreaColor(*manualAreaDrag.areaColors, area.name) : nullptr;
        const ImU32 fillColor = color != nullptr
            ? ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]))
            : ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.35f));
        drawList->AddRectFilled(nwScreen, seScreen, fillColor);

        if (IsMapAreasLayerEnabled(composite->Settings()) && suppressedIndex == selectedIndex)
            drawList->AddRect(nwScreen, seScreen, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));
    }

    // The 8 handles — selected-area-only. Unchanged.
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(areas.size())) {
        const Params::MapArea& area = areas[static_cast<std::size_t>(selectedIndex)];
        AreaHandleWorldPoint_UI handlePoints[8];
        ComputeAreaHandleWorldPoints(area, handlePoints);
        const ImU32 handleColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        for (const AreaHandleWorldPoint_UI& handlePoint : handlePoints)
            drawList->AddCircleFilled(ToScreen(handlePoint.worldX, handlePoint.worldZ),
                                      kAreaHandleScreenRadiusPixels, handleColor);
    }

    // Cursor-shape feedback — hover-only, re-hit-tested fresh against the CURRENT cursor position —
    // gated on the cursor being within the canvas region (view.RegionSidePixels(), not
    // ImGui::IsItemHovered(), since this pass runs before this frame's InvisibleButton is declared).
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(areas.size())) return;
    // STEP212 fix — a LOCKED selected area shows no drag-affordance cursor at all: falls through to
    // imgui's own default arrow (no ImGuiMouseCursor_NotAllowed substitution — see this ticket's own
    // "Interpretation calls made," item 3, for why). Reuses IsAreaLocked, the SAME query
    // TryBeginAreaDrag itself gates on (MapCanvas_AreaDragDispatch_UI.cpp) — one lock check, not a
    // second, independently-maintained one.
    if (IsAreaLocked(selectedIndex)) return;
    const ImGuiIO& io = ImGui::GetIO();
    const float hoverRegionLocalX = io.MousePos.x - regionOriginX;
    const float hoverRegionLocalY = io.MousePos.y - regionOriginY;
    const float regionSide = view.RegionSidePixels();
    if (hoverRegionLocalX < 0.0f || hoverRegionLocalY < 0.0f
        || hoverRegionLocalX > regionSide || hoverRegionLocalY > regionSide) return;

    const AreaHandle_UI hoveredHandle = HitTestAreaHandles(areas[static_cast<std::size_t>(selectedIndex)],
                                                           *composite, view, hoverRegionLocalX, hoverRegionLocalY);
    switch (hoveredHandle) {
        case AreaHandle_UI::N: case AreaHandle_UI::S: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS); break;
        case AreaHandle_UI::E: case AreaHandle_UI::W: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); break;
        case AreaHandle_UI::NE: case AreaHandle_UI::SW: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW); break;
        case AreaHandle_UI::NW: case AreaHandle_UI::SE: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE); break;
        case AreaHandle_UI::Center: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll); break;
        default: break;
    }
}

} // namespace Ui
} // namespace SanmapGen
```

---

## 9. Modified: `src/ui/Application_UI.cpp`

Replace the existing `SetManualAreaDragSource` call (current lines 143-148):

```cpp
    // ARCH §21.8 / §14.17 item 9/11 / STEP212 — the Area canvas gesture's drag source: `recipe.areas`,
    // `composite.Settings().areaColors`, `tabState.areas.areaLocks` and
    // `composite.Settings().mapAreaSuppressedIndex` are the SAME storage the tab/composite already
    // own — one source of truth, never a second copy. `areaLocks` replaces the retired
    // `&tabState.areas.bAreasLocked` (STEP212 — per-area lock); it stays TAB-owned, unlike
    // `areaColors`, because lock has no composite-side reader (AreaLockTable_UI.h's own ruling).
    canvas.SetManualAreaDragSource(&recipe.areas, &composite.Settings().areaColors,
                                   &tabState.areas.areaLocks, &tabState.areas.selectedAreaIndex,
                                   &composite.Settings().mapAreaSuppressedIndex);
```

No other line in this function changes — every other `canvas.Set*Source` call, and the unrelated
`canvas.SetAreaCompositeRefreshCallback([this] { previewDriver.NotifyParametersChanged(); });` a few
lines below it, are byte-identical to today's tree.

---

## 10. Modified: `src/ui/AreasTab_UI_Test.cpp`

**Remove** the now-non-compiling lines from `RunSliderAndSelectionChecks` (current lines 117-118):

```cpp
    const AreasTabState state;
    Check(state.bAreasLocked, "the tab opens locked, as v1 did");
```

(delete both lines outright — no replacement needed inside this function; the rest of
`RunSliderAndSelectionChecks`, up through the `AreasTabColorSwatchOptions` check immediately above, is
unchanged, and the function now ends there.)

**Add**, immediately after the existing `RunColorRenameRetargetingChecks` function (current lines
143-161), two new functions mirroring its own and `RunAreaColorResolutionChecks`' style exactly, one
table over:

```cpp
// STEP212: the per-area lock replaces the retired `AreasTabState::bAreasLocked`. `ResolveAreaLocked`
// mirrors `ResolveAreaColor`'s own lazy-append idiom, but must answer two different questions with
// two different defaults depending on the call site (an existing area vs. a freshly created one) —
// its own `bDefaultLocked` parameter is what this function exists to exercise.
void RunAreaLockResolutionChecks() {
    std::vector<AreaLockEntry> areaLocks;
    bool* const firstResolve = ResolveAreaLocked(areaLocks, "Base");
    Check(areaLocks.size() == 1u, "the first touch of a name appends one entry");
    Check(*firstResolve, "an ordinary (default-argument) resolve defaults LOCKED - matches the "
                        "retired global bAreasLocked's own default");

    *firstResolve = false;
    bool* const secondResolve = ResolveAreaLocked(areaLocks, "Base");
    Check(areaLocks.size() == 1u, "resolving the same name again appends nothing");
    Check(!*secondResolve, "and returns the SAME entry, edits intact");

    ResolveAreaLocked(areaLocks, "Other");
    Check(areaLocks.size() == 2u, "a different name gets its own entry");

    // The human's own explicit rule: a freshly created area starts UNLOCKED. The two creation call
    // sites (AreasTab_UI.cpp's Add New Area, MapCanvas_AreaDragDispatch_UI.cpp's CreateAreaFromDrag)
    // both pass bDefaultLocked=false explicitly for a name this table has never seen before.
    bool* const freshCreationResolve = ResolveAreaLocked(areaLocks, "FreshlyCreated", /*bDefaultLocked=*/false);
    Check(!*freshCreationResolve, "a name resolved with bDefaultLocked=false starts UNLOCKED");
    Check(areaLocks.size() == 3u, "and still only appends the one new entry");

    // Once an entry exists, a LATER resolve's own bDefaultLocked argument is irrelevant - the
    // existing value always wins, never silently re-defaulted.
    bool* const secondTouchIgnoresDefault = ResolveAreaLocked(areaLocks, "FreshlyCreated", /*bDefaultLocked=*/true);
    Check(!*secondTouchIgnoresDefault,
          "a second resolve's own default argument never overwrites an already-existing entry");
}

// Mirrors RunColorRenameRetargetingChecks exactly, one table over: AreasTab_UI.cpp's
// DrawAreaSettings now retargets BOTH the color entry and the lock entry on a committed rename.
void RunLockRenameRetargetingChecks() {
    std::vector<AreaLockEntry> areaLocks;
    bool* const originalLock = ResolveAreaLocked(areaLocks, "Base", /*bDefaultLocked=*/false);
    *originalLock = false;

    const std::string nameBeforeEdit = "Base";
    const std::string nameAfterEdit  = "Renamed";
    for (AreaLockEntry& entry : areaLocks)
        if (entry.name == nameBeforeEdit) { entry.name = nameAfterEdit; break; }

    Check(areaLocks.size() == 1u,
          "the rename retargets the existing entry in place rather than orphaning it");
    bool* const resolvedAfterRename = ResolveAreaLocked(areaLocks, nameAfterEdit);
    Check(areaLocks.size() == 1u,
          "resolving under the NEW name finds the retargeted entry - it does not create a second");
    Check(!*resolvedAfterRename,
          "the unlocked value survives the rename - not silently reset to the LOCKED default");
}
```

**Update** `main()` (current lines 183-194) to call both:

```cpp
int main() {
    RunPlayableAreaChecks();
    RunSetToMapSizeChecks();
    RunUniqueNameChecks();
    RunSliderAndSelectionChecks();
    RunAreaColorResolutionChecks();
    RunColorRenameRetargetingChecks();
    RunAreaLockResolutionChecks();
    RunLockRenameRetargetingChecks();
    RunFreshAreaSizeChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
```

No new `#include` needed — `AreaLockEntry`/`ResolveAreaLocked` are already visible transitively through
this file's existing `#include "AreasTab_UI.h"`, the same reason `AreaColorEntry`/`ResolveAreaColor` need
none today. This test binary is already registered in `CMakeLists.txt`
(`add_sangen_test(AreasTab_UI_Test src/ui/AreasTab_UI_Test.cpp)`) — no CMake change needed.

---

## ARCH rules invoked

- `ARCH_21_08_AreaCanvasGesture.md` (§21.8), original ruling AND its 2026-08-29 amendment — the binding
  law for the whole Area canvas-gesture surface this ticket narrowly amends: every piece of gesture math
  (`HitTestAreaHandles`, `Begin/Update/EndAreaDragGesture`, handle-priority-over-body, create-by-drag) is
  unmodified — this ticket only changes WHETHER a gesture is allowed to begin (the lock gate) and the
  cursor feedback layered on top of it. Its "Locked gates the whole surface, uniformly, including
  selection-by-click... Selecting a different area while locked is still possible through the Area Stack
  list" ruling is the direct precedent Fix 1's per-area re-application follows.
- `ARCH_14_17_MapAreaFieldLayer.md` (§14.17) — item 9 is the direct precedent for splitting a
  presentation-state side table into its own minimal header when a different owner needs a narrower
  dependency footprint (this ticket's `AreaLockTable_UI.h`, one tier over from `AreaColorTable_UI.h`);
  item 11's suppression/refresh-callback mechanism and item 12's border rule are untouched by this ticket
  and are re-stated here only to confirm they were read and preserved, never re-derived.
- STEP21 ruling #4 — presentation state with no `_PARAMS` home lives in a UI-only side table keyed by
  `MapArea::name`, not vector position — the precedent this ticket's new `AreaLockEntry`/
  `ResolveAreaLocked` follows verbatim, for the lock bit, exactly as `AreaColorEntry`/`ResolveAreaColor`
  already do for color.
- STEP21 ruling #5 — the rename-retargeting rule (a committed name edit must retarget every name-keyed
  side-table entry, or it silently reverts) — extended in this ticket to the new lock table alongside the
  existing color table, in the same place (`DrawAreaSettings`) the color repair already runs.
- Constitution §1 — UI sets PARAMS, never simulates; the lock bit is presentation state, never written
  into `Params::MapArea` (no schema change).
- Constitution §6 — an untrusted/missing source refuses rather than silently permits: `IsAreaLocked`
  refuses (answers locked) on a null `areaLocks`/`areas` pointer or an out-of-range index, mirroring
  `AreaGestureEligible`'s own existing null-refuses posture in the same file.
- Constitution §8 — no new hidden literal: `ResolveAreaLocked`'s `bDefaultLocked` argument is named by a
  `/*bDefaultLocked=*/` comment at every non-default call site, matching this codebase's own established
  convention used elsewhere.

## Explicit out-of-scope

- **STEP211's field-layer/GPU-composite/drag-suppression mechanism** — `PreviewLayerKind::MapAreas`, the
  `PreviewMapAreaRectangle` buffer, `BuildMapAreaConfigurations`, `mapAreaSuppressedIndex`,
  `SetAreaCompositeRefreshCallback`, `SetMapAreaSuppression`, and the fill/border rules in
  `MapCanvas_AreaDraw_UI.cpp` beyond the new lock gate on the cursor section — all untouched, all
  byte-identical to today's shipped tree.
- **No `Params::MapArea` schema change** — the lock stays UI-only presentation state, the same precedent
  as color; not a wire-format field, not an export-visible concept, no `.sanmap` version bump.
- **`PreviewCompositeSettings` does not gain an `areaLocks` field.** Deliberately: lock has no
  composite-side reader (unlike color), so it stays tab-owned (`AreasTabState::areaLocks`) — see
  `AreaLockTable_UI.h`'s own header comment and "Interpretation calls made" item 6.
- **No bulk "Lock All"/"Unlock All" control** — considered and rejected in favor of the already-designed,
  now-wired per-row `[U]/[L]` icon (Interpretation call 2).
- **No change to `AreaDragGesture_UI.h`/`.cpp`'s pure resize/move/hit-test math** (STEP210's own algorithm)
  — completely untouched by this ticket.
- **No new `ImGuiMouseCursor` glyph** for the locked state — suppression only (Interpretation call 3).
- **No new `PlacementCollectionKind_UI` entry, no `OverlayInstanceKeySet_UI`/multi-select participation
  for Areas** — untouched, same exclusion §21.8 already ruled.
- **No restructuring of when `MakeNamesUnique` runs** for the tab's own "Add New Area" button (still
  deferred to `DrawAreasTab`'s end-of-frame call, unchanged) — see Interpretation call 7 for the narrow,
  accepted risk this leaves.

## Acceptance test

1. Existing `AreasTab_UI_Test`/`AreaDragGesture_UI_Test` binaries keep passing; the two new
   `RunAreaLockResolutionChecks`/`RunLockRenameRetargetingChecks` functions pass.
2. A fresh area created via the tab's "Add New Area" button reads UNLOCKED from `state.areaLocks`
   immediately (same frame), and its Area Stack list row shows `[U]`, not `[L]`.
3. A fresh area created via the canvas's create-by-drag gesture reads UNLOCKED from
   `manualAreaDrag.areaLocks` immediately (post-`MakeNamesUnique`), and can be handle-resized AND
   body-moved in the very same session with no separate unlock step.
4. Any area never explicitly touched (including a freshly imported project, and the engine-required
   `PlayableArea`) resolves LOCKED by default: a press-drag-release on its handles or body produces zero
   change to `recipe.areas[index]` and zero selection change.
5. Clicking a row's `[U]/[L]` icon in the Area Stack list flips exactly that area's lock (confirmed via
   `state.areaLocks`) and immediately (same frame) changes whether that area's canvas handles/body
   respond to a drag — no other area's lock is affected.
6. Hovering the handles of a LOCKED, currently-selected area shows the default arrow cursor, never a
   resize cursor; after unlocking it via the row icon, hovering the same handles shows the correct
   directional resize cursor (`ResizeNS`/`ResizeEW`/`ResizeNESW`/`ResizeNWSE`/`ResizeAll`) exactly as
   before this ticket.
7. Renaming an area via the tab's Name field retargets its lock entry to the new name — a subsequent
   lock-icon click after the rename still flips the SAME area's real entry, not a freshly-appended,
   default-locked duplicate under the old name.
8. A locked, selected area's fill (while suppressed/dragged — not applicable while genuinely locked,
   since a locked area can never begin a drag) and, in the ordinary steady state, its composite-driven
   field-layer fill and handle circles render exactly as before this ticket — STEP211's fill/border
   mechanism is completely unaffected by lock state; only the cursor affordance is gated.
9. Full `SanGenV2` build stays clean; every existing test continues to pass.

## Interpretation calls made

1. **A LOCKED area is fully inert to canvas hit-testing, including re-selection via a body click — not a
   narrower "select but don't drag" relaxation.** `TryBeginAreaDrag`'s Step 2 skips a locked area
   entirely in its body-hit scan (never reassigns `*selectedAreaIndex` to it), mirroring ARCH_21_08's own
   ruling for the retired global lock at per-area granularity rather than inventing a new, narrower
   behavior. A locked area is still reachable via the Area Stack list's own `Select` signal, unaffected.
2. **The tab-wide "Lock Areas" checkbox is REMOVED outright, not repurposed as a "Lock All"/"Unlock All"
   bulk button.** The per-row `[U]/[L]` icon was already drawn (just inert) before this ticket — every
   area was already individually reachable through it, so removing the checkbox loses no capability. A
   bulk action is a genuinely separate feature (batch-editing N rows in one click) the diagnosed bug
   never asked for; it can be proposed as its own future ticket if the human wants it.
3. **No new "locked" cursor glyph** (e.g. a not-allowed icon) is substituted when hovering a locked area's
   handles — it falls back to imgui's plain default arrow, matching this section's own established
   caution about depending on an unverified `ImGuiMouseCursor_` enum member. The handle-CIRCLE and
   composite fill/border rendering are unchanged regardless of lock — only the cursor-shape override is
   gated, matching this ticket's own narrower "Fix 2" framing.
4. **`ApplyAreaListSignal`'s `ToggleLock` case returns `false`** (never folds into `bAreasMoved`/trips
   `NotifyPlacementChange`), since a lock flip has NO composite-side reader at all (unlike a color
   commit, which DOES fold into `bAreasMoved` today, an existing, un-touched minor asymmetry this ticket
   does not also introduce for lock) — lock changes nothing about the composited/rendered result and
   needs no recompute signal whatsoever.
5. **`MapCanvas::IsAreaLocked` is declared `const`** even though it can mutate the pointee of
   `manualAreaDrag.areaLocks` (a lazy append) — the same "logically const, mutates through a pointer
   member, never `this`" posture this class's existing calls into `ResolveAreaColor` (from
   `DrawAreaOverlayPass`) already establish. It is called from BOTH
   `MapCanvas_AreaDragDispatch_UI.cpp` (the gate) and `MapCanvas_AreaDraw_UI.cpp` (the cursor fix) — one
   private method, one source of truth, never two independently-maintained lock checks.
6. **A new, dedicated `AreaLockTable_UI.h` header — not inline in `AreasTab_List_UI.h`, and NOT owned by
   `PreviewCompositeSettings`.** This is the explicit "make the call" the dispatching instruction asked
   for. Justification: color's extraction (STEP211/§14.17 item 9) was driven entirely by the GPU
   composite needing `areaColors` without inheriting a tab header's dependency weight; lock has the
   *opposite* profile — zero composite need, purely a canvas-gesture-gating concern — so moving it into
   `PreviewCompositeSettings` would add a field the composite class has no legitimate reason to read,
   purely for superficial symmetry with color. Keeping it out of `AreasTab_List_UI.h` (rather than
   folding it into that file directly) mirrors the same minimal-footprint discipline
   `MapCanvas_ManualDragSources_UI.h` already applies for `areaColors` — that file includes
   `AreaColorTable_UI.h` directly, not the heavier `AreasTab_List_UI.h`, and the new lock field follows
   the identical shape.
7. **`CreateAreaFromDrag` inserts its fresh lock entry using the area's name AFTER `MakeNamesUnique` has
   already run** (which this function already calls immediately, per STEP210) — reading the FINAL,
   collision-resolved name back out of the vector, not the pre-dedup local copy — avoiding an
   orphaned-entry defect the tab-side path (item 8) is left narrowly exposed to.
8. **The tab-side "Add New Area" button inserts its lock entry using the name assigned BEFORE
   `DrawAreasTab`'s own deferred, end-of-frame `MakeNamesUnique` call.** This is a narrow, accepted latent
   collision-rename risk, identical in kind to the pre-existing (never fixed) exposure `AreaColorEntry`
   already has for the exact same reason. Not fixed here because doing so would mean restructuring WHEN
   "Add New Area" calls `MakeNamesUnique` (out of this ticket's scope) to guard against a collision that
   `NextAreaName`'s own count-based naming makes practically unreachable in this call path alone.

## Key files read/cited while drafting

`D:\Projects\Sanctuary\Map Generator\ARCH_21_08_AreaCanvasGesture.md` (fresh full read, including its
2026-08-29 amendment), `D:\Projects\Sanctuary\Map Generator\ARCH_14_17_MapAreaFieldLayer.md` (fresh full
read), `D:\Projects\Sanctuary\Map Generator\src\ui\AreaColorTable_UI.h` (fresh full read),
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_List_UI.h` (fresh full read),
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_UI.h` (fresh full read),
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_UI.cpp` (fresh full read),
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_UI_Test.cpp` (fresh full read),
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.h` (fresh full read),
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_ManualDragSources_UI.h` (fresh full read),
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_AreaDragDispatch_UI.cpp` (fresh full read,
post-STEP211 shape confirmed),
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_AreaDraw_UI.cpp` (fresh full read, post-STEP211
suppressed-index rewrite confirmed),
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_UI.cpp` (fresh read of the wiring block, lines
100-190), `D:\Projects\Sanctuary\Map Generator\src\ui\DraggableListWidget_RowAffordances_UI.h` (confirmed
`row.bLocked`/`DraggableListSignalKind::ToggleLock` already implemented, live),
`D:\Projects\Sanctuary\Map Generator\src\ui\DraggableListWidget_UI.h`,
and `D:\Projects\Sanctuary\Map Generator\work_orders\STEP210_AreaCanvasGesture_UI.md` (read in full, this
ticket's structural/rigor template). The prior stale draft at
`D:\Projects\Sanctuary\Map Generator\work_orders\STEP212_AreaPerAreaLockAndCursorFix_UI.md` was read only
to confirm which parts of its reasoning (Interpretation calls 1-3, the STEP21 ruling #4/#5 citations)
still apply unchanged at the per-area-lock level, and to confirm it does NOT reflect STEP211's real
`AreaColorTable_UI.h`/`PreviewCompositeSettings::areaColors`/five-parameter
`SetManualAreaDragSource`/suppressed-index draw-pass shape — every line number, struct shape, and code
block in this rewritten ticket was independently re-verified against the live tree, not copied from that
stale draft.
