// MarkersTab_BundleDelete_UI.cpp — see MarkersTab_BundleDelete_UI.h.
#include "MarkersTab_BundleDelete_UI.h"
#include "MarkerLayerIndexRepair_UI.h"
#include <algorithm>

namespace SanmapGen {
namespace Ui {
namespace {

// Every bundle identifier equal to `bundleIdentifier` or descended from it (self-inclusive) — the
// exact set "All" reaches into.
void CollectBundleAndDescendants(int bundleIdentifier, const std::vector<Params::MarkerLayerBundle>& bundles,
                                 std::vector<int>& outIdentifiers) {
    outIdentifiers.push_back(bundleIdentifier);
    for (const Params::MarkerLayerBundle& candidate : bundles)
        if (candidate.parentBundleIdentifier == bundleIdentifier)
            CollectBundleAndDescendants(candidate.identifier, bundles, outIdentifiers);
}

bool Contains(const std::vector<int>& values, int value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

} // namespace

void DeleteMarkerLayerBundleGroupOnly(int bundleIdentifier, std::vector<Params::MarkerLayerBundle>& bundles,
                                      std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                      std::vector<Params::MarkerInstanceLayer>& instanceLayers) {
    const auto bundleIt = std::find_if(bundles.begin(), bundles.end(),
        [&](const Params::MarkerLayerBundle& candidate) { return candidate.identifier == bundleIdentifier; });
    if (bundleIt == bundles.end()) return;
    const int parentIdentifier = bundleIt->parentBundleIdentifier;
    for (Params::MarkerLayerBundle& candidate : bundles)
        if (candidate.parentBundleIdentifier == bundleIdentifier) candidate.parentBundleIdentifier = parentIdentifier;
    for (Params::MarkerRuleLayer& layer : ruleLayers)
        if (layer.parentBundleIdentifier == bundleIdentifier) layer.parentBundleIdentifier = parentIdentifier;
    for (Params::MarkerInstanceLayer& layer : instanceLayers)
        if (layer.parentBundleIdentifier == bundleIdentifier) layer.parentBundleIdentifier = parentIdentifier;
    bundles.erase(bundleIt);
}

void DeleteMarkerLayerBundleCascade(int bundleIdentifier, std::vector<Params::MarkerLayerBundle>& bundles,
                                    std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                    std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                    std::vector<Params::MarkerInstanceGroup>& markers) {
    std::vector<int> deletedBundleIdentifiers;
    CollectBundleAndDescendants(bundleIdentifier, bundles, deletedBundleIdentifiers);

    // Instance Layers: cascade their own Instances too, one layer at a time (highest index first)
    // so each DeleteMarkerInstanceLayerCascade's own index-shift math stays correct for the rest.
    for (int layerIndex = static_cast<int>(instanceLayers.size()) - 1; layerIndex >= 0; --layerIndex)
        if (Contains(deletedBundleIdentifiers, instanceLayers[static_cast<std::size_t>(layerIndex)].parentBundleIdentifier))
            DeleteMarkerInstanceLayerCascade(layerIndex, instanceLayers, markers);

    // Rule Layers: no cross-referenced content to cascade, a plain erase.
    for (int i = static_cast<int>(ruleLayers.size()) - 1; i >= 0; --i)
        if (Contains(deletedBundleIdentifiers, ruleLayers[static_cast<std::size_t>(i)].parentBundleIdentifier))
            DeleteMarkerRuleLayer(i, ruleLayers);

    // The Bundles themselves, this one and every descendant.
    for (int i = static_cast<int>(bundles.size()) - 1; i >= 0; --i)
        if (Contains(deletedBundleIdentifiers, bundles[static_cast<std::size_t>(i)].identifier))
            bundles.erase(bundles.begin() + i);
}

void DeleteMarkerInstanceLayerOnly(int layerIndex, std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(instanceLayers.size())) return;
    ClampMarkerLayerIndicesForRemovedLayer(markers, layerIndex);
    instanceLayers.erase(instanceLayers.begin() + layerIndex);
}

void DeleteMarkerInstanceLayerCascade(int layerIndex, std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                      std::vector<Params::MarkerInstanceGroup>& markers) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(instanceLayers.size())) return;
    for (Params::MarkerInstanceGroup& group : markers)
        for (int t = static_cast<int>(group.transforms.size()) - 1; t >= 0; --t)
            if (group.transforms[static_cast<std::size_t>(t)].layerIndex == layerIndex)
                group.transforms.erase(group.transforms.begin() + t);
    // Every transform pointing PAST the removed layer shifts down one — the same convention
    // ClampMarkerLayerIndicesForRemovedLayer already uses for its own "keep" case.
    for (Params::MarkerInstanceGroup& group : markers)
        for (Params::MarkerTransform& transform : group.transforms)
            if (transform.layerIndex > layerIndex) --transform.layerIndex;
    instanceLayers.erase(instanceLayers.begin() + layerIndex);
}

void DeleteMarkerRuleLayer(int layerIndex, std::vector<Params::MarkerRuleLayer>& ruleLayers) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(ruleLayers.size())) return;
    ruleLayers.erase(ruleLayers.begin() + layerIndex);
}

} // namespace Ui
} // namespace SanmapGen
