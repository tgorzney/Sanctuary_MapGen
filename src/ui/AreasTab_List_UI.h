// AreasTab_List_UI.h — the pure lifecycle rules for the list of map areas. Layer: UI.
// Accuracy class: Visual-Exact (real `Params::MapArea` content). TAB_REBUILD_PLAN "§ Areas";
// tab-rebuild WO C4; retyped onto the real `Params::MapArea` by STEP21
// (`ENTITY_AUTHORING_PARAMS_SPEC.md`).
//
// Split out of AreasTab_UI.h so the tab header stays small (ARCH §1.5) and so the three rules that
// actually have teeth — the engine-required PlayableArea, the unique-name repair the export
// depends on, and "Set to Map Size" — are PURE and assertable with no imgui frame, window or GL
// context (WidgetHelpers_UI.h "THE SPLIT").
//
// v1 ran the unique-name repair as a loop tacked onto the end of the tab draw
// (gui/tabs/Tab_Areas.cpp), so it only ever ran while the tab was open. Here it is a function the
// tab calls and a test can call too.
//
// ARCH_14_17_MapAreaFieldLayer.md §14.17 item 9 — `AreaColorEntry`/`ResolveAreaColor` moved OUT of
// this file into the new minimal `AreaColorTable_UI.h`; this file includes it and re-exports both
// names, so every existing call site (`AreasTab_UI.cpp`, `MapCanvas_AreaDraw_UI.cpp`,
// `AreasTab_UI_Test.cpp`) keeps compiling unchanged against `AreasTab_List_UI.h`. The color table's
// single OWNER is now `PreviewCompositeSettings::areaColors` (see that header) — not
// `AreasTabState`, which no longer carries a color field of its own.
//
// ARCH_14_18_AreaLiveBlendFidelityAndPalette.md item 12 — `kPlayableAreaName` (formerly defined
// directly below) has ALSO moved into `AreaColorTable_UI.h`, for the same "one funnel, no upward
// dependency on a tab header" reason as the color table itself. It is re-exported here by the same
// inclusion, so `IsPlayableArea`/`EnsurePlayableArea`/every existing call site is unaffected.
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

// `kPlayableAreaName` itself now lives in AreaColorTable_UI.h (ARCH §14.18 item 12), re-exported
// here by the #include above — nothing below needs its own copy.

inline bool IsPlayableArea(const Params::MapArea& area) { return area.name == kPlayableAreaName; }
inline bool IsAreaRemovable(const Params::MapArea& area) { return !IsPlayableArea(area); }

// The label a row shows — never empty (Constitution §6).
inline const char* AreaRowLabel(const Params::MapArea& area) {
    return area.name.empty() ? "Area" : area.name.c_str();
}

// A map side that can be drawn on: a recipe carrying a nonsense size is repaired, never obeyed.
inline int ResolvedAreaMapSize(int mapSize) { return mapSize > 1 ? mapSize : 1; }

// "Set to Map Size": the whole map, origin at the corner. Reports whether the rectangle moved, so
// a button press that changes nothing costs no recomposite.
inline bool SetAreaToMapSize(Params::MapArea& area, int mapSize) {
    const float extent = static_cast<float>(ResolvedAreaMapSize(mapSize));
    const bool bMoved = area.originX != 0.0f || area.originZ != 0.0f
                     || area.width != extent || area.length != extent;
    area.originX = 0.0f;
    area.originZ = 0.0f;
    area.width   = extent;
    area.length  = extent;
    return bMoved;
}

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

// The name v1's Add New Area button coined, kept so an imported v1 project reads the same. Thin
// domain wrapper over the shared cross-entity template (UniqueNameList_UI.h, STEP20 ARCH ruling).
inline std::string NextAreaName(int areaCount) { return NextUniqueLabel("NewArea", areaCount); }

// The engine-required area is present or it is created, size-sorted into place (ARCH §14.19 — in
// practice this still lands PlayableArea at/near the back, since it is sized to the whole map and
// is therefore almost always the single largest entry). Reports whether the list moved.
inline bool EnsurePlayableArea(std::vector<Params::MapArea>& areas, int mapSize) {
    for (const Params::MapArea& area : areas)
        if (IsPlayableArea(area)) return false;
    Params::MapArea playableArea;
    playableArea.name = kPlayableAreaName;
    SetAreaToMapSize(playableArea, mapSize);
    Params::InsertMapAreaSortedBySize(areas, playableArea);
    return true;
}

} // namespace Ui
} // namespace SanmapGen
