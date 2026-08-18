// MarkerInstance_PARAMS.h — the hand-placed marker roster: `MarkerTransform`, `MarkerInstanceGroup`.
// Layer: PARAMS. Manually-authored, pass-through entity data (ENTITY_AUTHORING_PARAMS_SPEC "Scope"),
// same posture as Army_PARAMS.h: round-trip fidelity through the `.sanmap` `markers` two-level
// dictionary is the entire purpose, no PROC stage computes or reinterprets these fields.
// `MarkerTransform` COMPOSES `InstancedTransform` as a member — it does NOT flatten it like
// `UnitTransform` does (the spec's second-session ruling: `UnitTransform` was already shipped flat
// and is deliberately not retrofitted; `MarkerTransform` has no such history). Verbatim from
// ENTITY_AUTHORING_PARAMS_SPEC.md's "The types" section.
#pragma once
#include <string>
#include <vector>
#include "InstancedTransform_PARAMS.h"

namespace SanmapGen {
namespace Params {

struct MarkerTransform {
    std::string name;                  // folded-in inner dict key — instance name (e.g. "Mex 0")
    InstancedTransform transform;
    std::string alias;                 // SanGen-added, already-ratified SANMAP_FORMAT_SPEC Correction 11
};

struct MarkerInstanceGroup {
    std::string name;                        // folded-in outer dict key — marker TYPE name
                                              // (e.g. "Spawn"/"Alloys") — free-form std::string,
                                              // NOT MarkerCategory; see the spec's cardinality ruling
    bool bResource = false;                  // format's `resource`, b-prefixed per §1.1
    std::vector<MarkerTransform> transforms;
};

} // namespace Params
} // namespace SanmapGen
