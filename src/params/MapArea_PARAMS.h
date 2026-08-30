// MapArea_PARAMS.h — the hand-authored map-area rectangle (`MapArea`, e.g. `PlayableArea`).
// Layer: PARAMS. Manually-authored, pass-through entity data (ENTITY_AUTHORING_PARAMS_SPEC), the
// same "no PROC stage reinterprets it" posture as Army_PARAMS.h. Verbatim from
// ENTITY_AUTHORING_PARAMS_SPEC.md's "The types" section.
#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params {

struct MapArea {
    std::string name;          // folded-in dictionary key (areas[key]) — a LOAD-BEARING gameplay
                                // identifier: GameUtils.GetArea(name), UNIT_PROP_MARKER_DATA_SPEC
    float originX = 0.0f;      // format's `x`
    float originZ = 0.0f;      // format's `y`
    float width   = 0.0f;      // format's `width`
    float length  = 0.0f;      // format's `height`
};

// ARCH §14.19 — plain rectangle area, never a bounding diagonal or max(width, length).
inline float MapAreaSize(const MapArea& area) { return area.width * area.length; }

// The ONE way any layer adds an area. Keeps `areas` continuously sorted ascending by size.
// Ties are stable: inserts before the first existing entry STRICTLY LARGER than `area`, so
// equal-size entries keep first-come-first-served order. Returns the index the area landed at.
inline std::size_t InsertMapAreaSortedBySize(std::vector<MapArea>& areas, MapArea area) {
    std::size_t insertAt = areas.size();
    for (std::size_t index = 0; index < areas.size(); ++index) {
        if (MapAreaSize(areas[index]) > MapAreaSize(area)) { insertAt = index; break; }
    }
    areas.insert(areas.begin() + static_cast<std::ptrdiff_t>(insertAt), std::move(area));
    return insertAt;
}

} // namespace Params
} // namespace SanmapGen
