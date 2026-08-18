// MapArea_PARAMS.h — the hand-authored map-area rectangle (`MapArea`, e.g. `PlayableArea`).
// Layer: PARAMS. Manually-authored, pass-through entity data (ENTITY_AUTHORING_PARAMS_SPEC), the
// same "no PROC stage reinterprets it" posture as Army_PARAMS.h. Verbatim from
// ENTITY_AUTHORING_PARAMS_SPEC.md's "The types" section.
#pragma once
#include <string>

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

} // namespace Params
} // namespace SanmapGen
