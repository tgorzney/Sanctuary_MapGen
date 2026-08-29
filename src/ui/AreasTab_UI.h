// AreasTab_UI.h — the areas tab: the named rectangles a map carries beside its terrain (the
// engine-required PlayableArea plus whatever regions a designer adds). Layer: UI.
// Accuracy class: Visual/Exact. TAB_REBUILD_PLAN "§ Areas"; tab-rebuild WO C4; retyped onto the
// real `Params::MapArea` by STEP21 (`ENTITY_AUTHORING_PARAMS_SPEC.md`).
//
// The stack is a DraggableList — an ORDERED set of tens of rows where every row is a drop target,
// which is exactly what that widget exists for — and every scalar is a shared SliderScalar
// carrying its own RealtimeToggle. The color is the picker-only ColorSwatch with its alpha BAR
// enabled: the areas tab is the one caller ColorSwatchOptions::bAlphaBarShown was added for.
// The pure list rules live in AreasTab_List_UI.h. The UI-only per-area color side table lives in
// `PreviewCompositeSettings::areaColors` (`AreaColorTable_UI.h` for the type itself,
// ARCH_14_17_MapAreaFieldLayer.md §14.17 item 9) — this tab reaches it through a `DrawAreasTab`
// parameter, not a field of its own state.
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing type; reported, not invented):
//  1. They DO notify Pipeline::PreviewDriver. An area is drawn on the composite, and because no
//     generation stage hashes one the driver DERIVES PreviewRender from the stage hashes — the
//     recomposite alone, never a regeneration (PreviewDriver_PIPELINE.h). The tab maps no flag
//     itself; that derivation is the whole point of the two-tier model.
//  2. v1 dragged X / Y with an UNBOUNDED DragFloat. The shared SliderScalar is a bounded track, so
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

// SCOPE NOTE 2: one map width of slack on each side, so an area may legally hang off an edge.
inline ScalarSliderRange AreaOriginSliderRange(int mapSize) {
    const int resolvedMapSize = ResolvedAreaMapSize(mapSize);
    return IntegerScalarSliderRange(-resolvedMapSize, resolvedMapSize * 2, 1);
}

// The area the per-area controls edit, or null when the selection points at nothing
// (Constitution §6 — an index is validated, never trusted).
inline Params::MapArea* SelectedArea(std::vector<Params::MapArea>& areas, int selectedAreaIndex) {
    if (selectedAreaIndex < 0 || selectedAreaIndex >= static_cast<int>(areas.size())) return nullptr;
    return &areas[static_cast<std::size_t>(selectedAreaIndex)];
}

// The selection after a row is removed: the row that took its place, or the new last row, or
// nothing at all. Pure, so removing the bottom area is testable without a window.
inline int ResolvedAreaSelection(int selectedAreaIndex, int areaCount) {
    if (areaCount <= 0) return -1;
    if (selectedAreaIndex < 0) return -1;
    return selectedAreaIndex < areaCount ? selectedAreaIndex : areaCount - 1;
}

// `recipe.areas` is edited directly (STEP21) — the tab reads `geometry.mapSize` to size its
// sliders and the Set to Map Size button, and writes back through `recipe.areas`. `areaColors` is
// `PreviewCompositeSettings::areaColors` (ARCH §14.17 item 9) — the tab's own call site passes
// `composite.Settings().areaColors`, never a copy of its own.
void DrawAreasTab(Params::MapRecipe& recipe, AreasTabState& state,
                  Pipeline::PreviewDriver* previewDriver, std::vector<AreaColorEntry>& areaColors);

} // namespace Ui
} // namespace SanmapGen
