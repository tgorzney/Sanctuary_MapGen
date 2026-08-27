// MarkersTab_ManualInstanceSelection_UI.h — STEP141: Ctrl/Shift multi-select for manual Instance
// rows and drag-and-drop reparenting into a Manual Layer ("typical expectations", human's own
// instruction), mirroring MarkerLayerId_UI.h's single-purpose-file precedent. The selection set
// drives ONLY the list/tree's own row highlighting and bulk drag; `MarkersTabState::
// selectedManualInstanceIdentifier` is UNCHANGED and still the single "primary" selection
// MapCanvas's own highlight/drag-gesture reads (SetManualMarkerSelectionSource) — multi-instance
// canvas highlighting is a separate, larger change this ticket does not attempt.
#pragma once
#include <functional>
#include <vector>
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// One click's own outcome, "typical expectations" (the same convention every file manager/list UI
// already uses):
//   plain click:  replace the set with just this one; the anchor becomes this one too.
//   Ctrl click:   toggle this one in/out of the set, nothing else touched; anchor becomes this one
//                 (so a FOLLOWING Shift-click ranges from here).
//   Shift click:  replace the set with the contiguous range [anchor .. this] within `rowOrder`
//                 (inclusive both ends, whichever direction); the anchor itself does NOT move, so
//                 repeated Shift-clicks keep expanding/contracting from the SAME anchor.
//   Shift with no anchor yet (anchorIdentifier < 0): falls through to plain-click behavior.
// `rowOrder` is the CALLER's own display-order identifier list for the list the click happened in
// (one Layer's own Instances, or the base-section list) — range selection is scoped to that one
// list, never across separate lists (there is no single well-defined order spanning the whole tree).
void ApplyManualInstanceSelectionClick(const std::vector<int>& rowOrder, int clickedIdentifier,
                                       bool bCtrlHeld, bool bShiftHeld,
                                       std::vector<int>& selectedIdentifiers, int& anchorIdentifier);

inline bool IsManualInstanceSelected(const std::vector<int>& selectedIdentifiers, int identifier) {
    for (const int candidate : selectedIdentifiers) if (candidate == identifier) return true;
    return false;
}

// The caller-owned state one Instance row's click/drag needs, bundled so the row-draw function's
// own parameter list does not keep growing one field at a time (mirrors
// ProceduralInstanceListContext_UI's established shape, ProceduralInstanceRuleIndex_UI.h). Every
// pointer is caller-owned (MarkersTabState's own fields) and must outlive the call.
struct ManualInstanceRowInteractionContext_UI {
    int*                      primaryIdentifier   = nullptr;   // selectedManualInstanceIdentifier
    std::vector<int>*         selectedIdentifiers = nullptr;   // selectedManualInstanceIdentifiers
    int*                      anchorIdentifier     = nullptr;   // manualInstanceSelectionAnchorIdentifier
    const std::vector<int>*   rowOrder             = nullptr;   // THIS list's own display-order identifiers
    std::function<void(int)>  selectManualMarkerInstanceCallback;
};

// Reassigns every Instance transform in `markers` whose `instanceIdentifier` matches one of
// `movedIdentifiers` to `newLayerIndex` — a pure, in-place value change (no vector erased/resized),
// so it is always safe to call immediately, never deferred. Out-of-range/missing identifiers are a
// silent no-op per identifier (Constitution §6).
void ReassignManualInstanceLayers(std::vector<Params::MarkerInstanceGroup>& markers,
                                  const std::vector<int>& movedIdentifiers, int newLayerIndex);

// STEP148 — the same drop mechanics DrawManualLayerInstanceDropTarget wraps below, split out for a
// target that has no concrete `layerIndex` to reassign onto YET (a Group with no Manual Layer of
// its own — MarkersTab_BundleHeaderExtras_UI.cpp's own drop target on a Group header creates one).
// Returns the dropped/moved instance identifiers instead of reassigning immediately, so the caller
// can defer creating the Layer (a STRUCTURAL mutation to `instanceLayers`, unsafe mid-tree-walk,
// same posture as every other pending-delete field on MarkerLayerBundlesState) until after the
// walk finishes this frame. Empty when nothing was dropped this frame.
std::vector<int> DetectManualInstanceDropTarget(const std::vector<int>& selectedIdentifiers);

// Drag-drop TARGET only (draws nothing itself) — attaches to whatever the CALLER last submitted, so
// it must be called immediately after that row's own header widget (mirrors
// TreeListWidget_RowLayout_UI.h's own DetectTreeRowDragAndDrop contract). Accepts a "markerInstanceDrag"
// payload (the SmallButton-free row source, DrawManualInstanceRow's own drag source): if the dropped
// instance is part of the CURRENT multi-select, the WHOLE selection moves together; otherwise just
// the one dropped instance does (the standard "drag a non-selected item" convention).
void DrawManualLayerInstanceDropTarget(int layerIndex, std::vector<Params::MarkerInstanceGroup>& markers,
                                       const std::vector<int>& selectedIdentifiers);

} // namespace Ui
} // namespace SanmapGen
