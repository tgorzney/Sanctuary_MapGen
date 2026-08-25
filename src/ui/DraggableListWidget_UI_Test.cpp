// DraggableListWidget_UI_Test.cpp — M5-2 acceptance for DraggableList<T>: a synthetic drag produces
// the expected new order, and delete / visibility / lock signals fire with the right row index.
// Driven through the REAL imgui interaction path (headless), not by hand-built signals: the pointer
// is placed on the widget's own affordances and the emitted signal is read back.
// The scene and pointer helpers are in DraggableList_TestScene_UI.h; main() is in
// VirtualListWidget_UI_Test.cpp.
#include "DraggableList_TestScene_UI.h"
#include <vector>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

// The structural half, with no imgui at all: what the caller does with a signal.
void TestApplySignalMovesAndDeletes() {
    std::vector<int> items = {10, 20, 30, 40};
    DraggableListSignal signal;
    signal.kind = DraggableListSignalKind::Reorder;
    signal.sourceRowIndex = 0;
    signal.targetRowIndex = 2;
    CheckListWidgetExpectation(ApplyDraggableListSignal(items, signal), "downward reorder applies");
    CheckListWidgetExpectation(items[0] == 20 && items[1] == 30 && items[2] == 10 && items[3] == 40,
                               "dragged element lands ON the target index (downward)");
    signal.sourceRowIndex = 3;
    signal.targetRowIndex = 1;
    ApplyDraggableListSignal(items, signal);
    CheckListWidgetExpectation(items[0] == 20 && items[1] == 40 && items[2] == 30 && items[3] == 10,
                               "dragged element lands ON the target index (upward)");
    signal.kind = DraggableListSignalKind::Delete;
    signal.sourceRowIndex = 0;
    ApplyDraggableListSignal(items, signal);
    CheckListWidgetExpectation(items.size() == 3u && items[0] == 40, "delete removes its own row");
    signal.kind = DraggableListSignalKind::ToggleLock;
    CheckListWidgetExpectation(!ApplyDraggableListSignal(items, signal), "non-structural kinds no-op");
    signal.kind = DraggableListSignalKind::Delete;
    signal.sourceRowIndex = 99;
    CheckListWidgetExpectation(!ApplyDraggableListSignal(items, signal) && items.size() == 3u,
                               "out-of-range index is rejected");
}

// Click the widget's own affordances and read back what it reported. The columns are FOUND by
// sweeping the strip rather than recomputed from style constants, so the test cannot drift from
// the widget's layout.
void TestAffordanceSignalsCarryTheRightIndex() {
    HeadlessImguiSession session;
    DraggableScene scene = MakeDraggableScene();
    RunSceneFrame(scene, kMouseAway, false);
    RunSceneFrame(scene, kMouseAway, false);                   // settle layout, capture geometry
    float visibilityX = -1.0f, lockX = -1.0f, deleteX = -1.0f;
    for (float probeX = kSceneWindowSize.x - 110.0f; probeX < kSceneWindowSize.x - 2.0f; probeX += 2.0f) {
        const DraggableListSignal probe = ClickAt(scene, ImVec2(probeX, RowCenterY(scene, 0)));
        if (probe.sourceRowIndex != 0) continue;
        if (visibilityX < 0.0f && probe.kind == DraggableListSignalKind::ToggleVisibility) visibilityX = probeX;
        if (lockX < 0.0f && probe.kind == DraggableListSignalKind::ToggleLock) lockX = probeX;
        if (deleteX < 0.0f && probe.kind == DraggableListSignalKind::Delete) deleteX = probeX;
    }
    CheckListWidgetExpectation(visibilityX > 0.0f && lockX > visibilityX && deleteX > lockX,
                               "visibility, lock and delete affordances exist, left to right");
    const float rowTwoCenterY = RowCenterY(scene, 2);
    CheckListWidgetExpectation(SignalIs(ClickAt(scene, ImVec2(visibilityX, rowTwoCenterY)),
                                        DraggableListSignalKind::ToggleVisibility, 2),
                               "visibility signal carries row 2");
    CheckListWidgetExpectation(SignalIs(ClickAt(scene, ImVec2(lockX, rowTwoCenterY)),
                                        DraggableListSignalKind::ToggleLock, 2),
                               "lock signal carries row 2");
    CheckListWidgetExpectation(SignalIs(ClickAt(scene, ImVec2(deleteX, rowTwoCenterY)),
                                        DraggableListSignalKind::Delete, 2),
                               "delete signal carries row 2");
    CheckListWidgetExpectation(SignalIs(ClickAt(scene, ImVec2(scene.rowLeftX + 60.0f,
                                                              RowCenterY(scene, 1))),
                                        DraggableListSignalKind::Select, 1),
                               "label click selects row 1");
    CheckListWidgetExpectation(OrderIs(scene, 1, 2, 3, 4),
                               "the widget never mutated the caller's list");
}

// Press row 0, drag onto row 2, release: imgui's own drag-drop payload path end to end.
void TestSyntheticDragProducesTheExpectedOrder() {
    HeadlessImguiSession session;
    DraggableScene scene = MakeDraggableScene();
    RunSceneFrame(scene, kMouseAway, false);
    RunSceneFrame(scene, kMouseAway, false);
    const float labelX = scene.rowLeftX + 60.0f;
    const ImVec2 grabPoint(labelX, RowCenterY(scene, 0));
    const ImVec2 dropPoint(labelX, RowCenterY(scene, 2));
    RunSceneFrame(scene, grabPoint, false);                    // hover row 0 (AllowOverlap needs it)
    RunSceneFrame(scene, grabPoint, true);                     // press the row 0 header
    RunSceneFrame(scene, dropPoint, true);                     // drag past the threshold onto row 2
    DraggableListSignal dragSignal = RunSceneFrame(scene, dropPoint, true);
    if (dragSignal.kind != DraggableListSignalKind::Reorder)
        dragSignal = RunSceneFrame(scene, dropPoint, false);   // the drop delivers on release
    CheckListWidgetExpectation(dragSignal.kind == DraggableListSignalKind::Reorder &&
                               dragSignal.sourceRowIndex == 0 && dragSignal.targetRowIndex == 2,
                               "dragging row 0 onto row 2 signals that reorder");
    ApplyDraggableListSignal(scene.layers, dragSignal);
    CheckListWidgetExpectation(OrderIs(scene, 2, 3, 1, 4), "reorder produces the expected new order");
    std::printf("DraggableList: drag 0->2 gives order %d %d %d %d\n", scene.layers[0].identifier,
                scene.layers[1].identifier, scene.layers[2].identifier, scene.layers[3].identifier);
}

// STEP150: the OPTIONAL per-row extra button, generically. Row 0 opts in; every other row leaves
// `extraButtonLabel` null, same as this scene's rows always have — proving the strip and the
// header-click arbitration are UNCHANGED for a row/consumer that never populates the field, while
// the row that DOES gets a real, clickable fourth affordance right of `X##delete`.
void TestOptionalExtraButtonIsGenericAndRowScoped() {
    HeadlessImguiSession session;
    DraggableScene scene = MakeDraggableScene();
    scene.layers[0].extraButtonLabel = "Bake##testExtra";
    RunSceneFrame(scene, kMouseAway, false);
    RunSceneFrame(scene, kMouseAway, false);                   // settle layout, capture geometry

    // Row 1 never set extraButtonLabel -- its own X##delete must sit at the SAME offset from the
    // right edge the base (no-extra-button) test above found, proving row 1's strip is untouched.
    float row1DeleteX = -1.0f;
    for (float probeX = kSceneWindowSize.x - 110.0f; probeX < kSceneWindowSize.x - 2.0f; probeX += 2.0f) {
        const DraggableListSignal probe = ClickAt(scene, ImVec2(probeX, RowCenterY(scene, 1)));
        if (probe.kind == DraggableListSignalKind::Delete && probe.sourceRowIndex == 1) {
            row1DeleteX = probeX;
            break;
        }
    }
    CheckListWidgetExpectation(row1DeleteX > 0.0f,
                               "a row with no extra button keeps its ordinary delete affordance");

    // Row 0 DID set extraButtonLabel -- sweep further right of ITS delete button to find the new
    // fourth affordance and confirm it reports ExtraButton for row 0.
    float row0ExtraX = -1.0f;
    for (float probeX = kSceneWindowSize.x - 170.0f; probeX < kSceneWindowSize.x - 2.0f; probeX += 2.0f) {
        const DraggableListSignal probe = ClickAt(scene, ImVec2(probeX, RowCenterY(scene, 0)));
        if (probe.kind == DraggableListSignalKind::ExtraButton && probe.sourceRowIndex == 0) {
            row0ExtraX = probeX;
            break;
        }
    }
    CheckListWidgetExpectation(row0ExtraX > 0.0f,
                               "the opted-in row's extra button fires ExtraButton with its own row index");

    // Row 0's ordinary delete affordance must still exist too -- the extra button ADDS a fourth
    // slot, it does not replace the third.
    bool bRow0StillHasDelete = false;
    for (float probeX = kSceneWindowSize.x - 170.0f; probeX < kSceneWindowSize.x - 2.0f; probeX += 2.0f) {
        const DraggableListSignal probe = ClickAt(scene, ImVec2(probeX, RowCenterY(scene, 0)));
        if (probe.kind == DraggableListSignalKind::Delete && probe.sourceRowIndex == 0) {
            bRow0StillHasDelete = true;
            break;
        }
    }
    CheckListWidgetExpectation(bRow0StillHasDelete,
                               "and the opted-in row keeps its own visibility/lock/delete trio too");
    CheckListWidgetExpectation(OrderIs(scene, 1, 2, 3, 4),
                               "none of the probing above mutated the caller's list");
}

} // namespace

namespace SanmapGen {
namespace Ui {
void RunDraggableListAcceptance() {
    TestApplySignalMovesAndDeletes();
    TestAffordanceSignalsCarryTheRightIndex();
    TestSyntheticDragProducesTheExpectedOrder();
    TestOptionalExtraButtonIsGenericAndRowScoped();
}
} // namespace Ui
} // namespace SanmapGen
