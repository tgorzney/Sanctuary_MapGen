// MarkersTab_Bundles_UI.h — the Markers tab's Group/Bundle tree (STEP120, ARCH §19). Layer: UI.
// Edits recipe.markerLayerBundles (Params::MarkerLayerBundle, STEP119) and the back-reference
// parentBundleIdentifier STEP119 adds to both Params::MarkerRuleLayer and Params::MarkerInstanceLayer.
// Reuses DrawRuleLayerSettings/DrawLayerRowBody UNCHANGED as the tree's leaf-body callbacks
// (ARCH_19_07's "good news" finding, re-verified against live code this ticket).
//
// ARCH §1.5 aspect split (Coder-flagged, mirroring MarkersTab_RuleLayers_UI.h/
// MarkersTab_RuleLayerSettings_UI.cpp): the single MarkersTab_Bundles_UI.cpp the work-order drafted
// exceeded the 150-line hard ceiling once formatted. A Bundle node's own inline body — rename/type
// scope/add-layer-here/Move/Rotate/Delete — moved to the sibling MarkersTab_BundleNodeBody_UI.cpp,
// both fronted by this one header, same split MarkersTab_RuleLayers_UI.h/.cpp already uses.
#pragma once
#include <algorithm>
#include <unordered_map>
#include <vector>
#include "MarkerSymmetryFixCommand_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "TextInput_UI.h"
#include "TreeListWidget_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/MarkerLayerBundle_PARAMS.h"
#include "../params/MarkerLayerBundleQuery_PARAMS.h"
#include "../params/MarkerRule_PARAMS.h"

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct MarkersTabState;
struct IconAtlasManifest;

// A tree LEAF's opaque address: either a MarkerRuleLayer or a MarkerInstanceLayer, by index into its
// own array. Per-consumer, not shared with Assembly's own future AssemblyMemberKey_UI (ARCH_19_07).
struct MarkerGroupLeafKey_UI {
    enum class Kind : int { Procedural = 0, Manual };
    Kind kind       = Kind::Procedural;
    int  layerIndex = -1;
};
inline bool operator==(const MarkerGroupLeafKey_UI& a, const MarkerGroupLeafKey_UI& b) {
    return a.kind == b.kind && a.layerIndex == b.layerIndex;
}

struct MarkerLayerBundlesState {
    SectionState  section;
    TreeListState treeState;
    int           selectedBundleIdentifier = -1;
    // ONE shared scratch triple for whichever Bundle's own expanded node body is currently drawing
    // its Move/Rotate controls — Params::MarkerLayerBundle is a pure round-tripping type and cannot
    // carry UI-only scratch state (same constraint ManualMarkerLayersState's
    // selectedLayerColorToggle/selectedLayerIconScaleToggle already accept, MarkersTab_ManualLayers_UI.h:51-56).
    float             moveOffsetX     = 0.0f;
    float             moveOffsetZ     = 0.0f;
    float             rotationDegrees = 0.0f;
    ScalarSliderRange moveOffsetRange{ -512.0f, 512.0f, 0.0f };        // Constitution §8
    ScalarSliderRange rotationDegreesRange{ -180.0f, 180.0f, 0.0f };
    RealtimeToggle    moveOffsetXToggle{true};
    RealtimeToggle    moveOffsetZToggle{true};
    RealtimeToggle    rotationDegreesToggle{true};
};

// Mints a fresh, never-reused Bundle identifier — the exact NextMarkerLayerId pattern
// (MarkerLayerId_UI.h), applied one tier up.
inline int NextMarkerLayerBundleId(const std::vector<Params::MarkerLayerBundle>& bundles) {
    int maximumId = -1;
    for (const Params::MarkerLayerBundle& bundle : bundles) maximumId = std::max(maximumId, bundle.identifier);
    return maximumId + 1;
}

// Direct (non-recursive) child-Layer enumeration, built ONCE per frame by the caller — deliberately
// NOT CollectMarkerLayerBundleRecursiveLayerIndices (STEP119/§19.9's WIDE, recursive enumeration, a
// different consumer's job): the tree widget's own recursion already walks nested Bundles, so each
// node only needs its OWN direct leaves here.
struct MarkerLayerBundleLeafIndex_UI {
    std::unordered_map<int, std::vector<MarkerGroupLeafKey_UI>> leavesByBundleIdentifier;
};
MarkerLayerBundleLeafIndex_UI BuildMarkerLayerBundleLeafIndex(
    const std::vector<Params::MarkerRuleLayer>& ruleLayers,
    const std::vector<Params::MarkerInstanceLayer>& instanceLayers);

// MarkersTab_BundleNodeBody_UI.cpp — the aspect-split sibling (ARCH §1.5): Move/Rotate, both scoped
// to the Bundle's MANUAL-ONLY resolved membership (§19.9) — a Procedural Layer under a Bundle
// contributes zero members here, by design. Declared here (not anonymous-namespace-local) so
// MarkersTab_Bundles_UI_Test.cpp can exercise each Apply function's own call-boundary behavior.
void ApplyMarkerLayerBundleMove(int bundleIdentifier, const std::vector<Params::MarkerLayerBundle>& bundles,
                                const std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                std::vector<Params::MarkerInstanceGroup>& markers, float offsetX, float offsetZ);
void ApplyMarkerLayerBundleRotation(int bundleIdentifier, const std::vector<Params::MarkerLayerBundle>& bundles,
                                    const std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                    std::vector<Params::MarkerInstanceGroup>& markers, float degrees);

// One Bundle's own inline body, drawn by DrawMarkerLayerBundleTree's own `drawNodeBody` callback
// whenever that node's row is expanded. `rootState` is the whole MarkersTab state (needed for
// DrawAddMarkerRuleLayerButton's MarkersTabState& parameter).
void DrawMarkerLayerBundleNodeBody(int bundleIdentifier, std::vector<Params::MarkerLayerBundle>& bundles,
                                   std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                   std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers,
                                   MarkerLayerBundlesState& state, MarkersTabState& rootState,
                                   Pipeline::PreviewDriver* previewDriver);

// MarkersTab_Bundles_UI.cpp — the tree mechanics:

// The Bundle tree Section. `rootState` is the whole MarkersTab state (needed for
// DrawAddMarkerRuleLayerButton's MarkersTabState& parameter). `geometry`/`globalSymmetryMask`/
// `globalRadialRepeatCount`/`markerSymmetryFixSettings` thread straight through to each expanded
// Manual leaf's DrawLayerRowBody, same parameter list DrawManualMarkerLayers already takes.
void DrawMarkerLayerBundleTree(std::vector<Params::MarkerLayerBundle>& bundles,
                               std::vector<Params::MarkerRuleLayer>& ruleLayers,
                               std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                               std::vector<Params::MarkerInstanceGroup>& markers,
                               const Params::Geometry& geometry, int globalSymmetryMask,
                               int globalRadialRepeatCount,
                               Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                               MarkerLayerBundlesState& state, MarkersTabState& rootState,
                               Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest);

} // namespace Ui
} // namespace SanmapGen
