// MarkersTab_ManualInstanceSelection_UI.h — STEP141: Ctrl/Shift multi-select for manual Instance
// rows and drag-and-drop reparenting into a Manual Layer ("typical expectations", human's own
// instruction), mirroring MarkerLayerId_UI.h's single-purpose-file precedent. The selection set
// drives ONLY the list/tree's own row highlighting and bulk drag; `MarkersTabState::
// selectedManualInstanceIdentifier` is UNCHANGED and still the single "primary" selection
// MapCanvas's own highlight/drag-gesture reads (SetManualMarkerSelectionSource) — multi-instance
// canvas highlighting is a separate, larger change this ticket does not attempt.
#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
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
    // STEP205 — widened from `void(int)` so the row's own click could forward the SAME bCtrl/bShift it
    // already read for ApplyManualInstanceSelectionClick, instead of the canvas always Replacing.
    // STEP233 — widened AGAIN, and simplified: carries this row's own clicked identifier plus the
    // list's OWN already-resolved full selection (*interaction.selectedIdentifiers, written by
    // ApplyManualInstanceSelectionClick one call earlier in the SAME click) instead of raw bCtrl/bShift
    // — the canvas syncs its own manual-marker subset to match this resolution directly
    // (MapCanvas::SyncManualMarkerSelection) rather than re-deriving Toggle/Union/Replace against its
    // OWN, independently-touched copy of the set — the redundant-computation trap that caused STEP233's
    // own bug. Neither bCtrl nor bShift is forwarded any more: the canvas performs no modifier-driven
    // resolution of its own for a list-originated click, and Application::WireCallbacks()'s own tabState
    // resync is unconditionally suppressed for this path regardless of which modifier was held (see
    // MapCanvas_UI.h's SyncManualMarkerSelection and Application_UI.cpp's own bSuppressTabStateResync).
    std::function<void(int clickedInstanceIdentifier, const std::vector<int>& selectedInstanceIdentifiers)>
        selectManualMarkerInstanceCallback;
};

// Reassigns every Instance transform in `markers` whose `instanceIdentifier` matches one of
// `movedIdentifiers` to `newLayerIndex` — a pure, in-place value change (no vector erased/resized),
// so it is always safe to call immediately, never deferred. Out-of-range/missing identifiers are a
// silent no-op per identifier (Constitution §6).
void ReassignManualInstanceLayers(std::vector<Params::MarkerInstanceGroup>& markers,
                                  const std::vector<int>& movedIdentifiers, int newLayerIndex);

// STEP247 — "+Link"'s own per-instance tagging step: mirrors ReassignManualInstanceLayers's exact
// walk, one field over (`linkIdentifier` instead of `layerIndex`). Existing `layerIndex`/grouping is
// never touched by this call. Out-of-range/missing identifiers are a silent no-op per identifier
// (Constitution §6), same posture as ReassignManualInstanceLayers.
void TagManualInstancesWithLink(std::vector<Params::MarkerInstanceGroup>& markers,
                                const std::vector<int>& taggedIdentifiers, int linkIdentifier);

// STEP247 — "+Link"'s own no-op guard: true the moment ANY resolved identifier in
// `selectedIdentifiers` already carries `linkIdentifier >= 0` (already belongs to SOME existing
// Link). Mirrors IsManualInstanceSelectionEntirelyType's shape, but is an ANY-match with no type
// name involved. An unresolved/stale identifier is skipped (Constitution §6) — never itself a reason
// to block; an empty selection resolves false (nothing to check).
bool IsAnyManualInstanceSelectionAlreadyLinked(const std::vector<Params::MarkerInstanceGroup>& markers,
                                               const std::vector<int>& selectedIdentifiers);

// STEP235 — "+ Group"/"+ Layer" move a same-type selection into the new container: this predicate
// answers ONE question — is a same-type reassignment legal — never "is there anything to reassign"
// (an empty selection is NOT "entirely this type"; a caller that only wants to gate on non-empty must
// do that separately). True only when every one of `selectedIdentifiers` resolves to a transform
// whose OWN group, folded through Params::CanonicalMarkerTypeSectionName (the same alias-folding
// DrawBaseSectionManualInstanceList/FindOrCreateMarkerInstanceGroupByName already apply,
// MarkersTab_UI.cpp), equals `typeName`. An identifier that resolves to no transform at all (stale/
// out-of-range) also makes this false, same as a genuine type mismatch would.
bool IsManualInstanceSelectionEntirelyType(const std::vector<Params::MarkerInstanceGroup>& markers,
                                           const std::vector<int>& selectedIdentifiers,
                                           const std::string& typeName);

// STEP239 — "+Link"'s own cross-type partition: groups `selectedIdentifiers` by
// `Params::CanonicalMarkerTypeSectionName(group.name)` (the SAME alias-folding
// IsManualInstanceSelectionEntirelyType/DrawBaseSectionManualInstanceList already apply), mirroring
// BuildManualInstanceLayerIndex's own per-frame-map-build shape (ManualInstanceLayerIndex_UI.h),
// keyed by canonical type name instead of layerIndex. An identifier that resolves to no transform at
// all (stale/out-of-range) is silently omitted from every bucket (Constitution §6).
std::unordered_map<std::string, std::vector<int>> PartitionSelectedManualInstancesByType(
    const std::vector<Params::MarkerInstanceGroup>& markers, const std::vector<int>& selectedIdentifiers);

// STEP248 — the Links Section's own hierarchical body needs: "every instanceIdentifier currently
// tagged to Link X, grouped by canonical type name" — sibling of PartitionSelectedManualInstancesByType,
// keyed by `transform.linkIdentifier == linkIdentifier` instead of membership in a live selection set.
// Returns (groupIndex, transformIndex) pairs, matching DrawBaseSectionManualInstanceList's own item
// type (MarkersTab_UI.cpp) exactly, since that's what DrawManualInstanceRow/
// DrawSymmetryClusterInstanceList already consume.
std::unordered_map<std::string, std::vector<std::pair<int, int>>> PartitionLinkedManualInstancesByType(
    const std::vector<Params::MarkerInstanceGroup>& markers, int linkIdentifier);

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
