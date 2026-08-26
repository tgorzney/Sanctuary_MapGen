// TreeListWidget_TestScene_UI.h — the small 2-deep tree fixture the STEP120 TreeListWidget_UI_Test.cpp
// acceptance test drives, plus the synthetic-pointer helpers. Test-support only; no GL. Mirrors
// DraggableList_TestScene_UI.h's own shape.
//
// The scene is the CALLER's state: TreeListWidget_UI never writes it (but for TreeListState::
// expandedNodeIdentifiers, its one precedented exception) — every assertion is about what the test
// applied from a signal, never a mutation the widget itself made.
#pragma once
#include "ListWidget_TestFrame_UI.h"
#include "TreeListWidget_UI.h"
#include <cstdio>
#include <functional>
#include <unordered_map>
#include <vector>

namespace SanmapGen {
namespace Ui {

const ImVec2 kTreeSceneWindowSize = ImVec2(420.0f, 420.0f);
const ImVec2 kTreeMouseAway       = ImVec2(-FLT_MAX, -FLT_MAX);

struct TestNode {
    int         identifier;
    int         parentIdentifier;
    const char* name;
};

struct TreeScene {
    std::vector<TestNode> nodes;
    // node identifier -> its own direct leaf keys (int, the simplest legal LeafKeyT).
    std::unordered_map<int, std::vector<int>> leavesByNode;
    TreeListState     treeState;
    TreeListSignal<int> signal;
    int selectedNodeIdentifier = -1;

    // Geometry captured DURING the frame — the accessor lambdas double as the geometry probe, the
    // same technique DraggableList_TestScene_UI.h's own describeRow callback uses.
    std::unordered_map<int, ImVec2> nodeRowTopLeft;
    std::unordered_map<int, ImVec2> leafRowTopLeft;
    // The PRECISE item rect (min/max) of a node's own header row — captured one call later than
    // nodeRowTopLeft (at the START of the NEXT sibling's nameOf call, the previous header still
    // being the last-submitted imgui item); nodeRowTopLeft's own gap also includes ItemSpacing.
    std::unordered_map<int, ImVec2> nodeRowRectMin;
    std::unordered_map<int, ImVec2> nodeRowRectMax;
    int   previousNodeIdentifierThisFrame = -1;
    float rowLeftX          = 0.0f;
    float rootDropZoneTopY  = 0.0f;

    // Per-frame tallies, reset at the top of every RunTreeSceneFrame call.
    std::vector<int> nodeBodyCallsThisFrame;
    std::vector<int> leafRowDrawsThisFrame;   // leafLabel invoked (the row itself drew)
    std::vector<int> leafBodyCallsThisFrame;  // drawExpandedLeafBody invoked (that leaf was expanded)
};

// A 2-deep tree: Root1 (10) has one child Child1 (20); Root2 (30) is a separate root sibling. Two
// leaf keys (100, 101) are attached directly to Root1.
inline TreeScene MakeTreeScene() {
    TreeScene scene;
    scene.nodes = { {10, -1, "Root1"}, {20, 10, "Child1"}, {30, -1, "Root2"} };
    scene.leavesByNode[10] = { 100, 101 };
    return scene;
}

inline const char* TreeSceneLeafLabel(int leaf) {
    // Fixed labels for the two leaf keys this fixture ever uses — no dynamic buffer aliasing hazard.
    return leaf == 100 ? "Leaf100" : "Leaf101";
}

// STEP129: `headerExtraWidthPixels`/`drawNodeHeaderExtra`/`drawLeafHeaderExtra` are OPTIONAL — left
// at their defaults (0.0f / no-ops), this calls the ORIGINAL 7-callback `Render` overload, so every
// pre-STEP129 caller keeps its exact code path. A nonzero width switches to the 9-callback overload.
// `bForceNineCallbackOverload`: escape hatch so a regression test can force the NEW 9-callback
// overload at headerExtraWidthPixels == 0.0f, to diff its geometry against the delegator's own.
template <typename DrawNodeHeaderExtraFunction = std::function<void(int)>,
         typename DrawLeafHeaderExtraFunction = std::function<void(const int&)>>
inline TreeListSignal<int> RunTreeSceneFrame(TreeScene& scene, ImVec2 mousePosition, bool bLeftButtonDown,
    float headerExtraWidthPixels = 0.0f,
    DrawNodeHeaderExtraFunction drawNodeHeaderExtra = [](int) {},
    DrawLeafHeaderExtraFunction drawLeafHeaderExtra = [](const int&) {},
    bool bForceNineCallbackOverload = false) {
    HeadlessMouseState mouse;
    mouse.position = mousePosition;
    mouse.bLeftButtonDown = bLeftButtonDown;
    scene.nodeBodyCallsThisFrame.clear();
    scene.leafRowDrawsThisFrame.clear();
    scene.leafBodyCallsThisFrame.clear();
    scene.previousNodeIdentifierThisFrame = -1;
    RunHeadlessFrame(mouse, kTreeSceneWindowSize, [&] {
        scene.rootDropZoneTopY = ImGui::GetCursorScreenPos().y;
        const auto idOf = [](const TestNode& node) { return node.identifier; };
        const auto parentIdOf = [](const TestNode& node) { return node.parentIdentifier; };
        const auto nameOf = [&](const TestNode& node) {
            // The previous node's header rect is only available NOW, one nameOf call later (see
            // nodeRowRectMin/Max's own comment above).
            if (scene.previousNodeIdentifierThisFrame != -1) {
                scene.nodeRowRectMin[scene.previousNodeIdentifierThisFrame] = ImGui::GetItemRectMin();
                scene.nodeRowRectMax[scene.previousNodeIdentifierThisFrame] = ImGui::GetItemRectMax();
            }
            scene.previousNodeIdentifierThisFrame = node.identifier;
            const ImVec2 rowCorner = ImGui::GetCursorScreenPos();
            scene.nodeRowTopLeft[node.identifier] = rowCorner;
            scene.rowLeftX = rowCorner.x;
            return node.name;
        };
        const auto drawNodeBody = [&](int nodeIdentifier) { scene.nodeBodyCallsThisFrame.push_back(nodeIdentifier); };
        const auto describeLeaves = [&](int nodeIdentifier) -> const std::vector<int>& {
            static const std::vector<int> kEmpty;
            const auto it = scene.leavesByNode.find(nodeIdentifier);
            return it != scene.leavesByNode.end() ? it->second : kEmpty;
        };
        const auto leafLabel = [&](const int& leaf) -> const char* {
            scene.leafRowTopLeft[leaf] = ImGui::GetCursorScreenPos();
            scene.leafRowDrawsThisFrame.push_back(leaf);
            return TreeSceneLeafLabel(leaf);
        };
        const auto drawExpandedLeafBody = [&](const int& leaf) { scene.leafBodyCallsThisFrame.push_back(leaf); };
        if (headerExtraWidthPixels > 0.0f || bForceNineCallbackOverload)
            scene.signal = TreeListWidget_UI<TestNode, int>::Render("TestTree", scene.nodes, idOf, parentIdOf,
                nameOf, drawNodeBody, describeLeaves, leafLabel, drawExpandedLeafBody, drawNodeHeaderExtra,
                drawLeafHeaderExtra, headerExtraWidthPixels, scene.treeState, scene.selectedNodeIdentifier);
        else
            scene.signal = TreeListWidget_UI<TestNode, int>::Render("TestTree", scene.nodes, idOf, parentIdOf,
                nameOf, drawNodeBody, describeLeaves, leafLabel, drawExpandedLeafBody,
                scene.treeState, scene.selectedNodeIdentifier);   // the ORIGINAL 7-callback overload, unchanged path
    });
    return scene.signal;
}

// Hover, press, release — mirrors DraggableList_TestScene_UI.h's own ClickAt.
inline TreeListSignal<int> ClickAt(TreeScene& scene, ImVec2 position) {
    RunTreeSceneFrame(scene, position, false);
    const TreeListSignal<int> pressSignal = RunTreeSceneFrame(scene, position, true);
    const TreeListSignal<int> releaseSignal = RunTreeSceneFrame(scene, position, false);
    return pressSignal.bHasSignal() ? pressSignal : releaseSignal;
}

// Press at `grabPosition`, drag past the threshold onto `dropPosition`, release — mirrors
// DraggableListWidget_UI_Test.cpp's TestSyntheticDragProducesTheExpectedOrder sequence exactly.
inline TreeListSignal<int> DragOnto(TreeScene& scene, ImVec2 grabPosition, ImVec2 dropPosition) {
    RunTreeSceneFrame(scene, grabPosition, false);
    RunTreeSceneFrame(scene, grabPosition, true);
    RunTreeSceneFrame(scene, dropPosition, true);
    TreeListSignal<int> dragSignal = RunTreeSceneFrame(scene, dropPosition, true);
    if (dragSignal.kind != TreeListSignalKind::Reparent)
        dragSignal = RunTreeSceneFrame(scene, dropPosition, false);
    return dragSignal;
}

} // namespace Ui
} // namespace SanmapGen
