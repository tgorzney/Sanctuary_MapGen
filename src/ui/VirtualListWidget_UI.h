// VirtualListWidget_UI.h — draw only the VISIBLE rows of a list, however long the list is.
// Layer: UI. Accuracy class: Visual (presentation only — it derives no simulated quantity,
// so ARCH §3.2's "UI never simulates" is untouched).
//
// UI_FRAMEWORK_SPEC bypass item 2: ImGuiListClipper virtualization is the reason a 100k-entity
// list scrolls cheaply. Cost is O(visible rows) per frame, never O(rowCount) — the draw
// callback is invoked only for the rows inside the clip rectangle.
//
// Owns NO application state: the caller supplies the row count and a row-draw callback and
// keeps its data wherever it already lives. Rows are addressed by INDEX, so a struct-of-arrays
// column set virtualizes exactly as well as a contiguous array (UI_FRAMEWORK_SPEC item 6 — the
// prop SoA is never copied into a per-item list).
//
// The callback is a TEMPLATE parameter, not a std::function: a virtualized row callback is
// invoked every frame per visible row and must inline, with no type-erasure allocation
// (the legacy gui/UIHelpers.h list took std::function).
//
// Pure CPU. ImGuiListClipper needs a current imgui context with a frame begun and nothing else
// — no GL, no backend — so this unit is testable headless.
#pragma once
#include <vector>
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

// What a Render call actually drew this frame. `drawnRowCount` is exactly the number of
// draw-callback invocations, so it is also the instrumentation that proves the O(visible)
// claim (Constitution §3: report what the hot path really did, never assume it).
struct VirtualListVisibleRange {
    int firstRowIndex = 0;   // first row submitted this frame
    int drawnRowCount = 0;   // rows submitted == draw-callback invocations

    int RowIndexEndExclusive() const { return firstRowIndex + drawnRowCount; }
};

// The engine, and the struct-of-arrays entry point. `drawRow(int rowIndex)` draws one row and
// reads whichever columns it likes.
//
// `rowHeight` must be the true per-row height in pixels: the clipper maps scroll position onto
// row indices with it, so a wrong value scrolls to the wrong rows. `viewportHeight` is the
// scroll region's height (<= 0 means "fill the parent window's remaining height").
//
// Scrolling is the caller's: ImGui::SetNextWindowScroll(...) immediately before this call
// applies to the row region, because the region IS the next child window opened.
//
// Degenerate input draws nothing and reports an empty range rather than crashing (Constitution
// §6): a null identifier, a non-positive row count, or a non-positive/NaN row height. An empty
// list draws no placeholder text either — what "empty" should say is caller policy, not a
// widget's decision (the legacy renderer hardcoded "No items found.").
template <typename DrawRowFunction>
VirtualListVisibleRange RenderVirtualRows(const char* listIdentifier, int rowCount,
                                          float rowHeight, float viewportHeight,
                                          DrawRowFunction drawRow) {
    VirtualListVisibleRange visibleRange;
    if (listIdentifier == nullptr || rowCount <= 0 || !(rowHeight > 0.0f)) return visibleRange;

    const bool bRegionVisible = ImGui::BeginChild(listIdentifier, ImVec2(0.0f, viewportHeight),
                                                  ImGuiChildFlags_Borders);
    if (bRegionVisible) {
        ImGuiListClipper rowClipper;
        rowClipper.Begin(rowCount, rowHeight);
        while (rowClipper.Step()) {
            if (visibleRange.drawnRowCount == 0) visibleRange.firstRowIndex = rowClipper.DisplayStart;
            for (int rowIndex = rowClipper.DisplayStart; rowIndex < rowClipper.DisplayEnd; ++rowIndex) {
                drawRow(rowIndex);
                ++visibleRange.drawnRowCount;
            }
        }
        rowClipper.End();
    }
    ImGui::EndChild();   // always paired with BeginChild, visible or culled
    return visibleRange;
}

// The contiguous-array form (UI_FRAMEWORK_SPEC's `VirtualList<T>`): the callback receives the
// element as well as its index — `drawRow(int rowIndex, const T& item)`.
//
// The array is BORROWED for the duration of the call and never copied or retained. For a
// struct-of-arrays source call RenderVirtualRows directly; there is no array-of-structs
// requirement anywhere in this widget.
template <typename T>
class VirtualList {
public:
    template <typename DrawRowFunction>
    static VirtualListVisibleRange Render(const char* listIdentifier, const T* items, int itemCount,
                                          float rowHeight, float viewportHeight,
                                          DrawRowFunction drawRow) {
        if (items == nullptr) return VirtualListVisibleRange();
        return RenderVirtualRows(listIdentifier, itemCount, rowHeight, viewportHeight,
                                 [&](int rowIndex) { drawRow(rowIndex, items[rowIndex]); });
    }

    template <typename DrawRowFunction>
    static VirtualListVisibleRange Render(const char* listIdentifier, const std::vector<T>& items,
                                          float rowHeight, float viewportHeight,
                                          DrawRowFunction drawRow) {
        return Render(listIdentifier, items.data(), static_cast<int>(items.size()), rowHeight,
                      viewportHeight, drawRow);
    }
};

} // namespace Ui
} // namespace SanmapGen
