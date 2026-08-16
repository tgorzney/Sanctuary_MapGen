// IconGridWidget_UI_Test.cpp — acceptance test for the M5-3 icon grid: the UV-rect mapping and
// the click-to-id selection, against the MOCK atlas manifest in IconGridWidget_TestSupport_UI.h
// (the real atlas is M5-4, a separate work-order). The virtualization half of the acceptance
// runs in IconGridWidget_Virtualization_UI_Test.cpp; main() lives here.
#include "IconGridWidget_TestSupport_UI.h"

namespace SanmapGen {
namespace IconGridTest {
namespace {

void TestCellsMapToTheRightUvRects() {
    const Ui::IconAtlasManifest manifest = MakeMockAtlasManifest(10000);
    const Ui::IconGridLayout layout = MakeLayout(8);
    Ui::IconGridCellPlacement placement;

    Check(Ui::ComputeIconCellPlacement(manifest, layout, 0, placement), "cell 0 places");
    Check(placement.atlasPage == 0 && PlacementHasUvRect(placement, 0.0f, 0.0f), "cell 0 UV rect");
    Check(placement.iconId == 1000 && IsNear(placement.offsetX, 0.0f), "cell 0 id and offset");

    Check(Ui::ComputeIconCellPlacement(manifest, layout, 65, placement), "cell 65 places");
    Check(placement.atlasPage == 0 && PlacementHasUvRect(placement, kUvStep, kUvStep),
          "cell 65 UV rect is atlas row 1, column 1");

    Check(Ui::ComputeIconCellPlacement(manifest, layout, kIconsPerAtlasPage, placement),
          "the first cell of page 1 places");
    Check(placement.atlasPage == 1 && PlacementHasUvRect(placement, 0.0f, 0.0f),
          "page 1 restarts the UV rects");
    Check(Ui::ComputeIconCellPlacement(manifest, layout, 2 * kIconsPerAtlasPage + 130, placement),
          "a page 2 cell places");
    Check(placement.atlasPage == 2 && PlacementHasUvRect(placement, 2.0f * kUvStep, 2.0f * kUvStep),
          "page 2 UV rect");
}

void TestCellGeometryAndPageLookup() {
    const Ui::IconAtlasManifest manifest = MakeMockAtlasManifest(10000);
    const Ui::IconGridLayout layout = MakeLayout(8);
    Ui::IconGridCellPlacement placement;

    // Index 9 across 8 columns is row 1, column 1 — one 68px stride on each axis.
    Check(Ui::ComputeIconCellPlacement(manifest, layout, 9, placement), "cell 9 places");
    Check(IsNear(placement.offsetX, 68.0f) && IsNear(placement.offsetY, 68.0f), "cell 9 offsets");
    Check(!Ui::ComputeIconCellPlacement(manifest, layout, 10000, placement),
          "a past-the-end index is rejected");
    Check(!Ui::ComputeIconCellPlacement(manifest, layout, -1, placement),
          "a negative index is rejected");
    Check(manifest.PageTextureIdentifier(1) == 12u, "the page draw identifier is forwarded");
    Check(manifest.PageTextureIdentifier(9) == 0u, "an unknown page yields no texture");
}

void TestSelectionReturnsTheRightId() {
    const Ui::IconAtlasManifest manifest = MakeMockAtlasManifest(10000);
    const Ui::IconGridLayout layout = MakeLayout(8);

    const int firstIndex = Ui::IconIndexAtContentPosition(manifest, layout, 10.0f, 10.0f);
    Check(firstIndex == 0 && manifest.entries[0].iconId == 1000,
          "a click in cell 0 selects icon id 1000");
    const int ninthIndex = Ui::IconIndexAtContentPosition(manifest, layout, 70.0f, 70.0f);
    Check(ninthIndex == 9, "a click at (70,70) selects cell 9");
    Check(manifest.entries[static_cast<std::size_t>(ninthIndex)].iconId == 1009,
          "selection emits the manifest id, not the index");

    Check(Ui::IconIndexAtContentPosition(manifest, layout, 66.0f, 10.0f) == -1,
          "the horizontal gap between cells is not a hit");
    Check(Ui::IconIndexAtContentPosition(manifest, layout, 10.0f, 66.0f) == -1,
          "the vertical gap between cells is not a hit");
    Check(Ui::IconIndexAtContentPosition(manifest, layout, 554.0f, 10.0f) == -1,
          "past the last column is not a hit");
    Check(Ui::IconIndexAtContentPosition(manifest, layout, -1.0f, 10.0f) == -1,
          "a negative position is not a hit");

    const Ui::IconAtlasManifest shortManifest = MakeMockAtlasManifest(10);
    Check(Ui::IconIndexAtContentPosition(shortManifest, layout, 206.0f, 70.0f) == -1,
          "a cell past the end of a short manifest is not a hit");
}

} // namespace
} // namespace IconGridTest
} // namespace SanmapGen

int main() {
    using namespace SanmapGen::IconGridTest;
    TestCellsMapToTheRightUvRects();
    TestCellGeometryAndPageLookup();
    TestSelectionReturnsTheRightId();
    TestOnlyVisibleCellsAreVisited();
    TestColumnCountResolution();
    if (FailureCount() == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", FailureCount());
    return 1;
}
