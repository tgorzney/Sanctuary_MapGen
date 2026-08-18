// MarkerChain_PARAMS.h — the hand-authored marker chain: `ChainMarker`, `MarkerChain`.
// Layer: PARAMS. Manually-authored, pass-through entity data (ENTITY_AUTHORING_PARAMS_SPEC "Scope"),
// same posture as MapArea_PARAMS.h. `ChainMarker` is a deliberate rename of the format's bare C#
// `Marker` (naming exception of the same class as `Area.height` -> `length` — see the spec's
// "`Marker` -> `ChainMarker`" section: verbatim would collide with `MarkerTransform`/
// `MarkerInstanceGroup`/`MarkerRule`/`MarkerCategory` already live in this namespace). Verbatim from
// ENTITY_AUTHORING_PARAMS_SPEC.md's "The types" section.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params {

struct ChainMarker { std::string type; std::string name; };  // deliberately renamed from format's
                                                                // bare "Marker" — see naming
                                                                // exception in the spec

struct MarkerChain {
    std::string name;                   // folded-in outer dict key — chain name
    std::vector<ChainMarker> markers;   // ORDERED — semantically meaningful sequence, never
                                         // resorted. Field name borrowed directly from the
                                         // format's own MarkerChain.markers C# field.
};

} // namespace Params
} // namespace SanmapGen
