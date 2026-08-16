// IconGridWidget_UI.cpp — the pure layout, windowing and hit-testing of the icon grid (M5-3).
// No imgui, no GL, no file access: these are total functions of (manifest, layout, viewport), so
// the UV mapping, the selection and the virtualization are all assertable headless. The imgui
// drawing is the aspect twin, IconGridWidget_Draw_UI.cpp (ARCH §1.5), and it walks exactly the
// window ComputeVisibleIconRows defines — one windowing rule, not two.
#include "IconGridWidget_UI.h"
#include <cstddef>

namespace SanmapGen {
namespace Ui {
namespace {

float ColumnStride(const IconGridLayout& layout) { return layout.cellWidth + layout.cellSpacing; }

} // namespace

float IconGridRowStride(const IconGridLayout& layout) {
    return layout.cellHeight + layout.cellSpacing;
}

int ResolveIconGridColumnCount(const IconGridLayout& layout, float availableWidth) {
    const float columnStride = ColumnStride(layout);
    if (columnStride <= 0.0f || availableWidth < layout.cellWidth) return 1;
    // The trailing cell needs no trailing gap, hence the + cellSpacing; at least one always fits.
    const int columnCount = static_cast<int>((availableWidth + layout.cellSpacing) / columnStride);
    return columnCount > 0 ? columnCount : 1;
}

int IconGridRowCount(const IconGridLayout& layout, int iconCount) {
    if (iconCount <= 0 || layout.columnCount <= 0) return 0;
    return (iconCount + layout.columnCount - 1) / layout.columnCount;
}

float IconGridContentHeight(const IconGridLayout& layout, int iconCount) {
    const int rowCount = IconGridRowCount(layout, iconCount);
    if (rowCount <= 0) return 0.0f;
    return static_cast<float>(rowCount) * IconGridRowStride(layout) - layout.cellSpacing;
}

IconGridRowWindow ComputeVisibleIconRows(const IconGridLayout& layout, int iconCount,
                                         float scrollOffsetY, float viewportHeight) {
    IconGridRowWindow window;
    const int rowCount = IconGridRowCount(layout, iconCount);
    const float rowStride = IconGridRowStride(layout);
    if (rowCount <= 0 || rowStride <= 0.0f || viewportHeight <= 0.0f) return window;

    const float reciprocalRowStride = 1.0f / rowStride;   // one reciprocal, then multiply
    const float scrollTop = scrollOffsetY > 0.0f ? scrollOffsetY : 0.0f;
    const float scrollBottom = scrollTop + viewportHeight;

    int firstRow = static_cast<int>(scrollTop * reciprocalRowStride);
    if (firstRow > rowCount - 1) firstRow = rowCount - 1;

    int lastRow = static_cast<int>(scrollBottom * reciprocalRowStride);
    // A row whose top lands exactly on the bottom edge is not on screen.
    if (static_cast<float>(lastRow) * rowStride >= scrollBottom) --lastRow;
    if (lastRow > rowCount - 1) lastRow = rowCount - 1;
    if (lastRow < firstRow) lastRow = firstRow;

    window.firstRow = firstRow;
    window.rowCountVisible = lastRow - firstRow + 1;
    return window;
}

bool ComputeIconCellPlacement(const IconAtlasManifest& manifest, const IconGridLayout& layout,
                              int iconIndex, IconGridCellPlacement& outPlacement) {
    if (iconIndex < 0 || iconIndex >= manifest.EntryCount() || layout.columnCount <= 0) return false;
    const IconAtlasEntry& entry = manifest.entries[static_cast<std::size_t>(iconIndex)];
    const int row = iconIndex / layout.columnCount;
    const int column = iconIndex - row * layout.columnCount;

    outPlacement.iconIndex  = iconIndex;
    outPlacement.iconId     = entry.iconId;
    outPlacement.atlasPage  = entry.atlasPage;
    outPlacement.offsetX    = static_cast<float>(column) * ColumnStride(layout);
    outPlacement.offsetY    = static_cast<float>(row) * IconGridRowStride(layout);
    outPlacement.uvMinimumX = entry.uvMinimumX;
    outPlacement.uvMinimumY = entry.uvMinimumY;
    outPlacement.uvMaximumX = entry.uvMaximumX;
    outPlacement.uvMaximumY = entry.uvMaximumY;
    return true;
}

void ForEachVisibleIconCell(const IconAtlasManifest& manifest, const IconGridLayout& layout,
                            float scrollOffsetY, float viewportHeight,
                            const std::function<void(const IconGridCellPlacement&)>& visitCell) {
    if (!visitCell) return;
    const int iconCount = manifest.EntryCount();
    const IconGridRowWindow window =
        ComputeVisibleIconRows(layout, iconCount, scrollOffsetY, viewportHeight);
    const int firstIconIndex = window.firstRow * layout.columnCount;
    int endIconIndex = firstIconIndex + window.rowCountVisible * layout.columnCount;
    if (endIconIndex > iconCount) endIconIndex = iconCount;

    IconGridCellPlacement placement;
    for (int iconIndex = firstIconIndex; iconIndex < endIconIndex; ++iconIndex)
        if (ComputeIconCellPlacement(manifest, layout, iconIndex, placement)) visitCell(placement);
}

int IconIndexAtContentPosition(const IconAtlasManifest& manifest, const IconGridLayout& layout,
                               float contentX, float contentY) {
    const float columnStride = ColumnStride(layout);
    const float rowStride = IconGridRowStride(layout);
    if (contentX < 0.0f || contentY < 0.0f || layout.columnCount <= 0 ||
        columnStride <= 0.0f || rowStride <= 0.0f) return -1;

    const int column = static_cast<int>(contentX / columnStride);
    const int row = static_cast<int>(contentY / rowStride);
    if (column >= layout.columnCount) return -1;
    if (contentX - static_cast<float>(column) * columnStride > layout.cellWidth) return -1;
    if (contentY - static_cast<float>(row) * rowStride > layout.cellHeight) return -1;

    const int iconIndex = row * layout.columnCount + column;
    return iconIndex < manifest.EntryCount() ? iconIndex : -1;
}

} // namespace Ui
} // namespace SanmapGen
