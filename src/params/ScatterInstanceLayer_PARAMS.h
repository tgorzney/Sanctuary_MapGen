// ScatterInstanceLayer_PARAMS.h — Params::PropInstanceLayer/DecalInstanceLayer, the manual-layer
// metadata arrays for Props and Decals. Layer: PARAMS. Split out of PropInstance_PARAMS.h (ARCH
// §20) once these two types grew full field parity with Params::MarkerInstanceLayer
// (MarkerInstance_PARAMS.h) — too many new fields for PropInstance_PARAMS.h to stay under the
// ARCH §1.5 line ceiling once PropTransform/DecalTransform/PropInstanceGroup/DecalInstanceGroup
// also live there. Hand-written per domain, mirroring MarkerInstanceLayer field-for-field (ARCH
// §19.2/§19.8's standing domain-touching-PARAMS-gets-its-own-struct law, restated at ARCH §20 for
// Props/Decals) — NOT a shared template. `PropInstance_PARAMS.h` includes this file so every
// existing include site of it keeps compiling unchanged.
//
// The one field that does NOT mirror Markers 1:1: `propTypeName` exists only on
// `PropInstanceLayer` ("Prop"/"Reclaim", ARCH §20's Type-Section tag) — `DecalInstanceLayer` has no
// type-tag field at all, since Decals has exactly one Type Section (a UI presentation choice, not a
// PARAMS concept). `propTypeName` is spelled per-domain (not `markerTypeName` reused) per §1.1/§1.8;
// `PropInstanceGroup::bReclaimable` (PropInstance_PARAMS.h, asset-derived) stays permanently
// independent of it, the same closure already ruled for Markers' `category` vs `markerTypeName`.
#pragma once
#include <string>
#include <vector>
#include "Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Params {

struct PropInstanceLayer {
    std::string name;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float iconScale = 1.0f;
    int   layerId = -1;   // stable id, `-1` = unassigned; same convention as MarkerInstanceLayer.
    Params::SymmetrySetting symmetry;   // ARCH §20.1 — the layer's own mirror-mask setting.
    bool  bSymmetryEnabled = true;      // ARCH §19.24's convention, extended to Props.
    bool  bLocked = false;              // Blocks drag/reposition/add/remove for every prop on this layer.
    bool  bHidden = false;              // Hides every prop on this layer from the preview.
    bool  bGridSnapEnabled = false;
    float gridSnapSizeWorldUnits = 1.0f;
    bool  bColorOverrideEnabled = false;   // false: `color` is ignored, resolves the owning
                                            // Type Section's default (GlobalPropSettings) instead.
    int   parentBundleIdentifier = -1;     // -1 = root (ungrouped); PropLayerBundle membership.
    std::string propTypeName;              // "Prop" / "Reclaim" — see file header. Additive.
};

struct DecalInstanceLayer {
    std::string name;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float iconScale = 1.0f;
    int   layerId = -1;
    Params::SymmetrySetting symmetry;
    bool  bSymmetryEnabled = true;
    bool  bLocked = false;
    bool  bHidden = false;
    bool  bGridSnapEnabled = false;
    float gridSnapSizeWorldUnits = 1.0f;
    bool  bColorOverrideEnabled = false;
    int   parentBundleIdentifier = -1;   // -1 = root (ungrouped); DecalLayerBundle membership.
    // No type-tag field — Decals has exactly one Type Section (see file header).
};

// ARCH §14.15's Prop/Decal-side resolvers, moved here verbatim from PropInstance_PARAMS.h: single
// source of truth for resolving a PropTransform/DecalTransform's positional `layerIndex` to its
// owning layer's stable `layerId`, bounds-checked with a -1 sentinel.
inline int ResolvePropInstanceLayerId(int layerIndex, const std::vector<PropInstanceLayer>& layers) {
    if (layerIndex < 0 || static_cast<std::size_t>(layerIndex) >= layers.size()) return -1;
    return layers[layerIndex].layerId;
}
inline int ResolveDecalInstanceLayerId(int layerIndex, const std::vector<DecalInstanceLayer>& layers) {
    if (layerIndex < 0 || static_cast<std::size_t>(layerIndex) >= layers.size()) return -1;
    return layers[layerIndex].layerId;
}

// The same out-of-range-safe posture as the Id resolvers above, resolving the layer's authored
// `color` instead of its stable `layerId` — out-of-range defaults to white (no strong color opinion).
inline void ResolvePropInstanceLayerColor(int layerIndex, const std::vector<PropInstanceLayer>& layers,
                                          float& outRed, float& outGreen, float& outBlue) {
    if (layerIndex < 0 || static_cast<std::size_t>(layerIndex) >= layers.size()) { outRed = outGreen = outBlue = 1.0f; return; }
    outRed = layers[static_cast<std::size_t>(layerIndex)].color[0];
    outGreen = layers[static_cast<std::size_t>(layerIndex)].color[1];
    outBlue = layers[static_cast<std::size_t>(layerIndex)].color[2];
}
inline void ResolveDecalInstanceLayerColor(int layerIndex, const std::vector<DecalInstanceLayer>& layers,
                                           float& outRed, float& outGreen, float& outBlue) {
    if (layerIndex < 0 || static_cast<std::size_t>(layerIndex) >= layers.size()) { outRed = outGreen = outBlue = 1.0f; return; }
    outRed = layers[static_cast<std::size_t>(layerIndex)].color[0];
    outGreen = layers[static_cast<std::size_t>(layerIndex)].color[1];
    outBlue = layers[static_cast<std::size_t>(layerIndex)].color[2];
}

} // namespace Params
} // namespace SanmapGen
