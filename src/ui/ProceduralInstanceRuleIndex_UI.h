// ProceduralInstanceRuleIndex_UI.h — new, single-purpose (mirrors ManualInstanceLayerIndex_UI.h's
// own exact shape and non-persistence posture): a per-frame index from ruleIndex -> every array
// position among `Data::PlacementInstances::markers` whose own `ruleIndex` matches, consumed by
// the Rule row's new per-Rule instance list (STEP132, ARCH_19_27_ProceduralInstanceSelectionMechanism.md).
// Rebuilt every frame it is used from the current snapshot, never persisted — zero dirty-hash/DAG
// participation, a pure derived index over already-baked DATA (ARCH §19.27's own binding ruling).
#pragma once
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>
#include "../data/PlacementInstances_DATA.h"
#include "../params/MarkerRule_PARAMS.h"

namespace SanmapGen {
namespace Ui {

struct ProceduralInstanceRuleIndex_UI {
    // array position -- into markers.positionX[position] / markers.ruleIndex[position] / etc.
    std::unordered_map<int, std::vector<int>> instancesByRuleIndex;
};

inline ProceduralInstanceRuleIndex_UI BuildProceduralInstanceRuleIndex(
        const Data::PlacementInstances& markers) {
    ProceduralInstanceRuleIndex_UI index;
    for (int position = 0; position < static_cast<int>(markers.Count()); ++position)
        index.instancesByRuleIndex[markers.ruleIndex[static_cast<std::size_t>(position)]]
            .push_back(position);
    return index;
}

// A rule's own flat, PROC-matching `ruleIndex`: its position in the markerRuleLayers/rules nest,
// counting EVERY rule encountered before it (including disabled/hidden ones) — the IDENTICAL flat
// counter Placement_MarkerRules_PROC.cpp's own AppendMarkerRules walks (re-derived here, not
// duplicated as a stored field: PROC owns no UI-only bookkeeping, and this index changes with every
// reorder anyway).
inline int FlatMarkerRuleIndexBase(const std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                                   int layerIndex) {
    int base = 0;
    for (int index = 0; index < layerIndex && index < static_cast<int>(markerRuleLayers.size()); ++index)
        base += static_cast<int>(markerRuleLayers[static_cast<std::size_t>(index)].rules.size());
    return base;
}

// STEP132 — bundles the per-Rule instance-list inputs into ONE value, threaded as a single parameter
// down DrawRuleLayerListBody -> DrawRuleLayerBody -> DrawRuleSettings -> DrawRuleInstanceList instead
// of four, so none of those signatures balloon. `ruleIndexLookup` points at the caller's own
// once-per-list-body-call build, whose lifetime (the enclosing DrawRuleLayerListBody call) outlives
// every use below it in the same frame.
struct ProceduralInstanceListContext_UI {
    const Data::PlacementInstances*       placedMarkers   = nullptr;
    const ProceduralInstanceRuleIndex_UI* ruleIndexLookup = nullptr;
    int                                   flatRuleIndex   = -1;
    // STEP205 — widened from `void(int)` so DrawProceduralInstanceRow can forward its own new
    // Ctrl/Shift read through to the canvas's `ApplySelectionGesture`, joining/ranging into the
    // canvas's real multi-select instead of always Replace.
    std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>  selectProceduralMarkerInstanceCallback;
};

} // namespace Ui
} // namespace SanmapGen
