// DraggableListWidget_UI_Test.cpp — M5-2 acceptance for DraggableList<T>: a synthetic drag produces
// the expected new order, and delete / visibility / lock signals fire with the right row index.
// Driven through the REAL imgui interaction path (headless), not by hand-built signals: the pointer
// is placed on the widget's own affordances and the emitted signal is read back.
// The scene and pointer helpers are in DraggableList_TestScene_UI.h; main() is in
// VirtualListWidget_UI_Test.cpp.
#include "DraggableList_TestScene_UI.h"
#include <cmath>
#include <functional>
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

// Hover, press, release with the header-extra callback/width threaded through every frame — the
// header-extra analogue of DraggableList_TestScene_UI.h's own ClickAt, which is fixed to the
// zero-reservation 2-callback path.
DraggableListSignal ClickAtWithHeaderExtra(DraggableScene& scene, ImVec2 position,
                                           float headerExtraWidthPixels,
                                           const std::function<void(int)>& drawRowHeaderExtra) {
    RunSceneFrame(scene, position, false, headerExtraWidthPixels, drawRowHeaderExtra);
    const DraggableListSignal pressSignal =
        RunSceneFrame(scene, position, true, headerExtraWidthPixels, drawRowHeaderExtra);
    const DraggableListSignal releaseSignal =
        RunSceneFrame(scene, position, false, headerExtraWidthPixels, drawRowHeaderExtra);
    return pressSignal.bHasSignal() ? pressSignal : releaseSignal;
}

bool OffsetsMatchWithinSweepStep(float a, float b) { return std::fabs(a - b) <= 2.0f; }

// STEP123: the OPTIONAL per-row header-extra slot, generically (mirrors
// TestOptionalExtraButtonIsGenericAndRowScoped's own precedent). A scene that opts into a nonzero
// `headerExtraWidthPixels` gets a probe button drawn to the LEFT of the ordinary [o]/[U]/X strip; a
// scene that never opts in (headerExtraWidthPixels == 0.0f, the pre-STEP123 2-callback overload)
// must keep its strip at the exact same offset it always had — proof the additive overload changes
// nothing for a consumer that doesn't opt in (Fix §1's "why this shape" reasoning).
void TestOptionalHeaderExtraContentIsGenericAndRowScoped() {
    HeadlessImguiSession session;
    constexpr float kTestHeaderExtraWidthPixels = 60.0f;

    // (c) Baseline: a scene that never opts in. The SAME sweep TestAffordanceSignalsCarryTheRightIndex
    // already uses against the SAME window/style, so any drift here is drift in the widget itself --
    // this reproduces that test's visibilityX/lockX/deleteX byte-for-byte.
    DraggableScene baselineScene = MakeDraggableScene();
    RunSceneFrame(baselineScene, kMouseAway, false);
    RunSceneFrame(baselineScene, kMouseAway, false);
    float baselineVisibilityX = -1.0f, baselineLockX = -1.0f, baselineDeleteX = -1.0f;
    for (float probeX = kSceneWindowSize.x - 110.0f; probeX < kSceneWindowSize.x - 2.0f; probeX += 2.0f) {
        const DraggableListSignal probe = ClickAt(baselineScene, ImVec2(probeX, RowCenterY(baselineScene, 0)));
        if (probe.sourceRowIndex != 0) continue;
        if (baselineVisibilityX < 0.0f && probe.kind == DraggableListSignalKind::ToggleVisibility) baselineVisibilityX = probeX;
        if (baselineLockX < 0.0f && probe.kind == DraggableListSignalKind::ToggleLock) baselineLockX = probeX;
        if (baselineDeleteX < 0.0f && probe.kind == DraggableListSignalKind::Delete) baselineDeleteX = probeX;
    }
    CheckListWidgetExpectation(baselineVisibilityX > 0.0f && baselineLockX > baselineVisibilityX &&
                               baselineDeleteX > baselineLockX,
                               "a scene that never opts in keeps the ordinary strip order (baseline)");

    // The opted-in scene: a probe button drawn via the new header-extra callback, capturing its own
    // row index in a scene-local out-param -- ExtraButton's DraggableListSignalKind is unrelated, a
    // raw captured int is sufficient, no new signal kind needed.
    DraggableScene headerScene = MakeDraggableScene();
    int probeRowIndex = -1;
    const std::function<void(int)> drawProbe = [&](int rowIndex) {
        if (ImGui::SmallButton("##probe")) probeRowIndex = rowIndex;
    };
    RunSceneFrame(headerScene, kMouseAway, false, kTestHeaderExtraWidthPixels, drawProbe);
    RunSceneFrame(headerScene, kMouseAway, false, kTestHeaderExtraWidthPixels, drawProbe);

    // (a) Sweeping X in the reserved band left of the strip finds a clickable probe reporting the
    // correct row index for row 0, and the SAME X column on row 2 reports row 2's own index.
    float probeX0 = -1.0f;
    for (float probeX = kSceneWindowSize.x - 220.0f; probeX < kSceneWindowSize.x - 2.0f; probeX += 2.0f) {
        probeRowIndex = -1;
        ClickAtWithHeaderExtra(headerScene, ImVec2(probeX, RowCenterY(headerScene, 0)),
                               kTestHeaderExtraWidthPixels, drawProbe);
        if (probeRowIndex == 0) { probeX0 = probeX; break; }
    }
    CheckListWidgetExpectation(probeX0 > 0.0f,
                               "sweeping the reserved band finds a clickable header-extra probe for row 0");

    probeRowIndex = -1;
    ClickAtWithHeaderExtra(headerScene, ImVec2(probeX0, RowCenterY(headerScene, 2)),
                           kTestHeaderExtraWidthPixels, drawProbe);
    CheckListWidgetExpectation(probeRowIndex == 2,
                               "the same X column on row 2 reports row 2's own index -- the slot is row-scoped");

    // (b) STEP127 items 8/9 fix: the visibility/lock/delete strip's own X-offsets, found by the SAME
    // sweep technique, now land at the SAME row-relative position as the zero-reservation baseline --
    // the strip right-aligns against the row's TRUE right edge regardless of headerExtraWidthPixels,
    // with the header-extra control occupying its own reserved band strictly to the strip's LEFT.
    // (Before the fix this strip shifted left by exactly headerExtraWidthPixels, landing it ON TOP of
    // the header-extra control's own band instead of beside it -- that was items 8/9's bug.)
    //
    // STEP127 investigation: this CLICK-sweep technique has a pre-existing, unrelated ~8px hover-
    // resolution slop specific to the FIRST strip widget (the visibility icon) when something else was
    // drawn on the same line before it -- ImGuiTreeNodeFlags_AllowOverlap defers hover priority near a
    // widget's own left edge, and an extra preceding widget on the row measurably narrows that dead
    // zone. Proven NOT a real positional shift: ImGui::GetContentRegionAvail().x at row 0 is BIT-
    // IDENTICAL between this scene and the baseline scene above (both feed the SAME rowAvailWidthPixels
    // into the SAME DrawRowAffordances SameLine() offset, so the strip's real screen X cannot differ),
    // and TestHeaderExtraAffordanceStripGeometryDoesNotOverlapOrGap below reads the widgets' own real
    // ImGui item rects (not a click sweep) to confirm the exact, unshifted position directly. lockX/
    // deleteX (each one widget further from the header, unaffected by this edge case) already match
    // the baseline EXACTLY with no slop at all, which is itself confirmation this is a boundary
    // artifact of probing right next to the header, not a widened/shifted strip.
    float headerVisibilityX = -1.0f, headerLockX = -1.0f, headerDeleteX = -1.0f;
    for (float probeX = kSceneWindowSize.x - 110.0f; probeX < kSceneWindowSize.x - 2.0f; probeX += 2.0f) {
        const DraggableListSignal probe = ClickAtWithHeaderExtra(headerScene,
            ImVec2(probeX, RowCenterY(headerScene, 0)), kTestHeaderExtraWidthPixels, drawProbe);
        if (probe.sourceRowIndex != 0) continue;
        if (headerVisibilityX < 0.0f && probe.kind == DraggableListSignalKind::ToggleVisibility) headerVisibilityX = probeX;
        if (headerLockX < 0.0f && probe.kind == DraggableListSignalKind::ToggleLock) headerLockX = probeX;
        if (headerDeleteX < 0.0f && probe.kind == DraggableListSignalKind::Delete) headerDeleteX = probeX;
    }
    CheckListWidgetExpectation(headerVisibilityX > 0.0f && headerLockX > headerVisibilityX &&
                               headerDeleteX > headerLockX,
                               "the opted-in scene keeps the same strip order, header slot or not");
    constexpr float kAllowOverlapBoundarySlopPixels = 10.0f;   // covers the ~8px hover-resolution
        // artifact documented above; still far smaller than kTestHeaderExtraWidthPixels (60px), the
        // shift the pre-fix bug actually produced, so this cannot mask a regression back to it.
    CheckListWidgetExpectation(
        std::fabs(headerVisibilityX - baselineVisibilityX) < kAllowOverlapBoundarySlopPixels &&
        OffsetsMatchWithinSweepStep(headerLockX, baselineLockX) &&
        OffsetsMatchWithinSweepStep(headerDeleteX, baselineDeleteX),
        "STEP127: the strip stays at the row's true right edge, unshifted, once headerExtraWidthPixels "
        "reserves its own band strictly to the left of it");
}

// STEP127 items 8/9 — synthetic-frame geometry regression, mirroring MarkersTab_ManualLayerColor
// OverrideHeader_UI_Test.cpp's own HeadlessImguiSession/RunHeadlessFrame harness (real ImGui item
// rects) rather than this file's own sweep-probe DraggableList_TestScene_UI.h harness — proves
// (a) the header-extra control's own item rect and the affordance strip's own first item no longer
// overlap on X, and (b) the strip's own rightmost item lands within a small epsilon of the row's
// TRUE right edge, not headerExtraWidthPixels short of it. Both the 2-callback (headerExtraWidthPixels
// == 0) and 3-callback paths run: the 2-callback run is the regression check for the 19+ existing
// DraggableList<T>::Render call sites that never opt into a header-extra control at all.
struct RowGeometryFrame {
    float  rowAvailWidthPixels = 0.0f;
    float  contentRegionMaxX   = 0.0f;   // the row's own TRUE right edge (GetContentRegionAvail's own
                                          // definition: rowOrigin.x + rowAvailWidthPixels, exact)
    float  windowLeftEdgeX     = 0.0f;   // window->Pos.x - ScrollX -- the SAME reference
                                          // ImGui::SameLine(offset_from_start_x) resolves against
                                          // (imgui.cpp's own SameLine, verified against source)
    bool   bHeaderExtraDrawn   = false;
    ImVec2 headerExtraMin, headerExtraMax;
    ImVec2 stripRightmostMin, stripRightmostMax;
};

// One row is enough for geometry; two settle frames match this file's own established convention
// (imgui's first frame is a layout-settling frame, see VirtualListWidget_UI_Test.cpp's own comment).
RowGeometryFrame RunRowGeometryFrame(float headerExtraWidthPixels) {
    RowGeometryFrame result;
    std::vector<TestLayer> layers = {{"Alpha", 1, true, false}};
    for (int settleFrame = 0; settleFrame < 2; ++settleFrame) {
        RunHeadlessFrame(HeadlessMouseState(), kSceneWindowSize, [&] {
            const auto describeRow = [&](int rowIndex) {
                const ImVec2 rowOrigin = ImGui::GetCursorScreenPos();
                result.rowAvailWidthPixels = ImGui::GetContentRegionAvail().x;
                result.contentRegionMaxX   = rowOrigin.x + result.rowAvailWidthPixels;
                result.windowLeftEdgeX     = ImGui::GetWindowPos().x - ImGui::GetScrollX();
                DraggableListRow row;
                row.label    = layers[rowIndex].name;
                row.bVisible = layers[rowIndex].bVisible;
                row.bLocked  = layers[rowIndex].bLocked;
                return row;
            };
            // Collapsible layout, DefaultOpen: the strip finishes drawing immediately before this
            // callback runs, so the LAST item submitted is the strip's own rightmost affordance
            // (X##delete -- this row never sets extraButtonLabel, so there is no fourth item).
            const auto drawRowBody = [&](int) {
                result.stripRightmostMin = ImGui::GetItemRectMin();
                result.stripRightmostMax = ImGui::GetItemRectMax();
            };
            const auto drawRowHeaderExtra = [&](int) {
                ImGui::SmallButton("##geometryProbe");
                result.bHeaderExtraDrawn = true;
                result.headerExtraMin = ImGui::GetItemRectMin();
                result.headerExtraMax = ImGui::GetItemRectMax();
            };
            if (headerExtraWidthPixels > 0.0f)
                DraggableList<TestLayer>::Render("GeometryRow", layers, describeRow, drawRowBody,
                                                 drawRowHeaderExtra, headerExtraWidthPixels);
            else
                DraggableList<TestLayer>::Render("GeometryRow", layers, describeRow, drawRowBody);
        });
    }
    return result;
}

void TestHeaderExtraAffordanceStripGeometryDoesNotOverlapOrGap() {
    HeadlessImguiSession session;
    constexpr float kTestHeaderExtraWidthPixels = 60.0f;
    // Covers WindowPadding.x plus SmallButton width-calibration slop against the named
    // kAffordanceStripWidthPixels budget; far smaller than the 60px gap items 8/9's bug produced,
    // so it cannot mask a regression back to the old behavior.
    constexpr float kEpsilonPixels = 15.0f;

    // 3-callback path: a header-extra control IS reserved.
    const RowGeometryFrame withHeaderExtra = RunRowGeometryFrame(kTestHeaderExtraWidthPixels);
    CheckListWidgetExpectation(withHeaderExtra.bHeaderExtraDrawn,
                               "the 3-callback path actually drew the header-extra control");
    const float predictedStripStartX = withHeaderExtra.windowLeftEdgeX
        + (withHeaderExtra.rowAvailWidthPixels - static_cast<float>(kAffordanceStripWidthPixels));
    CheckListWidgetExpectation(withHeaderExtra.headerExtraMax.x <= predictedStripStartX + 1.0f,
                               "(a) the header-extra control's own item rect ends at or before the "
                               "affordance strip's own first item starts -- no more overlap (items 8/9)");
    CheckListWidgetExpectation(
        std::fabs(withHeaderExtra.contentRegionMaxX - withHeaderExtra.stripRightmostMax.x) < kEpsilonPixels,
        "(b) the affordance strip's own rightmost item ends within a small epsilon of the row's TRUE "
        "right edge, not headerExtraWidthPixels (60px) short of it (items 8/9)");

    // 2-callback path: headerExtraWidthPixels == 0.0f -- the regression check every existing
    // DraggableList<T>::Render call site (19+ sites, none of which opt into a header-extra control)
    // actually exercises.
    const RowGeometryFrame withoutHeaderExtra = RunRowGeometryFrame(0.0f);
    CheckListWidgetExpectation(!withoutHeaderExtra.bHeaderExtraDrawn,
                               "the 2-callback path never invokes the header-extra callback at all");
    CheckListWidgetExpectation(
        std::fabs(withoutHeaderExtra.contentRegionMaxX - withoutHeaderExtra.stripRightmostMax.x) < kEpsilonPixels,
        "(b) regression: the strip's rightmost item still ends at the row's true right edge for the "
        "existing no-header-extra callers, unaffected by the fix");
}

} // namespace

namespace SanmapGen {
namespace Ui {
void RunDraggableListAcceptance() {
    TestApplySignalMovesAndDeletes();
    TestAffordanceSignalsCarryTheRightIndex();
    TestSyntheticDragProducesTheExpectedOrder();
    TestOptionalExtraButtonIsGenericAndRowScoped();
    TestOptionalHeaderExtraContentIsGenericAndRowScoped();
    TestHeaderExtraAffordanceStripGeometryDoesNotOverlapOrGap();
}
} // namespace Ui
} // namespace SanmapGen
