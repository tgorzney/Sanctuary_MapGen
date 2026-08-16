// VirtualListWidget_UI_Test.cpp — M5-2 acceptance for VirtualList<T>: with 100k items and a small
// viewport, the draw callback runs for the VISIBLE rows only, never for n. Holds main() for the
// ListWidgets_UI_Test binary; the DraggableList half lives in DraggableListWidget_UI_Test.cpp.
#include "ListWidget_TestFrame_UI.h"
#include "VirtualListWidget_UI.h"
#include <vector>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace SanmapGen { namespace Ui { void RunDraggableListAcceptance(); } }

namespace {

constexpr int   kHugeItemCount  = 100000;
constexpr float kRowHeight      = 20.0f;
constexpr float kViewportHeight = 200.0f;
const ImVec2    kWindowSize     = ImVec2(420.0f, 300.0f);

// Everything one frame's draw callbacks reported about themselves.
struct DrawTally {
    int  invocationCount   = 0;
    int  lowestRowIndex    = kHugeItemCount;
    int  highestRowIndex   = -1;
    bool bEveryItemMatched = true;   // the element handed over really is items[rowIndex]
};

void TallyRow(DrawTally& tally, int rowIndex, int itemValue) {
    ++tally.invocationCount;
    if (rowIndex < tally.lowestRowIndex)  tally.lowestRowIndex = rowIndex;
    if (rowIndex > tally.highestRowIndex) tally.highestRowIndex = rowIndex;
    if (itemValue != rowIndex * 2) tally.bEveryItemMatched = false;
    ImGui::TextUnformatted("row");
}

std::vector<int> MakeHugeList() {
    std::vector<int> items(kHugeItemCount);
    for (int index = 0; index < kHugeItemCount; ++index) items[index] = index * 2;
    return items;
}

// The acceptance case. Three frames are run because a window's first frame is imgui's layout
// settling frame; the assertions read the settled one.
void TestOnlyVisibleRowsInvokeTheCallback() {
    HeadlessImguiSession session;
    const std::vector<int> items = MakeHugeList();
    DrawTally tally;
    VirtualListVisibleRange visibleRange;
    for (int frameIndex = 0; frameIndex < 3; ++frameIndex) {
        tally = DrawTally();
        RunHeadlessFrame(HeadlessMouseState(), kWindowSize, [&] {
            visibleRange = VirtualList<int>::Render("HugeList", items, kRowHeight, kViewportHeight,
                [&](int rowIndex, const int& itemValue) { TallyRow(tally, rowIndex, itemValue); });
        });
    }
    std::printf("VirtualList: %d draw callbacks for %d items (rows %d..%d)\n",
                tally.invocationCount, kHugeItemCount, tally.lowestRowIndex, tally.highestRowIndex);
    CheckListWidgetExpectation(tally.invocationCount != kHugeItemCount, "not O(n) draws");
    // A 200px viewport of 20px rows is ~10 rows; the ceiling is generous but nowhere near 100000.
    CheckListWidgetExpectation(tally.invocationCount >= 4 && tally.invocationCount <= 32,
                               "callback count is approximately the visible row count");
    CheckListWidgetExpectation(visibleRange.drawnRowCount == tally.invocationCount,
                               "reported range matches the callbacks actually made");
    CheckListWidgetExpectation(visibleRange.firstRowIndex == 0 && tally.lowestRowIndex == 0,
                               "unscrolled list starts at row 0");
    CheckListWidgetExpectation(tally.highestRowIndex == visibleRange.RowIndexEndExclusive() - 1,
                               "drawn rows are one contiguous visible run");
    CheckListWidgetExpectation(tally.bEveryItemMatched, "each callback got its own element");
}

// Scrolling moves the visible window without changing its cost.
void TestScrollMovesTheVisibleRunOnly() {
    HeadlessImguiSession session;
    const std::vector<int> items = MakeHugeList();
    const float scrollPixels = 20000.0f;                       // row 1000 of 100000
    DrawTally tally;
    VirtualListVisibleRange visibleRange;
    for (int frameIndex = 0; frameIndex < 3; ++frameIndex) {
        tally = DrawTally();
        RunHeadlessFrame(HeadlessMouseState(), kWindowSize, [&] {
            ImGui::SetNextWindowScroll(ImVec2(-1.0f, scrollPixels));
            visibleRange = VirtualList<int>::Render("HugeList", items, kRowHeight, kViewportHeight,
                [&](int rowIndex, const int& itemValue) { TallyRow(tally, rowIndex, itemValue); });
        });
    }
    std::printf("VirtualList scrolled: %d draw callbacks, first row %d\n",
                tally.invocationCount, visibleRange.firstRowIndex);
    CheckListWidgetExpectation(tally.invocationCount >= 4 && tally.invocationCount <= 32,
                               "scrolled list still draws only the visible rows");
    CheckListWidgetExpectation(visibleRange.firstRowIndex >= 995 && visibleRange.firstRowIndex <= 1005,
                               "visible run follows the scroll position");
    CheckListWidgetExpectation(tally.bEveryItemMatched, "scrolled callbacks got their own elements");
}

// The struct-of-arrays entry point: no array of structs anywhere, two independent columns.
void TestStructOfArraysColumnsVirtualizeToo() {
    HeadlessImguiSession session;
    std::vector<float> heightColumn(kHugeItemCount, 1.0f);
    std::vector<int>   identifierColumn(kHugeItemCount, 7);
    int invocationCount = 0;
    VirtualListVisibleRange visibleRange;
    for (int frameIndex = 0; frameIndex < 3; ++frameIndex) {
        invocationCount = 0;
        RunHeadlessFrame(HeadlessMouseState(), kWindowSize, [&] {
            visibleRange = RenderVirtualRows("SoaList", kHugeItemCount, kRowHeight, kViewportHeight,
                [&](int rowIndex) {
                    ++invocationCount;
                    ImGui::Text("%d %.1f", identifierColumn[rowIndex], heightColumn[rowIndex]);
                });
        });
    }
    CheckListWidgetExpectation(invocationCount == visibleRange.drawnRowCount && invocationCount <= 32,
                               "SoA columns virtualize identically");
}

// Degenerate input draws nothing instead of crashing (Constitution §6).
void TestDegenerateInputIsSafe() {
    HeadlessImguiSession session;
    const std::vector<int> emptyList;
    int invocationCount = 0;
    VirtualListVisibleRange emptyRange, nullIdentifierRange, badHeightRange, nullPointerRange;
    RunHeadlessFrame(HeadlessMouseState(), kWindowSize, [&] {
        const auto countRow = [&](int) { ++invocationCount; };
        emptyRange = VirtualList<int>::Render("EmptyList", emptyList, kRowHeight, kViewportHeight,
                                              [&](int, const int&) { ++invocationCount; });
        nullIdentifierRange = RenderVirtualRows(nullptr, 10, kRowHeight, kViewportHeight, countRow);
        badHeightRange = RenderVirtualRows("BadHeight", 10, 0.0f, kViewportHeight, countRow);
        nullPointerRange = VirtualList<int>::Render("NullList", nullptr, 10, kRowHeight,
                                                    kViewportHeight, [&](int, const int&) {});
    });
    CheckListWidgetExpectation(invocationCount == 0, "degenerate lists draw no rows");
    CheckListWidgetExpectation(emptyRange.drawnRowCount == 0 && nullIdentifierRange.drawnRowCount == 0 &&
                               badHeightRange.drawnRowCount == 0 && nullPointerRange.drawnRowCount == 0,
                               "degenerate lists report an empty range");
}

} // namespace

int main() {
    TestOnlyVisibleRowsInvokeTheCallback();
    TestScrollMovesTheVisibleRunOnly();
    TestStructOfArraysColumnsVirtualizeToo();
    TestDegenerateInputIsSafe();
    RunDraggableListAcceptance();
    if (listWidgetTestFailureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", listWidgetTestFailureCount);
    return 1;
}
