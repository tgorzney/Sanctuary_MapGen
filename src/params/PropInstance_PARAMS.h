// PropInstance_PARAMS.h — `PropTransform`, `DecalTransform`, `PropInstanceGroup`,
// `DecalInstanceGroup`. Layer: PARAMS. Verbatim from ENTITY_AUTHORING_PARAMS_SPEC.md's "The types"
// section (third session, ARCH §12) — `PropTransform`/`DecalTransform` compose `InstancedTransform`
// plus `layerIndex`, SUPERSEDING the second session's now-overturned "props/decals need no wrapper
// transform type" ruling (see that spec section for the full reasoning).
// `PropInstanceLayer`/`DecalInstanceLayer` (and their Id/Color resolvers) moved to
// `ScatterInstanceLayer_PARAMS.h` (ARCH §20) once they grew full field parity with
// `MarkerInstanceLayer` — included below so every existing include site of this file still compiles.
#pragma once
#include <string>
#include <vector>
#include "InstancedTransform_PARAMS.h"
#include "ScatterInstanceLayer_PARAMS.h"

namespace SanmapGen {
namespace Params {

// Third session (ARCH §12): thin named wrapper types, not a bare InstancedTransform — see
// "Why props/decals now need a wrapper transform type" in ENTITY_AUTHORING_PARAMS_SPEC.md.
struct PropTransform  { InstancedTransform transform; int layerIndex = 0; };
struct DecalTransform { InstancedTransform transform; int layerIndex = 0; };

// `blueprintPath`/`transforms` are an ORDERED ARRAY, not a dictionary — the format's own
// `PropType[]`/`DecalType[]` (`SanMap.cs:153,157`) have no per-instance key to fold in.
struct PropInstanceGroup  { std::string blueprintPath; bool bReclaimable = false; std::vector<PropTransform>  transforms; };
struct DecalInstanceGroup { std::string blueprintPath; std::vector<DecalTransform> transforms; };

} // namespace Params
} // namespace SanmapGen
