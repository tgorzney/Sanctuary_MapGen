// ScatterLayerBundle_PARAMS.h — Params::PropLayerBundle/DecalLayerBundle: the Group-above-Layer
// containers ARCH §20 ratifies for Props/Decals, mirroring Params::MarkerLayerBundle
// (MarkerLayerBundle_PARAMS.h) field-for-field. Layer: PARAMS. Hand-written per domain (ARCH
// §19.2/§19.8's standing law, restated at §20) — NOT a shared template with MarkerLayerBundle,
// mirroring ResolvePropInstanceLayerId/ResolveDecalInstanceLayerId's own per-domain-repeated shape.
//
// Sibling of ScatterRule_PARAMS.h/ScatterInstanceLayer_PARAMS.h — parent of neither, same
// not-nested-inside-either posture MarkerLayerBundle takes toward MarkerRule_PARAMS.h/
// MarkerInstance_PARAMS.h.
#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params {

// Wire array `PropLayerBundles`. Additive-only; `MapRecipe::propLayerBundles` is a fresh vector.
// `propTypeName` mirrors `PropInstanceLayer::propTypeName` — see `ScatterInstanceLayer_PARAMS.h`'s
// header note on why this is spelled per-domain, not `markerTypeName` reused.
struct PropLayerBundle {
    int identifier             = -1;   // stable, survives reorder/delete — not this array's position.
    std::string name;
    int parentBundleIdentifier = -1;   // -1 = root; enables Bundle-in-Bundle nesting.
    std::string propTypeName;          // single-Type-Section scope ("Prop"/"Reclaim").
    int assemblyIdentifier     = -1;   // inert Assembly-references-Bundle hook, mirrors Markers'.
};

// Wire array `DecalLayerBundles`. No type-tag field — Decals has exactly one Type Section (see
// `ScatterInstanceLayer_PARAMS.h`'s header note); a single-value tag on every bundle would be dead
// data.
struct DecalLayerBundle {
    int identifier             = -1;
    std::string name;
    int parentBundleIdentifier = -1;
    int assemblyIdentifier     = -1;
};

// True when reparenting `candidateId` under `newParentId` would create a cycle (including
// `candidateId == newParentId` itself). Mirrors WouldReparentMarkerLayerBundleCreateCycle exactly,
// bounded to `bundles.size()+1` steps so an already-corrupt/cyclic table cannot hang the caller.
inline bool WouldReparentPropLayerBundleCreateCycle(int candidateId, int newParentId,
                                                    const std::vector<PropLayerBundle>& bundles) {
    if (candidateId == newParentId) return true;
    int walk = newParentId;
    std::size_t stepsRemaining = bundles.size() + 1;
    while (walk != -1 && stepsRemaining > 0) {
        if (walk == candidateId) return true;
        int nextWalk = -1;
        for (const PropLayerBundle& bundle : bundles) {
            if (bundle.identifier == walk) { nextWalk = bundle.parentBundleIdentifier; break; }
        }
        walk = nextWalk;
        --stepsRemaining;
    }
    return false;
}

inline bool WouldReparentDecalLayerBundleCreateCycle(int candidateId, int newParentId,
                                                     const std::vector<DecalLayerBundle>& bundles) {
    if (candidateId == newParentId) return true;
    int walk = newParentId;
    std::size_t stepsRemaining = bundles.size() + 1;
    while (walk != -1 && stepsRemaining > 0) {
        if (walk == candidateId) return true;
        int nextWalk = -1;
        for (const DecalLayerBundle& bundle : bundles) {
            if (bundle.identifier == walk) { nextWalk = bundle.parentBundleIdentifier; break; }
        }
        walk = nextWalk;
        --stepsRemaining;
    }
    return false;
}

} // namespace Params
} // namespace SanmapGen
