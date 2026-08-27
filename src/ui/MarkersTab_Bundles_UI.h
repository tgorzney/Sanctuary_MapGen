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
#include <functional>
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

    // STEP140: the body no longer draws Name/Move/Rotate — the header-extra slot now carries
    // rename (double-click) and delete (see MarkersTab_BundleHeaderExtras_UI.h). -1 = no rename;
    // renaming edits `bundle.name` directly (non-structural, safe mid-walk — unlike delete/reorder,
    // nothing else this frame indexes by name), so there is no separate scratch buffer to hold.
    int renamingBundleIdentifier = -1;

    // STEP140: pending deletes, applied AFTER the tree's walk finishes this frame — never mid-walk,
    // see MarkersTab_BundleDelete_UI.h. Bundle: Group Only vs cascade All. Manual Layer: Layer Only
    // vs cascade All. Procedural Layer: single action (its Rules ARE the layer, nothing to keep).
    int  pendingDeleteBundleIdentifier     = -1;
    bool bPendingDeleteBundleCascade       = false;
    int  pendingDeleteManualLayerIndex     = -1;
    bool bPendingDeleteManualLayerCascade  = false;
    int  pendingDeleteProceduralLayerIndex = -1;
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
// whenever that node's row is expanded. STEP140/human's own correction: intentionally empty now —
// Name/Move/Rotate/Delete all moved out (rename + delete now live in the header-extra slot,
// MarkersTab_BundleHeaderExtras_UI.h); kept as a real function (not a bare `[](int){}` inline) so
// the call site and this declaration don't need to change again if the body gains real content
// later. `rootState` is unused today but kept for the same reason.
void DrawMarkerLayerBundleNodeBody(int bundleIdentifier, std::vector<Params::MarkerLayerBundle>& bundles,
                                   std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                   std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers,
                                   MarkerLayerBundlesState& state, MarkersTabState& rootState,
                                   Pipeline::PreviewDriver* previewDriver);

// MarkersTab_BundleHeaderExtras_UI.cpp (STEP140) — the Bundle tree's `drawNodeHeaderExtra`/
// `drawLeafHeaderExtra` bodies: a Group's own double-click-to-rename + "X" delete (Group Only/All),
// and a Layer leaf's own "X" delete (Manual: Layer Only/All; Procedural: single action), alongside
// STEP130's pre-existing Symmetry/Color Override controls on Manual leaves. Both record into
// `MarkerLayerBundlesState`'s pending-rename/pending-delete fields ONLY — never mutate
// bundles/ruleLayers/instanceLayers directly (mid-tree-walk mutation would desync the walk's own
// position-based lookups for the rest of this frame); the caller applies them AFTER the tree
// returns (MarkersTab_BundleDelete_UI.h). Declared here so MarkersTab_Bundles_UI_Test.cpp can drive
// both directly.
void DrawMarkerLayerBundleNodeHeaderExtra(int bundleIdentifier,
                                          std::vector<Params::MarkerLayerBundle>& bundles,
                                          MarkerLayerBundlesState& state);
void DrawMarkerGroupLeafHeaderExtra(const MarkerGroupLeafKey_UI& leaf,
                                    std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                    ManualMarkerLayersState& manualLayersState,
                                    MarkerLayerBundlesState& bundlesState, bool& bAnyCommitted);

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
// ARCH §19.25, item 5: `selectManualMarkerInstanceCallback` threads straight through to the tree's
// Manual leaf body (DrawLayerRowBody, via DrawMarkerGroupLeafBody). Empty default — every existing
// call site compiles unchanged.
void DrawMarkerLayerBundleTree(std::vector<Params::MarkerLayerBundle>& bundles,
                               std::vector<Params::MarkerRuleLayer>& ruleLayers,
                               std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                               std::vector<Params::MarkerInstanceGroup>& markers,
                               const Params::Geometry& geometry, int globalSymmetryMask,
                               int globalRadialRepeatCount,
                               Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                               MarkerLayerBundlesState& state, MarkersTabState& rootState,
                               Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                               const std::string& markerTypeNameFilter,
                               const std::function<void(int)>& selectManualMarkerInstanceCallback = {});

} // namespace Ui
} // namespace SanmapGen
