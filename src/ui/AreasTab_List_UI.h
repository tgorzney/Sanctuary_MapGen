// AreasTab_List_UI.h — the pure lifecycle rules for the list of map areas, plus the UI-only color
// side table an area's rectangle is drawn with. Layer: UI. Accuracy class: Visual (color) /
// Visual-Exact (the rectangle fields, real `Params::MapArea` content). TAB_REBUILD_PLAN "§ Areas";
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
// COLOR HAS NO `_PARAMS` HOME (STEP21 ruling #4 — the ratified `Params::MapArea` shape never gave
// an area a color field, unlike `Params::Army::armyColor`) — it stays UI-only, but its VALUE must
// survive a selection change or a Reorder, so it lives in a small side table keyed by
// `MapArea::name`, NOT vector position (position drifts under Reorder for no reason color needs to
// care about — Constitution §6).
#pragma once
#include <string>
#include <vector>
#include "ColorSwatch_UI.h"
#include "RtToggleWidget_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/MapArea_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// The one area the engine requires. v1 keyed "cannot be removed" off the NAME, and so does v2:
// the name is what the exported map file carries, so the name is the identity.
inline constexpr const char* kPlayableAreaName = "PlayableArea";

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

// The name v1's Add New Area button coined, kept so an imported v1 project reads the same. Thin
// domain wrapper over the shared cross-entity template (UniqueNameList_UI.h, STEP20 ARCH ruling).
inline std::string NextAreaName(int areaCount) { return NextUniqueLabel("NewArea", areaCount); }

// The engine-required area is present or it is created, at the FRONT and sized to the map. Reports
// whether the list moved.
inline bool EnsurePlayableArea(std::vector<Params::MapArea>& areas, int mapSize) {
    for (const Params::MapArea& area : areas)
        if (IsPlayableArea(area)) return false;
    Params::MapArea playableArea;
    playableArea.name = kPlayableAreaName;
    SetAreaToMapSize(playableArea, mapSize);
    areas.insert(areas.begin(), playableArea);
    return true;
}

// A UI-only color, keyed by area NAME (STEP21 ruling #4) — the default matches what
// `MapAreaRectangle::color` used to default to, and what `EnsurePlayableArea` used to set.
struct AreaColorEntry {
    std::string name;
    float color[kColorSwatchChannelCount] = { 1.0f, 1.0f, 1.0f, 0.35f };
};

// Finds the color entry for `areaName`, or appends a fresh default-colored one on first touch —
// the same linear-scan idiom `NameIsTakenBefore` already uses. Returns the channel array directly
// so a caller can hand it straight to `DrawColorSwatch`.
inline float* ResolveAreaColor(std::vector<AreaColorEntry>& areaColors, const std::string& areaName) {
    for (AreaColorEntry& entry : areaColors)
        if (entry.name == areaName) return entry.color;
    AreaColorEntry entry;
    entry.name = areaName;
    areaColors.push_back(entry);
    return areaColors.back().color;
}

} // namespace Ui
} // namespace SanmapGen
