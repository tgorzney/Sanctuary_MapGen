// DecalsTab_Bundles_UI.h — the Decals tab's Group/Bundle tree (ARCH §20; mirrors
// PropsTab_Bundles_UI.h, minus the Type Section scoping — Decals has exactly one implicit Type
// Section, so there is no `propTypeNameFilter`-equivalent parameter anywhere here). Layer: UI.
//
// Same trims as PropsTab_Bundles_UI.h relative to the Marker original, for the same reasons: one
// leaf kind (a manual `Params::DecalInstanceLayer` index — a bare `int`), no procedural rule-layer
// leaves (`Params::DecalRuleLayer` doesn't exist yet, ARCH §20.5), no per-instance multi-select/
// drag substrate (ARCH §20.4) and so no per-leaf instance sublist.
#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include "TreeListWidget_UI.h"
#include "../params/PropInstance_PARAMS.h"
#include "../params/ScatterLayerBundle_PARAMS.h"

namespace SanmapGen {
namespace Ui {

struct ManualDecalLayersState;   // DecalsTab_Manual_UI.h — forward-declared, reference-only.

struct DecalLayerBundlesState {
    TreeListState treeState;
    int selectedBundleIdentifier = -1;
    int selectedLeafLayerIndex   = -1;

    int         renamingBundleIdentifier  = -1;
    std::string renameBundleScratchText;
    bool        bRenameBundleFocusPending = false;
    int         renamingLayerIndex        = -1;
    std::string renameLayerScratchText;
    bool        bRenameLayerFocusPending  = false;

    int  pendingDeleteBundleIdentifier = -1;
    bool bPendingDeleteBundleCascade   = false;
    int  pendingDeleteLayerIndex       = -1;
    bool bPendingDeleteLayerCascade    = false;
};

inline int NextDecalLayerBundleId(const std::vector<Params::DecalLayerBundle>& bundles) {
    int maximumId = -1;
    for (const Params::DecalLayerBundle& bundle : bundles) maximumId = std::max(maximumId, bundle.identifier);
    return maximumId + 1;
}

void DeleteDecalLayerBundleGroupOnly(int bundleIdentifier, std::vector<Params::DecalLayerBundle>& bundles,
                                     std::vector<Params::DecalInstanceLayer>& decalLayers);
void DeleteDecalLayerBundleCascade(int bundleIdentifier, std::vector<Params::DecalLayerBundle>& bundles,
                                   std::vector<Params::DecalInstanceLayer>& decalLayers,
                                   std::vector<Params::DecalInstanceGroup>& decals);
void DeleteDecalInstanceLayerOnly(int layerIndex, std::vector<Params::DecalInstanceLayer>& decalLayers,
                                  std::vector<Params::DecalInstanceGroup>& decals);
void DeleteDecalInstanceLayerCascade(int layerIndex, std::vector<Params::DecalInstanceLayer>& decalLayers,
                                     std::vector<Params::DecalInstanceGroup>& decals);

// The tree mechanics — no Section wrap of its own, no type filter (Decals has exactly one implicit
// Type Section; every bundle is drawn, unlike Props' per-type filtered copy).
void DrawDecalLayerBundleTree(std::vector<Params::DecalLayerBundle>& bundles,
                              std::vector<Params::DecalInstanceLayer>& decalLayers,
                              std::vector<Params::DecalInstanceGroup>& decals,
                              DecalLayerBundlesState& state, ManualDecalLayersState& manualLayersState);

} // namespace Ui
} // namespace SanmapGen
