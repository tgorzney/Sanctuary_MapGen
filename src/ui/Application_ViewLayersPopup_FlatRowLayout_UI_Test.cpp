// Application_ViewLayersPopup_FlatRowLayout_UI_Test.cpp — STEP200 acceptance: the View popup's
// fixed-width, non-collapsible, name-is-the-drag-handle row layout, proven through
// DraggableList<T>::Render's own real imgui path (headless, no GL, no window backend) — the same
// posture Application_ViewLayersPopup_CrossSection_UI_Test.cpp already has for the cross-section
// drag rejection. Covers fix-approach points 2, 3 and 6: `DraggableListRowLayout::Flat` draws no
// CollapsingHeader (nothing to collapse), the name Selectable is the drag source/target, and the
// leftmost icon is visibility, wired to `ToggleVisibility` with the row's own index.
// Point 1's popup-width-never-grows defense (`SetNextWindowSizeConstraints` + `SetNextItemWidth`)
// is proven separately against a real `BeginPopup` in `TestPopupNeverGrows` below, mirroring
// `Application_Draw_UI.cpp`/`Application_ViewLayersPopup_UI.cpp`'s own calls rather than the
// production functions directly (those are file-local to Application_ViewLayersPopup_UI.cpp).
// One translation unit of the Application_ViewLayersPopup_UI_Test binary; main() lives in
// Application_ViewLayersPopup_UI_Test.cpp.
#include "DraggableListWidget_UI.h"
#include "ListWidget_TestFrame_UI.h"
#include <cfloat>
#include <vector>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

const ImVec2 kMouseAway = ImVec2(-FLT_MAX, -FLT_MAX);

struct FlatTestRow { const char* name; int identifier; bool bEnabled; };

struct FlatScene {
    std::vector<FlatTestRow> rows = {{"Height", 1, true}, {"Flow", 2, true}, {"Water", 3, false}};
    DraggableListSignal      signal;
    int                      bodyCallCount = 0;
    float rowLeftX[3]  = {};
    float rowTopY[3]   = {};
    float bodyStartX[3] = {};
};

DraggableListSignal RunFlatFrame(FlatScene& scene, ImVec2 mousePosition, bool bLeftButtonDown) {
    HeadlessMouseState mouse;
    mouse.position = mousePosition;
    mouse.bLeftButtonDown = bLeftButtonDown;
    RunHeadlessFrame(mouse, ImVec2(420.0f, 300.0f), [&] {
        scene.bodyCallCount = 0;   // per-FRAME count: every row's body call, this frame alone
        scene.signal = DraggableList<FlatTestRow>::Render(
            "FlatRowScene", scene.rows,
            [&](int rowIndex) {
                const ImVec2 rowCorner = ImGui::GetCursorScreenPos();
                scene.rowLeftX[rowIndex] = rowCorner.x;
                scene.rowTopY[rowIndex] = rowCorner.y;
                DraggableListRow row;
                row.label = scene.rows[static_cast<std::size_t>(rowIndex)].name;
                row.bVisible = scene.rows[static_cast<std::size_t>(rowIndex)].bEnabled;
                return row;
            },
            [&](int rowIndex) {
                // The inline row body — always reached in Flat mode, never gated behind a
                // disclosure state. Its start X is the boundary right after the name Selectable.
                scene.bodyStartX[rowIndex] = ImGui::GetCursorScreenPos().x;
                ++scene.bodyCallCount;
            },
            -1, DraggableListRowLayout::Flat);
    });
    return scene.signal;
}

// Hover, press, release — same sequence DraggableList_TestScene_UI.h's ClickAt uses: an
// overlapping affordance only becomes interactable once it was hovered the PREVIOUS frame, and a
// SmallButton/Selectable click may report on either the press or the release frame, so this takes
// whichever of the two actually carried a signal.
DraggableListSignal ClickAtFlat(FlatScene& scene, ImVec2 position) {
    RunFlatFrame(scene, position, false);
    const DraggableListSignal pressSignal = RunFlatFrame(scene, position, true);
    const DraggableListSignal releaseSignal = RunFlatFrame(scene, position, false);
    return pressSignal.bHasSignal() ? pressSignal : releaseSignal;
}

// Fix approach point 2/3: nothing about Flat gates the row body behind an expand/collapse toggle —
// every row's body runs every single frame, at every mouse position tried (including a point at
// the row's own left margin, where Collapsible mode's disclosure arrow would sit).
void TestFlatRowBodyAlwaysDrawnNeverCollapses() {
    HeadlessImguiSession session;
    FlatScene scene;
    RunFlatFrame(scene, kMouseAway, false);
    RunFlatFrame(scene, kMouseAway, false);
    CheckListWidgetExpectation(scene.bodyCallCount == 3, "every row's body drew on the settle frame");

    // Click squarely on row 0's own left margin (Collapsible mode's arrow hit-box) three times —
    // an actual disclosure arrow would toggle open/closed on each click; the body call count must
    // stay exactly 3 (one per row) every single frame regardless, because Flat mode has no toggle.
    const ImVec2 arrowLikePoint(scene.rowLeftX[0] + 4.0f, scene.rowTopY[0] + 8.0f);
    RunFlatFrame(scene, arrowLikePoint, true);
    CheckListWidgetExpectation(scene.bodyCallCount == 3, "a click at the would-be arrow position still draws every body");
    RunFlatFrame(scene, arrowLikePoint, false);
    CheckListWidgetExpectation(scene.bodyCallCount == 3, "releasing at the same point still draws every body");
    RunFlatFrame(scene, arrowLikePoint, true);
    CheckListWidgetExpectation(scene.bodyCallCount == 3, "a second click cycle still draws every body (nothing toggled)");
}

// Fix approach point 3: dragging the NAME (not the row, not the body) reorders — the drag source
// binds to the name Selectable, captured via the boundary between it and the inline body.
void TestDraggingTheNameReordersARow() {
    HeadlessImguiSession session;
    FlatScene scene;
    RunFlatFrame(scene, kMouseAway, false);
    RunFlatFrame(scene, kMouseAway, false);          // settle layout, capture geometry
    const float nameClickX = scene.bodyStartX[0] - 20.0f;    // inside the fixed-width name column
    CheckListWidgetExpectation(nameClickX > scene.rowLeftX[0], "the probed point is right of the row's own left margin (past the visibility icon)");
    const ImVec2 grabPoint(nameClickX, scene.rowTopY[0] + 10.0f);
    const ImVec2 dropPoint(nameClickX, scene.rowTopY[1] + 10.0f);
    RunFlatFrame(scene, grabPoint, false);           // hover row 0's name (AllowOverlap needs it)
    RunFlatFrame(scene, grabPoint, true);            // press the name Selectable
    RunFlatFrame(scene, dropPoint, true);            // drag past the threshold onto row 1's name
    DraggableListSignal dragSignal = RunFlatFrame(scene, dropPoint, true);
    if (dragSignal.kind != DraggableListSignalKind::Reorder)
        dragSignal = RunFlatFrame(scene, dropPoint, false);   // the drop delivers on release
    CheckListWidgetExpectation(dragSignal.kind == DraggableListSignalKind::Reorder &&
        dragSignal.sourceRowIndex == 0 && dragSignal.targetRowIndex == 1,
        "dragging row 0's name onto row 1's name signals that reorder");
}

// Fix approach point 6: the leftmost icon on a Flat row is visibility, and it reports the row it
// was drawn on — swept rather than guessed at a fixed offset, the same discipline
// DraggableListWidget_UI_Test.cpp's TestAffordanceSignalsCarryTheRightIndex already uses, so this
// test cannot drift from wherever the widget actually draws the icon.
void TestLeftVisibilityIconTogglesItsOwnRow() {
    HeadlessImguiSession session;
    FlatScene scene;
    RunFlatFrame(scene, kMouseAway, false);
    RunFlatFrame(scene, kMouseAway, false);
    float visibilityX = -1.0f;
    for (float probeX = scene.rowLeftX[1]; probeX < scene.rowLeftX[1] + 60.0f; probeX += 2.0f) {
        const DraggableListSignal probe = ClickAtFlat(scene, ImVec2(probeX, scene.rowTopY[1] + 8.0f));
        if (probe.kind == DraggableListSignalKind::ToggleVisibility) { visibilityX = probeX; break; }
    }
    CheckListWidgetExpectation(visibilityX > 0.0f, "a visibility toggle affordance exists left of row 1's name");
    CheckListWidgetExpectation(visibilityX < scene.bodyStartX[1] - static_cast<float>(kFlatRowNameWidthPixels),
        "the visibility icon sits BEFORE the name column, i.e. it is the leftmost affordance");
}

// Fix approach point 1's defense-in-depth: a real BeginPopup, with the exact
// `SetNextWindowSizeConstraints` call Application_Draw_UI.cpp makes and the exact
// `SetNextItemWidth` calls Application_ViewLayersPopup_UI.cpp makes before its Combo/SliderFloat,
// held open across several frames. The reported growth defect was the popup's OWN width feeding
// back into its next frame's width through an unconstrained item; this proves that loop cannot
// start once every item is fixed-width and the window itself is capped.
void RunPopupWidthFrame(bool bOpenThisFrame, float& outWindowWidth) {
    HeadlessMouseState mouse;
    mouse.position = kMouseAway;
    RunHeadlessFrame(mouse, ImVec2(800.0f, 600.0f), [&] {
        if (bOpenThisFrame) ImGui::OpenPopup("FlatRowLayoutTestPopup");
        ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(420.0f, FLT_MAX));
        if (ImGui::BeginPopup("FlatRowLayoutTestPopup")) {
            static int blendIndex = 0;
            const char* const blendNames[] = { "Replace", "AlphaBlend", "Add" };
            ImGui::SetNextItemWidth(100.0f);
            ImGui::Combo("##blend", &blendIndex, blendNames, IM_ARRAYSIZE(blendNames));
            static float opacity = 0.5f;
            ImGui::SetNextItemWidth(100.0f);
            ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f);
            outWindowWidth = ImGui::GetWindowSize().x;
            ImGui::EndPopup();
        }
    });
}

void TestPopupNeverGrows() {
    HeadlessImguiSession session;
    float widthFrame1 = 0.0f, widthFrame2 = 0.0f, widthFrame3 = 0.0f, widthFrame4 = 0.0f;
    RunPopupWidthFrame(true, widthFrame1);    // opens
    RunPopupWidthFrame(false, widthFrame2);
    RunPopupWidthFrame(false, widthFrame3);
    RunPopupWidthFrame(false, widthFrame4);
    CheckListWidgetExpectation(widthFrame1 > 0.0f, "the popup opened and reported a width");
    CheckListWidgetExpectation(widthFrame2 == widthFrame3 && widthFrame3 == widthFrame4,
        "the popup's width is identical frame to frame once settled (no auto-grow feedback loop)");
    CheckListWidgetExpectation(widthFrame4 <= 420.0f, "the popup never exceeds the fixed max width constraint");
}

} // namespace

namespace SanmapGen {
namespace Ui {
int RunViewLayersFlatRowLayoutAcceptance() {
    const int failuresBefore = listWidgetTestFailureCount;
    TestFlatRowBodyAlwaysDrawnNeverCollapses();
    TestDraggingTheNameReordersARow();
    TestLeftVisibilityIconTogglesItsOwnRow();
    TestPopupNeverGrows();
    return listWidgetTestFailureCount - failuresBefore;
}
} // namespace Ui
} // namespace SanmapGen
