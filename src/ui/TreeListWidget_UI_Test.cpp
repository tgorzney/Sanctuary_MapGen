// TreeListWidget_UI_Test.cpp — STEP120 acceptance for TreeListWidget_UI<T, LeafKeyT>: id->children
// build, Select/Reparent signal correctness, drop-zone geometry, cycle-refusal being the CALLER's
// job (never the widget's), and the "mutates nothing but its own expand-state" contract. Driven
// through the REAL imgui interaction path (headless), not by hand-built signals — the same posture
// DraggableListWidget_UI_Test.cpp already established. The fixture and pointer helpers are in
// TreeListWidget_TestScene_UI.h; main() is in VirtualListWidget_UI_Test.cpp.
#include "TreeListWidget_TestScene_UI.h"

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

// The node header's own OpenOnArrow hit zone is a narrow band near the row's left edge — found by
// sweeping rather than hardcoded, so this test cannot drift from the widget's own layout (the same
// posture TestAffordanceSignalsCarryTheRightIndex's sweep-and-probe takes for DraggableList).
float FindNodeArrowOffsetX(int nodeIdentifier) {
    HeadlessImguiSession session;
    for (float offset = 2.0f; offset <= 40.0f; offset += 2.0f) {
        TreeScene probe = MakeTreeScene();
        RunTreeSceneFrame(probe, kTreeMouseAway, false);
        RunTreeSceneFrame(probe, kTreeMouseAway, false);
        const float rowHeight = probe.nodeRowTopLeft[30].y - probe.nodeRowTopLeft[10].y;
        const ImVec2 clickPoint(probe.nodeRowTopLeft[nodeIdentifier].x + offset,
                                probe.nodeRowTopLeft[nodeIdentifier].y + rowHeight * 0.5f);
        ClickAt(probe, clickPoint);
        if (probe.treeState.expandedNodeIdentifiers[nodeIdentifier]) return offset;
    }
    return -1.0f;
}

// Clicking a root node's own label emits Select{Node, that node's id} — and, since OpenOnArrow gates
// the expand toggle to the arrow zone specifically, this label click must NOT also flip
// expandedNodeIdentifiers for that node (the "mutates nothing but via the arrow" half of the
// contract; TestExpandingNodeInvokesBodyAndLeafRows below proves the arrow DOES flip it).
void TestClickingRootNodeHeaderEmitsSelect() {
    HeadlessImguiSession session;
    TreeScene scene = MakeTreeScene();
    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    const float rowHeight = scene.nodeRowTopLeft[30].y - scene.nodeRowTopLeft[10].y;
    const ImVec2 labelPoint(scene.rowLeftX + 60.0f, scene.nodeRowTopLeft[10].y + rowHeight * 0.5f);
    const TreeListSignal<int> signal = ClickAt(scene, labelPoint);
    CheckListWidgetExpectation(signal.kind == TreeListSignalKind::Select
                               && signal.sourceKind == TreeNodeSourceKind::Node
                               && signal.sourceNodeIdentifier == 10,
                               "clicking a root node's label selects that node");
    CheckListWidgetExpectation(scene.treeState.expandedNodeIdentifiers.find(10)
                               == scene.treeState.expandedNodeIdentifiers.end()
                               || !scene.treeState.expandedNodeIdentifiers[10],
                               "a label click away from the arrow never mutates expandedNodeIdentifiers");
}

// A synthetic click on the node's own arrow, then a second frame: drawNodeBody is invoked with that
// node's identifier, and — only while expanded — describeLeaves's own leaves each get a row drawn.
// Collapsed, neither drawNodeBody nor any leaf row draws at all.
void TestExpandingNodeInvokesBodyAndLeafRows() {
    const float arrowOffsetX = FindNodeArrowOffsetX(10);
    CheckListWidgetExpectation(arrowOffsetX > 0.0f, "the node header's own arrow hit zone was found");

    HeadlessImguiSession session;
    TreeScene scene = MakeTreeScene();
    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    RunTreeSceneFrame(scene, kTreeMouseAway, false);   // read the settled COLLAPSED tallies
    CheckListWidgetExpectation(scene.nodeBodyCallsThisFrame.empty(),
                               "a collapsed node never invokes drawNodeBody");
    CheckListWidgetExpectation(scene.leafRowDrawsThisFrame.empty(),
                               "a collapsed node draws no leaf rows at all");

    const float rowHeight = scene.nodeRowTopLeft[30].y - scene.nodeRowTopLeft[10].y;
    const ImVec2 arrowPoint(scene.nodeRowTopLeft[10].x + arrowOffsetX,
                            scene.nodeRowTopLeft[10].y + rowHeight * 0.5f);
    ClickAt(scene, arrowPoint);
    RunTreeSceneFrame(scene, kTreeMouseAway, false);   // the second, settled frame

    bool bNodeBodyCalledForRoot1 = false;
    for (int id : scene.nodeBodyCallsThisFrame) if (id == 10) bNodeBodyCalledForRoot1 = true;
    CheckListWidgetExpectation(bNodeBodyCalledForRoot1,
                               "expanding the node invokes drawNodeBody with its own identifier");
    bool bLeaf100RowDrawn = false, bLeaf101RowDrawn = false;
    for (int leaf : scene.leafRowDrawsThisFrame) {
        if (leaf == 100) bLeaf100RowDrawn = true;
        if (leaf == 101) bLeaf101RowDrawn = true;
    }
    CheckListWidgetExpectation(bLeaf100RowDrawn && bLeaf101RowDrawn,
                               "expanding the node draws both of describeLeaves's own leaf rows");
}

// Clicking a leaf row's own label selects that LEAF (sourceKind == Leaf, carrying its own key) —
// never a Node signal.
void TestClickingLeafRowEmitsSelect() {
    HeadlessImguiSession session;
    TreeScene scene = MakeTreeScene();
    scene.treeState.expandedNodeIdentifiers[10] = true;   // caller-owned state (Section_UI.h posture)
    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    CheckListWidgetExpectation(scene.leafRowTopLeft.count(100) == 1,
                               "leaf 100's row drew once its own node is expanded");
    const ImVec2 leafPoint(scene.rowLeftX + 30.0f, scene.leafRowTopLeft[100].y + 4.0f);
    const TreeListSignal<int> signal = ClickAt(scene, leafPoint);
    CheckListWidgetExpectation(signal.kind == TreeListSignalKind::Select
                               && signal.sourceKind == TreeNodeSourceKind::Leaf && signal.sourceLeaf == 100,
                               "clicking a leaf row selects that leaf, carrying its own key");
}

// Press on a leaf row, drag onto a different node's row middle band, release: a Leaf-sourced
// Reparent naming that node as the new parent (dropZone is meaningless for a leaf target, always
// OnAsChild — a leaf reparent has no Above/Below sibling concept).
void TestDragLeafOntoNodeRowMiddleEmitsReparent() {
    HeadlessImguiSession session;
    TreeScene scene = MakeTreeScene();
    scene.treeState.expandedNodeIdentifiers[10] = true;
    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    const float nodeRowHeight = scene.nodeRowTopLeft[20].y - scene.nodeRowTopLeft[10].y;
    const ImVec2 grabPoint(scene.rowLeftX + 30.0f, scene.leafRowTopLeft[100].y + 4.0f);
    const ImVec2 dropPoint(scene.rowLeftX + 30.0f, scene.nodeRowTopLeft[30].y + nodeRowHeight * 0.5f);
    const TreeListSignal<int> signal = DragOnto(scene, grabPoint, dropPoint);
    CheckListWidgetExpectation(signal.kind == TreeListSignalKind::Reparent
                               && signal.sourceKind == TreeNodeSourceKind::Leaf && signal.sourceLeaf == 100
                               && signal.targetNodeIdentifier == 30 && signal.dropZone == TreeDropZone::OnAsChild,
                               "dragging a leaf onto a node's middle band reparents it under that node");
}

// A synthetic drag from one node row onto another node row's TOP/BOTTOM/middle band emits
// Above/Below/OnAsChild respectively — the three-zone geometry split.
void TestDragNodeOntoAnotherNodesThreeBands() {
    HeadlessImguiSession session;
    TreeScene scene = MakeTreeScene();
    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    // The PRECISE item rect (not the cursor-to-cursor gap, which also carries ItemSpacing) is what
    // DetectTreeRowDragAndDrop itself bands relativeY against.
    const float itemTopY    = scene.nodeRowRectMin[10].y;
    const float itemHeight  = scene.nodeRowRectMax[10].y - scene.nodeRowRectMin[10].y;
    const float rowHeight   = scene.nodeRowTopLeft[30].y - scene.nodeRowTopLeft[10].y;
    const ImVec2 grabPoint(scene.rowLeftX + 30.0f, scene.nodeRowTopLeft[30].y + rowHeight * 0.5f);

    const ImVec2 topBand(scene.rowLeftX + 30.0f, itemTopY + itemHeight * 0.1f);
    const TreeListSignal<int> aboveSignal = DragOnto(scene, grabPoint, topBand);
    CheckListWidgetExpectation(aboveSignal.kind == TreeListSignalKind::Reparent
                               && aboveSignal.sourceNodeIdentifier == 30
                               && aboveSignal.targetNodeIdentifier == 10
                               && aboveSignal.dropZone == TreeDropZone::Above,
                               "dragging onto a node row's top band signals Above");

    RunTreeSceneFrame(scene, kTreeMouseAway, false);   // clear latent drag state between probes
    const ImVec2 bottomBand(scene.rowLeftX + 30.0f, itemTopY + itemHeight * 0.9f);
    const TreeListSignal<int> belowSignal = DragOnto(scene, grabPoint, bottomBand);
    CheckListWidgetExpectation(belowSignal.kind == TreeListSignalKind::Reparent
                               && belowSignal.sourceNodeIdentifier == 30
                               && belowSignal.targetNodeIdentifier == 10
                               && belowSignal.dropZone == TreeDropZone::Below,
                               "dragging onto a node row's bottom band signals Below");

    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    const ImVec2 middleBand(scene.rowLeftX + 30.0f, itemTopY + itemHeight * 0.5f);
    const TreeListSignal<int> childSignal = DragOnto(scene, grabPoint, middleBand);
    CheckListWidgetExpectation(childSignal.kind == TreeListSignalKind::Reparent
                               && childSignal.sourceNodeIdentifier == 30
                               && childSignal.targetNodeIdentifier == 10
                               && childSignal.dropZone == TreeDropZone::OnAsChild,
                               "dragging onto a node row's middle band signals OnAsChild");
}

// A synthetic drag from a node row onto the always-visible root/"Ungrouped" drop zone row emits a
// Reparent naming the root (-1) as the target.
void TestDragNodeOntoRootDropZoneEmitsRootTarget() {
    HeadlessImguiSession session;
    TreeScene scene = MakeTreeScene();
    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    const float rowHeight = scene.nodeRowTopLeft[30].y - scene.nodeRowTopLeft[10].y;
    const ImVec2 grabPoint(scene.rowLeftX + 30.0f, scene.nodeRowTopLeft[10].y + rowHeight * 0.5f);
    const ImVec2 rootDropPoint(scene.rowLeftX + 30.0f, scene.rootDropZoneTopY + 4.0f);
    const TreeListSignal<int> signal = DragOnto(scene, grabPoint, rootDropPoint);
    CheckListWidgetExpectation(signal.kind == TreeListSignalKind::Reparent
                               && signal.sourceNodeIdentifier == 10 && signal.targetNodeIdentifier == -1,
                               "dragging a node onto the root drop zone signals the root as the target");
}

// A node dragged onto ITSELF emits no signal at all (the bSelfDrop guard) — cycle prevention beyond
// that is explicitly NOT this widget's job (ARCH_19_08); the caller checks
// WouldReparentMarkerLayerBundleCreateCycle before applying anything.
void TestNodeDraggedOntoItselfEmitsNoSignal() {
    HeadlessImguiSession session;
    TreeScene scene = MakeTreeScene();
    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    RunTreeSceneFrame(scene, kTreeMouseAway, false);
    const float rowHeight = scene.nodeRowTopLeft[30].y - scene.nodeRowTopLeft[10].y;
    const ImVec2 selfPoint(scene.rowLeftX + 30.0f, scene.nodeRowTopLeft[30].y + rowHeight * 0.5f);
    const TreeListSignal<int> signal = DragOnto(scene, selfPoint, selfPoint);
    CheckListWidgetExpectation(!signal.bHasSignal(), "a node dragged onto itself produces no signal");
}

} // namespace

namespace SanmapGen {
namespace Ui {
void RunTreeListAcceptance() {
    TestClickingRootNodeHeaderEmitsSelect();
    TestExpandingNodeInvokesBodyAndLeafRows();
    TestClickingLeafRowEmitsSelect();
    TestDragLeafOntoNodeRowMiddleEmitsReparent();
    TestDragNodeOntoAnotherNodesThreeBands();
    TestDragNodeOntoRootDropZoneEmitsRootTarget();
    TestNodeDraggedOntoItselfEmitsNoSignal();
}
} // namespace Ui
} // namespace SanmapGen
