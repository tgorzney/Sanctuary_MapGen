// AreasTab_UI.h — the areas tab: the named rectangles a map carries beside its terrain (the
// engine-required PlayableArea plus whatever regions a designer adds). Layer: UI.
// Accuracy class: Visual. TAB_REBUILD_PLAN "§ Areas"; tab-rebuild WO C4.
//
// The stack is a DraggableList — an ORDERED set of tens of rows where every row is a drop target,
// which is exactly what that widget exists for — and every scalar is a shared SliderScalar
// carrying its own RealtimeToggle. The color is the picker-only ColorSwatch with its alpha BAR
// enabled: the areas tab is the one caller ColorSwatchOptions::bAlphaBarShown was added for.
// The pure list rules live in AreasTab_List_UI.h.
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing type; reported, not invented):
//  1. AN AREA HAS NO `_PARAMS` HOME. There is no `Params::MapArea` and no `MapRecipe` slice to
//     hold one, so the rectangles are CALLER-OWNED UI state the app shell (WO E) fills — the same
//     standing HeightmapTab_UI's global gravity and SystemTab_UI's asset-cache directory already
//     have. They are NOT serialized; a durable `MapArea_PARAMS` plus its sanmap round trip is its
//     own work-order, and it is the reason this tab takes the recipe as CONST: it reads the map
//     size and writes nothing.
//  2. They DO notify Pipeline::PreviewDriver. An area is drawn on the composite, and because no
//     generation stage hashes one the driver DERIVES PreviewRender from the stage hashes — the
//     recomposite alone, never a regeneration (PreviewDriver_PIPELINE.h). The tab maps no flag
//     itself; that derivation is the whole point of the two-tier model.
//  3. v1 dragged X / Y with an UNBOUNDED DragFloat. The shared SliderScalar is a bounded track, so
//     the origin is fenced to one map width outside the map on each side. That is a limit v1
//     lacked, not a setting v1 had.
#pragma once
#include "AreasTab_List_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct AreasTabState {
    SectionState       globalSection;
    SectionState       areaSection;
    ColorSwatchOptions colorOptions = ColorSwatchOptions();
    std::vector<MapAreaRectangle> areas;
    int  selectedAreaIndex = -1;
    bool bAreasLocked      = true;    // v1 parity, including v1's default: while set, the map
                                      // canvas may not drag or resize an area (WO E reads it)
};

// The swatch an area color is edited with: alpha is a real channel (the overlay's opacity), so the
// alpha channel and the picker's vertical alpha bar are both on. A tab state is seeded from this
// once by the host rather than each draw re-deciding it (Constitution §8).
inline ColorSwatchOptions AreasTabColorSwatchOptions() {
    ColorSwatchOptions options;
    options.bAlphaEnabled  = true;
    options.bAlphaBarShown = true;
    return options;
}

// v1 fenced Width / Length to 1..2x the map side; both limits move with the map size, so a resize
// can never leave an extent slider unable to express the map it belongs to.
inline ScalarSliderRange AreaExtentSliderRange(int mapSize) {
    return IntegerScalarSliderRange(1, ResolvedAreaMapSize(mapSize) * 2, 1);
}

// SCOPE NOTE 3: one map width of slack on each side, so an area may legally hang off an edge.
inline ScalarSliderRange AreaOriginSliderRange(int mapSize) {
    const int resolvedMapSize = ResolvedAreaMapSize(mapSize);
    return IntegerScalarSliderRange(-resolvedMapSize, resolvedMapSize * 2, 1);
}

// The area the per-area controls edit, or null when the selection points at nothing
// (Constitution §6 — an index is validated, never trusted).
inline MapAreaRectangle* SelectedArea(AreasTabState& state) {
    if (state.selectedAreaIndex < 0
        || state.selectedAreaIndex >= static_cast<int>(state.areas.size())) return nullptr;
    return &state.areas[static_cast<std::size_t>(state.selectedAreaIndex)];
}

// The selection after a row is removed: the row that took its place, or the new last row, or
// nothing at all. Pure, so removing the bottom area is testable without a window.
inline int ResolvedAreaSelection(int selectedAreaIndex, int areaCount) {
    if (areaCount <= 0) return -1;
    if (selectedAreaIndex < 0) return -1;
    return selectedAreaIndex < areaCount ? selectedAreaIndex : areaCount - 1;
}

// `recipe` is READ-ONLY here (SCOPE NOTE 1): the tab reads `geometry.mapSize` to size its sliders
// and the Set to Map Size button, and writes nothing back.
void DrawAreasTab(const Params::MapRecipe& recipe, AreasTabState& state,
                  Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
