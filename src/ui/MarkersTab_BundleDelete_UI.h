// MarkersTab_BundleDelete_UI.h — STEP140: the pure delete-apply logic for a Group (Bundle) or a
// Layer leaf, both triggered from the Bundle tree's own header-extra "X"
// (MarkersTab_BundleHeaderExtras_UI.h) and applied by the caller AFTER the tree's recursive walk
// finishes this frame (MarkersTab_UI.cpp) — never mid-walk (see MarkerLayerBundlesState's own
// pending-delete field comments, MarkersTab_Bundles_UI.h). Mutates nothing but its own explicit
// vector parameters; owns no selection-index clamping (the caller re-validates its own tab-state
// selections, same separation ApplyMarkerLayerBundleMove/Rotation already keep from
// MarkerLayerBundlesState::selectedBundleIdentifier).
#pragma once
#include <vector>
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/MarkerLayerBundle_PARAMS.h"
#include "../params/MarkerRule_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// "Group Only": this Bundle is erased, every DIRECT child (sub-Bundle, Rule Layer, Instance Layer)
// is promoted to ITS OWN parent — the exact "Delete Group (promotes children)" behavior the body
// button used to draw, moved here unchanged. Out-of-range identifier is a silent no-op
// (Constitution §6 — an id is validated, never trusted).
void DeleteMarkerLayerBundleGroupOnly(int bundleIdentifier, std::vector<Params::MarkerLayerBundle>& bundles,
                                      std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                      std::vector<Params::MarkerInstanceLayer>& instanceLayers);

// "All": this Bundle, every descendant Bundle, every Layer (Rule or Instance) parented to any of
// them, AND (Instance Layers only) every Instance transform that belonged to one of those layers —
// all erased together.
void DeleteMarkerLayerBundleCascade(int bundleIdentifier, std::vector<Params::MarkerLayerBundle>& bundles,
                                    std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                    std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                    std::vector<Params::MarkerInstanceGroup>& markers);

// A Manual Layer's own "Layer Only": the layer is erased, every Instance that referenced it is
// re-clamped to layer 0 — the SAME safe convention a single ungrouped-layer delete already applies
// (ClampMarkerLayerIndicesForRemovedLayer, MarkerLayerIndexRepair_UI.h), just reachable from a
// bundled/grouped leaf too, which had no delete affordance at all before this ticket.
void DeleteMarkerInstanceLayerOnly(int layerIndex, std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers);

// A Manual Layer's own "All": the layer AND every Instance that referenced it are both erased.
void DeleteMarkerInstanceLayerCascade(int layerIndex, std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                      std::vector<Params::MarkerInstanceGroup>& markers);

// A Procedural Layer's own single delete action — its Rules are owned inline (`layer.rules`), so
// there is no separable "keep the contents" case the way a Manual Layer's Instances give one.
void DeleteMarkerRuleLayer(int layerIndex, std::vector<Params::MarkerRuleLayer>& ruleLayers);

} // namespace Ui
} // namespace SanmapGen
