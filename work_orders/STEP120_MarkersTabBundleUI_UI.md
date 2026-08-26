# STEP120 — Markers tab Bundle UI (`TreeListWidget_UI<T>` + Bundle tree + tab-driven Move/Rotate)

**Layer:** UI (widget library + Markers tab). **Domain:** new `TreeListWidget_UI<T, LeafKeyT>`
(ARCH §19.7), new `MarkersTab_Bundles_UI.h/.cpp`, `MarkersTab_UI.cpp/.h` restructure,
`DraggableListWidget_Types_UI.h`/`DraggableListWidget_RowLayout_UI.h` (one additive field), new
`RigidTransformPivot_MATH.h`. Ticket B per `DESIGN_MarkerGroupLayerRestructure_R1.md` §6.

**Sequencing — do not implement concurrently with STEP119, and do not dispatch until STEP119 has
landed against live code.** STEP119 (drafted in parallel this session) is the PARAMS+IO half:
`Params::MarkerLayerBundle` (new `MarkerLayerBundle_PARAMS.h`), `MapRecipe::markerLayerBundles`,
`parentBundleIdentifier` added to both `Params::MarkerRuleLayer` and `Params::MarkerInstanceLayer`,
and the three pure resolvers `WouldReparentMarkerLayerBundleCreateCycle`,
`CollectMarkerLayerBundleRecursiveLayerIndices`, `CollectMarkerLayerBundleRecursiveManualMembers`
(ARCH_19_03/§19.8). Every code block below calls these exact names/signatures. This is a real
compile-time dependency, not a soft preference — this ticket will not build until STEP119's
declarations exist verbatim as ARCH_19_03/§19.8 specify them. Re-verify STEP119's actual landed
signatures against live code before implementing (field names/line numbers may drift from what's
quoted here, the way STEP106's own revision note describes).

## Problem

`src/ui/MarkersTab_UI.cpp` (`DrawMarkersTab`, lines 40-60) draws five flat sibling sections —
Globals, Procedural Rules (`DrawRuleStack`, lines 20-25), Manual Marker Layers
(`DrawManualMarkerLayers`), Manual Markers roster (`DrawManualMarkers`), Placed Markers — with no
container above Layer, confirmed still true by direct read this session. No tree/hierarchy widget
exists anywhere in `src/ui/` (`DraggableListWidget_UI.h` is strictly flat — one `std::vector<T>`,
index-based reorder, confirmed by reading `DraggableListWidget_RowLayout_UI.h:64-96`).

`DraggableList<T>::Render` (`DraggableListWidget_RowLayout_UI.h:68-95`) always draws every row of
the vector it's given, index 0..size-1 — it has no "skip this row" mechanism. This blocks the
design's own §4 requirement ("Ungrouped Procedural Rules"/"Ungrouped Manual Marker Layers" show
only layers with `parentBundleIdentifier == -1`, bundled layers show in the tree instead) — a real,
concrete gap the design doc's "filtered to X == -1" phrasing glosses over. Confirmed by reading
`DraggableListWidget_Types_UI.h` (`DraggableListRow`, lines 22-32) and the `Render` loop
(`DraggableListWidget_RowLayout_UI.h:79-93`): no field or loop branch exists to suppress a row.

`DrawRuleLayerSettings` (`MarkersTab_RuleLayerSettings_UI.cpp:83-95`) and `DrawLayerRowBody`
(`MarkersTab_ManualLayers_UI.cpp:34-61`) are confirmed, by direct read, already per-row/
non-selected-gated (STEP110's refactor) — both are legal, zero-rewrite leaf-body callbacks for the
new tree widget, as ARCH_19_07 states.

No rigid-transform math (rotate-around-pivot) exists with a `MATH`-layer, zero-`Params::` signature
anywhere in `src/math/`. The closest precedent is inline PROC code:
`Placement_SymmetryOrbit_PROC.h`'s `AppendRadialTurns` (lines 104-132, the
`offsetX*cosine - offsetY*sine` / `offsetX*sine + offsetY*cosine` rotate-around-center formula) and
`Placement_Transform_PROC.h`'s `QuaternionMultiply` (lines 31-38) + yaw-quaternion construction
(lines 74-77) — both PROC-owned, not reusable from UI without a PROC dependency, and not
`Params::`-free at the placement-rule level ARCH_19_08 requires.

## Fix

### 1. New MATH — `src/math/RigidTransformPivot_MATH.h`
Per ARCH §19.8: "the rigid rotate/translate-around-centroid math... zero `Params::` types in its
signature → MATH." Replicates the exact rotate formula already proven at
`Placement_SymmetryOrbit_PROC.h:118-126` and the exact quaternion Hamilton-product/yaw-construction
already proven at `Placement_Transform_PROC.h:31-38,74-77`, with plain scalar in/out params so both
Bundle's and (later) Assembly's rotate call the same function (§19.8 item 9 — "one shared function,
not two copies"). Deterministic: `Math::Sine`/`Math::Cosine`, never `std::sin`/`std::cos`
(`DETERMINISM_SPEC`, matching `Trigonometry_MATH.h`'s own header).
```cpp
// RigidTransformPivot_MATH.h — pure rigid-transform math over bare scalars: zero Params:: types,
// reusable by any domain's rotate-around-pivot need (ARCH_19_08 — Bundle's move/rotate and
// Assembly's own future rotate share this one function, not two copies). Layer: MATH.
#pragma once
#include "Trigonometry_MATH.h"

namespace SanmapGen {
namespace Math {

// Rotates (x, z) by angleRadians (counter-clockwise — same convention as AppendRadialTurns,
// Placement_SymmetryOrbit_PROC.h:120-126) around (pivotX, pivotZ). outX/outZ may alias x/z.
inline void RotatePointAroundPivot(float x, float z, float pivotX, float pivotZ, float angleRadians,
                                   float& outX, float& outZ) {
    const float offsetX = x - pivotX;
    const float offsetZ = z - pivotZ;
    const float cosine  = Cosine(angleRadians);
    const float sine    = Sine(angleRadians);
    outX = pivotX + (offsetX * cosine - offsetZ * sine);
    outZ = pivotZ + (offsetX * sine   + offsetZ * cosine);
}

// Hamilton product: applies `second` first, then `first` — same order convention as
// Placement_Transform_PROC.h's QuaternionMultiply, replicated with plain scalar out-params (zero
// Params::/Proc:: types) so a UI-layer rigid-body rotate can call it without a PROC dependency.
inline void MultiplyQuaternions(float firstX, float firstY, float firstZ, float firstW,
                                float secondX, float secondY, float secondZ, float secondW,
                                float& outX, float& outY, float& outZ, float& outW) {
    outX = firstW*secondX + firstX*secondW + firstY*secondZ - firstZ*secondY;
    outY = firstW*secondY - firstX*secondZ + firstY*secondW + firstZ*secondX;
    outZ = firstW*secondZ + firstX*secondY - firstY*secondX + firstZ*secondW;
    outW = firstW*secondW - firstX*secondX - firstY*secondY - firstZ*secondZ;
}

// A yaw-only quaternion (rotation about world Y by angleRadians) — same construction as
// Placement_Transform_PROC.h:74-77 (yawX=0, yawZ=0).
inline void YawQuaternion(float angleRadians, float& outX, float& outY, float& outZ, float& outW) {
    const float halfAngle = angleRadians * 0.5f;
    outX = 0.0f; outY = Sine(halfAngle); outZ = 0.0f; outW = Cosine(halfAngle);
}

} // namespace Math
} // namespace SanmapGen
```
New test `src/math/RigidTransformPivot_MATH_Test.cpp`, own `main()`, mirroring
`Trigonometry_MATH_Test.cpp`'s shape exactly (plain `std::printf`/exit-code, no imgui) — see Verify.

### 2. New widget types — `src/ui/TreeListWidget_Types_UI.h`
Genericizes `DESIGN_Assembly_R1.md` §1's Assembly-specific sketch (`sourceAssemblyId`,
`expandedAssemblyIds`) per ARCH_19_07's explicit instruction ("not Assembly-specific as written...
genericize it per §1 into `TreeListWidget_UI<T>`"). `LeafKeyT` is templated, not hardcoded to either
consumer's leaf payload (ARCH_19_07's "leaf-key type stays per-consumer" ruling) — so this file
templates `TreeListSignal` over `LeafKeyT` rather than embedding `AssemblyMemberKey_UI` directly the
way the original sketch did.
```cpp
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
```

### 3. New widget drag-drop detail — `src/ui/TreeListWidget_DragDrop_UI.h`
Generalizes `DraggableListWidget_RowAffordances_UI.h`'s `DetectRowDragAndDrop`
(lines 71-86 — single `int` payload, whole-row drop target) to a richer payload (source kind + node
identifier + generic `LeafKeyT`) and three drop-zone bands per `DESIGN_Assembly_R1.md` §1's
Above/Below/OnAsChild convention.
```cpp
// TreeListWidget_DragDrop_UI.h — the drag-source/drop-target detector TreeListWidget_UI<T,LeafKeyT>
// uses for every node and leaf row. Owns no application state and mutates nothing but the signal
// out-param (ARCH §3.2), same posture as the DraggableListWidget_RowAffordances_UI.h precedent it
// generalizes.
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
```

### 4. New widget facade — `src/ui/TreeListWidget_UI.h`
The primitive itself (ARCH_19_07 — "Markers' Ticket B builds it, generically, first... lives in
`TreeListWidget_UI.h`, a UI-framework primitive sibling of `DraggableListWidget_UI.h`, never inside
a `MarkersTab_*` file"). Builds its own id→children index once per `Render` call
(O(nodeCount), authoring scale — same "not virtualized on purpose" posture
`DraggableListWidget_UI.h`'s own header states, `DESIGN_Assembly_R1.md` §1's identical ruling).
Cycle prevention is explicitly NOT this widget's job (§19.8/DESIGN_Assembly_R1 §1) — the caller
checks `WouldReparentMarkerLayerBundleCreateCycle` before applying a `Reparent` signal.

Includes the additive `drawExpandedLeafBody(leafKey)` callback from the START of this build
(ARCH_19_07: "the widget contract needs one small addition from the start... not as a later
signature change"), alongside a `drawNodeBody(nodeIdentifier)` callback for the node's own inline
content (mirrors `DraggableList`'s `drawRowBody`; `DESIGN_Assembly_R1.md` §3's "inline rename on the
expanded node body" confirms Assembly's own tab expects this too).
```cpp
// TreeListWidget_UI.h — the generic tree/hierarchy widget (ARCH_19_07). Params::MarkerLayerBundle
// is the first instantiation (this ticket); Params::Assembly is a later, still-unbuilt one — built
// here unchanged. Layer: UI. Owns no application state and mutates nothing but
// TreeListState::expandedNodeIdentifiers (Section_UI.h's precedented exception,
// DESIGN_Assembly_R1.md §1); every PARAMS-touching consequence is reported as a TreeListSignal, the
// caller applies it. Not virtualized on purpose — authoring scale (tens of nodes/leaves).
//
// If formatted length exceeds ARCH §1.5's hard 150-line ceiling, split RenderNode/RenderLeaf into a
// fourth file (TreeListWidget_RowLayout_UI.h), mirroring DraggableList's own 4-file split
// (Types/DragDrop/RowLayout/facade) — not pre-emptively split here since exact line count depends
// on final formatting.
#pragma once
#include <cstring>
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

} // namespace TreeListDetail

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
            RenderNode(payloadIdentifier, nodes, rootIndex, childrenOf, idOf, parentIdOf, nameOf,
                      drawNodeBody, describeLeaves, leafLabel, drawExpandedLeafBody, state,
                      selectedNodeIdentifier, signal);

        ImGui::PopID();
        return signal;
    }

private:
    template <typename IdOfFn, typename ParentIdOfFn, typename NameOfFn, typename DrawNodeBodyFn,
             typename DescribeLeavesFn, typename LeafLabelFn, typename DrawExpandedLeafBodyFn>
    static void RenderNode(const char* payloadIdentifier, const std::vector<T>& nodes, int nodeIndex,
        const std::unordered_map<int, std::vector<int>>& childrenOf, IdOfFn idOf, ParentIdOfFn parentIdOf,
        NameOfFn nameOf, DrawNodeBodyFn drawNodeBody, DescribeLeavesFn describeLeaves, LeafLabelFn leafLabel,
        DrawExpandedLeafBodyFn drawExpandedLeafBody, TreeListState& state, int selectedNodeIdentifier,
        TreeListSignal<LeafKeyT>& signal) {
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
            TreeListDetail::RecordTreeSignal(signal, TreeListSignalKind::Select, TreeNodeSourceKind::Node,
                                             nodeIdentifier, LeafKeyT{}, -1, TreeDropZone::OnAsChild);
        TreeListDetail::TreeDragPayload<LeafKeyT> payload;
        payload.sourceKind = TreeNodeSourceKind::Node; payload.nodeIdentifier = nodeIdentifier;
        TreeListDetail::DetectTreeRowDragAndDrop(payloadIdentifier, payload, nodeIdentifier, true, signal);
        if (bExpanded) {
            ImGui::Indent();
            drawNodeBody(nodeIdentifier);
            const auto childIt = childrenOf.find(nodeIdentifier);
            if (childIt != childrenOf.end())
                for (int childIndex : childIt->second)
                    RenderNode(payloadIdentifier, nodes, childIndex, childrenOf, idOf, parentIdOf, nameOf,
                              drawNodeBody, describeLeaves, leafLabel, drawExpandedLeafBody, state,
                              selectedNodeIdentifier, signal);
            for (const LeafKeyT& leaf : describeLeaves(nodeIdentifier))
                RenderLeaf(payloadIdentifier, leaf, leafLabel, drawExpandedLeafBody, state, signal);
            ImGui::Unindent();
        }
        ImGui::PopID();
    }

    template <typename LeafLabelFn, typename DrawExpandedLeafBodyFn>
    static void RenderLeaf(const char* payloadIdentifier, const LeafKeyT& leaf, LeafLabelFn leafLabel,
        DrawExpandedLeafBodyFn drawExpandedLeafBody, TreeListState& state, TreeListSignal<LeafKeyT>& signal) {
        // A leaf has no stable int identifier of its own to key expand-state by (unlike a node) — this
        // v1 keys off imgui's own per-label id scope instead (bExpanded local, not persisted in
        // TreeListState), acceptable since a leaf's inline body is cheap to redraw/recollapse; label
        // uniqueness within one node's own leaf set is the caller's job, same posture MakeNamesUnique
        // already gives Manual Layers elsewhere.
        ImGui::PushID(leafLabel(leaf));
        const bool bExpanded = ImGui::TreeNodeEx(leafLabel(leaf),
            ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            TreeListDetail::RecordTreeSignal(signal, TreeListSignalKind::Select, TreeNodeSourceKind::Leaf,
                                             -1, leaf, -1, TreeDropZone::OnAsChild);
        TreeListDetail::TreeDragPayload<LeafKeyT> payload;
        payload.sourceKind = TreeNodeSourceKind::Leaf; payload.leaf = leaf;
        TreeListDetail::DetectTreeRowDragAndDrop(payloadIdentifier, payload, -1, false, signal);
        if (bExpanded) { ImGui::Indent(); drawExpandedLeafBody(leaf); ImGui::Unindent(); }
        ImGui::PopID();
    }
};

} // namespace Ui
} // namespace SanmapGen
```
Exact imgui flags for the leaf row's own expand/collapse are the Coder's to verify empirically
against the new headless test (Verify, below) — the binding two-state contract is "click toggles,
expanded draws `drawExpandedLeafBody`," not this literal flag combination.

### 5. `DraggableListRow` gains `bRowSuppressed` — required for the "Ungrouped ..." filtered sections
Additive, default `false` (zero behavior change for every existing consumer — same posture STEP150's
`extraButtonLabel` addition used). `DraggableListWidget_Types_UI.h`, `DraggableListRow`
(lines 23-32), add one field after `extraButtonLabel`:
```cpp
    // STEP120: when true, this row draws NOTHING — no header, no body, no affordances, no drag
    // source/target. Lets a caller present a FILTERED view of its own backing vector (the "Ungrouped
    // ..." Markers-tab sections) without DraggableList itself gaining filtering logic; reorder/delete
    // still operate on real vector indices exactly as today — a suppressed row simply cannot be a
    // drag source or drop target, an accepted consequence, not a defect (a downward drag can still
    // "land" past a suppressed row's real index; standard behavior for any filtered reorderable list).
    bool bRowSuppressed = false;
```
`DraggableListWidget_RowLayout_UI.h`, `Render`'s loop (lines 79-93) — skip both layouts entirely for
a suppressed row, before either width computation runs:
```cpp
        for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
            ImGui::PushID(rowIndex);
            const DraggableListRow row = describeRow(rowIndex);
            if (!row.bRowSuppressed) {
                const float extraButtonWidthPixels = DraggableListExtraButtonWidthPixels(row);
                const float rowAvailWidthPixels = ImGui::GetContentRegionAvail().x;
                if (layout == DraggableListRowLayout::Flat)
                    RowLayoutDetail::RenderFlatRow(payloadIdentifier, row, rowIndex, rowAvailWidthPixels,
                        extraButtonWidthPixels, selectedRowIndex, drawRowBody, signal);
                else
                    RowLayoutDetail::RenderCollapsibleRow(payloadIdentifier, row, rowIndex, rowAvailWidthPixels,
                        extraButtonWidthPixels, selectedRowIndex, drawRowBody, signal);
            }
            ImGui::PopID();
        }
```

### 6. Filter the two existing lists to "Ungrouped ..." only
`MarkersTab_RuleLayers_UI.cpp`, `DrawRuleLayerListBody`'s `describeRow` lambda (lines 77-89) — one
new line:
```cpp
        [&](int rowIndex) {
            const Params::MarkerRuleLayer& layer = markerRuleLayers[static_cast<std::size_t>(rowIndex)];
            DraggableListRow row;
            row.bRowSuppressed = (layer.parentBundleIdentifier != -1);   // NEW — STEP120: bundled
                                                                          // layers show in the tree
            ...
```
`MarkersTab_ManualLayers_UI.cpp`, `DrawLayerList`'s `describeRow` lambda (lines 74-78) — same:
```cpp
        [&](int rowIndex) {
            DraggableListRow row;
            row.bRowSuppressed = (markerLayers[static_cast<std::size_t>(rowIndex)].parentBundleIdentifier != -1);
            row.label   = ManualMarkerLayerRowLabel(markerLayers[static_cast<std::size_t>(rowIndex)]);
            row.bLocked = markerLayers[static_cast<std::size_t>(rowIndex)].bLocked;
            return row;
        },
```
Section header text renamed to match the new meaning (design §4): `MarkersTab_UI.cpp:22`,
`DrawSectionBegin("Procedural Rules", ...)` → `DrawSectionBegin("Ungrouped Procedural Rules", ...)`.
`MarkersTab_ManualLayers_UI.cpp:137`, `DrawSectionBegin("Manual Marker Layers", state.section)` →
`DrawSectionBegin("Ungrouped Manual Marker Layers", state.section)`.

### 7. Add-Layer buttons gain an optional Bundle-scoped parent parameter
**Procedural** — `MarkersTab_RuleLayerSettings_UI.cpp`, extract the "Add Layer" block (current
lines 54-59) out of `DrawRuleLayerButtons` into its own function, declared in
`MarkersTab_RuleLayers_UI.h` (after the existing `DrawRuleLayerButtons` declaration, line 93):
```cpp
bool DrawAddMarkerRuleLayerButton(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                                  int parentBundleIdentifierForNewLayer = -1);
```
Body (`MarkersTab_RuleLayerSettings_UI.cpp`):
```cpp
bool DrawAddMarkerRuleLayerButton(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                                  int parentBundleIdentifierForNewLayer) {
    if (!ImGui::Button(parentBundleIdentifierForNewLayer < 0 ? "Add Layer" : "Add Procedural Layer Here"))
        return false;
    Params::MarkerRuleLayer layer;
    layer.parentBundleIdentifier = parentBundleIdentifierForNewLayer;   // STEP119 field
    markerRuleLayers.push_back(layer);
    state.selectedRuleLayerIndex = static_cast<int>(markerRuleLayers.size()) - 1;
    state.selectedRuleIndex      = 0;
    return true;
}

void DrawRuleLayerButtons(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                          Pipeline::PreviewDriver* previewDriver) {
    bool bRecipeMoved = DrawAddMarkerRuleLayerButton(markerRuleLayers, state, -1);   // root, unchanged behavior
    Params::MarkerRuleLayer* const layer = SelectedMarkerRuleLayer(markerRuleLayers, state);
    ImGui::SameLine();
    ImGui::BeginDisabled(layer == nullptr);
    ... (Add Rule / Remove Selected Rule blocks unchanged) ...
```
**Manual** — `MarkersTab_ManualLayers_UI.cpp`'s `DrawLayerListButtons` (currently inside the
anonymous namespace, lines 121-129) moves OUT of that namespace (relocate the definition to
immediately after the namespace's closing `}` at line 131, before `DrawManualMarkerLayers`), gains
the same parameter, and is declared in `MarkersTab_ManualLayers_UI.h` (after `SelectedManualMarkerLayer`):
```cpp
bool DrawLayerListButtons(std::vector<Params::MarkerInstanceLayer>& markerLayers, ManualMarkerLayersState& state,
                          int parentBundleIdentifierForNewLayer = -1);
```
```cpp
bool DrawLayerListButtons(std::vector<Params::MarkerInstanceLayer>& markerLayers, ManualMarkerLayersState& state,
                          int parentBundleIdentifierForNewLayer) {
    if (!ImGui::Button(parentBundleIdentifierForNewLayer < 0 ? "Add Marker Layer" : "Add Manual Layer Here"))
        return false;
    Params::MarkerInstanceLayer layer;
    layer.name                   = NextMarkerLayerName(static_cast<int>(markerLayers.size()));
    layer.layerId                = NextMarkerLayerId(markerLayers);
    layer.parentBundleIdentifier = parentBundleIdentifierForNewLayer;   // STEP119 field
    markerLayers.push_back(layer);
    state.selectedLayerIndex = static_cast<int>(markerLayers.size()) - 1;
    return true;
}
```
Its own existing call site (`DrawManualMarkerLayers`, line 139) becomes
`bool bLayersMoved = DrawLayerListButtons(markerLayers, state, -1);` — root-scope, unchanged behavior.

### 8. New leaf key + Bundle state — `src/ui/MarkersTab_Bundles_UI.h`
```cpp
// MarkersTab_Bundles_UI.h — the Markers tab's Group/Bundle tree (STEP120, ARCH §19). Layer: UI.
// Edits recipe.markerLayerBundles (Params::MarkerLayerBundle, STEP119) and the back-reference
// parentBundleIdentifier STEP119 adds to both Params::MarkerRuleLayer and Params::MarkerInstanceLayer.
// Reuses DrawRuleLayerSettings/DrawLayerRowBody UNCHANGED as the tree's leaf-body callbacks
// (ARCH_19_07's "good news" finding, re-verified against live code this ticket).
#pragma once
#include <algorithm>
#include <unordered_map>
#include <vector>
#include "MarkerSymmetryFixCommand_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "TextInput_UI.h"
#include "TreeListWidget_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/MarkerLayerBundle_PARAMS.h"
#include "../params/MarkerRule_PARAMS.h"

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct MarkersTabState;
struct IconAtlasManifest;

// A tree LEAF's opaque address: either a MarkerRuleLayer or a MarkerInstanceLayer, by index into its
// own array. Per-consumer, not shared with Assembly's own future AssemblyMemberKey_UI (ARCH_19_07).
struct MarkerGroupLeafKey_UI {
    enum class Kind : int { Procedural = 0, Manual };
    Kind kind       = Kind::Procedural;
    int  layerIndex = -1;
};
inline bool operator==(const MarkerGroupLeafKey_UI& a, const MarkerGroupLeafKey_UI& b) {
    return a.kind == b.kind && a.layerIndex == b.layerIndex;
}

struct MarkerLayerBundlesState {
    SectionState  section;
    TreeListState treeState;
    int           selectedBundleIdentifier = -1;
    // ONE shared scratch triple for whichever Bundle's own expanded node body is currently drawing
    // its Move/Rotate controls — Params::MarkerLayerBundle is a pure round-tripping type and cannot
    // carry UI-only scratch state (same constraint ManualMarkerLayersState's
    // selectedLayerColorToggle/selectedLayerIconScaleToggle already accept, MarkersTab_ManualLayers_UI.h:51-56).
    float             moveOffsetX     = 0.0f;
    float             moveOffsetZ     = 0.0f;
    float             rotationDegrees = 0.0f;
    ScalarSliderRange moveOffsetRange{ -512.0f, 512.0f, 0.0f };        // Constitution §8
    ScalarSliderRange rotationDegreesRange{ -180.0f, 180.0f, 0.0f };
    RealtimeToggle    moveOffsetXToggle{true};
    RealtimeToggle    moveOffsetZToggle{true};
    RealtimeToggle    rotationDegreesToggle{true};
};

// Mints a fresh, never-reused Bundle identifier — the exact NextMarkerLayerId pattern
// (MarkerLayerId_UI.h), applied one tier up.
inline int NextMarkerLayerBundleId(const std::vector<Params::MarkerLayerBundle>& bundles) {
    int maximumId = -1;
    for (const Params::MarkerLayerBundle& bundle : bundles) maximumId = std::max(maximumId, bundle.identifier);
    return maximumId + 1;
}

// Direct (non-recursive) child-Layer enumeration, built ONCE per frame by the caller — deliberately
// NOT CollectMarkerLayerBundleRecursiveLayerIndices (STEP119/§19.9's WIDE, recursive enumeration, a
// different consumer's job): the tree widget's own recursion already walks nested Bundles, so each
// node only needs its OWN direct leaves here.
struct MarkerLayerBundleLeafIndex_UI {
    std::unordered_map<int, std::vector<MarkerGroupLeafKey_UI>> leavesByBundleIdentifier;
};
MarkerLayerBundleLeafIndex_UI BuildMarkerLayerBundleLeafIndex(
    const std::vector<Params::MarkerRuleLayer>& ruleLayers,
    const std::vector<Params::MarkerInstanceLayer>& instanceLayers);

// The Bundle tree Section. `rootState` is the whole MarkersTab state (needed for
// DrawAddMarkerRuleLayerButton's MarkersTabState& parameter). `geometry`/`globalSymmetryMask`/
// `globalRadialRepeatCount`/`markerSymmetryFixSettings` thread straight through to each expanded
// Manual leaf's DrawLayerRowBody, same parameter list DrawManualMarkerLayers already takes.
void DrawMarkerLayerBundleTree(std::vector<Params::MarkerLayerBundle>& bundles,
                               std::vector<Params::MarkerRuleLayer>& ruleLayers,
                               std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                               std::vector<Params::MarkerInstanceGroup>& markers,
                               const Params::Geometry& geometry, int globalSymmetryMask,
                               int globalRadialRepeatCount,
                               Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                               MarkerLayerBundlesState& state, MarkersTabState& rootState,
                               Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest);

} // namespace Ui
} // namespace SanmapGen
```

### 9. `src/ui/MarkersTab_Bundles_UI.cpp`
```cpp
#include "MarkersTab_Bundles_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include "MarkersTab_RuleLayers_UI.h"
#include "MarkersTab_UI.h"
#include "PlacementRuleSections_UI.h"
#include "../math/RigidTransformPivot_MATH.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

constexpr float kBundleRotatePi = 3.14159265358979323846f;   // per-file local literal — established
                                                              // convention (symmetryPi, RadialClearance_MATH's pi)

// Move/Rotate, both scoped to the Bundle's MANUAL-ONLY resolved membership (§19.9) — a Procedural
// Layer under this Bundle contributes zero members here, by design.
void ApplyMarkerLayerBundleMove(int bundleIdentifier, const std::vector<Params::MarkerLayerBundle>& bundles,
                                const std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                std::vector<Params::MarkerInstanceGroup>& markers, float offsetX, float offsetZ) {
    const auto members = Params::CollectMarkerLayerBundleRecursiveManualMembers(bundleIdentifier, bundles,
                                                                                instanceLayers, markers);
    for (const auto& member : members) {
        auto& transform = markers[static_cast<std::size_t>(member.first)]
            .transforms[static_cast<std::size_t>(member.second)];
        transform.transform.positionX += offsetX;
        transform.transform.positionZ += offsetZ;
    }
}

void ApplyMarkerLayerBundleRotation(int bundleIdentifier, const std::vector<Params::MarkerLayerBundle>& bundles,
                                    const std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                    std::vector<Params::MarkerInstanceGroup>& markers, float degrees) {
    const auto members = Params::CollectMarkerLayerBundleRecursiveManualMembers(bundleIdentifier, bundles,
                                                                                instanceLayers, markers);
    if (members.empty()) return;
    float centroidX = 0.0f, centroidZ = 0.0f;
    for (const auto& member : members) {
        const auto& transform = markers[static_cast<std::size_t>(member.first)]
            .transforms[static_cast<std::size_t>(member.second)];
        centroidX += transform.transform.positionX; centroidZ += transform.transform.positionZ;
    }
    centroidX /= static_cast<float>(members.size()); centroidZ /= static_cast<float>(members.size());
    const float angleRadians = degrees * (kBundleRotatePi / 180.0f);
    float yawX, yawY, yawZ, yawW;
    Math::YawQuaternion(angleRadians, yawX, yawY, yawZ, yawW);
    for (const auto& member : members) {
        auto& transform = markers[static_cast<std::size_t>(member.first)]
            .transforms[static_cast<std::size_t>(member.second)];
        float newX, newZ;
        Math::RotatePointAroundPivot(transform.transform.positionX, transform.transform.positionZ,
                                     centroidX, centroidZ, angleRadians, newX, newZ);
        transform.transform.positionX = newX; transform.transform.positionZ = newZ;
        float newRotX, newRotY, newRotZ, newRotW;
        Math::MultiplyQuaternions(yawX, yawY, yawZ, yawW, transform.transform.rotationX,
                                  transform.transform.rotationY, transform.transform.rotationZ,
                                  transform.transform.rotationW, newRotX, newRotY, newRotZ, newRotW);
        transform.transform.rotationX = newRotX; transform.transform.rotationY = newRotY;
        transform.transform.rotationZ = newRotZ; transform.transform.rotationW = newRotW;
    }
}

// One Bundle's own inline body: rename, type scope, per-Bundle "add a Layer here", Move/Rotate,
// Delete (promotes children, never cascades). Never "selected"-gated (STEP110 posture).
void DrawMarkerLayerBundleNodeBody(int bundleIdentifier, std::vector<Params::MarkerLayerBundle>& bundles,
                                   std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                   std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers,
                                   MarkerLayerBundlesState& state, MarkersTabState& rootState,
                                   Pipeline::PreviewDriver* previewDriver) {
    const auto bundleIt = std::find_if(bundles.begin(), bundles.end(),
        [&](const Params::MarkerLayerBundle& candidate) { return candidate.identifier == bundleIdentifier; });
    if (bundleIt == bundles.end()) return;   // Constitution §6 — an id is validated, never trusted
    Params::MarkerLayerBundle& bundle = *bundleIt;

    TextInputRules nameRules;
    nameRules.maximumLength = 48; nameRules.bAllowEmpty = false; nameRules.fallbackText = "Group";
    DrawTextInput("Name", bundle.name, nameRules);
    TextInputRules typeRules;
    typeRules.maximumLength = 48; typeRules.bAllowEmpty = true;
    DrawTextInput("Marker Type", bundle.markerTypeName, typeRules);   // free text — soft-validated
                                                                       // only (§19.12), no combo built

    if (DrawAddMarkerRuleLayerButton(ruleLayers, rootState, bundleIdentifier))
        NotifyPlacementChange(true, previewDriver);
    ImGui::SameLine();
    DrawLayerListButtons(instanceLayers, rootState.manualLayers, bundleIdentifier);   // no PreviewDriver
                                                                                      // notify — SCOPE NOTE 3

    ImGui::Separator();
    ImGui::TextUnformatted("Move");
    DrawSliderScalar("Offset X", state.moveOffsetX, state.moveOffsetRange, state.moveOffsetXToggle);
    DrawSliderScalar("Offset Z", state.moveOffsetZ, state.moveOffsetRange, state.moveOffsetZToggle);
    if (ImGui::Button("Apply Move"))
        ApplyMarkerLayerBundleMove(bundleIdentifier, bundles, instanceLayers, markers,
                                   state.moveOffsetX, state.moveOffsetZ);
    ImGui::TextUnformatted("Rotate");
    DrawSliderScalar("Degrees", state.rotationDegrees, state.rotationDegreesRange, state.rotationDegreesToggle);
    if (ImGui::Button("Apply Rotation"))
        ApplyMarkerLayerBundleRotation(bundleIdentifier, bundles, instanceLayers, markers, state.rotationDegrees);

    ImGui::Separator();
    if (ImGui::Button("Delete Group (promotes children)")) {
        const int parentIdentifier = bundle.parentBundleIdentifier;
        for (Params::MarkerLayerBundle& candidate : bundles)
            if (candidate.parentBundleIdentifier == bundleIdentifier) candidate.parentBundleIdentifier = parentIdentifier;
        for (Params::MarkerRuleLayer& layer : ruleLayers)
            if (layer.parentBundleIdentifier == bundleIdentifier) layer.parentBundleIdentifier = parentIdentifier;
        for (Params::MarkerInstanceLayer& layer : instanceLayers)
            if (layer.parentBundleIdentifier == bundleIdentifier) layer.parentBundleIdentifier = parentIdentifier;
        bundles.erase(bundleIt);
        if (state.selectedBundleIdentifier == bundleIdentifier) state.selectedBundleIdentifier = -1;
    }
}

// One Layer LEAF's own inline body — unchanged reuse (ARCH_19_07's "good news" finding).
void DrawMarkerGroupLeafBody(const MarkerGroupLeafKey_UI& leaf, std::vector<Params::MarkerRuleLayer>& ruleLayers,
                             std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                             std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                             int globalSymmetryMask, int globalRadialRepeatCount,
                             Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                             MarkersTabState& rootState, Pipeline::PreviewDriver* previewDriver) {
    if (leaf.kind == MarkerGroupLeafKey_UI::Kind::Procedural) {
        if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(ruleLayers.size())) return;
        DrawRuleLayerSettings(ruleLayers[static_cast<std::size_t>(leaf.layerIndex)], previewDriver);
    } else {
        if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(instanceLayers.size())) return;
        DrawLayerRowBody(instanceLayers[static_cast<std::size_t>(leaf.layerIndex)], leaf.layerIndex,
                         instanceLayers, markers, geometry, globalSymmetryMask, globalRadialRepeatCount,
                         markerSymmetryFixSettings, rootState.manualLayers);
    }
}

const char* MarkerGroupLeafLabel(const MarkerGroupLeafKey_UI& leaf,
                                 const std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                 const std::vector<Params::MarkerInstanceLayer>& instanceLayers) {
    if (leaf.kind == MarkerGroupLeafKey_UI::Kind::Procedural)
        return (leaf.layerIndex >= 0 && leaf.layerIndex < static_cast<int>(ruleLayers.size())
               && !ruleLayers[static_cast<std::size_t>(leaf.layerIndex)].name.empty())
             ? ruleLayers[static_cast<std::size_t>(leaf.layerIndex)].name.c_str() : "Marker Layer";
    return (leaf.layerIndex >= 0 && leaf.layerIndex < static_cast<int>(instanceLayers.size()))
         ? ManualMarkerLayerRowLabel(instanceLayers[static_cast<std::size_t>(leaf.layerIndex)]) : "Marker Layer";
}

} // namespace

MarkerLayerBundleLeafIndex_UI BuildMarkerLayerBundleLeafIndex(
        const std::vector<Params::MarkerRuleLayer>& ruleLayers,
        const std::vector<Params::MarkerInstanceLayer>& instanceLayers) {
    MarkerLayerBundleLeafIndex_UI index;
    for (int i = 0; i < static_cast<int>(ruleLayers.size()); ++i)
        if (ruleLayers[static_cast<std::size_t>(i)].parentBundleIdentifier >= 0)
            index.leavesByBundleIdentifier[ruleLayers[static_cast<std::size_t>(i)].parentBundleIdentifier]
                .push_back(MarkerGroupLeafKey_UI{MarkerGroupLeafKey_UI::Kind::Procedural, i});
    for (int i = 0; i < static_cast<int>(instanceLayers.size()); ++i)
        if (instanceLayers[static_cast<std::size_t>(i)].parentBundleIdentifier >= 0)
            index.leavesByBundleIdentifier[instanceLayers[static_cast<std::size_t>(i)].parentBundleIdentifier]
                .push_back(MarkerGroupLeafKey_UI{MarkerGroupLeafKey_UI::Kind::Manual, i});
    return index;
}

void DrawMarkerLayerBundleTree(std::vector<Params::MarkerLayerBundle>& bundles,
                               std::vector<Params::MarkerRuleLayer>& ruleLayers,
                               std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                               std::vector<Params::MarkerInstanceGroup>& markers,
                               const Params::Geometry& geometry, int globalSymmetryMask,
                               int globalRadialRepeatCount,
                               Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                               MarkerLayerBundlesState& state, MarkersTabState& rootState,
                               Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest*) {
    if (!DrawSectionBegin("Groups", state.section)) return;

    if (ImGui::Button("Add Group")) {
        Params::MarkerLayerBundle bundle;
        bundle.identifier = NextMarkerLayerBundleId(bundles);
        bundle.name       = "Group";
        bundles.push_back(bundle);
        state.selectedBundleIdentifier = bundle.identifier;
    }

    const MarkerLayerBundleLeafIndex_UI leafIndex = BuildMarkerLayerBundleLeafIndex(ruleLayers, instanceLayers);

    const TreeListSignal<MarkerGroupLeafKey_UI> signal =
        TreeListWidget_UI<Params::MarkerLayerBundle, MarkerGroupLeafKey_UI>::Render(
            "markerLayerBundles", bundles,
            [](const Params::MarkerLayerBundle& bundle) { return bundle.identifier; },
            [](const Params::MarkerLayerBundle& bundle) { return bundle.parentBundleIdentifier; },
            [](const Params::MarkerLayerBundle& bundle) { return bundle.name.empty() ? "Group" : bundle.name.c_str(); },
            [&](int bundleIdentifier) {
                DrawMarkerLayerBundleNodeBody(bundleIdentifier, bundles, ruleLayers, instanceLayers, markers,
                                              state, rootState, previewDriver);
            },
            [&](int bundleIdentifier) -> const std::vector<MarkerGroupLeafKey_UI>& {
                static const std::vector<MarkerGroupLeafKey_UI> kNoLeaves;
                const auto it = leafIndex.leavesByBundleIdentifier.find(bundleIdentifier);
                return it != leafIndex.leavesByBundleIdentifier.end() ? it->second : kNoLeaves;
            },
            [&](const MarkerGroupLeafKey_UI& leaf) { return MarkerGroupLeafLabel(leaf, ruleLayers, instanceLayers); },
            [&](const MarkerGroupLeafKey_UI& leaf) {
                DrawMarkerGroupLeafBody(leaf, ruleLayers, instanceLayers, markers, geometry, globalSymmetryMask,
                                        globalRadialRepeatCount, markerSymmetryFixSettings, rootState, previewDriver);
            },
            state.treeState, state.selectedBundleIdentifier);

    if (signal.kind == TreeListSignalKind::Select && signal.sourceKind == TreeNodeSourceKind::Node)
        state.selectedBundleIdentifier = signal.sourceNodeIdentifier;

    if (signal.kind == TreeListSignalKind::Reparent) {
        if (signal.sourceKind == TreeNodeSourceKind::Leaf) {
            if (signal.sourceLeaf.kind == MarkerGroupLeafKey_UI::Kind::Procedural) {
                if (signal.sourceLeaf.layerIndex >= 0 && signal.sourceLeaf.layerIndex < static_cast<int>(ruleLayers.size()))
                    ruleLayers[static_cast<std::size_t>(signal.sourceLeaf.layerIndex)].parentBundleIdentifier =
                        signal.targetNodeIdentifier;
            } else if (signal.sourceLeaf.layerIndex >= 0
                      && signal.sourceLeaf.layerIndex < static_cast<int>(instanceLayers.size())) {
                instanceLayers[static_cast<std::size_t>(signal.sourceLeaf.layerIndex)].parentBundleIdentifier =
                    signal.targetNodeIdentifier;
            }
        } else if (!Params::WouldReparentMarkerLayerBundleCreateCycle(
                      signal.sourceNodeIdentifier, signal.targetNodeIdentifier, bundles)) {
            int newParent = signal.targetNodeIdentifier;
            if (signal.dropZone != TreeDropZone::OnAsChild) {   // Above/Below: same parent as target (sibling)
                newParent = -1;
                for (const Params::MarkerLayerBundle& target : bundles)
                    if (target.identifier == signal.targetNodeIdentifier) { newParent = target.parentBundleIdentifier; break; }
            }
            for (Params::MarkerLayerBundle& bundle : bundles)
                if (bundle.identifier == signal.sourceNodeIdentifier) { bundle.parentBundleIdentifier = newParent; break; }
        }
        // sourceNodeIdentifier == -1 with kind == Reparent cannot occur — the root drop zone is a
        // TARGET only (DrawRootDropZoneRow never itself emits Select/originates a drag).
    }

    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
```

### 10. `MarkersTab_UI.cpp`/`.h` restructure
`MarkersTab_UI.h`: add `#include "MarkersTab_Bundles_UI.h"`; `MarkersTabState` gains, after
`manualLayers` (line 99):
```cpp
    // STEP120: the Group/Bundle tree, drawn before the two "Ungrouped ..." sections it filters
    // (MarkersTab_Bundles_UI.h).
    MarkerLayerBundlesState bundles;
```
`MarkersTab_UI.cpp`, `DrawMarkersTab` (lines 40-60) — insert the tree BEFORE `DrawRuleStack`, right
after Globals:
```cpp
void DrawMarkersTab(Params::MapRecipe& recipe, MarkersTabState& state,
                    Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                    const Data::PlacementInstances* placedMarkers) {
    ImGui::PushID("markersTab");
    DrawMarkersTabGlobals(state.globals, iconManifest);
    DrawMarkerLayerBundleTree(recipe.markerLayerBundles, recipe.markerRuleLayers, recipe.markerLayers,
                              recipe.markers, recipe.geometry, recipe.globalSymmetryMask,
                              recipe.radialSymmetryRepeatCount, recipe.markerSymmetryFixSettings,
                              state.bundles, state, previewDriver, iconManifest);
    DrawRuleStack(recipe, state, previewDriver, iconManifest);
    DrawManualMarkerLayers(state.manualLayers, recipe.markerLayers, recipe.markers, recipe.geometry,
                          recipe.globalSymmetryMask, recipe.radialSymmetryRepeatCount,
                          recipe.markerSymmetryFixSettings);
    state.manual.positionHorizontalRange = MarkerPositionHorizontalSliderRange(recipe.geometry.mapSize);
    DrawManualMarkers(recipe.markers, recipe.armies, recipe.markerLayers, state.manual,
                      state.manualLayers.selectedLayerIndex, iconManifest);
    DrawPlacedMarkerList(placedMarkers, state.placedList);
    ImGui::PopID();
}
```
(`recipe.markerSymmetryFixSettings`/`recipe.globalSymmetryMask`/`recipe.radialSymmetryRepeatCount`
are the same fields `DrawManualMarkerLayers`'s own existing call already reads two lines below —
verify their exact `MapRecipe_PARAMS.h` field spellings match at implementation time, since this
ticket did not re-grep that file line-by-line.)

## Out of scope
- **Canvas live-drag of a Bundle.** ARCH §19.10 — tab-driven Move/Rotate only, no new canvas
  gesture. `MapCanvas_Draw_UI.cpp`/`MapCanvas_MarkerDrag_UI.cpp` untouched by this ticket.
- **Assembly's own tab/ticket.** This ticket builds `TreeListWidget_UI<T, LeafKeyT>` generically but
  Assembly's own consumer (`Params::Assembly`, `AssemblyMemberKey_UI`, the Assemblies tab,
  `CollectAssemblyRecursiveMembership`'s Bundle-table-walking extension) is a separate, later,
  still-unratified-in-full ticket (§19.5's explicit sequencing note).
- **Props/Decals Bundle twins.** `PropLayerBundle`/`DecalLayerBundle` are independently ticketed
  later (§19.2), same generic foundation, not built here.
- **Hard/import-time validation of Bundle→`markerTypeName` consistency.** §19.12 — soft,
  UI-displayed-as-authored only. No combo auto-populated from existing `MarkerInstanceGroup` names;
  a plain free-text field, flagged as a legitimate V2 refinement, not required for correctness.
- **Symmetry-follow composition on a Bundle move/rotate.** §19.10's STEP94-interaction note —
  deferred, same as Assembly's own identical deferred hazard; v1 applies the flat rigid delta to
  exactly the resolved member set, nothing else.
- **Bundle name uniqueness / `MakeNamesUnique`-style repair.** Bundles are keyed by `identifier` on
  the wire (ARCH_19_04), not by name — no export-key parity need, unlike Manual/Rule Layers.
- **`bColorOverrideEnabled`/icon/color rendering consequences of Bundle membership.** A Bundle is
  purely organizational; it carries no color/icon/visibility of its own, and none of this ticket's
  code changes how a marker actually draws.
- **`CollectAssemblyRecursiveMembership`'s own extension, §19.6's cutoff rule's actual
  implementation.** Both are PARAMS-layer, both belong to Assembly's future ticket per §19.5's
  explicit sequencing note — this ticket's own `WouldReparentMarkerLayerBundleCreateCycle` /
  `CollectMarkerLayerBundleRecursiveManualMembers` calls only CONSUME STEP119's functions, never
  reimplement or extend them.

## Files touched
- `src/math/RigidTransformPivot_MATH.h` — NEW: `RotatePointAroundPivot`, `MultiplyQuaternions`,
  `YawQuaternion`
- `src/math/RigidTransformPivot_MATH_Test.cpp` — NEW
- `src/ui/TreeListWidget_Types_UI.h` — NEW
- `src/ui/TreeListWidget_DragDrop_UI.h` — NEW
- `src/ui/TreeListWidget_UI.h` — NEW
- `src/ui/TreeListWidget_UI_Test.cpp` — NEW
- `src/ui/MarkersTab_Bundles_UI.h` — NEW
- `src/ui/MarkersTab_Bundles_UI.cpp` — NEW
- `src/ui/DraggableListWidget_Types_UI.h` — `DraggableListRow` gains `bRowSuppressed`
- `src/ui/DraggableListWidget_RowLayout_UI.h` — `Render`'s loop skips suppressed rows
- `src/ui/MarkersTab_RuleLayers_UI.h` — new `DrawAddMarkerRuleLayerButton` declaration
- `src/ui/MarkersTab_RuleLayerSettings_UI.cpp` — extracts `DrawAddMarkerRuleLayerButton` out of
  `DrawRuleLayerButtons`
- `src/ui/MarkersTab_RuleLayers_UI.cpp` — `DrawRuleLayerListBody`'s `describeRow` sets `bRowSuppressed`
- `src/ui/MarkersTab_ManualLayers_UI.h` — new `DrawLayerListButtons` declaration (moved out of the
  anonymous namespace, gains `parentBundleIdentifierForNewLayer`)
- `src/ui/MarkersTab_ManualLayers_UI.cpp` — `DrawLayerListButtons` relocated + gains parameter;
  `DrawLayerList`'s `describeRow` sets `bRowSuppressed`; Section title renamed
- `src/ui/MarkersTab_UI.h` — `#include "MarkersTab_Bundles_UI.h"`; `MarkersTabState` gains `bundles`
- `src/ui/MarkersTab_UI.cpp` — `DrawMarkersTab` calls `DrawMarkerLayerBundleTree`; Section title renamed
- `CMakeLists.txt` — `ListWidgets_UI_Test` gains `src/ui/TreeListWidget_UI_Test.cpp`; new
  `add_sangen_test(RigidTransformPivot_MATH_Test src/math/RigidTransformPivot_MATH_Test.cpp)`
- `src/ui/VirtualListWidget_UI_Test.cpp` — forward-declares and calls `RunTreeListAcceptance()`

## Verify
Acceptance bar: the tree widget's structural contract (id→children build, Select/Reparent/Delete
signal correctness, drop-zone geometry, cycle refusal) is proven headless via synthetic imgui
frames, mirroring `DraggableListWidget_UI_Test.cpp`'s own established pattern
(`HeadlessImguiSession`/`RunHeadlessFrame`, `ListWidget_TestFrame_UI.h`); every pure (non-imgui)
helper this ticket adds gets its own pure unit test; the MATH functions get their own numeric
acceptance test, same shape as `Trigonometry_MATH_Test.cpp`.

- **`src/math/RigidTransformPivot_MATH_Test.cpp`** (own `main()`, own `add_sangen_test` entry,
  mirroring `Trigonometry_MATH_Test.cpp`'s plain-`printf` shape, no imgui):
  - `RotatePointAroundPivot`: a point at `(pivotX+1, pivotZ)` rotated 90° lands within float
    tolerance at `(pivotX, pivotZ+1)` (confirm sign against the function's own counter-clockwise
    convention, matching `AppendRadialTurns`'s); a 360° rotation returns to the original point
    within tolerance; a point AT the pivot stays at the pivot for any angle.
  - `YawQuaternion`: angle `0` returns the identity quaternion `(0,0,0,1)`; angle `π` returns
    `(0,1,0,0)` within tolerance (matches the half-angle sine/cosine construction).
  - `MultiplyQuaternions`: multiplying by the identity quaternion (as either operand) returns the
    other operand unchanged; multiplying two opposite 180° yaw quaternions returns the identity
    (within tolerance).
- **`src/ui/TreeListWidget_UI_Test.cpp`** (added to the `ListWidgets_UI_Test` binary; own
  `RunTreeListAcceptance()` called from `VirtualListWidget_UI_Test.cpp`'s existing `main()`,
  mirroring `RunDraggableListAcceptance()`'s own wiring exactly):
  - A small fixture (3 `struct TestNode { int identifier; int parentIdentifier; const char* name; }`
    forming a 2-deep tree, plus 2 leaf keys attached to one node) drives `TreeListWidget_UI<TestNode,
    int>::Render` (leaf key `int` for the test — simplest legal `LeafKeyT`).
  - Clicking a root node's header emits `Select{sourceKind=Node, sourceNodeIdentifier=that node's id}`.
  - Expanding a node (synthetic click on its header, then a second frame) causes `drawNodeBody` to
    be invoked with that node's identifier, and — only while expanded — `describeLeaves`'s returned
    leaves each get a leaf row drawn; collapsed, neither `drawNodeBody` nor any leaf row's body draws
    (tally invocation counts, mirroring `VirtualListWidget_UI_Test.cpp`'s `DrawTally` pattern).
  - Clicking a leaf row emits `Select{sourceKind=Leaf, sourceLeaf=that leaf's key}`.
  - A synthetic drag (press on a leaf row, drag onto a different node's row middle band, release)
    emits `Reparent{sourceKind=Leaf, targetNodeIdentifier=that node's id, dropZone=OnAsChild}`.
  - A synthetic drag from a node row onto another node row's TOP band emits `Reparent{sourceKind=Node,
    dropZone=Above}`; onto the BOTTOM band emits `dropZone=Below`; onto the middle emits
    `dropZone=OnAsChild` — proves the three-zone geometry split works (probe several Y offsets within
    the row rect, mirroring `TestAffordanceSignalsCarryTheRightIndex`'s sweep-and-probe approach).
  - A synthetic drag from a node row onto the root drop zone row emits
    `Reparent{targetNodeIdentifier=-1}`.
  - A node dragged onto ITSELF emits no signal (`bSelfDrop` guard).
  - `TreeListState::expandedNodeIdentifiers` is never mutated by a click that does NOT hit that
    node's own header (proves the widget "mutates nothing but its own expand-state" contract holds).
- **`src/ui/MarkersTab_Bundles_UI_Test.cpp`** (new, pure logic only — no imgui needed for these,
  matching STEP106's own "defer the imgui-coupled path, test the definitely-pure pieces" posture
  where a full click-path harness isn't warranted for a small helper):
  - `BuildMarkerLayerBundleLeafIndex`: a fixture with 3 rule layers and 2 instance layers, mixed
    `parentBundleIdentifier` values (`-1`, `0`, `0`, `1`, `-1`) asserts the returned index has exactly
    the expected `{Kind, layerIndex}` entries under bundle `0` and bundle `1`, and nothing under any
    other key (in particular, no key for `-1`).
  - `NextMarkerLayerBundleId`: empty vector returns `0`; a vector with identifiers `{0, 3, 1}`
    returns `4` (mirrors `NextMarkerLayerId`'s own existing test shape).
  - `ApplyMarkerLayerBundleMove`/`ApplyMarkerLayerBundleRotation` (needs STEP119's
    `CollectMarkerLayerBundleRecursiveManualMembers` — sequenced after STEP119 per the Sequencing
    note above): a fixture with one Bundle, one Manual layer under it, two `MarkerTransform`s on that
    layer; `ApplyMarkerLayerBundleMove(id, ..., 5.0f, -3.0f)` adds exactly that offset to both
    transforms' `positionX`/`positionZ` and leaves `positionY`/rotation untouched;
    `ApplyMarkerLayerBundleRotation(id, ..., 90.0f)` moves each transform's `(positionX, positionZ)`
    to the expected 90°-about-centroid position within float tolerance, AND changes each transform's
    `rotationY`/`rotationW` from the identity (proves the yaw-quaternion compose actually ran, not
    just the position rotate); a Bundle whose only member is a PROCEDURAL layer (no Manual layer)
    resolves to an empty member set and both Apply functions no-op (§19.9's manual-only membership,
    proven at this ticket's own call boundary, not re-testing STEP119's own resolver).
- **Existing suites stay green with no behavior change to any assertion this ticket does not itself
  add**: `ListWidgets_UI_Test` (every existing `DraggableListWidget_UI_Test.cpp`/
  `VirtualListWidget_UI_Test.cpp` assertion, since `bRowSuppressed` defaults `false` and the new
  `main()` addition is purely additive), and every existing `MarkersTab_*`/`MarkerDragGesture_*` test
  binary — `DrawRuleLayerButtons`'s extraction and `DrawLayerListButtons`'s relocation/signature
  change must not alter either function's OWN behavior at its existing (root-scoped, `-1`) call site.
- **No manual/interactive verification substitutes for the above** — every piece here has a real
  headless seam (either pure-logic or synthetic-imgui-frame); none of this ticket's surface is
  flagged as untestable.

---

**Files read to ground this ticket** (absolute paths under `D:\Projects\Sanctuary\Map Generator\`):
`ARCH_19_MarkerLayerBundle.md` + all twelve `ARCH_19_01`–`ARCH_19_12` subsection files;
`work_orders\DESIGN_MarkerGroupLayerRestructure_R1.md`; `work_orders\BRIEF_MarkerGroupLayerRestructure_R1.md`;
`work_orders\DESIGN_Assembly_R1.md`; `work_orders\STEP106_MarkerLayerLockAndGridSnap_PARAMS.md`;
`src\ui\MarkersTab_UI.cpp`/`.h`; `src\ui\MarkersTab_RuleLayers_UI.cpp`/`.h`;
`src\ui\MarkersTab_RuleLayerSettings_UI.cpp`; `src\ui\MarkersTab_ManualLayers_UI.cpp`/`.h`;
`src\ui\MarkersTab_Manual_UI.h`; `src\ui\DraggableListWidget_UI.h`,
`DraggableListWidget_Types_UI.h`, `DraggableListWidget_RowLayout_UI.h`,
`DraggableListWidget_RowAffordances_UI.h`, `DraggableList_TestScene_UI.h`,
`DraggableListWidget_UI_Test.cpp`; `src\ui\ListWidget_TestFrame_UI.h`,
`VirtualListWidget_UI_Test.cpp`; `src\ui\Section_UI.h`, `Checkbox_UI.h`, `TextInput_UI.h`,
`ConfirmDialog_UI.h`, `SliderScalar_UI.h`, `WidgetHelpers_UI.h`, `MarkerLayerId_UI.h`;
`src\params\MarkerRule_PARAMS.h`, `MarkerInstance_PARAMS.h`, `InstancedTransform_PARAMS.h`;
`src\math\Trigonometry_MATH.h`, `Trigonometry_MATH_Test.cpp`, `RadialClearance_MATH.h`;
`src\proc\Placement_SymmetryOrbit_PROC.h`, `Placement_Transform_PROC.h`; `CMakeLists.txt`
(lines 404-406, 659-666).
