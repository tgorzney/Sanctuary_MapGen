// MarkersTab_BundleTreeSignals_UI.cpp — BuildFilteredMarkerLayerBundlesByType and
// ApplyMarkerLayerBundleTreeSignal, the further aspect-split sibling of MarkersTab_Bundles_UI.cpp
// (ARCH §1.5 — MarkersTab_Bundles_UI.cpp alone crossed the 150-line hard ceiling once ARCH
// §19.15(a)'s filtered-copy plumbing landed there, STEP125 §6's own coder-flagged remediation
// clause), both declared by MarkersTab_Bundles_UI.h — the SAME precedent
// MarkersTab_BundleNodeBody_UI.cpp already established for the identical reason (STEP120).
#include "MarkersTab_Bundles_UI.h"
#include "../params/MarkerLayerBundleQuery_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// The filtered COPY (ARCH §19.15(a)).
std::vector<Params::MarkerLayerBundle> BuildFilteredMarkerLayerBundlesByType(
        const std::vector<Params::MarkerLayerBundle>& bundles, const std::string& markerTypeNameFilter) {
    std::vector<Params::MarkerLayerBundle> filtered;
    for (const Params::MarkerLayerBundle& bundle : bundles)
        if (bundle.markerTypeName == markerTypeNameFilter) filtered.push_back(bundle);
    return filtered;
}

// The Select/Reparent signal-application logic, unchanged from STEP120 — extracted verbatim into a
// named function (STEP125) so a test fixture can drive it directly without an imgui frame.
// sourceNodeIdentifier == -1 with kind == Reparent cannot occur — the root drop zone is a TARGET
// only (DrawRootDropZoneRow never itself emits Select/originates a drag).
void ApplyMarkerLayerBundleTreeSignal(const TreeListSignal<MarkerGroupLeafKey_UI>& signal,
                                      std::vector<Params::MarkerLayerBundle>& bundles,
                                      std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                      std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                      MarkerLayerBundlesState& state) {
    if (signal.kind == TreeListSignalKind::Select && signal.sourceKind == TreeNodeSourceKind::Node)
        state.selectedBundleIdentifier = signal.sourceNodeIdentifier;

    if (signal.kind == TreeListSignalKind::Reparent) {
        if (signal.sourceKind == TreeNodeSourceKind::Leaf) {
            if (signal.sourceLeaf.kind == MarkerGroupLeafKey_UI::Kind::Procedural) {
                if (signal.sourceLeaf.layerIndex >= 0 && signal.sourceLeaf.layerIndex < static_cast<int>(ruleLayers.size()))
                    ruleLayers[static_cast<std::size_t>(signal.sourceLeaf.layerIndex)].parentBundleIdentifier =
                        signal.targetNodeIdentifier;
            } else if (signal.sourceLeaf.layerIndex >= 0
                      && signal.sourceLeaf.layerIndex < static_cast<int>(instanceLayers.size())) {
                instanceLayers[static_cast<std::size_t>(signal.sourceLeaf.layerIndex)].parentBundleIdentifier =
                    signal.targetNodeIdentifier;
            }
        } else if (!Params::WouldReparentMarkerLayerBundleCreateCycle(
                      signal.sourceNodeIdentifier, signal.targetNodeIdentifier, bundles)) {
            int newParent = signal.targetNodeIdentifier;
            if (signal.dropZone != TreeDropZone::OnAsChild) {   // Above/Below: same parent as target (sibling)
                newParent = -1;
                for (const Params::MarkerLayerBundle& target : bundles)
                    if (target.identifier == signal.targetNodeIdentifier) { newParent = target.parentBundleIdentifier; break; }
            }
            for (Params::MarkerLayerBundle& bundle : bundles)
                if (bundle.identifier == signal.sourceNodeIdentifier) { bundle.parentBundleIdentifier = newParent; break; }
        }
    }
}

} // namespace Ui
} // namespace SanmapGen
