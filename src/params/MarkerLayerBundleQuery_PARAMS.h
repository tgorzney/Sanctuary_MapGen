// MarkerLayerBundleQuery_PARAMS.h — the two ARCH §19.9 recursive-membership resolvers plus their
// shared descendant-collection helper, split out of MarkerLayerBundle_PARAMS.h (STEP119) once that
// file's own line count crossed ARCH §1.5's 150-line hard ceiling — the exact split the ticket
// itself flagged, not an undocumented ceiling violation. Mirrors how
// MapImporter_MarkerLayerReconcile_IO.cpp was split out of MapImporter_Markers_IO.cpp for the
// identical reason (STEP115). Layer: PARAMS, same as its parent file — every function below still
// carries a Params::-typed parameter, so ARCH §3.5/§19.8 places it here, not in MATH.
#pragma once
#include <cstddef>
#include <utility>
#include <vector>
#include "MarkerInstance_PARAMS.h"
#include "MarkerLayerBundle_PARAMS.h"
#include "MarkerRule_PARAMS.h"

namespace SanmapGen {
namespace Params {

// Internal to this header, shared by the two Collect* functions below — not itself part of the
// documented per-domain resolver family. Collects `rootBundleIdentifier` and every identifier
// reachable by descending parentBundleIdentifier child links, into `outIdentifiers` (root always
// first). Cycle-safe: an identifier already collected is never re-expanded, so a corrupt/cyclic
// table (before RepairCyclicMarkerLayerBundleParents has run, MapImporter_MarkerLayerBundle_IO.cpp)
// cannot loop forever — Constitution §6 defensive posture.
inline void CollectMarkerLayerBundleDescendantIdentifiers(int rootBundleIdentifier,
                                                           const std::vector<MarkerLayerBundle>& bundles,
                                                           std::vector<int>& outIdentifiers) {
    outIdentifiers.push_back(rootBundleIdentifier);
    for (std::size_t frontierIndex = 0; frontierIndex < outIdentifiers.size(); ++frontierIndex) {
        const int currentIdentifier = outIdentifiers[frontierIndex];
        for (const MarkerLayerBundle& bundle : bundles) {
            if (bundle.parentBundleIdentifier != currentIdentifier) continue;
            bool bAlreadyCollected = false;
            for (int collected : outIdentifiers) {
                if (collected == bundle.identifier) { bAlreadyCollected = true; break; }
            }
            if (!bAlreadyCollected) outIdentifiers.push_back(bundle.identifier);
        }
    }
}

// ARCH §19.9's WIDE enumeration — every MarkerRuleLayer/MarkerInstanceLayer index (Procedural AND
// Manual) organizationally nested under `bundleIdentifier`, direct or via a descendant Bundle. Feeds
// the tree widget's leaf-enumeration callback (Ticket B); deliberately does NOT filter out
// Procedural layers (they are legitimate tree members, just zero-member for Move/Rotate — see
// CollectMarkerLayerBundleRecursiveManualMembers below, the separate NARROW function).
inline void CollectMarkerLayerBundleRecursiveLayerIndices(int bundleIdentifier,
    const std::vector<MarkerLayerBundle>& bundles, const std::vector<MarkerRuleLayer>& ruleLayers,
    const std::vector<MarkerInstanceLayer>& instanceLayers, std::vector<int>& outRuleLayerIndices,
    std::vector<int>& outInstanceLayerIndices) {
    outRuleLayerIndices.clear();
    outInstanceLayerIndices.clear();
    std::vector<int> inScopeIdentifiers;
    CollectMarkerLayerBundleDescendantIdentifiers(bundleIdentifier, bundles, inScopeIdentifiers);
    for (std::size_t layerIndex = 0; layerIndex < ruleLayers.size(); ++layerIndex)
        for (int identifier : inScopeIdentifiers)
            if (ruleLayers[layerIndex].parentBundleIdentifier == identifier) {
                outRuleLayerIndices.push_back(static_cast<int>(layerIndex));
                break;
            }
    for (std::size_t layerIndex = 0; layerIndex < instanceLayers.size(); ++layerIndex)
        for (int identifier : inScopeIdentifiers)
            if (instanceLayers[layerIndex].parentBundleIdentifier == identifier) {
                outInstanceLayerIndices.push_back(static_cast<int>(layerIndex));
                break;
            }
}

// ARCH §19.9's NARROW enumeration — {markerInstanceGroupIndex, transformIndex} pairs for every
// MarkerTransform whose layerIndex resolves into a MarkerInstanceLayer organizationally nested
// under `bundleIdentifier`. MANUAL ONLY, deliberately excludes Procedural layers (Data::
// PlacementInstances has no cross-bake stable identity to hang a persisted tag on, ARCH §14.8 —
// same restriction, same reasoning, Assembly's own already-ratified AssemblyId scoping, one tier
// up, ARCH §19.9). This is the one function BOTH the tab-driven Move/Rotate Apply (Ticket B) and
// the future CollectAssemblyRecursiveMembership Bundle-walking extension (ARCH §19.5, NOT this
// ticket) call — see this ticket's Out-of-Scope note on why the §19.6 assemblyIdentifier-cutoff rule
// is NOT implemented in this function's own recursion (Params::Assembly does not exist yet).
inline std::vector<std::pair<int,int>> CollectMarkerLayerBundleRecursiveManualMembers(
    int bundleIdentifier, const std::vector<MarkerLayerBundle>& bundles,
    const std::vector<MarkerInstanceLayer>& instanceLayers,
    const std::vector<MarkerInstanceGroup>& markers) {
    std::vector<std::pair<int,int>> outMembers;
    std::vector<int> inScopeIdentifiers;
    CollectMarkerLayerBundleDescendantIdentifiers(bundleIdentifier, bundles, inScopeIdentifiers);
    std::vector<int> inScopeInstanceLayerIndices;
    for (std::size_t layerIndex = 0; layerIndex < instanceLayers.size(); ++layerIndex)
        for (int identifier : inScopeIdentifiers)
            if (instanceLayers[layerIndex].parentBundleIdentifier == identifier) {
                inScopeInstanceLayerIndices.push_back(static_cast<int>(layerIndex));
                break;
            }
    for (std::size_t groupIndex = 0; groupIndex < markers.size(); ++groupIndex) {
        const MarkerInstanceGroup& group = markers[groupIndex];
        for (std::size_t transformIndex = 0; transformIndex < group.transforms.size(); ++transformIndex) {
            const int transformLayerIndex = group.transforms[transformIndex].layerIndex;
            for (int inScopeLayerIndex : inScopeInstanceLayerIndices)
                if (transformLayerIndex == inScopeLayerIndex) {
                    outMembers.emplace_back(static_cast<int>(groupIndex), static_cast<int>(transformIndex));
                    break;
                }
        }
    }
    return outMembers;
}

} // namespace Params
} // namespace SanmapGen
