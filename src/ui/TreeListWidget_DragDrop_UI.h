// TreeListWidget_DragDrop_UI.h — the drag-source/drop-target detector TreeListWidget_UI<T,LeafKeyT>
// uses for every node and leaf row. Generalizes DraggableListWidget_RowAffordances_UI.h's
// DetectRowDragAndDrop (single int payload, whole-row drop target) to a richer payload (source kind
// + node identifier + generic LeafKeyT) and three drop-zone bands per DESIGN_Assembly_R1.md §1's
// Above/Below/OnAsChild convention. Owns no application state and mutates nothing but the signal
// out-param (ARCH §3.2), same posture as the precedent it generalizes.
#pragma once
#include <imgui.h>
#include "TreeListWidget_Types_UI.h"

namespace SanmapGen {
namespace Ui {
namespace TreeListDetail {

template <typename LeafKeyT>
struct TreeDragPayload {
    TreeNodeSourceKind sourceKind     = TreeNodeSourceKind::Node;
    int                nodeIdentifier = -1;   // valid when sourceKind == Node
    LeafKeyT           leaf{};                // valid when sourceKind == Leaf
};

template <typename LeafKeyT>
inline void RecordTreeSignal(TreeListSignal<LeafKeyT>& signal, TreeListSignalKind kind,
                             TreeNodeSourceKind sourceKind, int sourceNodeIdentifier,
                             const LeafKeyT& sourceLeaf, int targetNodeIdentifier, TreeDropZone dropZone) {
    if (signal.bHasSignal()) return;             // first signal of the frame wins
    signal.kind = kind; signal.sourceKind = sourceKind;
    signal.sourceNodeIdentifier = sourceNodeIdentifier; signal.sourceLeaf = sourceLeaf;
    signal.targetNodeIdentifier = targetNodeIdentifier; signal.dropZone = dropZone;
}

// Registers the LAST-submitted imgui item (the row's own header/selectable, drawn by the caller
// immediately before this call — same binding convention DetectRowDragAndDrop uses) as a drag
// source carrying `payload`, and, when `bCanBeChildTarget` (false for a leaf — a leaf cannot have
// children, enforced structurally here rather than caught after the fact, Constitution §6), a drop
// target subdivided into three vertical bands via the item's own rect.
template <typename LeafKeyT>
inline void DetectTreeRowDragAndDrop(const char* payloadIdentifier, const TreeDragPayload<LeafKeyT>& payload,
                                     int targetNodeIdentifierIfDropped, bool bCanBeChildTarget,
                                     TreeListSignal<LeafKeyT>& signal) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        ImGui::SetDragDropPayload(payloadIdentifier, &payload, sizeof(payload));
        ImGui::TextUnformatted("Moving...");
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* const dropped = ImGui::AcceptDragDropPayload(payloadIdentifier);
        if (dropped != nullptr && dropped->DataSize == static_cast<int>(sizeof(TreeDragPayload<LeafKeyT>))) {
            const TreeDragPayload<LeafKeyT>& source =
                *static_cast<const TreeDragPayload<LeafKeyT>*>(dropped->Data);
            const ImVec2 itemMin = ImGui::GetItemRectMin();
            const ImVec2 itemMax = ImGui::GetItemRectMax();
            const float rowHeight = itemMax.y - itemMin.y;
            const float edgeFraction = static_cast<float>(kTreeDropZoneEdgeFractionPercent) / 100.0f;
            const float relativeY = (ImGui::GetMousePos().y - itemMin.y) / (rowHeight > 0.0f ? rowHeight : 1.0f);
            TreeDropZone zone = TreeDropZone::OnAsChild;
            if (relativeY < edgeFraction) zone = TreeDropZone::Above;
            else if (relativeY > 1.0f - edgeFraction) zone = TreeDropZone::Below;
            if (zone == TreeDropZone::OnAsChild && !bCanBeChildTarget) zone = TreeDropZone::Below;
            const bool bSelfDrop = source.sourceKind == TreeNodeSourceKind::Node
                && source.nodeIdentifier == targetNodeIdentifierIfDropped;
            if (!bSelfDrop)
                RecordTreeSignal(signal, TreeListSignalKind::Reparent, source.sourceKind,
                                 source.nodeIdentifier, source.leaf, targetNodeIdentifierIfDropped, zone);
        }
        ImGui::EndDragDropTarget();
    }
}

} // namespace TreeListDetail
} // namespace Ui
} // namespace SanmapGen
