// IconGridWidget_Virtualization_UI_Test.cpp — the virtualization half of the M5-3 acceptance
// test: with a 10,000-icon manifest and a viewport far smaller than the content, ONLY the
// visible cells are visited (the M5-2 VirtualList / ImGuiListClipper semantics). The draw path
// walks exactly this window, so asserting it here asserts the drawn set without a live imgui
// frame. Row geometry is written out by hand: 64px cells + 4px spacing = a 68px row stride.
#include "IconGridWidget_TestSupport_UI.h"

namespace SanmapGen {
namespace IconGridTest {

void TestOnlyVisibleCellsAreVisited() {
    const Ui::IconAtlasManifest manifest = MakeMockAtlasManifest(10000);
    const Ui::IconGridLayout layout = MakeLayout(8);          // 1250 rows of 8 cells
    Check(Ui::IconGridRowCount(layout, manifest.EntryCount()) == 1250, "row count");
    Check(IsNear(Ui::IconGridRowStride(layout), 68.0f), "row stride");
    Check(IsNear(Ui::IconGridContentHeight(layout, manifest.EntryCount()), 84996.0f),
          "content height");

    // A 200px viewport shows 3 of the 68px rows: 24 cells out of 10,000.
    int visitedCellCount = 0;
    int firstVisitedIconId = -1;
    Ui::ForEachVisibleIconCell(manifest, layout, 0.0f, 200.0f,
        [&](const Ui::IconGridCellPlacement& placement) {
            if (visitedCellCount == 0) firstVisitedIconId = placement.iconId;
            ++visitedCellCount;
        });
    Check(visitedCellCount == 24, "only the visible cells are visited at the top");
    Check(visitedCellCount < manifest.EntryCount() / 100, "the cost is O(visible), not O(icons)");
    Check(firstVisitedIconId == 1000, "the first visited cell is icon 0");

    // Scrolled to row 100 (100 * 68px): the same 24 cells, now starting at icon index 800.
    visitedCellCount = 0;
    Ui::ForEachVisibleIconCell(manifest, layout, 6800.0f, 200.0f,
        [&](const Ui::IconGridCellPlacement& placement) {
            if (visitedCellCount == 0) firstVisitedIconId = placement.iconId;
            ++visitedCellCount;
        });
    Check(visitedCellCount == 24, "only the visible cells are visited when scrolled");
    Check(firstVisitedIconId == 1800, "the scrolled window starts at row 100");

    const Ui::IconGridRowWindow window =
        Ui::ComputeVisibleIconRows(layout, manifest.EntryCount(), 0.0f, 200.0f);
    Check(window.firstRow == 0 && window.rowCountVisible == 3, "the row window itself");
}

void TestColumnCountResolution() {
    const Ui::IconAtlasManifest manifest = MakeMockAtlasManifest(10000);
    const Ui::IconGridLayout layout = MakeLayout(8);

    // A scroll past the end clamps to the last row instead of walking off it.
    int visitedCellCount = 0;
    int lastVisitedIconId = -1;
    Ui::ForEachVisibleIconCell(manifest, layout, 1.0e6f, 200.0f,
        [&](const Ui::IconGridCellPlacement& placement) {
            lastVisitedIconId = placement.iconId; ++visitedCellCount; });
    Check(visitedCellCount == 8 && lastVisitedIconId == 10999, "a past-the-end scroll clamps");

    int emptyVisitCount = 0;
    Ui::ForEachVisibleIconCell(Ui::IconAtlasManifest(), layout, 0.0f, 200.0f,
        [&](const Ui::IconGridCellPlacement&) { ++emptyVisitCount; });
    Check(emptyVisitCount == 0, "an empty manifest visits nothing");

    // 8 cells + 7 gaps = 8*64 + 7*4 = 540px, the trailing cell needing no trailing gap.
    const Ui::IconGridLayout narrowLayout = MakeLayout(1);
    Check(Ui::ResolveIconGridColumnCount(narrowLayout, 540.0f) == 8,
          "8 cells fit in exactly 540px");
    Check(Ui::ResolveIconGridColumnCount(narrowLayout, 539.0f) == 7, "one pixel short fits 7");
    Check(Ui::ResolveIconGridColumnCount(narrowLayout, 10.0f) == 1,
          "a narrow pane still shows one column");
}

} // namespace IconGridTest
} // namespace SanmapGen
