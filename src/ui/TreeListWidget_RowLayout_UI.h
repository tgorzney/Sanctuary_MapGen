// TreeListWidget_RowLayout_UI.h — TreeListWidget_UI<T,LeafKeyT>'s own per-row recursion body
// (RenderNode/RenderLeaf) plus the always-visible root/"Ungrouped" drop zone row. Split out of
// TreeListWidget_UI.h (ARCH §1.5 — the facade's own formatted length crossed the hard 150-line
// ceiling once this recursion was included), mirroring DraggableListWidget_RowLayout_UI.h's own
// split from DraggableListWidget_UI.h. Cycle prevention is explicitly NOT this widget's job
// (§19.8/DESIGN_Assembly_R1 §1) — the caller checks WouldReparentMarkerLayerBundleCreateCycle before
// applying a Reparent signal.
#pragma once
#include <unordered_map>
#include <vector>
#include <imgui.h>
#include "TreeListWidget_DragDrop_UI.h"
#include "TreeListWidget_Types_UI.h"

namespace SanmapGen {
namespace Ui {
namespace TreeListDetail {

// The always-visible root/"Ungrouped" drop zone — a real, low-key row (not an invisible padding
// strip: cheaper to hit-test and to drive from a synthetic-pointer test), target identifier -1.
template <typename LeafKeyT>
inline void DrawRootDropZoneRow(const char* payloadIdentifier, TreeListSignal<LeafKeyT>& signal) {
    ImGui::TextDisabled("(root - drop here to clear a Group)");
    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* const dropped = ImGui::AcceptDragDropPayload(payloadIdentifier);
        if (dropped != nullptr && dropped->DataSize == static_cast<int>(sizeof(TreeDragPayload<LeafKeyT>))) {
            const TreeDragPayload<LeafKeyT>& source =
                *static_cast<const TreeDragPayload<LeafKeyT>*>(dropped->Data);
            RecordTreeSignal(signal, TreeListSignalKind::Reparent, source.sourceKind,
                             source.nodeIdentifier, source.leaf, -1, TreeDropZone::OnAsChild);
        }
        ImGui::EndDragDropTarget();
    }
}

// STEP129: `headerExtraWidthPixels`/`drawLeafHeaderExtra` are the OPTIONAL header-extra slot — a
// fixed-width caller-drawn control drawn INLINE on the leaf row's own line, right-aligned against
// the row's own edge, after click/drag-drop detection and before the (bExpanded) body.
// `headerExtraWidthPixels == 0.0f` (the 7-callback delegator's default) draws nothing and reserves
// nothing — byte-identical to the pre-STEP129 layout. See ARCH_19_23 for why this is a SEPARATE
// leaf-keyed callback rather than sharing the node's own drawNodeHeaderExtra.
template <typename LeafLabelFn, typename DrawExpandedLeafBodyFn, typename DrawLeafHeaderExtraFn,
         typename LeafKeyT>
inline void RenderLeaf(const char* payloadIdentifier, const LeafKeyT& leaf, LeafLabelFn leafLabel,
    DrawExpandedLeafBodyFn drawExpandedLeafBody, DrawLeafHeaderExtraFn drawLeafHeaderExtra,
    float headerExtraWidthPixels, TreeListState& state, TreeListSignal<LeafKeyT>& signal) {
    // A leaf has no stable int identifier of its own to key expand-state by (unlike a node) — this
    // v1 keys off imgui's own per-label id scope instead (bExpanded local, not persisted in
    // TreeListState), acceptable since a leaf's inline body is cheap to redraw/recollapse; label
    // uniqueness within one node's own leaf set is the caller's job, same posture MakeNamesUnique
    // already gives Manual Layers elsewhere.
    ImGui::PushID(leafLabel(leaf));
    // Human's own instruction — a Layer needs a single click to SELECT and a double click to
    // collapse: without OpenOnArrow/OpenOnDoubleClick, TreeNodeEx's own default toggles open/closed
    // on ANY single click (label included), fighting every plain select click below. OpenOnArrow
    // keeps the arrow's own single-click toggle (mirrors RenderNode's Group header, just below,
    // which already carries OpenOnArrow); OpenOnDoubleClick adds the label's own double-click toggle.
    const bool bExpanded = ImGui::TreeNodeEx(leafLabel(leaf),
        ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen
        | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        RecordTreeSignal(signal, TreeListSignalKind::Select, TreeNodeSourceKind::Leaf,
                         -1, leaf, -1, TreeDropZone::OnAsChild);
    TreeDragPayload<LeafKeyT> payload;
    payload.sourceKind = TreeNodeSourceKind::Leaf; payload.leaf = leaf;
    DetectTreeRowDragAndDrop(payloadIdentifier, payload, -1, false, signal);
    if (headerExtraWidthPixels > 0.0f) {
        const float rowAvailWidthPixels = ImGui::GetContentRegionAvail().x;
        ImGui::SameLine(rowAvailWidthPixels - headerExtraWidthPixels);
        drawLeafHeaderExtra(leaf);
    }
    if (bExpanded) { ImGui::Indent(); drawExpandedLeafBody(leaf); ImGui::Unindent(); }
    ImGui::PopID();
}

template <typename T, typename LeafKeyT, typename IdOfFn, typename ParentIdOfFn, typename NameOfFn,
         typename DrawNodeBodyFn, typename DescribeLeavesFn, typename LeafLabelFn,
         typename DrawExpandedLeafBodyFn, typename DrawNodeHeaderExtraFn, typename DrawLeafHeaderExtraFn>
inline void RenderNode(const char* payloadIdentifier, const std::vector<T>& nodes, int nodeIndex,
    const std::unordered_map<int, std::vector<int>>& childrenOf, IdOfFn idOf, ParentIdOfFn parentIdOf,
    NameOfFn nameOf, DrawNodeBodyFn drawNodeBody, DescribeLeavesFn describeLeaves, LeafLabelFn leafLabel,
    DrawExpandedLeafBodyFn drawExpandedLeafBody, DrawNodeHeaderExtraFn drawNodeHeaderExtra,
    DrawLeafHeaderExtraFn drawLeafHeaderExtra, float headerExtraWidthPixels, TreeListState& state,
    int selectedNodeIdentifier, TreeListSignal<LeafKeyT>& signal) {
    const T& node = nodes[static_cast<std::size_t>(nodeIndex)];
    const int nodeIdentifier = idOf(node);
    ImGui::PushID(nodeIdentifier);
    bool& bExpanded = state.expandedNodeIdentifiers[nodeIdentifier];   // default false, first sight
    ImGui::SetNextItemOpen(bExpanded, ImGuiCond_Always);   // caller-owned state drives imgui, not
                                                            // the reverse (Section_UI.h posture)
    const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanFullWidth
        | ImGuiTreeNodeFlags_OpenOnArrow
        | (nodeIdentifier == selectedNodeIdentifier ? ImGuiTreeNodeFlags_Selected : 0);
    bExpanded = ImGui::CollapsingHeader(nameOf(node), flags);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        RecordTreeSignal(signal, TreeListSignalKind::Select, TreeNodeSourceKind::Node,
                         nodeIdentifier, LeafKeyT{}, -1, TreeDropZone::OnAsChild);
    TreeDragPayload<LeafKeyT> payload;
    payload.sourceKind = TreeNodeSourceKind::Node; payload.nodeIdentifier = nodeIdentifier;
    DetectTreeRowDragAndDrop(payloadIdentifier, payload, nodeIdentifier, true, signal);
    // STEP129: the OPTIONAL header-extra slot — see RenderLeaf's own comment above for the contract;
    // headerExtraWidthPixels == 0.0f draws nothing and reserves nothing.
    if (headerExtraWidthPixels > 0.0f) {
        const float rowAvailWidthPixels = ImGui::GetContentRegionAvail().x;
        ImGui::SameLine(rowAvailWidthPixels - headerExtraWidthPixels);
        drawNodeHeaderExtra(nodeIdentifier);
    }
    if (bExpanded) {
        ImGui::Indent();
        drawNodeBody(nodeIdentifier);
        const auto childIt = childrenOf.find(nodeIdentifier);
        if (childIt != childrenOf.end())
            for (int childIndex : childIt->second)
                RenderNode<T, LeafKeyT>(payloadIdentifier, nodes, childIndex, childrenOf, idOf, parentIdOf, nameOf,
                          drawNodeBody, describeLeaves, leafLabel, drawExpandedLeafBody, drawNodeHeaderExtra,
                          drawLeafHeaderExtra, headerExtraWidthPixels, state, selectedNodeIdentifier, signal);
        for (const LeafKeyT& leaf : describeLeaves(nodeIdentifier))
            RenderLeaf(payloadIdentifier, leaf, leafLabel, drawExpandedLeafBody, drawLeafHeaderExtra,
                      headerExtraWidthPixels, state, signal);
        ImGui::Unindent();
    }
    ImGui::PopID();
}

} // namespace TreeListDetail
} // namespace Ui
} // namespace SanmapGen
