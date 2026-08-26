// TreeListWidget_Types_UI.h — the wire types TreeListWidget_UI<T, LeafKeyT> and its caller share.
// Domain-agnostic per ARCH_19_07: genericized over BOTH the node type T (Params::MarkerLayerBundle
// here, Params::Assembly later) and the leaf-key type LeafKeyT (MarkerGroupLeafKey_UI here,
// AssemblyMemberKey_UI later) — never field-name-coupled to either. Split out of TreeListWidget_UI.h
// (ARCH §1.5), mirroring DraggableListWidget_Types_UI.h's own split.
#pragma once
#include <unordered_map>

namespace SanmapGen {
namespace Ui {

enum class TreeListSignalKind : int { None = 0, Select, Reparent, Delete };
enum class TreeDropZone : int { Above, Below, OnAsChild };
enum class TreeNodeSourceKind : int { Node, Leaf };

// One frame produces at most ONE signal (first wins), mirroring DraggableListSignal's own contract.
template <typename LeafKeyT>
struct TreeListSignal {
    TreeListSignalKind kind       = TreeListSignalKind::None;
    TreeNodeSourceKind sourceKind = TreeNodeSourceKind::Node;
    int      sourceNodeIdentifier = -1;   // valid when sourceKind == Node
    LeafKeyT sourceLeaf{};                // valid when sourceKind == Leaf
    int      targetNodeIdentifier = -1;   // -1 = the root/"Ungrouped" drop zone
    TreeDropZone dropZone = TreeDropZone::OnAsChild;   // meaningless for a Leaf source
    bool bHasSignal() const { return kind != TreeListSignalKind::None; }
};

// Caller-owned, never a function static (Section_UI.h's rule; Application_TabState_UI.h's "one
// instance each" posture). Keyed by stable node identifier, not index: reparenting changes a
// node's draw-order position every frame but never its identity. DELIBERATE deviation from
// DraggableList's "mutates nothing" purity — same justification DESIGN_Assembly_R1.md §1 already
// gives for this exact field (expand/collapse is presentation-only, zero PARAMS consequence,
// Section_UI.h's own precedent for owning state directly).
struct TreeListState {
    std::unordered_map<int, bool> expandedNodeIdentifiers;   // default false (collapsed) on first sight
};

// Top/bottom edge band of a row's rect that reads Above/Below rather than OnAsChild — a named
// tweakable (Constitution §8), not a literal at the drop-zone-detection call site. Integer percent;
// divide by 100.0f at the point of use.
enum { kTreeDropZoneEdgeFractionPercent = 25 };

} // namespace Ui
} // namespace SanmapGen
