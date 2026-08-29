// MarkersTab_ManualInstanceListRows_UI_Test.cpp — STEP126 headless-imgui acceptance coverage for
// DrawLayerRowBody's new per-Layer instance list (Open Q7). Mirrors
// MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp's HeadlessImguiSession/RunHeadlessFrame
// harness (ListWidget_TestFrame_UI.h). This is a leaf imgui function, testable standalone — no
// DraggableList::Render wrapper needed.
//
// Row positions are located the same way MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp's
// own SwatchCenter helper locates a sibling widget: from the LAST item's own rect (imgui's own
// GetItemRectMin/Max after the call — the last-drawn instance row, or "(none)" when the layer has
// no matching instances) plus the block's own known ItemSpacing, rather than reaching into imgui
// internals to enumerate every item drawn this frame.
#include "ListWidget_TestFrame_UI.h"
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include "SymmetryClusterInstanceList_UI.h"
#include <cmath>
#include <cstdio>
#include <functional>
#include <utility>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

// DrawLayerRowBody draws its full body above the Instances list (name, color override, icon scale,
// grid snap, symmetry section) before the two Selectable rows this test clicks -- tall enough that a
// too-short window clips them out of the hoverable/clickable area even though GetItemRectMin/Max
// still reports a valid (but off-window) rect. 700px gives comfortable margin over the ~470px this
// fixture's two-row case actually reaches.
const ImVec2 kWindowSize = ImVec2(400.0f, 700.0f);

int failureCount = 0;
void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

struct FrameResult {
    ImVec2 lastItemMin;
    ImVec2 lastItemMax;
    ImGuiID lastItemId  = 0;
    bool    bReturned   = false;
};

// STEP205 — gains an optional `selectManualMarkerInstanceCallback` (default `{}`, every pre-existing
// call site compiles unchanged) so a test can capture what DrawLayerRowBody's own canvas-sync
// callback was invoked with, threaded straight through to DrawLayerRowBody.
FrameResult RunRowBodyFrame(HeadlessMouseState mouse, Params::MarkerInstanceLayer& layer,
                            const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            std::vector<Params::MarkerInstanceGroup>& markers,
                            const ManualInstanceLayerIndex_UI& instanceIndex, ManualMarkerLayersState& state,
                            int& selectedManualInstanceIdentifier,
                            std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
                            const std::function<void(int, bool, bool)>&
                                selectManualMarkerInstanceCallback = {}) {
    FrameResult result;
    const Params::Geometry geometry;
    Params::MarkerSymmetryFixSettings symmetryFixSettings;
    RunHeadlessFrame(mouse, kWindowSize, [&] {
        result.bReturned = DrawLayerRowBody(layer, /*layerIndex=*/0, markerLayers, markers, geometry,
                                            Params::SymmetryAxis::None, 3, symmetryFixSettings, state,
                                            instanceIndex, selectedManualInstanceIdentifier,
                                            selectedManualInstanceIdentifiers, anchorIdentifier,
                                            selectManualMarkerInstanceCallback);
        result.lastItemMin = ImGui::GetItemRectMin();
        result.lastItemMax = ImGui::GetItemRectMax();
        result.lastItemId  = ImGui::GetItemID();
    });
    return result;
}

// STEP205 — gains optional `bCtrlHeld`/`bShiftHeld`/`selectManualMarkerInstanceCallback` (every
// pre-existing call site's three trailing defaults keep it a byte-identical plain click with no
// callback wired), held across all three synthetic frames (hover/press/release) so whichever frame
// imgui's own Selectable resolves the click on sees the SAME modifier state.
FrameResult ClickAt(ImVec2 position, Params::MarkerInstanceLayer& layer,
                    const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                    std::vector<Params::MarkerInstanceGroup>& markers,
                    const ManualInstanceLayerIndex_UI& instanceIndex, ManualMarkerLayersState& state,
                    int& selectedManualInstanceIdentifier,
                    std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
                    bool bCtrlHeld = false, bool bShiftHeld = false,
                    const std::function<void(int, bool, bool)>&
                        selectManualMarkerInstanceCallback = {}) {
    HeadlessMouseState hover;   hover.position = position; hover.bCtrlHeld = bCtrlHeld; hover.bShiftHeld = bShiftHeld;
    HeadlessMouseState press   = hover; press.bLeftButtonDown   = true;
    HeadlessMouseState release = hover; release.bLeftButtonDown = false;
    RunRowBodyFrame(hover, layer, markerLayers, markers, instanceIndex, state, selectedManualInstanceIdentifier,
                   selectedManualInstanceIdentifiers, anchorIdentifier, selectManualMarkerInstanceCallback);
    const FrameResult pressResult =
        RunRowBodyFrame(press, layer, markerLayers, markers, instanceIndex, state, selectedManualInstanceIdentifier,
                        selectedManualInstanceIdentifiers, anchorIdentifier, selectManualMarkerInstanceCallback);
    RunRowBodyFrame(release, layer, markerLayers, markers, instanceIndex, state, selectedManualInstanceIdentifier,
                    selectedManualInstanceIdentifiers, anchorIdentifier, selectManualMarkerInstanceCallback);
    return pressResult;
}

// Two transforms tagged layerIndex 0 (distinct instanceIdentifiers 100/101) and one transform
// tagged layerIndex 1 (identifier 200, a DIFFERENT layer). Clicking each of layer 0's own rows sets
// selectedManualInstanceIdentifier to that row's own identifier and nothing else; the function's own
// return value stays false (a selection click never triggers MakeNamesUnique).
void RunInstanceRowClickChecks() {
    HeadlessImguiSession session;
    std::vector<Params::MarkerInstanceLayer> markerLayers(1);
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].name = "Resources";
    Params::MarkerTransform first;  first.name = "A";  first.layerIndex = 0; first.instanceIdentifier = 100;
    Params::MarkerTransform second; second.name = "B"; second.layerIndex = 0; second.instanceIdentifier = 101;
    Params::MarkerTransform other;  other.name = "C";  other.layerIndex = 1; other.instanceIdentifier = 200;
    markers[0].transforms.push_back(first);
    markers[0].transforms.push_back(second);
    markers[0].transforms.push_back(other);
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);
    Check(instanceIndex.instancesByLayerIndex.at(0).size() == 2u,
          "the index maps exactly 2 (group,transform) pairs to layerIndex 0 — the layerIndex-1 transform is excluded");

    ManualMarkerLayersState state;
    int selectedManualInstanceIdentifier = -1;
    std::vector<int> selectedManualInstanceIdentifiers;
    int anchorIdentifier = -1;

    const FrameResult settle = RunRowBodyFrame(HeadlessMouseState(), markerLayers[0], markerLayers, markers,
                                               instanceIndex, state, selectedManualInstanceIdentifier,
                                               selectedManualInstanceIdentifiers, anchorIdentifier);
    Check(settle.lastItemId != 0, "the last-drawn item is a real, interactive Selectable row, not inert text");
    const float rowHeight  = settle.lastItemMax.y - settle.lastItemMin.y;
    const float rowSpacing = ImGui::GetStyle().ItemSpacing.y;
    const ImVec2 secondRowCenter((settle.lastItemMin.x + settle.lastItemMax.x) * 0.5f,
                                 (settle.lastItemMin.y + settle.lastItemMax.y) * 0.5f);
    const ImVec2 firstRowCenter(secondRowCenter.x, secondRowCenter.y - (rowHeight + rowSpacing));

    const FrameResult firstClick = ClickAt(firstRowCenter, markerLayers[0], markerLayers, markers,
                                           instanceIndex, state, selectedManualInstanceIdentifier,
                                           selectedManualInstanceIdentifiers, anchorIdentifier);
    Check(selectedManualInstanceIdentifier == 100, "clicking the first row selects its own instanceIdentifier (100)");
    Check(!firstClick.bReturned, "a selection click never sets the function's own commit return value");

    const FrameResult secondClick = ClickAt(secondRowCenter, markerLayers[0], markerLayers, markers,
                                            instanceIndex, state, selectedManualInstanceIdentifier,
                                            selectedManualInstanceIdentifiers, anchorIdentifier);
    Check(selectedManualInstanceIdentifier == 101, "clicking the SECOND row updates to its own instanceIdentifier (101), not additive/toggled");
    Check(!secondClick.bReturned, "and still reports no commit");
}

// A layer with zero matching instances in the index renders "(none)" — the last item is inert
// (imgui text widgets push no interactive ID), never a clickable Selectable.
void RunNoInstancesRendersNoneChecks() {
    HeadlessImguiSession session;
    std::vector<Params::MarkerInstanceLayer> markerLayers(1);
    std::vector<Params::MarkerInstanceGroup> markers(1);
    Params::MarkerTransform onlyOther;
    onlyOther.layerIndex = 1;   // never layerIndex 0
    onlyOther.instanceIdentifier = 5;
    markers[0].transforms.push_back(onlyOther);
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);
    Check(instanceIndex.instancesByLayerIndex.find(0) == instanceIndex.instancesByLayerIndex.end(),
          "layerIndex 0 has no entry at all in the index (find() == end(), not an empty vector)");

    ManualMarkerLayersState state;
    int selectedManualInstanceIdentifier = -1;
    std::vector<int> selectedManualInstanceIdentifiers;
    int anchorIdentifier = -1;
    const FrameResult settle = RunRowBodyFrame(HeadlessMouseState(), markerLayers[0], markerLayers, markers,
                                               instanceIndex, state, selectedManualInstanceIdentifier,
                                               selectedManualInstanceIdentifiers, anchorIdentifier);
    Check(settle.lastItemId == 0, "the last-drawn item is inert (\"(none)\" text), not an interactive Selectable");

    const ImVec2 rowCenter((settle.lastItemMin.x + settle.lastItemMax.x) * 0.5f,
                           (settle.lastItemMin.y + settle.lastItemMax.y) * 0.5f);
    ClickAt(rowCenter, markerLayers[0], markerLayers, markers, instanceIndex, state, selectedManualInstanceIdentifier,
           selectedManualInstanceIdentifiers, anchorIdentifier);
    Check(selectedManualInstanceIdentifier == -1, "clicking where a row would have been does nothing — no row exists");
}

// STEP130: DrawLayerRowBody's own body no longer draws a Color-Override checkbox/swatch block for
// EITHER a bundled or an ungrouped layer -- the block used to gate on `!state.bUseGroupColor`
// (STEP123's own comment for why it survived: bundled layers had no other way to reach the
// control). Proof: the row's own rendered HEIGHT is now IDENTICAL regardless of
// state.bUseGroupColor, since nothing left in the body reacts to that flag -- mirrors
// MarkersTab_ManualLayers_UI_Test.cpp's own RunManualLayerMarkerTypeFieldConditionalCheck
// height-diff technique.
float RunRowBodyHeight(bool bUseGroupColor) {
    HeadlessImguiSession session;
    std::vector<Params::MarkerInstanceLayer> markerLayers(1);
    std::vector<Params::MarkerInstanceGroup> markers;
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);
    ManualMarkerLayersState state;
    state.bUseGroupColor = bUseGroupColor;
    int selectedManualInstanceIdentifier = -1;
    std::vector<int> selectedManualInstanceIdentifiers;
    int anchorIdentifier = -1;
    const Params::Geometry geometry;
    Params::MarkerSymmetryFixSettings symmetryFixSettings;
    float height = 0.0f;
    RunHeadlessFrame(HeadlessMouseState(), kWindowSize, [&] {
        const float startY = ImGui::GetCursorPosY();
        DrawLayerRowBody(markerLayers[0], 0, markerLayers, markers, geometry, Params::SymmetryAxis::None, 3,
                         symmetryFixSettings, state, instanceIndex, selectedManualInstanceIdentifier,
                         selectedManualInstanceIdentifiers, anchorIdentifier);
        height = ImGui::GetCursorPosY() - startY;
    });
    return height;
}

void RunColorOverrideBodyCopyRemovedCheck() {
    const float heightWithGroupColorOff = RunRowBodyHeight(false);
    const float heightWithGroupColorOn  = RunRowBodyHeight(true);
    Check(std::fabs(heightWithGroupColorOff - heightWithGroupColorOn) < 0.01f,
          "DrawLayerRowBody's own rendered height is identical regardless of state.bUseGroupColor -- "
          "proof the body's own Color-Override checkbox/swatch block (formerly gated on "
          "!state.bUseGroupColor) no longer exists, for either a bundled or an ungrouped layer");
}

// STEP132 (ARCH §19.26) — the ticket's own literal fixture: 5 instances, 2 in symmetry group 3, 2 in
// symmetry group 7, 1 ungrouped (== 0). Drives the SAME shared helper DrawLayerRowBody now calls
// (SymmetryClusterInstanceList_UI.h), over the REAL (groupIndex, transformIndex) item shape and the
// REAL `symmetryGroupIdentifier` predicate, proving cluster members visit BEFORE any flat row, each
// cluster's own members stay in their original relative order, and the flat tail preserves its own
// original order too — the bucket-count/order acceptance this ticket's Verify section asks for.
void RunManualSymmetryClusterOrderChecks() {
    HeadlessImguiSession session;
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].name = "Resources";
    auto makeTransform = [](int identifier, int symmetryGroupIdentifier) {
        Params::MarkerTransform transform;
        transform.layerIndex             = 0;
        transform.instanceIdentifier     = identifier;
        transform.symmetryGroupIdentifier = symmetryGroupIdentifier;
        return transform;
    };
    markers[0].transforms.push_back(makeTransform(1, 3));
    markers[0].transforms.push_back(makeTransform(2, 3));
    markers[0].transforms.push_back(makeTransform(3, 7));
    markers[0].transforms.push_back(makeTransform(4, 7));
    markers[0].transforms.push_back(makeTransform(5, 0));
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);
    const std::vector<std::pair<int, int>>& pairs = instanceIndex.instancesByLayerIndex.at(0);
    Check(pairs.size() == 5u, "all 5 instances map to layerIndex 0");

    std::vector<int> visitOrder;
    RunHeadlessFrame(HeadlessMouseState(), ImVec2(300.0f, 400.0f), [&] {
        DrawSymmetryClusterInstanceList<std::pair<int, int>>(pairs,
            [&](const std::pair<int, int>& groupTransformIndex) {
                return markers[static_cast<std::size_t>(groupTransformIndex.first)]
                    .transforms[static_cast<std::size_t>(groupTransformIndex.second)].symmetryGroupIdentifier;
            },
            [](int groupIdentifier, int /*bucketSize*/) { return groupIdentifier != 0; },
            [&](const std::pair<int, int>& groupTransformIndex) {
                visitOrder.push_back(markers[static_cast<std::size_t>(groupTransformIndex.first)]
                    .transforms[static_cast<std::size_t>(groupTransformIndex.second)].instanceIdentifier);
            });
    });
    Check(visitOrder.size() == 5u, "every one of the 5 instances is visited exactly once");
    Check(visitOrder[0] == 1 && visitOrder[1] == 2, "symmetry group 3's 2 members render FIRST, as their own cluster, in order");
    Check(visitOrder[2] == 3 && visitOrder[3] == 4, "symmetry group 7's 2 members render NEXT, as their own cluster, in order");
    Check(visitOrder[4] == 5, "the ungrouped (== 0) instance renders LAST, as exactly 1 flat row");
}

// STEP132 (ARCH §19.26) — the SAME two rows RunInstanceRowClickChecks exercises, now both tagged
// into ONE non-zero symmetry group (a real cluster): per-row click-to-select stays byte-identical —
// the cluster's own collapsible header is an extra item ABOVE the two rows, nothing else changes.
void RunSymmetryGroupedInstanceRowClickChecks() {
    HeadlessImguiSession session;
    std::vector<Params::MarkerInstanceLayer> markerLayers(1);
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].name = "Resources";
    Params::MarkerTransform first;  first.name = "A"; first.layerIndex = 0; first.instanceIdentifier = 300;
    first.symmetryGroupIdentifier  = 9;
    Params::MarkerTransform second; second.name = "B"; second.layerIndex = 0; second.instanceIdentifier = 301;
    second.symmetryGroupIdentifier = 9;
    markers[0].transforms.push_back(first);
    markers[0].transforms.push_back(second);
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);

    ManualMarkerLayersState state;
    int selectedManualInstanceIdentifier = -1;
    std::vector<int> selectedManualInstanceIdentifiers;
    int anchorIdentifier = -1;

    const FrameResult settle = RunRowBodyFrame(HeadlessMouseState(), markerLayers[0], markerLayers, markers,
                                               instanceIndex, state, selectedManualInstanceIdentifier,
                                               selectedManualInstanceIdentifiers, anchorIdentifier);
    Check(settle.lastItemId != 0, "the last-drawn item, inside the open cluster, is a real, interactive row");
    const float rowHeight  = settle.lastItemMax.y - settle.lastItemMin.y;
    const float rowSpacing = ImGui::GetStyle().ItemSpacing.y;
    const ImVec2 secondRowCenter((settle.lastItemMin.x + settle.lastItemMax.x) * 0.5f,
                                 (settle.lastItemMin.y + settle.lastItemMax.y) * 0.5f);
    const ImVec2 firstRowCenter(secondRowCenter.x, secondRowCenter.y - (rowHeight + rowSpacing));

    ClickAt(firstRowCenter, markerLayers[0], markerLayers, markers, instanceIndex, state,
           selectedManualInstanceIdentifier, selectedManualInstanceIdentifiers, anchorIdentifier);
    Check(selectedManualInstanceIdentifier == 300,
          "clicking the first row inside an open cluster still selects its own instanceIdentifier (300)");

    ClickAt(secondRowCenter, markerLayers[0], markerLayers, markers, instanceIndex, state,
           selectedManualInstanceIdentifier, selectedManualInstanceIdentifiers, anchorIdentifier);
    Check(selectedManualInstanceIdentifier == 301,
          "clicking the second row inside the SAME cluster still updates to its own id (301), not additive");
}

// STEP205 (ARCH §21.1's own deferred follow-up) — a Ctrl-held row click must widen BOTH the
// tab-local multi-select write (ApplyManualInstanceSelectionClick, already inside DrawManualInstanceRow)
// AND the canvas-sync callback with the SAME real modifier state, so the two agree instead of one
// clobbering the other within the same click (the root problem this ticket fixes). Reuses the SAME
// two-row fixture RunInstanceRowClickChecks does.
void RunCtrlHeldClickSyncsTabLocalAndCanvasCallbackCheck() {
    HeadlessImguiSession session;
    std::vector<Params::MarkerInstanceLayer> markerLayers(1);
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].name = "Resources";
    Params::MarkerTransform first;  first.name = "A"; first.layerIndex = 0; first.instanceIdentifier = 100;
    Params::MarkerTransform second; second.name = "B"; second.layerIndex = 0; second.instanceIdentifier = 101;
    markers[0].transforms.push_back(first);
    markers[0].transforms.push_back(second);
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);

    ManualMarkerLayersState state;
    int selectedManualInstanceIdentifier = -1;
    std::vector<int> selectedManualInstanceIdentifiers;
    int anchorIdentifier = -1;

    int  reportedIdentifier = -1;
    bool bReportedCtrl      = false;
    bool bReportedShift     = false;
    int  callbackFireCount  = 0;
    const std::function<void(int, bool, bool)> selectManualMarkerInstanceCallback =
        [&](int instanceIdentifier, bool bCtrlHeld, bool bShiftHeld) {
            reportedIdentifier = instanceIdentifier;
            bReportedCtrl      = bCtrlHeld;
            bReportedShift     = bShiftHeld;
            ++callbackFireCount;
        };

    const FrameResult settle = RunRowBodyFrame(HeadlessMouseState(), markerLayers[0], markerLayers, markers,
                                               instanceIndex, state, selectedManualInstanceIdentifier,
                                               selectedManualInstanceIdentifiers, anchorIdentifier,
                                               selectManualMarkerInstanceCallback);
    const float rowHeight  = settle.lastItemMax.y - settle.lastItemMin.y;
    const float rowSpacing = ImGui::GetStyle().ItemSpacing.y;
    const ImVec2 secondRowCenter((settle.lastItemMin.x + settle.lastItemMax.x) * 0.5f,
                                 (settle.lastItemMin.y + settle.lastItemMax.y) * 0.5f);
    const ImVec2 firstRowCenter(secondRowCenter.x, secondRowCenter.y - (rowHeight + rowSpacing));

    // Baseline: a plain click on the FIRST row establishes the tab-local multi-select at {100}, and
    // fires the canvas-sync callback with (100, false, false) — the pre-STEP205 shape, unchanged.
    ClickAt(firstRowCenter, markerLayers[0], markerLayers, markers, instanceIndex, state,
           selectedManualInstanceIdentifier, selectedManualInstanceIdentifiers, anchorIdentifier,
           /*bCtrlHeld=*/false, /*bShiftHeld=*/false, selectManualMarkerInstanceCallback);
    Check(selectedManualInstanceIdentifier == 100, "the plain baseline click selects the first row (100)");
    Check(selectedManualInstanceIdentifiers.size() == 1 && selectedManualInstanceIdentifiers[0] == 100,
          "the plain baseline click's own tab-local multi-select is exactly {100}");
    Check(callbackFireCount == 1 && reportedIdentifier == 100 && !bReportedCtrl && !bReportedShift,
          "the plain baseline click's own canvas-sync callback fires with (100, false, false)");

    // A Ctrl-held click on the SECOND row: the callback must fire with (101, true, false) — proving
    // the widened signature actually carries the real modifier state through — while the tab-local
    // multi-select write (ApplyManualInstanceSelectionClick, inside the SAME click) still contains the
    // FIRST row's id (100): the two writes agree (both Ctrl-toggle 101 in) instead of the canvas-sync
    // half unconditionally Replacing and clobbering what the tab-local half just wrote.
    callbackFireCount = 0;
    ClickAt(secondRowCenter, markerLayers[0], markerLayers, markers, instanceIndex, state,
           selectedManualInstanceIdentifier, selectedManualInstanceIdentifiers, anchorIdentifier,
           /*bCtrlHeld=*/true, /*bShiftHeld=*/false, selectManualMarkerInstanceCallback);
    Check(callbackFireCount == 1 && reportedIdentifier == 101 && bReportedCtrl && !bReportedShift,
          "a Ctrl-held click on the second row fires the canvas-sync callback with (101, true, false)");
    Check(selectedManualInstanceIdentifiers.size() == 2
              && selectedManualInstanceIdentifiers[0] == 100 && selectedManualInstanceIdentifiers[1] == 101,
          "the tab-local multi-select still contains the FIRST row's id (100) after the Ctrl-held click "
          "— proving the tab-local write and the canvas-sync write agree instead of racing");
}

} // namespace

int main() {
    RunInstanceRowClickChecks();
    RunNoInstancesRendersNoneChecks();
    RunColorOverrideBodyCopyRemovedCheck();
    RunManualSymmetryClusterOrderChecks();
    RunSymmetryGroupedInstanceRowClickChecks();
    RunCtrlHeldClickSyncsTabLocalAndCanvasCallbackCheck();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
