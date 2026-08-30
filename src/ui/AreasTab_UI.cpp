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
                      std::vector<AreaColorEntry>& areaColors,
                      std::vector<AreaVisibilityEntry>& areaVisibility) {
    const ScalarSliderRange originRange = AreaOriginSliderRange(mapSize);
    const ScalarSliderRange extentRange = AreaExtentSliderRange(mapSize);
    bool bCommitted = false;
    if (IsPlayableArea(area)) {
        ImGui::TextDisabled("PlayableArea is required by the engine: it cannot be renamed or removed.");
    } else {
        // Captured BEFORE the edit: if the name commits to something new, the color, lock, AND
        // visibility entries keyed on the OLD name must all be retargeted, or a rename silently
        // reverts the area's color to default, its lock to LOCKED, and its visibility to VISIBLE
        // next frame (STEP21 ruling #5 for color; STEP212 extends the same repair to the per-area
        // lock table; STEP223 extends it a third time, to the visibility table).
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
            // STEP223 — the same repair extended to STEP222's visibility table: without this, a
            // rename silently resets a hidden area back to default-visible on the next resolve.
            for (AreaVisibilityEntry& entry : areaVisibility)
                if (entry.name == nameBeforeEdit) { entry.name = area.name; break; }
        }
    }
    ImGui::TextUnformatted("X");
    ImGui::SameLine();
    bCommitted = DrawSliderScalarCompact("X Position", area.originX, originRange, state.originXToggle,
                                         kAreaScalarCompactTrackWidthPixels,
                                         kAreaScalarCompactFieldWidthPixels, WidgetStyle(), "%.0f",
                                         /*bShowRealtimeToggle=*/false).bCommitted || bCommitted;
    ImGui::SameLine();
    ImGui::TextUnformatted("Z");
    ImGui::SameLine();
    bCommitted = DrawSliderScalarCompact("Z Position", area.originZ, originRange, state.originZToggle,
                                         kAreaScalarCompactTrackWidthPixels,
                                         kAreaScalarCompactFieldWidthPixels, WidgetStyle(), "%.0f",
                                         /*bShowRealtimeToggle=*/false).bCommitted || bCommitted;

    ImGui::TextUnformatted("W");
    ImGui::SameLine();
    bCommitted = DrawSliderScalarCompact("Width", area.width, extentRange, state.widthToggle,
                                         kAreaScalarCompactTrackWidthPixels,
                                         kAreaScalarCompactFieldWidthPixels, WidgetStyle(), "%.0f",
                                         /*bShowRealtimeToggle=*/false).bCommitted || bCommitted;
    ImGui::SameLine();
    ImGui::TextUnformatted("L");
    ImGui::SameLine();
    bCommitted = DrawSliderScalarCompact("Length", area.length, extentRange, state.lengthToggle,
                                         kAreaScalarCompactTrackWidthPixels,
                                         kAreaScalarCompactFieldWidthPixels, WidgetStyle(), "%.0f",
                                         /*bShowRealtimeToggle=*/false).bCommitted || bCommitted;
    // ARCH §14.17 item 10 / §14.18 item 16 — PlayableArea is always Green and non-editable: re-pin
    // its color before drawing (the swatch below is the only OTHER path that could ever set a
    // PlayableArea color) and disable the control so a designer cannot pick a different one.
    // `kDefaultAreaColor` is retired — `kPlayableAreaColor` is the pinned reserved color now that
    // ordinary areas draw from the 16-entry palette instead.
    float* const color = ResolveAreaColor(areaColors, area.name);
    if (IsPlayableArea(area)) {
        color[0] = kPlayableAreaColor[0]; color[1] = kPlayableAreaColor[1];
        color[2] = kPlayableAreaColor[2]; color[3] = kPlayableAreaColor[3];
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
                                 int mapSize, std::vector<AreaColorEntry>& areaColors,
                                 std::vector<AreaVisibilityEntry>& areaVisibility, bool& bAreasMoved) {
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
            // STEP222 — activates the previously-stubbed [o]/[-] icon: a real per-area bit now,
            // not the DraggableListRow default.
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

// STEP222 — ToggleVisibility is no longer ignored: it flips the per-area AreaVisibilityEntry the
// row's own [o]/[-] icon now displays, and (unlike ToggleLock) trips the composite recompose since
// a hidden area actually changes what BuildMapAreaConfigurations draws.
// STEP212 — ToggleLock is NO LONGER ignored: it flips the per-area lock entry the row's own [U]/[L]
// icon now displays (DrawAreaList above). Delete is refused on the engine-required PlayableArea,
// exactly as v1 refused it. Reports whether the list actually moved (a lock flip is presentation
// state — it does not move the recipe and has no composite-side reader at all, so it is deliberately
// NOT folded into the `true` this function can return for other signals; it trips no recompose and
// needs no PreviewDriver notify).
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
    // The export keys areas by NAME, so the duplicate repair runs on the frames a name settled —
    // not every frame, which would rename a row mid-typing.
    if (bAreasMoved) MakeNamesUnique(recipe.areas);
    NotifyPlacementChange(bAreasMoved, previewDriver);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
