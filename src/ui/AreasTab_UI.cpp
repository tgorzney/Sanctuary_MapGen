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
DraggableListSignal DrawAreaList(const AreasTabState& state) {
    return DraggableList<MapAreaRectangle>::Render(
        "areas", state.areas,
        [&](int rowIndex) {
            const MapAreaRectangle& area = state.areas[static_cast<std::size_t>(rowIndex)];
            DraggableListRow row;
            row.label   = AreaRowLabel(area);
            row.bLocked = !IsAreaRemovable(area);      // the PlayableArea's protection, shown
            return row;
        },
        [](int) {},                                    // header-only rows: the editor is below
        state.selectedAreaIndex);
}

// AFFORDANCE SCOPE: an area owns no visibility bit and no per-row lock — the lock v1 modelled is
// the tab-wide "Lock Areas" the map canvas reads — so those two signals are IGNORED rather than
// given a second, rival meaning (ARCH §4). Delete is refused on the engine-required PlayableArea,
// exactly as v1 refused it. Reports whether the list actually moved.
bool ApplyAreaListSignal(AreasTabState& state, const DraggableListSignal& signal) {
    const int rowIndex = signal.sourceRowIndex;
    const bool bRowValid = rowIndex >= 0 && rowIndex < static_cast<int>(state.areas.size());
    if (signal.kind == DraggableListSignalKind::Select) {
        if (bRowValid) state.selectedAreaIndex = rowIndex;
        return false;
    }
    if (signal.kind == DraggableListSignalKind::ToggleVisibility
        || signal.kind == DraggableListSignalKind::ToggleLock) return false;
    if (signal.kind == DraggableListSignalKind::Delete
        && (!bRowValid || !IsAreaRemovable(state.areas[static_cast<std::size_t>(rowIndex)])))
        return false;
    if (!ApplyDraggableListSignal(state.areas, signal)) return false;
    state.selectedAreaIndex = ResolvedAreaSelection(state.selectedAreaIndex,
                                                   static_cast<int>(state.areas.size()));
    return true;
}

// Add New Area and the tab-wide lock.
bool DrawAreasGlobals(AreasTabState& state) {
    if (!DrawSectionBegin("Areas", state.globalSection)) return false;
    ImGui::TextWrapped("Areas are named rectangles exported beside the terrain. The map canvas can "
                       "drag and resize them unless they are locked.");
    bool bAreasMoved = false;
    if (ImGui::Button("Add New Area")) {
        MapAreaRectangle area;
        area.name = NextAreaName(static_cast<int>(state.areas.size()));
        state.areas.push_back(area);
        state.selectedAreaIndex = static_cast<int>(state.areas.size()) - 1;
        bAreasMoved = true;
    }
    bAreasMoved = DrawCheckbox("Lock Areas", state.bAreasLocked).bCommitted || bAreasMoved;
    DrawSectionEnd();
    return bAreasMoved;
}

// The selected rectangle. Every scalar is whole-cell (the slider ranges snap to 1), because an
// area is exported in map cells.
bool DrawAreaSettings(MapAreaRectangle& area, AreasTabState& state, int mapSize) {
    const ScalarSliderRange originRange = AreaOriginSliderRange(mapSize);
    const ScalarSliderRange extentRange = AreaExtentSliderRange(mapSize);
    bool bCommitted = false;
    if (IsPlayableArea(area)) {
        ImGui::TextDisabled("PlayableArea is required by the engine: it cannot be renamed or removed.");
    } else {
        TextInputRules nameRules;
        nameRules.maximumLength = 48;
        nameRules.bAllowEmpty   = false;
        nameRules.fallbackText  = "Area";
        bCommitted = DrawTextInput("Name", area.name, nameRules).bCommitted;
    }
    bCommitted = DrawSliderScalar("X Position", area.originX, originRange, area.originXToggle,
                                  WidgetStyle(), "%.0f").bCommitted || bCommitted;
    bCommitted = DrawSliderScalar("Z Position", area.originZ, originRange, area.originZToggle,
                                  WidgetStyle(), "%.0f").bCommitted || bCommitted;
    bCommitted = DrawSliderScalar("Width", area.width, extentRange, area.widthToggle,
                                  WidgetStyle(), "%.0f").bCommitted || bCommitted;
    bCommitted = DrawSliderScalar("Length", area.length, extentRange, area.lengthToggle,
                                  WidgetStyle(), "%.0f").bCommitted || bCommitted;
    bCommitted = DrawColorSwatch("Color", area.color, state.colorOptions,
                                 area.colorToggle).bCommitted || bCommitted;
    if (ImGui::Button("Set to Map Size")) bCommitted = SetAreaToMapSize(area, mapSize) || bCommitted;
    return bCommitted;
}

} // namespace

void DrawAreasTab(const Params::MapRecipe& recipe, AreasTabState& state,
                  Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("areasTab");
    const int mapSize = recipe.geometry.mapSize;
    bool bAreasMoved = EnsurePlayableArea(state.areas, mapSize);
    bAreasMoved = DrawAreasGlobals(state) || bAreasMoved;
    if (DrawSectionBegin("Area Stack", state.areaSection)) {
        const DraggableListSignal signal = DrawAreaList(state);
        if (signal.bHasSignal()) bAreasMoved = ApplyAreaListSignal(state, signal) || bAreasMoved;
        MapAreaRectangle* const area = SelectedArea(state);
        if (area == nullptr) ImGui::TextUnformatted("Select an area to edit it.");
        else bAreasMoved = DrawAreaSettings(*area, state, mapSize) || bAreasMoved;
        DrawSectionEnd();
    }
    // The export keys areas by NAME, so the duplicate repair runs on the frames a name settled —
    // not every frame, which would rename a row mid-typing.
    if (bAreasMoved) MakeAreaNamesUnique(state.areas);
    NotifyPlacementChange(bAreasMoved, previewDriver);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
