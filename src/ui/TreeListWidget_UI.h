// TreeListWidget_UI.h — the generic tree/hierarchy widget (ARCH_19_07). Params::MarkerLayerBundle
// is the first instantiation (this ticket); Params::Assembly is a later, still-unbuilt one — built
// here unchanged. Layer: UI. Owns no application state and mutates nothing but
// TreeListState::expandedNodeIdentifiers (Section_UI.h's precedented exception,
// DESIGN_Assembly_R1.md §1); every PARAMS-touching consequence is reported as a TreeListSignal, the
// caller applies it. Not virtualized on purpose — authoring scale (tens of nodes/leaves).
//
// RenderNode/RenderLeaf (the per-row recursion body) live in TreeListWidget_RowLayout_UI.h — split
// out once this file's own formatted length crossed ARCH §1.5's hard 150-line ceiling, mirroring
// DraggableList's own 4-file split (Types/DragDrop/RowLayout/facade).
#pragma once
#include <cstring>
#include <unordered_map>
#include <vector>
#include <imgui.h>
#include "TreeListWidget_DragDrop_UI.h"
#include "TreeListWidget_RowLayout_UI.h"
#include "TreeListWidget_Types_UI.h"

namespace SanmapGen {
namespace Ui {

template <typename T, typename LeafKeyT>
class TreeListWidget_UI {
public:
    // idOf/parentIdOf/nameOf: accessor lambdas over T (§19.3 — never field-name-coupled).
    // describeLeaves(nodeIdentifier) -> const std::vector<LeafKeyT>&: a CALLER-BUILT index, built
    // once per frame BEFORE Render (DESIGN_Assembly_R1.md §1's "build the index once per frame,
    // don't re-scan per node" rule) — this widget never dereferences into a domain collection.
    // drawNodeBody(nodeIdentifier): the node's own inline content when its row is expanded.
    // drawExpandedLeafBody(leafKey): the additive piece ARCH_19_07 requires — a no-op lambda for a
    // read-only-leaf consumer (Assembly's later ticket).
    template <typename IdOfFn, typename ParentIdOfFn, typename NameOfFn, typename DrawNodeBodyFn,
             typename DescribeLeavesFn, typename LeafLabelFn, typename DrawExpandedLeafBodyFn>
    static TreeListSignal<LeafKeyT> Render(const char* treeIdentifier, const std::vector<T>& nodes,
        IdOfFn idOf, ParentIdOfFn parentIdOf, NameOfFn nameOf, DrawNodeBodyFn drawNodeBody,
        DescribeLeavesFn describeLeaves, LeafLabelFn leafLabel, DrawExpandedLeafBodyFn drawExpandedLeafBody,
        TreeListState& state, int selectedNodeIdentifier = -1) {
        TreeListSignal<LeafKeyT> signal;
        if (treeIdentifier == nullptr) return signal;
        const char* const payloadIdentifier =
            (std::strlen(treeIdentifier) < 32u) ? treeIdentifier : "SanGenTreeListRow";
        ImGui::PushID(treeIdentifier);

        // id -> children index, rebuilt once per Render call. A node whose own parent id does not
        // resolve to another node in `nodes` (including -1) is a ROOT — the same "dangling
        // reference degrades to root" posture ARCH_19_04 already rules for import.
        std::unordered_map<int, std::vector<int>> childrenOf;
        std::vector<int> rootIndices;
        for (int index = 0; index < static_cast<int>(nodes.size()); ++index) {
            const int parentIdentifier = parentIdOf(nodes[static_cast<std::size_t>(index)]);
            bool bParentExists = false;
            for (const T& candidate : nodes) if (idOf(candidate) == parentIdentifier) { bParentExists = true; break; }
            if (parentIdentifier < 0 || !bParentExists) rootIndices.push_back(index);
            else childrenOf[parentIdentifier].push_back(index);
        }

        TreeListDetail::DrawRootDropZoneRow<LeafKeyT>(payloadIdentifier, signal);
        for (int rootIndex : rootIndices)
            TreeListDetail::RenderNode(payloadIdentifier, nodes, rootIndex, childrenOf, idOf, parentIdOf, nameOf,
                                       drawNodeBody, describeLeaves, leafLabel, drawExpandedLeafBody, state,
                                       selectedNodeIdentifier, signal);

        ImGui::PopID();
        return signal;
    }
};

} // namespace Ui
} // namespace SanmapGen
