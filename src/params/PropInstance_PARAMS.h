// PropInstance_PARAMS.h — `PropTransform`, `DecalTransform`, `PropInstanceGroup`,
// `DecalInstanceGroup`, `PropInstanceLayer`, `DecalInstanceLayer` together, mirroring the existing
// `ScatterRule_PARAMS.h` multi-type-per-file precedent (`PropRule`/`DecalRule`/`UnitRule` already
// share one file for the same reason: near-identical shapes, always touched together).
// Layer: PARAMS. Verbatim from ENTITY_AUTHORING_PARAMS_SPEC.md's "The types" section (third
// session, ARCH §12) — `PropTransform`/`DecalTransform` compose `InstancedTransform` plus
// `layerIndex`, SUPERSEDING the second session's now-overturned "props/decals need no wrapper
// transform type" ruling (see that spec section for the full reasoning).
#pragma once
#include <string>
#include <vector>
#include "InstancedTransform_PARAMS.h"

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

// Third session (ARCH §12): the separate manual-layer metadata array, one entry per authored
// layer, indexed by PropTransform/DecalTransform::layerIndex. Same shape for both domains. Wire
// keys are `PropGroups`/`DecalGroups` (SANMAP_FORMAT_SPEC Correction 14), PascalCase.
struct PropInstanceLayer  { std::string name; float color[4] = {1.0f,1.0f,1.0f,1.0f}; float iconScale = 1.0f; int layerId = -1; bool bLocked = false; };
struct DecalInstanceLayer { std::string name; float color[4] = {1.0f,1.0f,1.0f,1.0f}; float iconScale = 1.0f; int layerId = -1; bool bLocked = false; };

// ARCH §14.15: single source of truth for resolving a PropTransform/DecalTransform's positional
// `layerIndex` to its owning layer's stable `layerId`, bounds-checked with a -1 sentinel. Both
// `Placement_Manual_PROC.cpp` (baked-copy resolution) and `MapCanvas_IconLayer_CullManual_UI.cpp`
// (live cull-path resolution) call these instead of each carrying their own formula.
inline int ResolvePropInstanceLayerId(int layerIndex, const std::vector<PropInstanceLayer>& layers) {
    if (layerIndex < 0 || static_cast<std::size_t>(layerIndex) >= layers.size()) return -1;
    return layers[layerIndex].layerId;
}
inline int ResolveDecalInstanceLayerId(int layerIndex, const std::vector<DecalInstanceLayer>& layers) {
    if (layerIndex < 0 || static_cast<std::size_t>(layerIndex) >= layers.size()) return -1;
    return layers[layerIndex].layerId;
}

} // namespace Params
} // namespace SanmapGen
