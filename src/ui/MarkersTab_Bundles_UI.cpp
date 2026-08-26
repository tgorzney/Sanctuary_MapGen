// MarkersTab_Bundles_UI.cpp — the tree mechanics: BuildMarkerLayerBundleLeafIndex,
// DrawMarkerLayerBundleTree, and the leaf-body dispatch (reusing DrawRuleLayerSettings/
// DrawLayerRowBody UNCHANGED). A Bundle node's own inline body (rename/Move/Rotate/Delete) is the
// aspect-split sibling MarkersTab_BundleNodeBody_UI.cpp (ARCH §1.5 — see MarkersTab_Bundles_UI.h).
#include "MarkersTab_Bundles_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include "MarkersTab_RuleLayers_UI.h"
#include "MarkersTab_UI.h"
#include "PlacementRuleSections_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// One Layer LEAF's own inline body — unchanged reuse (ARCH_19_07's "good news" finding).
void DrawMarkerGroupLeafBody(const MarkerGroupLeafKey_UI& leaf, std::vector<Params::MarkerRuleLayer>& ruleLayers,
                             std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                             std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                             int globalSymmetryMask, int globalRadialRepeatCount,
                             Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                             MarkersTabState& rootState, Pipeline::PreviewDriver* previewDriver) {
    if (leaf.kind == MarkerGroupLeafKey_UI::Kind::Procedural) {
        if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(ruleLayers.size())) return;
        DrawRuleLayerSettings(ruleLayers[static_cast<std::size_t>(leaf.layerIndex)], previewDriver);
    } else {
        if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(instanceLayers.size())) return;
        DrawLayerRowBody(instanceLayers[static_cast<std::size_t>(leaf.layerIndex)], leaf.layerIndex,
                         instanceLayers, markers, geometry, globalSymmetryMask, globalRadialRepeatCount,
                         markerSymmetryFixSettings, rootState.manualLayers);
    }
}

const char* MarkerGroupLeafLabel(const MarkerGroupLeafKey_UI& leaf,
                                 const std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                 const std::vector<Params::MarkerInstanceLayer>& instanceLayers) {
    if (leaf.kind == MarkerGroupLeafKey_UI::Kind::Procedural)
        return (leaf.layerIndex >= 0 && leaf.layerIndex < static_cast<int>(ruleLayers.size())
               && !ruleLayers[static_cast<std::size_t>(leaf.layerIndex)].name.empty())
             ? ruleLayers[static_cast<std::size_t>(leaf.layerIndex)].name.c_str() : "Marker Layer";
    return (leaf.layerIndex >= 0 && leaf.layerIndex < static_cast<int>(instanceLayers.size()))
         ? ManualMarkerLayerRowLabel(instanceLayers[static_cast<std::size_t>(leaf.layerIndex)]) : "Marker Layer";
}

} // namespace

MarkerLayerBundleLeafIndex_UI BuildMarkerLayerBundleLeafIndex(
        const std::vector<Params::MarkerRuleLayer>& ruleLayers,
        const std::vector<Params::MarkerInstanceLayer>& instanceLayers) {
    MarkerLayerBundleLeafIndex_UI index;
    for (int i = 0; i < static_cast<int>(ruleLayers.size()); ++i)
        if (ruleLayers[static_cast<std::size_t>(i)].parentBundleIdentifier >= 0)
            index.leavesByBundleIdentifier[ruleLayers[static_cast<std::size_t>(i)].parentBundleIdentifier]
                .push_back(MarkerGroupLeafKey_UI{MarkerGroupLeafKey_UI::Kind::Procedural, i});
    for (int i = 0; i < static_cast<int>(instanceLayers.size()); ++i)
        if (instanceLayers[static_cast<std::size_t>(i)].parentBundleIdentifier >= 0)
            index.leavesByBundleIdentifier[instanceLayers[static_cast<std::size_t>(i)].parentBundleIdentifier]
                .push_back(MarkerGroupLeafKey_UI{MarkerGroupLeafKey_UI::Kind::Manual, i});
    return index;
}

void DrawMarkerLayerBundleTree(std::vector<Params::MarkerLayerBundle>& bundles,
                               std::vector<Params::MarkerRuleLayer>& ruleLayers,
                               std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                               std::vector<Params::MarkerInstanceGroup>& markers,
                               const Params::Geometry& geometry, int globalSymmetryMask,
                               int globalRadialRepeatCount,
                               Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                               MarkerLayerBundlesState& state, MarkersTabState& rootState,
                               Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest*) {
    if (!DrawSectionBegin("Groups", state.section)) return;

    if (ImGui::Button("Add Group")) {
        Params::MarkerLayerBundle bundle;
        bundle.identifier = NextMarkerLayerBundleId(bundles);
        bundle.name       = "Group";
        bundles.push_back(bundle);
        state.selectedBundleIdentifier = bundle.identifier;
    }

    const MarkerLayerBundleLeafIndex_UI leafIndex = BuildMarkerLayerBundleLeafIndex(ruleLayers, instanceLayers);

    const TreeListSignal<MarkerGroupLeafKey_UI> signal =
        TreeListWidget_UI<Params::MarkerLayerBundle, MarkerGroupLeafKey_UI>::Render(
            "markerLayerBundles", bundles,
            [](const Params::MarkerLayerBundle& bundle) { return bundle.identifier; },
            [](const Params::MarkerLayerBundle& bundle) { return bundle.parentBundleIdentifier; },
            [](const Params::MarkerLayerBundle& bundle) { return bundle.name.empty() ? "Group" : bundle.name.c_str(); },
            [&](int bundleIdentifier) {
                DrawMarkerLayerBundleNodeBody(bundleIdentifier, bundles, ruleLayers, instanceLayers, markers,
                                              state, rootState, previewDriver);
            },
            [&](int bundleIdentifier) -> const std::vector<MarkerGroupLeafKey_UI>& {
                static const std::vector<MarkerGroupLeafKey_UI> kNoLeaves;
                const auto it = leafIndex.leavesByBundleIdentifier.find(bundleIdentifier);
                return it != leafIndex.leavesByBundleIdentifier.end() ? it->second : kNoLeaves;
            },
            [&](const MarkerGroupLeafKey_UI& leaf) { return MarkerGroupLeafLabel(leaf, ruleLayers, instanceLayers); },
            [&](const MarkerGroupLeafKey_UI& leaf) {
                DrawMarkerGroupLeafBody(leaf, ruleLayers, instanceLayers, markers, geometry, globalSymmetryMask,
                                        globalRadialRepeatCount, markerSymmetryFixSettings, rootState, previewDriver);
            },
            state.treeState, state.selectedBundleIdentifier);

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
        // sourceNodeIdentifier == -1 with kind == Reparent cannot occur — the root drop zone is a
        // TARGET only (DrawRootDropZoneRow never itself emits Select/originates a drag).
    }

    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
