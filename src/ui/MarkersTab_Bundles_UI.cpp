// MarkersTab_Bundles_UI.cpp — the tree mechanics: BuildMarkerLayerBundleLeafIndex,
// DrawMarkerLayerBundleTree, and the leaf-body dispatch (reusing DrawRuleLayerSettings/
// DrawLayerRowBody UNCHANGED). A Bundle node's own inline body (rename/Move/Rotate/Delete) is the
// aspect-split sibling MarkersTab_BundleNodeBody_UI.cpp (ARCH §1.5 — see MarkersTab_Bundles_UI.h).
// STEP125: BuildFilteredMarkerLayerBundlesByType/ApplyMarkerLayerBundleTreeSignal moved out to the
// further aspect-split sibling MarkersTab_BundleTreeSignals_UI.cpp — this file alone still crossed
// the 150-line hard ceiling once ARCH §19.15(a)'s filtered-copy plumbing landed here (§6's own
// coder-flagged remediation clause, mirroring the BundleNodeBody split's identical precedent).
#include "MarkersTab_Bundles_UI.h"
#include "MarkersTab_ManualLayerHelpers_UI.h"
#include "MarkersTab_ManualLayerRowBody_UI.h"
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
                             MarkersTabState& rootState, Pipeline::PreviewDriver* previewDriver,
                             const ManualInstanceLayerIndex_UI& instanceIndex) {
    if (leaf.kind == MarkerGroupLeafKey_UI::Kind::Procedural) {
        if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(ruleLayers.size())) return;
        DrawRuleLayerSettings(ruleLayers[static_cast<std::size_t>(leaf.layerIndex)], previewDriver);
    } else {
        if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(instanceLayers.size())) return;
        DrawLayerRowBody(instanceLayers[static_cast<std::size_t>(leaf.layerIndex)], leaf.layerIndex,
                         instanceLayers, markers, geometry, globalSymmetryMask, globalRadialRepeatCount,
                         markerSymmetryFixSettings, rootState.manualLayers,
                         instanceIndex, rootState.selectedManualInstanceIdentifier);
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

// STEP130 (ARCH §19.24, item 7(b)): the Bundle tree's `drawLeafHeaderExtra` body — the FIRST real
// consumer of STEP129's slot. Rule (Procedural) leaves have no color/symmetry field, so the guard
// returns before resolving into `instanceLayers`; a Manual leaf draws [Symmetry toggle][Color
// Override], same order/shape as the flat/ungrouped DraggableList rows. Declared in the header so
// MarkersTab_Bundles_UI_Test.cpp can drive it directly without a full tree frame.
void DrawMarkerGroupLeafHeaderExtra(const MarkerGroupLeafKey_UI& leaf,
                                    std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                    ManualMarkerLayersState& manualLayersState, bool& bAnyCommitted) {
    if (leaf.kind != MarkerGroupLeafKey_UI::Kind::Manual) return;
    if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(instanceLayers.size())) return;
    Params::MarkerInstanceLayer& layer = instanceLayers[static_cast<std::size_t>(leaf.layerIndex)];
    DrawMarkerLayerSymmetryToggleHeaderControl(layer, bAnyCommitted);
    ImGui::SameLine();
    DrawManualMarkerLayerColorOverrideHeaderControl(layer, manualLayersState, bAnyCommitted);
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
                               const std::string& markerTypeNameFilter) {
    if (ImGui::Button("Add Group")) {
        Params::MarkerLayerBundle bundle;
        bundle.identifier     = NextMarkerLayerBundleId(bundles);   // scans the REAL, unfiltered vector
        bundle.name           = "Group";
        bundle.markerTypeName = markerTypeNameFilter;                // ARCH §19.15(a)
        bundles.push_back(bundle);
        state.selectedBundleIdentifier = bundle.identifier;
    }

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
            [](const Params::MarkerLayerBundle& bundle) { return bundle.name.empty() ? "Group" : bundle.name.c_str(); },
            [&](int bundleIdentifier) {
                DrawMarkerLayerBundleNodeBody(bundleIdentifier, bundles, ruleLayers, instanceLayers, markers,
                                              state, rootState, previewDriver);   // unchanged: real bundles
            },
            [&](int bundleIdentifier) -> const std::vector<MarkerGroupLeafKey_UI>& {
                static const std::vector<MarkerGroupLeafKey_UI> kNoLeaves;
                const auto it = leafIndex.leavesByBundleIdentifier.find(bundleIdentifier);
                return it != leafIndex.leavesByBundleIdentifier.end() ? it->second : kNoLeaves;
            },
            [&](const MarkerGroupLeafKey_UI& leaf) { return MarkerGroupLeafLabel(leaf, ruleLayers, instanceLayers); },
            [&](const MarkerGroupLeafKey_UI& leaf) {
                DrawMarkerGroupLeafBody(leaf, ruleLayers, instanceLayers, markers, geometry, globalSymmetryMask,
                                        globalRadialRepeatCount, markerSymmetryFixSettings, rootState, previewDriver,
                                        instanceIndex);
            },
            [](int) {},   // drawNodeHeaderExtra — no-op, Bundle nodes have no color/symmetry field of
                          // their own (ARCH §19.24's controls are per-Layer, not per-Group).
            [&](const MarkerGroupLeafKey_UI& leaf) {
                DrawMarkerGroupLeafHeaderExtra(leaf, instanceLayers, rootState.manualLayers, bHeaderExtraCommitted);
            },
            kMarkerLayerHeaderExtraCombinedWidthPixels,
            state.treeState, state.selectedBundleIdentifier);

    ApplyMarkerLayerBundleTreeSignal(signal, bundles, ruleLayers, instanceLayers, state);
}

} // namespace Ui
} // namespace SanmapGen
