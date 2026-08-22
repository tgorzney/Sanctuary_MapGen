// Application_ViewLayersPopup_CrossSection_UI_Test.cpp — STEP54 acceptance, part 2: the
// "ViewListField"/"ViewListOverlay" payload-identifier cross-section rejection, proven through
// DraggableList<T>::Render's own real drag-drop path (headless, no GL, no window backend) — not by
// handing a synthetic DraggableListSignal to ApplyViewLayerSignal, because a DraggableListSignal
// carries no payload identifier at all: the rejection happens one layer down, inside Render's own
// ImGui::AcceptDragDropPayload call (DraggableListWidget_UI.h:122-138), before a signal ever exists.
// One translation unit of the Application_ViewLayersPopup_UI_Test binary; main() lives in
// Application_ViewLayersPopup_UI_Test.cpp.
#include "DraggableListWidget_UI.h"
#include "ListWidget_TestFrame_UI.h"
#include <vector>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

const ImVec2 kMouseAway = ImVec2(-FLT_MAX, -FLT_MAX);

struct TestRow { const char* name; int identifier; bool bEnabled; };

// Two independent lists in the same frame, exactly like DrawTerrainSection/DrawOverlaySection do —
// the two real production payload identifiers, "ViewListField" and "ViewListOverlay", are used
// verbatim so this test exercises the exact strings the popup ships (both well under
// DraggableListWidget_UI.h's 32-character fallback-collapse threshold: 13 and 15 characters).
struct TwoSectionScene {
    std::vector<TestRow> fieldRows   = {{"Height", 1, true}, {"Flow", 2, true}};
    std::vector<TestRow> overlayRows = {{"Alloy", 10, true}, {"Units", 20, true}};
    DraggableListSignal  fieldSignal;
    DraggableListSignal  overlaySignal;
    float fieldRowTopY[2]   = {};
    float overlayRowTopY[2] = {};
    float rowLeftX          = 0.0f;
};

DraggableListSignal RunTwoSectionFrame(TwoSectionScene& scene, ImVec2 mousePosition, bool bLeftButtonDown) {
    HeadlessMouseState mouse;
    mouse.position = mousePosition;
    mouse.bLeftButtonDown = bLeftButtonDown;
    RunHeadlessFrame(mouse, ImVec2(420.0f, 300.0f), [&] {
        scene.fieldSignal = DraggableList<TestRow>::Render("ViewListField", scene.fieldRows,
            [&](int rowIndex) {
                const ImVec2 rowCorner = ImGui::GetCursorScreenPos();
                scene.fieldRowTopY[rowIndex] = rowCorner.y;
                scene.rowLeftX = rowCorner.x;
                DraggableListRow row;
                row.label = scene.fieldRows[static_cast<std::size_t>(rowIndex)].name;
                row.bVisible = scene.fieldRows[static_cast<std::size_t>(rowIndex)].bEnabled;
                return row;
            }, [](int) {});
        scene.overlaySignal = DraggableList<TestRow>::Render("ViewListOverlay", scene.overlayRows,
            [&](int rowIndex) {
                const ImVec2 rowCorner = ImGui::GetCursorScreenPos();
                scene.overlayRowTopY[rowIndex] = rowCorner.y;
                DraggableListRow row;
                row.label = scene.overlayRows[static_cast<std::size_t>(rowIndex)].name;
                row.bVisible = scene.overlayRows[static_cast<std::size_t>(rowIndex)].bEnabled;
                return row;
            }, [](int) {});
    });
    return scene.fieldSignal.bHasSignal() ? scene.fieldSignal : scene.overlaySignal;
}

// Press row 0 of the field section, drag onto row 0 of the overlay section, release: the payload
// type mismatch means ImGui::AcceptDragDropPayload never returns non-null for that drop, so neither
// section's Render ever emits a Reorder signal for it.
void TestCrossSectionDragIsRejected() {
    HeadlessImguiSession session;
    TwoSectionScene scene;
    RunTwoSectionFrame(scene, kMouseAway, false);
    RunTwoSectionFrame(scene, kMouseAway, false);          // settle layout, capture geometry
    const ImVec2 grabPoint(scene.rowLeftX + 60.0f, scene.fieldRowTopY[0] + 10.0f);
    const ImVec2 dropPoint(scene.rowLeftX + 60.0f, scene.overlayRowTopY[0] + 10.0f);
    RunTwoSectionFrame(scene, grabPoint, false);           // hover row 0 (AllowOverlap needs it)
    RunTwoSectionFrame(scene, grabPoint, true);            // press the field row 0 header
    RunTwoSectionFrame(scene, dropPoint, true);             // drag past the threshold onto the overlay row
    DraggableListSignal dragSignal = RunTwoSectionFrame(scene, dropPoint, true);
    if (dragSignal.kind != DraggableListSignalKind::Reorder)
        dragSignal = RunTwoSectionFrame(scene, dropPoint, false);   // the drop would deliver on release
    CheckListWidgetExpectation(dragSignal.kind != DraggableListSignalKind::Reorder,
        "a drag started under \"ViewListField\" dropped onto \"ViewListOverlay\" never signals Reorder");
    CheckListWidgetExpectation(
        scene.fieldRows[0].identifier == 1 && scene.overlayRows[0].identifier == 10,
        "row order in both sections is unchanged after the rejected cross-section drop");
}

// Positive control: a within-section drag still reorders normally, so the rejection above is a
// real cross-section effect and not a widget that has stopped signaling reorders at all.
void TestWithinSectionDragStillReorders() {
    HeadlessImguiSession session;
    TwoSectionScene scene;
    RunTwoSectionFrame(scene, kMouseAway, false);
    RunTwoSectionFrame(scene, kMouseAway, false);
    const ImVec2 grabPoint(scene.rowLeftX + 60.0f, scene.fieldRowTopY[0] + 10.0f);
    const ImVec2 dropPoint(scene.rowLeftX + 60.0f, scene.fieldRowTopY[1] + 10.0f);
    RunTwoSectionFrame(scene, grabPoint, false);
    RunTwoSectionFrame(scene, grabPoint, true);
    RunTwoSectionFrame(scene, dropPoint, true);
    DraggableListSignal dragSignal = RunTwoSectionFrame(scene, dropPoint, true);
    if (dragSignal.kind != DraggableListSignalKind::Reorder)
        dragSignal = RunTwoSectionFrame(scene, dropPoint, false);
    CheckListWidgetExpectation(dragSignal.kind == DraggableListSignalKind::Reorder &&
        dragSignal.sourceRowIndex == 0 && dragSignal.targetRowIndex == 1,
        "a drag within \"ViewListField\" alone still signals Reorder");
}

} // namespace

namespace SanmapGen {
namespace Ui {
int RunViewLayersCrossSectionAcceptance() {
    const int failuresBefore = listWidgetTestFailureCount;
    TestCrossSectionDragIsRejected();
    TestWithinSectionDragStillReorders();
    return listWidgetTestFailureCount - failuresBefore;
}
} // namespace Ui
} // namespace SanmapGen
