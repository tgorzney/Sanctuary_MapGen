// InstancedTransform_PARAMS.h — the shared `{position, rotation, scale}` base every resolved/
// baked-instance transform type composes. Layer: PARAMS. Promoted in ENTITY_AUTHORING_PARAMS_SPEC's
// second session (`SanMap.Types.cs:179`'s `InstancedTransform`, confirmed shared by
// `PropTransform`/`DecalTransform`/`MarkerTransform` on the wire). `UnitTransform` is deliberately
// NOT retrofitted to compose this — it already shipped flat; see the spec's own ruling. Verbatim
// from ENTITY_AUTHORING_PARAMS_SPEC.md's "The types" section.
#pragma once

namespace SanmapGen {
namespace Params {

struct InstancedTransform {
    float positionX = 0.0f;            // world/game units, absolute (SANMAP_FORMAT_SPEC)
    float positionY = 0.0f;            // elevation
    float positionZ = 0.0f;
    float rotationX = 0.0f;            // quaternion (x, y, z, w)
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    float rotationW = 1.0f;
    float scaleX    = 1.0f;
    float scaleY    = 1.0f;
    float scaleZ    = 1.0f;
};

} // namespace Params
} // namespace SanmapGen
