// AreasTab_UI.cpp — the imgui composition of the areas tab. Layer: UI.
// Shared widgets only: DraggableList for the ordered area stack, TextInput for the name,
// SliderScalar for the four rectangle scalars, ColorSwatch (alpha bar) for the overlay tint,
// Checkbox for the tab-wide lock and Section for the two blocks. No ImGui::SliderFloat /
// DragFloat / ColorEdit4 in this file — v1 called all three here.
#include "AreasTab_UI.h"
#include "Checkbox_UI.h"
#include "DraggableListWidget_UI.h"
#include "PlacementRuleSections_UI.h"
#include "TextInput_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The area stack. MUTATES NOTHING while drawing: the signal is applied after the list has closed,
// so the vector never moves under a live row (the v1 erase-while-iterating defect).
DraggableListSignal DrawAreaList(const std::vector<Params::MapArea>& areas, int selectedAreaIndex) {
    return DraggableList<Params::MapArea>::Render(
        "areas", areas,
        [&](int rowIndex) {
            const Params::MapArea& area = areas[static_cast<std::size_t>(rowIndex)];
            DraggableListRow row;
            row.label   = AreaRowLabel(area);
            row.bLocked = !IsAreaRemovable(area);      // the PlayableArea's protection, shown
            return row;
        },
        [](int) {},                                    // header-only rows: the editor is below
        selectedAreaIndex);
}

// AFFORDANCE SCOPE: an area owns no visibility bit and no per-row lock — the lock v1 modelled is
// the tab-wide "Lock Areas" the map canvas reads — so those two signals are IGNORED rather than
// given a second, rival meaning (ARCH §4). Delete is refused on the engine-required PlayableArea,
// exactly as v1 refused it. Reports whether the list actually moved.
bool ApplyAreaListSignal(std::vector<Params::MapArea>& areas, AreasTabState& state,
                         const DraggableListSignal& signal) {
    const int rowIndex = signal.sourceRowIndex;
    const bool bRowValid = rowIndex >= 0 && rowIndex < static_cast<int>(areas.size());
    if (signal.kind == DraggableListSignalKind::Select) {
        if (bRowValid) state.selectedAreaIndex = rowIndex;
        return false;
    }
    if (signal.kind == DraggableListSignalKind::ToggleVisibility
        || signal.kind == DraggableListSignalKind::ToggleLock) return false;
    if (signal.kind == DraggableListSignalKind::Delete
        && (!bRowValid || !IsAreaRemovable(areas[static_cast<std::size_t>(rowIndex)])))
        return false;
    if (!ApplyDraggableListSignal(areas, signal)) return false;
    state.selectedAreaIndex = ResolvedAreaSelection(state.selectedAreaIndex,
                                                   static_cast<int>(areas.size()));
    return true;
}

// Add New Area and the tab-wide lock.
bool DrawAreasGlobals(std::vector<Params::MapArea>& areas, AreasTabState& state) {
    if (!DrawSectionBegin("Areas", state.globalSection)) return false;
    ImGui::TextWrapped("Areas are named rectangles exported beside the terrain. The map canvas can "
                       "drag and resize them unless they are locked.");
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
        bAreasMoved = true;
    }
    bAreasMoved = DrawCheckbox("Lock Areas", state.bAreasLocked).bCommitted || bAreasMoved;
    DrawSectionEnd();
    return bAreasMoved;
}

// The selected rectangle. Every scalar is whole-cell (the slider ranges snap to 1), because an
// area is exported in map cells. Color has no `_PARAMS` home (STEP21 ruling #4) and is resolved
// from the UI-only side table, keyed by name.
bool DrawAreaSettings(Params::MapArea& area, AreasTabState& state, int mapSize) {
    const ScalarSliderRange originRange = AreaOriginSliderRange(mapSize);
    const ScalarSliderRange extentRange = AreaExtentSliderRange(mapSize);
    bool bCommitted = false;
    if (IsPlayableArea(area)) {
        ImGui::TextDisabled("PlayableArea is required by the engine: it cannot be renamed or removed.");
    } else {
        // Captured BEFORE the edit: if the name commits to something new, the color entry keyed on
        // the OLD name must be retargeted, or a rename silently reverts the area's color to default
        // next frame (STEP21 ruling #5 — a real regression from today, where color lives on the row
        // object itself and is immune to renames).
        const std::string nameBeforeEdit = area.name;
        TextInputRules nameRules;
        nameRules.maximumLength = 48;
        nameRules.bAllowEmpty   = false;
        nameRules.fallbackText  = "Area";
        bCommitted = DrawTextInput("Name", area.name, nameRules).bCommitted;
        if (bCommitted && area.name != nameBeforeEdit) {
            for (AreaColorEntry& entry : state.areaColors)
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
    float* const color = ResolveAreaColor(state.areaColors, area.name);
    bCommitted = DrawColorSwatch("Color", color, state.colorOptions,
                                 state.colorToggle).bCommitted || bCommitted;
    if (ImGui::Button("Set to Map Size")) bCommitted = SetAreaToMapSize(area, mapSize) || bCommitted;
    return bCommitted;
}

} // namespace

void DrawAreasTab(Params::MapRecipe& recipe, AreasTabState& state,
                  Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("areasTab");
    const int mapSize = recipe.geometry.mapSize;
    bool bAreasMoved = EnsurePlayableArea(recipe.areas, mapSize);
    bAreasMoved = DrawAreasGlobals(recipe.areas, state) || bAreasMoved;
    if (DrawSectionBegin("Area Stack", state.areaSection)) {
        const DraggableListSignal signal = DrawAreaList(recipe.areas, state.selectedAreaIndex);
        if (signal.bHasSignal()) bAreasMoved = ApplyAreaListSignal(recipe.areas, state, signal) || bAreasMoved;
        Params::MapArea* const area = SelectedArea(recipe.areas, state.selectedAreaIndex);
        if (area == nullptr) ImGui::TextUnformatted("Select an area to edit it.");
        else bAreasMoved = DrawAreaSettings(*area, state, mapSize) || bAreasMoved;
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
