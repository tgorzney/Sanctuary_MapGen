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
#include <string>
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
struct ManualMarkerLayersState;   // MarkersTab_ManualLayers_UI.h — forward-declared, reference-only.

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

// STEP130 (ARCH §19.24, item 7(b)): the Bundle tree's own `drawLeafHeaderExtra` body (see
// MarkersTab_Bundles_UI.cpp). Declared here so MarkersTab_Bundles_UI_Test.cpp can drive it directly.
void DrawMarkerGroupLeafHeaderExtra(const MarkerGroupLeafKey_UI& leaf,
                                    std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                    ManualMarkerLayersState& manualLayersState, bool& bAnyCommitted);

// MarkersTab_Bundles_UI.cpp — the tree mechanics:

// The filtered COPY (ARCH §19.15(a)) — safe to pass as TreeListWidget_UI's `nodes` parameter because
// Render uses `nodes` for tree LAYOUT only; every mutation path resolves the REAL
// Params::MarkerLayerBundle& by identifier-keyed lookup into the caller's own `bundles` vector
// (ApplyMarkerLayerBundleTreeSignal, below), never by position within this copy.
std::vector<Params::MarkerLayerBundle> BuildFilteredMarkerLayerBundlesByType(
    const std::vector<Params::MarkerLayerBundle>& bundles, const std::string& markerTypeNameFilter);

// The Select/Reparent signal-application logic DrawMarkerLayerBundleTree already ran inline
// (STEP120) — extracted verbatim, UNCHANGED behavior, purely so it has a name and can be driven
// directly by a test fixture without an imgui frame (STEP125's own required "filtered-copy write
// safety" coverage, see Verify).
void ApplyMarkerLayerBundleTreeSignal(const TreeListSignal<MarkerGroupLeafKey_UI>& signal,
                                      std::vector<Params::MarkerLayerBundle>& bundles,
                                      std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                      std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                      MarkerLayerBundlesState& state);

// The Bundle tree mechanics — no Section wrap of its own (STEP125: the Type-section's own outer
// DrawSectionBegin, MarkersTab_TypeSections_UI.cpp, supplies that collapsible now). `rootState` is
// the whole MarkersTab state (needed for DrawAddMarkerRuleLayerButton's MarkersTabState& parameter).
// `geometry`/`globalSymmetryMask`/`globalRadialRepeatCount`/`markerSymmetryFixSettings` thread
// straight through to each expanded Manual leaf's DrawLayerRowBody. `markerTypeNameFilter`
// (ARCH §19.15(a)) scopes both the tree's own filtered copy and "Add Group"'s seeded
// `bundle.markerTypeName` — every call site is now type-scoped, there is no more "root/global" tree
// render.
void DrawMarkerLayerBundleTree(std::vector<Params::MarkerLayerBundle>& bundles,
                               std::vector<Params::MarkerRuleLayer>& ruleLayers,
                               std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                               std::vector<Params::MarkerInstanceGroup>& markers,
                               const Params::Geometry& geometry, int globalSymmetryMask,
                               int globalRadialRepeatCount,
                               Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                               MarkerLayerBundlesState& state, MarkersTabState& rootState,
                               Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                               const std::string& markerTypeNameFilter);

} // namespace Ui
} // namespace SanmapGen
