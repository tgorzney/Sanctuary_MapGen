// MarkersTab_Bundles_UI.cpp — the tree mechanics: BuildMarkerLayerBundleLeafIndex,
// DrawMarkerLayerBundleTree, and the leaf-body dispatch (reusing DrawRuleLayerSettings/
// DrawLayerRowBody UNCHANGED). A Bundle node's own inline body (rename/Move/Rotate/Delete) is the
// aspect-split sibling MarkersTab_BundleNodeBody_UI.cpp (ARCH §1.5 — see MarkersTab_Bundles_UI.h).
// STEP125: BuildFilteredMarkerLayerBundlesByType/ApplyMarkerLayerBundleTreeSignal moved out to the
// further aspect-split sibling MarkersTab_BundleTreeSignals_UI.cpp — this file alone still crossed
// the 150-line hard ceiling once ARCH §19.15(a)'s filtered-copy plumbing landed here (§6's own
// coder-flagged remediation clause, mirroring the BundleNodeBody split's identical precedent).
#include "MarkersTab_Bundles_UI.h"
#include "MarkerLayerId_UI.h"
#include "MarkersTab_ManualInstanceSelection_UI.h"
#include "MarkersTab_ManualLayerHelpers_UI.h"
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include "MarkersTab_MarkerLinkResolvers_UI.h"
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
                             MarkersTabState& rootState, Pipeline::PreviewDriver* previewDriver,
                             const ManualInstanceLayerIndex_UI& instanceIndex,
                             const std::function<void(int clickedInstanceIdentifier,
                                                      const std::vector<int>& selectedInstanceIdentifiers)>&
                                 selectManualMarkerInstanceCallback) {
    if (leaf.kind == MarkerGroupLeafKey_UI::Kind::Procedural) {
        if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(ruleLayers.size())) return;
        DrawRuleLayerSettings(ruleLayers[static_cast<std::size_t>(leaf.layerIndex)], previewDriver);
    } else {
        if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(instanceLayers.size())) return;
        DrawLayerRowBody(instanceLayers[static_cast<std::size_t>(leaf.layerIndex)], leaf.layerIndex,
                         instanceLayers, markers, geometry, globalSymmetryMask, globalRadialRepeatCount,
                         markerSymmetryFixSettings, rootState.manualLayers,
                         instanceIndex, rootState.selectedManualInstanceIdentifier,
                         rootState.selectedManualInstanceIdentifiers,
                         rootState.manualInstanceSelectionAnchorIdentifier,
                         selectManualMarkerInstanceCallback);
    }
}

// STEP241, ARCH §19.31 correction: a Manual leaf's own displayed label resolves through `links`
// (the two-arg ManualMarkerLayerRowLabel, MarkersTab_MarkerLinkResolvers_UI.h) instead of the raw
// `name` field, so a Link-bound Layer's tree row shows the Link's live name — the same treatment
// DrawMarkerLayerBundleTree's own node-label lambda (below) already gives a Bundle's name.
const char* MarkerGroupLeafLabel(const MarkerGroupLeafKey_UI& leaf,
                                 const std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                 const std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                 const std::vector<Params::MarkerLink>& links) {
    if (leaf.kind == MarkerGroupLeafKey_UI::Kind::Procedural)
        return (leaf.layerIndex >= 0 && leaf.layerIndex < static_cast<int>(ruleLayers.size())
               && !ruleLayers[static_cast<std::size_t>(leaf.layerIndex)].name.empty())
             ? ruleLayers[static_cast<std::size_t>(leaf.layerIndex)].name.c_str() : "Marker Layer";
    return (leaf.layerIndex >= 0 && leaf.layerIndex < static_cast<int>(instanceLayers.size()))
         ? ManualMarkerLayerRowLabel(instanceLayers[static_cast<std::size_t>(leaf.layerIndex)], links)
         : "Marker Layer";
}

} // namespace

// DrawMarkerLayerBundleNodeHeaderExtra/DrawMarkerGroupLeafHeaderExtra (STEP130/STEP140) now live in
// the aspect-split sibling MarkersTab_BundleHeaderExtras_UI.cpp — this file had no headroom left.

int FirstManualLayerIndexInBundle(int bundleIdentifier,
                                  const std::vector<Params::MarkerInstanceLayer>& instanceLayers) {
    for (int i = 0; i < static_cast<int>(instanceLayers.size()); ++i)
        if (instanceLayers[static_cast<std::size_t>(i)].parentBundleIdentifier == bundleIdentifier) return i;
    return -1;
}

void ApplyPendingCreateLayerForBundle(int bundleIdentifier, const std::string& markerTypeName,
                                      const std::vector<int>& instanceIdentifiers,
                                      std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                      std::vector<Params::MarkerInstanceGroup>& markers) {
    Params::MarkerInstanceLayer newLayer;
    newLayer.name                   = NextMarkerLayerName(static_cast<int>(markerLayers.size()));
    newLayer.layerId                = NextMarkerLayerId(markerLayers);
    newLayer.parentBundleIdentifier = bundleIdentifier;
    newLayer.markerTypeName         = markerTypeName;
    markerLayers.push_back(newLayer);
    ReassignManualInstanceLayers(markers, instanceIdentifiers, static_cast<int>(markerLayers.size()) - 1);
}

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
                               Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest*,
                               const std::string& markerTypeNameFilter,
                               const std::function<void(int clickedInstanceIdentifier,
                                                        const std::vector<int>& selectedInstanceIdentifiers)>&
                                   selectManualMarkerInstanceCallback,
                               const std::vector<Params::MarkerLink>& links) {
    // STEP138/human's own correction: no "Add Group" button here — the Type-section header's own
    // "+ Group" (MarkersTab_UI.cpp) already owns this job; a second one here duplicated it and the
    // two, both minting via NextMarkerLayerBundleId against the same vector, produced confusing
    // double-adds. `markerTypeNameFilter` stays a parameter (still scopes the tree below) even though
    // it no longer seeds a button here.
    const std::vector<Params::MarkerLayerBundle> filteredBundles =
        BuildFilteredMarkerLayerBundlesByType(bundles, markerTypeNameFilter);
    const MarkerLayerBundleLeafIndex_UI leafIndex = BuildMarkerLayerBundleLeafIndex(ruleLayers, instanceLayers);
    // ^ UNCHANGED, still built over the full ruleLayers/instanceLayers: a Bundle's own DIRECT leaves
    // are looked up by that Bundle's own (globally unique) identifier regardless of which
    // Type-section is currently rendering, so this index needs no filtering of its own.
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);
    bool bHeaderExtraCommitted = false;   // STEP130: discarded — no downstream consumer today, same
                                          // posture as DrawLayerList's own bAnyNameCommitted before a
                                          // future name-uniqueness-style repair needs it here too.

    const TreeListSignal<MarkerGroupLeafKey_UI> signal =
        TreeListWidget_UI<Params::MarkerLayerBundle, MarkerGroupLeafKey_UI>::Render(
            "markerLayerBundles", filteredBundles,   // <-- the ONLY thing that changed inside Render's
            [](const Params::MarkerLayerBundle& bundle) { return bundle.identifier; },              // own call: filteredBundles, not bundles.
            [](const Params::MarkerLayerBundle& bundle) { return bundle.parentBundleIdentifier; },
            [&](const Params::MarkerLayerBundle& bundle) {
                // STEP241, ARCH §19.31 correction: a Link-bound Group's own displayed name resolves
                // live from the Link (EffectiveMarkerLayerBundleName) instead of the raw `name` field.
                const std::string& effectiveName = EffectiveMarkerLayerBundleName(bundle, links);
                return effectiveName.empty() ? "Group" : effectiveName.c_str();
            },
            [&](int bundleIdentifier) {
                DrawMarkerLayerBundleNodeBody(bundleIdentifier, bundles, ruleLayers, instanceLayers, markers,
                                              state, rootState, previewDriver);   // unchanged: real bundles
            },
            [&](int bundleIdentifier) -> const std::vector<MarkerGroupLeafKey_UI>& {
                static const std::vector<MarkerGroupLeafKey_UI> kNoLeaves;
                const auto it = leafIndex.leavesByBundleIdentifier.find(bundleIdentifier);
                return it != leafIndex.leavesByBundleIdentifier.end() ? it->second : kNoLeaves;
            },
            [&](const MarkerGroupLeafKey_UI& leaf) { return MarkerGroupLeafLabel(leaf, ruleLayers, instanceLayers, links); },
            [&](const MarkerGroupLeafKey_UI& leaf) {
                DrawMarkerGroupLeafBody(leaf, ruleLayers, instanceLayers, markers, geometry, globalSymmetryMask,
                                        globalRadialRepeatCount, markerSymmetryFixSettings, rootState, previewDriver,
                                        instanceIndex, selectManualMarkerInstanceCallback);
            },
            [&](int bundleIdentifier) {   // STEP140 — a Group's own rename/delete
                DrawMarkerLayerBundleNodeHeaderExtra(bundleIdentifier, bundles, instanceLayers, markers,
                                                     rootState.selectedManualInstanceIdentifiers, state);
            },
            [&](const MarkerGroupLeafKey_UI& leaf) {
                DrawMarkerGroupLeafHeaderExtra(leaf, ruleLayers, instanceLayers, markers, rootState.manualLayers,
                                               state, rootState.selectedManualInstanceIdentifiers, previewDriver,
                                               bHeaderExtraCommitted, links);
            },
            kMarkerLayerHeaderExtraCombinedWidthPixels,
            state.treeState, state.selectedBundleIdentifier, state.selectedLeaf);

    ApplyMarkerLayerBundleTreeSignal(signal, bundles, ruleLayers, instanceLayers, markers, instanceIndex, state,
                                     rootState.selectedManualInstanceIdentifier,
                                     rootState.selectedManualInstanceIdentifiers,
                                     rootState.manualInstanceSelectionAnchorIdentifier);
}

} // namespace Ui
} // namespace SanmapGen
