// AreasTab_List_UI.h — one map area rectangle, and the pure lifecycle rules of the list of them.
// Layer: UI. Accuracy class: Visual. TAB_REBUILD_PLAN "§ Areas"; tab-rebuild WO C4.
//
// Split out of AreasTab_UI.h so the tab header stays small (ARCH §1.5) and so the three rules that
// actually have teeth — the engine-required PlayableArea, the unique-name repair the export
// depends on, and "Set to Map Size" — are PURE and assertable with no imgui frame, window or GL
// context (WidgetHelpers_UI.h "THE SPLIT").
//
// v1 ran the unique-name repair as a loop tacked onto the end of the tab draw
// (gui/tabs/Tab_Areas.cpp), so it only ever ran while the tab was open. Here it is a function the
// tab calls and a test can call too.
#pragma once
#include <string>
#include <vector>
#include "ColorSwatch_UI.h"
#include "RtToggleWidget_UI.h"

namespace SanmapGen {
namespace Ui {

// The one area the engine requires. v1 keyed "cannot be removed" off the NAME, and so does v2:
// the name is what the exported map file carries, so the name is the identity.
inline constexpr const char* kPlayableAreaName = "PlayableArea";

// One named rectangle in map-cell space. Alpha is a real channel here — an area draws as a
// translucent overlay — which is the one place the library's alpha bar is asked for.
struct MapAreaRectangle {
    std::string name;
    float originX = 0.0f;
    float originZ = 0.0f;
    float width   = 100.0f;
    float length  = 100.0f;
    float color[kColorSwatchChannelCount] = { 1.0f, 1.0f, 1.0f, 0.35f };

    RealtimeToggle originXToggle;
    RealtimeToggle originZToggle;
    RealtimeToggle widthToggle;
    RealtimeToggle lengthToggle;
    RealtimeToggle colorToggle;
};

inline bool IsPlayableArea(const MapAreaRectangle& area) { return area.name == kPlayableAreaName; }
inline bool IsAreaRemovable(const MapAreaRectangle& area) { return !IsPlayableArea(area); }

// The label a row shows — never empty (Constitution §6).
inline const char* AreaRowLabel(const MapAreaRectangle& area) {
    return area.name.empty() ? "Area" : area.name.c_str();
}

// A map side that can be drawn on: a recipe carrying a nonsense size is repaired, never obeyed.
inline int ResolvedAreaMapSize(int mapSize) { return mapSize > 1 ? mapSize : 1; }

// "Set to Map Size": the whole map, origin at the corner. Reports whether the rectangle moved, so
// a button press that changes nothing costs no recomposite.
inline bool SetAreaToMapSize(MapAreaRectangle& area, int mapSize) {
    const float extent = static_cast<float>(ResolvedAreaMapSize(mapSize));
    const bool bMoved = area.originX != 0.0f || area.originZ != 0.0f
                     || area.width != extent || area.length != extent;
    area.originX = 0.0f;
    area.originZ = 0.0f;
    area.width   = extent;
    area.length  = extent;
    return bMoved;
}

// The name v1's Add New Area button coined, kept so an imported v1 project reads the same.
inline std::string NextAreaName(int areaCount) {
    return std::string("NewArea_") + std::to_string(areaCount < 0 ? 0 : areaCount);
}

// True when some EARLIER row already answers to `name` — the half of the uniqueness rule that
// decides which of two clashing rows is the one that gets renamed (the later one).
inline bool AreaNameIsTakenBefore(const std::vector<MapAreaRectangle>& areas, std::size_t areaIndex,
                                  const std::string& name) {
    const std::size_t rowCount = areaIndex < areas.size() ? areaIndex : areas.size();
    for (std::size_t rowIndex = 0u; rowIndex < rowCount; ++rowIndex)
        if (areas[rowIndex].name == name) return true;
    return false;
}

// Repairs duplicate names by suffixing the later row (`Base`, `Base_1`, `Base_2` ...). The export
// keys areas by name, so two rows sharing one would silently drop an area — this is Constitution
// §6 applied to a name the designer typed. Reports whether any name moved.
inline bool MakeAreaNamesUnique(std::vector<MapAreaRectangle>& areas) {
    bool bNamesMoved = false;
    for (std::size_t areaIndex = 0u; areaIndex < areas.size(); ++areaIndex) {
        if (!AreaNameIsTakenBefore(areas, areaIndex, areas[areaIndex].name)) continue;
        const std::string baseName = areas[areaIndex].name;
        int suffix = 1;
        do {
            areas[areaIndex].name = baseName + "_" + std::to_string(suffix++);
        } while (AreaNameIsTakenBefore(areas, areaIndex, areas[areaIndex].name));
        bNamesMoved = true;
    }
    return bNamesMoved;
}

// The engine-required area is present or it is created, at the FRONT and sized to the map. Reports
// whether the list moved.
inline bool EnsurePlayableArea(std::vector<MapAreaRectangle>& areas, int mapSize) {
    for (const MapAreaRectangle& area : areas)
        if (IsPlayableArea(area)) return false;
    MapAreaRectangle playableArea;
    playableArea.name = kPlayableAreaName;
    SetAreaToMapSize(playableArea, mapSize);
    areas.insert(areas.begin(), playableArea);
    return true;
}

} // namespace Ui
} // namespace SanmapGen
