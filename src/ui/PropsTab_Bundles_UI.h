// PropsTab_Bundles_UI.h — the Props tab's Group/Bundle tree (ARCH §20; mirrors
// MarkersTab_Bundles_UI.h at a deliberately trimmed scope). Layer: UI.
//
// Trims relative to the Marker original, both load-bearing, not omissions: (1) Props has no
// procedural rule-layer leaves yet — `Params::PropRuleLayer` does not exist (its two-tier
// restructuring is gated behind a separate IO Architecture Expert consult, ARCH §20.5), so this
// tree has exactly ONE leaf kind (a manual `Params::PropInstanceLayer` index — a bare `int`, no
// `Kind` sum type needed). (2) There is no per-instance multi-select/canvas-drag substrate for
// Props yet (gated behind a separate UI Expert design round, ARCH §20.4), so a leaf carries no
// per-layer instance sublist and no drag-drop-instance-onto-layer target — the existing tab-level
// read-only Transforms list (PropsTab_Manual_UI.h SCOPE NOTE 2) stays the only instance view.
#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include "TreeListWidget_UI.h"
#include "../params/PropInstance_PARAMS.h"
#include "../params/ScatterLayerBundle_PARAMS.h"

namespace SanmapGen {
namespace Ui {

struct ManualPropLayersState;   // PropsTab_Manual_UI.h — forward-declared, reference-only.

struct PropLayerBundlesState {
    TreeListState treeState;
    int selectedBundleIdentifier = -1;
    int selectedLeafLayerIndex   = -1;   // -1 never collides with a real layer index

    // Scratch-buffer rename (double-click the header) — not `bundle.name`/`layer.name` directly:
    // TreeNodeEx/CollapsingHeader compute their own row id FROM the label text they draw, so
    // live-editing the real field every keystroke churns that id and collapses the row out from
    // under whoever is typing (the exact bug Markers' own STEP142/STEP140 fixed this way).
    int         renamingBundleIdentifier    = -1;
    std::string renameBundleScratchText;
    bool        bRenameBundleFocusPending   = false;
    int         renamingLayerIndex          = -1;
    std::string renameLayerScratchText;
    bool        bRenameLayerFocusPending    = false;

    // Pending deletes, applied AFTER the tree's own walk finishes this frame — never mid-walk (a
    // structural erase would desync the walk's own position-based lookups for the rest of this
    // frame), mirroring MarkerLayerBundlesState's identical posture.
    int  pendingDeleteBundleIdentifier = -1;
    bool bPendingDeleteBundleCascade   = false;
    int  pendingDeleteLayerIndex       = -1;
    bool bPendingDeleteLayerCascade    = false;
};

// Mints a fresh, never-reused Bundle identifier — the exact NextPropLayerId pattern
// (PropsTab_Manual_UI.h), applied one tier up.
inline int NextPropLayerBundleId(const std::vector<Params::PropLayerBundle>& bundles) {
    int maximumId = -1;
    for (const Params::PropLayerBundle& bundle : bundles) maximumId = std::max(maximumId, bundle.identifier);
    return maximumId + 1;
}

// "Group Only": this Bundle is erased, every direct child (sub-Bundle or Layer) is promoted to ITS
// OWN parent. Out-of-range identifier is a silent no-op (Constitution §6).
void DeletePropLayerBundleGroupOnly(int bundleIdentifier, std::vector<Params::PropLayerBundle>& bundles,
                                    std::vector<Params::PropInstanceLayer>& propLayers);
// "All": this Bundle, every descendant Bundle, every Layer parented to any of them, AND every prop
// transform that referenced one of those layers — all erased together.
void DeletePropLayerBundleCascade(int bundleIdentifier, std::vector<Params::PropLayerBundle>& bundles,
                                  std::vector<Params::PropInstanceLayer>& propLayers,
                                  std::vector<Params::PropInstanceGroup>& props);
// A Layer's own "Layer Only": the layer is erased, every prop transform that referenced it is
// re-clamped to layer 0 (ClampPropLayerIndicesForRemovedLayer's own convention).
void DeletePropInstanceLayerOnly(int layerIndex, std::vector<Params::PropInstanceLayer>& propLayers,
                                 std::vector<Params::PropInstanceGroup>& props);
// A Layer's own "All": the layer AND every prop transform that referenced it are both erased.
void DeletePropInstanceLayerCascade(int layerIndex, std::vector<Params::PropInstanceLayer>& propLayers,
                                    std::vector<Params::PropInstanceGroup>& props);

// The tree mechanics — no Section wrap of its own (the Type-section's own outer DrawSectionBegin
// supplies that, PropsTab_UI.cpp). `propTypeNameFilter` scopes both the tree's own filtered-copy
// render and "+ Group"'s seeded `bundle.propTypeName`.
void DrawPropLayerBundleTree(std::vector<Params::PropLayerBundle>& bundles,
                             std::vector<Params::PropInstanceLayer>& propLayers,
                             std::vector<Params::PropInstanceGroup>& props,
                             PropLayerBundlesState& state, ManualPropLayersState& manualLayersState,
                             const std::string& propTypeNameFilter);

} // namespace Ui
} // namespace SanmapGen
