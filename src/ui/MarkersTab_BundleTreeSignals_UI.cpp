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
                                      const std::vector<Params::MarkerInstanceGroup>& markers,
                                      const ManualInstanceLayerIndex_UI& instanceIndex,
                                      MarkerLayerBundlesState& state, int& selectedManualInstanceIdentifier,
                                      std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
                                      const std::function<void(int clickedInstanceIdentifier,
                                                               const std::vector<int>& selectedInstanceIdentifiers)>&
                                          selectManualMarkerInstanceCallback) {
    if (signal.kind == TreeListSignalKind::Select) {
        if (signal.sourceKind == TreeNodeSourceKind::Node) {
            state.selectedBundleIdentifier = signal.sourceNodeIdentifier;
            // Human's own bug report (Bug 1) — a single click on a GROUP header now does what a Layer
            // header already did below: selects every Instance organizationally under it. RECURSIVE
            // (nested sub-Groups included) and MANUAL-ONLY (a Procedural layer contributes no members) —
            // the SAME resolution ApplyMarkerLayerBundleMove/Rotation already use
            // (Params::CollectMarkerLayerBundleRecursiveManualMembers, MarkerLayerBundleQuery_PARAMS.h,
            // MarkersTab_BundleNodeBody_UI.cpp), NOT Delete's own "Group Only" narrower scope — that
            // split exists because deleting is destructive and needs an escape hatch; selecting has no
            // analogous need, so there is no "Group Only" select mode to offer.
            selectedManualInstanceIdentifiers.clear();
            const std::vector<std::pair<int, int>> members =
                Params::CollectMarkerLayerBundleRecursiveManualMembers(signal.sourceNodeIdentifier, bundles,
                                                                       instanceLayers, markers);
            for (const std::pair<int, int>& groupTransformIndex : members)
                selectedManualInstanceIdentifiers.push_back(
                    markers[static_cast<std::size_t>(groupTransformIndex.first)]
                        .transforms[static_cast<std::size_t>(groupTransformIndex.second)]
                        .instanceIdentifier);
            anchorIdentifier = selectedManualInstanceIdentifiers.empty()
                              ? -1 : selectedManualInstanceIdentifiers.front();
            selectedManualInstanceIdentifier = anchorIdentifier;
            // Human's own bug report (Bug 2) — writing the three tabState fields above alone leaves the
            // canvas's own independent selection copy (MapCanvas::selectedInstanceKeys) stale; only this
            // callback keeps it in sync (MapCanvas::SyncManualMarkerSelection). Guarded exactly like the
            // established call site (MarkersTab_ManualLayerRowBody_UI.cpp's own
            // `if (interaction.selectManualMarkerInstanceCallback)`).
            if (selectManualMarkerInstanceCallback)
                selectManualMarkerInstanceCallback(anchorIdentifier, selectedManualInstanceIdentifiers);
        } else {
            // Human's own bug report — a single click on a Layer header selects that Layer (the
            // highlight) AND every Instance it owns (a Procedural leaf owns none, so it clears the
            // manual selection instead — it is not a "no selection change" no-op).
            state.selectedLeaf = signal.sourceLeaf;
            selectedManualInstanceIdentifiers.clear();
            if (signal.sourceLeaf.kind == MarkerGroupLeafKey_UI::Kind::Manual) {
                const auto memberIt = instanceIndex.instancesByLayerIndex.find(signal.sourceLeaf.layerIndex);
                if (memberIt != instanceIndex.instancesByLayerIndex.end())
                    for (const std::pair<int, int>& groupTransformIndex : memberIt->second)
                        selectedManualInstanceIdentifiers.push_back(
                            markers[static_cast<std::size_t>(groupTransformIndex.first)]
                                .transforms[static_cast<std::size_t>(groupTransformIndex.second)]
                                .instanceIdentifier);
            }
            anchorIdentifier = selectedManualInstanceIdentifiers.empty()
                              ? -1 : selectedManualInstanceIdentifiers.front();
            selectedManualInstanceIdentifier = anchorIdentifier;
            // Bug 2 fix — same reasoning as the Node branch above.
            if (selectManualMarkerInstanceCallback)
                selectManualMarkerInstanceCallback(anchorIdentifier, selectedManualInstanceIdentifiers);
        }
    }

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
