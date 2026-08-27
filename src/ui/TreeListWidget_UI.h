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
    // STEP129 (ARCH §19.23): the original 7-callback shape every existing call site binds. A thin
    // delegator onto the 9-callback overload below with no-op header-extra callbacks and a 0.0f
    // reserved width — every current call site recompiles unchanged; arity alone disambiguates
    // which overload a 7-callback caller resolves to (the exact STEP123 DraggableList precedent).
    template <typename IdOfFn, typename ParentIdOfFn, typename NameOfFn, typename DrawNodeBodyFn,
             typename DescribeLeavesFn, typename LeafLabelFn, typename DrawExpandedLeafBodyFn>
    static TreeListSignal<LeafKeyT> Render(const char* treeIdentifier, const std::vector<T>& nodes,
        IdOfFn idOf, ParentIdOfFn parentIdOf, NameOfFn nameOf, DrawNodeBodyFn drawNodeBody,
        DescribeLeavesFn describeLeaves, LeafLabelFn leafLabel, DrawExpandedLeafBodyFn drawExpandedLeafBody,
        TreeListState& state, int selectedNodeIdentifier = -1, const LeafKeyT& selectedLeaf = LeafKeyT()) {
        return Render(treeIdentifier, nodes, idOf, parentIdOf, nameOf, drawNodeBody, describeLeaves,
                     leafLabel, drawExpandedLeafBody, [](int) {}, [](const LeafKeyT&) {}, 0.0f, state,
                     selectedNodeIdentifier, selectedLeaf);
    }

    // STEP129 (ARCH §19.23): the OPTIONAL per-row header-extra slot — TWO callbacks, not one,
    // because a tree row has two distinct row kinds with two distinct identity types (Node's own
    // `int nodeIdentifier` vs. Leaf's own `const LeafKeyT&`), a deliberate, permanent divergence
    // from DraggableList<T>::Render's single-callback shape (ARCH_19_23 — ratified as designed, not
    // an inconsistency to unify). `headerExtraWidthPixels` is a FIXED width the CALLER supplies and
    // is reserved UNCONDITIONALLY on every row so the control sits at one constant right-aligned
    // offset regardless of any individual row's own state.
    // headerExtraWidthPixels == 0.0f (the overload above) draws nothing and reserves nothing.
    template <typename IdOfFn, typename ParentIdOfFn, typename NameOfFn, typename DrawNodeBodyFn,
             typename DescribeLeavesFn, typename LeafLabelFn, typename DrawExpandedLeafBodyFn,
             typename DrawNodeHeaderExtraFn, typename DrawLeafHeaderExtraFn>
    static TreeListSignal<LeafKeyT> Render(const char* treeIdentifier, const std::vector<T>& nodes,
        IdOfFn idOf, ParentIdOfFn parentIdOf, NameOfFn nameOf, DrawNodeBodyFn drawNodeBody,
        DescribeLeavesFn describeLeaves, LeafLabelFn leafLabel, DrawExpandedLeafBodyFn drawExpandedLeafBody,
        DrawNodeHeaderExtraFn drawNodeHeaderExtra, DrawLeafHeaderExtraFn drawLeafHeaderExtra,
        float headerExtraWidthPixels, TreeListState& state, int selectedNodeIdentifier = -1,
        const LeafKeyT& selectedLeaf = LeafKeyT()) {
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
                                       drawNodeBody, describeLeaves, leafLabel, drawExpandedLeafBody,
                                       drawNodeHeaderExtra, drawLeafHeaderExtra, headerExtraWidthPixels, state,
                                       selectedNodeIdentifier, selectedLeaf, signal);

        ImGui::PopID();
        return signal;
    }
};

} // namespace Ui
} // namespace SanmapGen
