// PropInstance_PARAMS.h — `PropTransform`, `DecalTransform`, `PropInstanceGroup`,
// `DecalInstanceGroup`. Layer: PARAMS. Verbatim from ENTITY_AUTHORING_PARAMS_SPEC.md's "The types"
// section (third session, ARCH §12) — `PropTransform`/`DecalTransform` compose `InstancedTransform`
// plus `layerIndex`, SUPERSEDING the second session's now-overturned "props/decals need no wrapper
// transform type" ruling (see that spec section for the full reasoning).
// `PropInstanceLayer`/`DecalInstanceLayer` (and their Id/Color resolvers) moved to
// `ScatterInstanceLayer_PARAMS.h` (ARCH §20) once they grew full field parity with
// `MarkerInstanceLayer` — included below so every existing include site of this file still compiles.
#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include "InstancedTransform_PARAMS.h"
#include "ScatterInstanceLayer_PARAMS.h"

namespace SanmapGen {
namespace Params {

// Third session (ARCH §12): thin named wrapper types, not a bare InstancedTransform — see
// "Why props/decals now need a wrapper transform type" in ENTITY_AUTHORING_PARAMS_SPEC.md.
// `instanceIdentifier`/`symmetryGroupIdentifier` (ARCH §21.4) verbatim mirror
// `MarkerTransform`'s own fields — stable/minted/never-reused selection addressing and symmetry-
// clone correspondence, each domain (Props, Decals) with its own independent number space, never
// shared with Markers' or each other's.
struct PropTransform  { InstancedTransform transform; int layerIndex = 0;
                        int instanceIdentifier = -1; int symmetryGroupIdentifier = 0; };
struct DecalTransform { InstancedTransform transform; int layerIndex = 0;
                        int instanceIdentifier = -1; int symmetryGroupIdentifier = 0; };

// `blueprintPath`/`transforms` are an ORDERED ARRAY, not a dictionary — the format's own
// `PropType[]`/`DecalType[]` (`SanMap.cs:153,157`) have no per-instance key to fold in.
struct PropInstanceGroup  { std::string blueprintPath; bool bReclaimable = false; std::vector<PropTransform>  transforms; };
struct DecalInstanceGroup { std::string blueprintPath; std::vector<DecalTransform> transforms; };

// ARCH §21.4 — max(instanceIdentifier)+1 scanned across EVERY group's transforms (roster-wide, not
// per-group), mirroring NextMarkerInstanceIdentifier's exact shape (MarkerInstanceId_UI.h). Placed
// in PARAMS, co-located with the structs they walk — not mirroring that function's own UI-layer
// misplacement (a standing, non-blocking defect §20.2 already recorded).
inline int NextPropInstanceIdentifier(const std::vector<PropInstanceGroup>& props) {
    int maximumId = -1;
    for (const PropInstanceGroup& group : props)
        for (const PropTransform& transform : group.transforms)
            maximumId = std::max(maximumId, transform.instanceIdentifier);
    return maximumId + 1;
}
inline int NextDecalInstanceIdentifier(const std::vector<DecalInstanceGroup>& decals) {
    int maximumId = -1;
    for (const DecalInstanceGroup& group : decals)
        for (const DecalTransform& transform : group.transforms)
            maximumId = std::max(maximumId, transform.instanceIdentifier);
    return maximumId + 1;
}

} // namespace Params
} // namespace SanmapGen
