// MarkersTab_TypeSectionPendingApply_UI.h — STEP254: the Bundle tree's own header-extra "X"/drop
// pending mutations, applied AFTER the tree's recursive walk finishes this frame (never mid-walk —
// see MarkerLayerBundlesState's own pending-field comments, MarkersTab_Bundles_UI.h), relocated out
// of MarkersTab_UI.cpp's file-size-ceiling split into 4 SEPARATELY named functions: the bundle case
// is ID-equality-only, the manual-layer case is positional-index-with-decrement, the procedural-layer
// case is positional-index-with-decrement PLUS resets `selectedRuleIndex` and sets `bRecipeMoved`,
// and the create-layer case has no delete/cascade branching at all — confirmed NOT structurally
// identical, so this file never merges them into one generic function.
#pragma once
#include <vector>
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/MarkerLayerBundle_PARAMS.h"
#include "../params/MarkerRule_PARAMS.h"

namespace SanmapGen {
namespace Ui {

struct MarkerLayerBundlesState;

void ApplyPendingBundleDelete(MarkerLayerBundlesState& bundlesState,
                              std::vector<Params::MarkerLayerBundle>& bundles,
                              std::vector<Params::MarkerRuleLayer>& ruleLayers,
                              std::vector<Params::MarkerInstanceLayer>& markerLayers,
                              std::vector<Params::MarkerInstanceGroup>& markers);

void ApplyPendingManualLayerDelete(MarkerLayerBundlesState& bundlesState, int& selectedManualLayerIndex,
                                   std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers);

// Returns true only when a delete was actually pending and applied this call — caller ORs this into
// its own accumulated bRecipeMoved, mirroring the original inline block's exact behavior.
bool ApplyPendingProceduralLayerDelete(MarkerLayerBundlesState& bundlesState, int& selectedRuleLayerIndex,
                                       int& selectedRuleIndex,
                                       std::vector<Params::MarkerRuleLayer>& ruleLayers);

// Thin wrapper around the PRE-EXISTING MarkersTab_Bundles_UI.h::ApplyPendingCreateLayerForBundle
// (unchanged, NOT duplicated here) — this function only relocates the guard + call + 3-field reset
// that used to sit inline in DrawMarkersTab (lines 460–468), so it reads as the 4th named applier
// alongside the 3 above rather than a bare unnamed if-block.
void ApplyPendingCreateLayerForBundleIfRequested(MarkerLayerBundlesState& bundlesState,
                                                 std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                                 std::vector<Params::MarkerInstanceGroup>& markers);

} // namespace Ui
} // namespace SanmapGen
