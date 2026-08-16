// IconGridWidget_UI.h — the scrollable grid of atlas-thumbnail buttons (M5-3).
// Layer: UI (UI_FRAMEWORK_SPEC "universal widget library"). Accuracy class: Visual.
//
// Every cell samples the RESIDENT texture atlas by UV — never a file, never a per-item decode
// (ASSET_LOADING_SPEC: "virtualized lists and the preview sample the resident atlas by UV, zero
// per-item file I/O"). The grid is virtualized with the M5-2 VirtualList semantics
// (ImGuiListClipper over rows), so a 100k-icon set costs O(visible), not O(iconCount).
//
// SCOPE NOTE (ARCH §8.4): the atlas image and its real manifest are built by the asset pipeline
// (M5-4), which is a SEPARATE work-order and had not landed when this widget was written. The
// two structs below are the MINIMAL read-only view this widget consumes — an icon id plus the UV
// rect of the page holding it — declared here, in a file this work-order names, and nowhere
// else. When M5-4 lands it either publishes this shape or an adapter fills it; no new type was
// created in a folder this work-order does not own.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace SanmapGen {
namespace Ui {

// One thumbnail's place in the resident atlas.
struct IconAtlasEntry {
    int   iconId     = -1;     // identity the grid emits on click (upstream: keyed by tpId)
    int   atlasPage  = 0;      // which resident atlas page holds this thumbnail
    float uvMinimumX = 0.0f;
    float uvMinimumY = 0.0f;
    float uvMaximumX = 1.0f;
    float uvMaximumY = 1.0f;
};

struct IconAtlasManifest {
    std::vector<IconAtlasEntry> entries;
    // One draw identifier per atlas page, supplied by the atlas owner (IO/SYS). The UI only
    // forwards it to the draw list; it never creates, binds or frees a GPU handle (ARCH §3.2).
    std::vector<std::uint64_t> pageTextureIdentifiers;

    int EntryCount() const { return static_cast<int>(entries.size()); }
    std::uint64_t PageTextureIdentifier(int atlasPage) const {
        return (atlasPage >= 0 && atlasPage < static_cast<int>(pageTextureIdentifiers.size()))
                   ? pageTextureIdentifiers[static_cast<std::size_t>(atlasPage)]
                   : std::uint64_t(0);
    }
};

// Cell metrics in pixels. Every number is a tweakable (Constitution §8), never hardcoded below.
struct IconGridLayout {
    float cellWidth   = 64.0f;
    float cellHeight  = 64.0f;
    float cellSpacing = 4.0f;
    int   columnCount = 1;     // resolved from the available width each frame
};

// The rows a viewport can show — the ImGuiListClipper windowing expressed as pure math, so
// "only visible cells are drawn" is assertable without a live imgui frame.
struct IconGridRowWindow {
    int firstRow        = 0;
    int rowCountVisible = 0;
};

// Where one cell sits in the grid content, and the atlas rect it samples.
struct IconGridCellPlacement {
    int   iconIndex  = -1;
    int   iconId     = -1;
    int   atlasPage  = 0;
    float offsetX    = 0.0f;   // relative to the grid content origin
    float offsetY    = 0.0f;
    float uvMinimumX = 0.0f;
    float uvMinimumY = 0.0f;
    float uvMaximumX = 1.0f;
    float uvMaximumY = 1.0f;
};

// Caller-owned interaction state; holds no atlas data and no GPU handle.
struct IconGridState {
    IconGridLayout layout;
    int selectedIconIndex = -1;
    int selectedIconId    = -1;   // the id emitted on click, -1 when nothing is selected
};

// ---- pure layout / windowing / hit-testing (IconGridWidget_UI.cpp) --------------------------
int   ResolveIconGridColumnCount(const IconGridLayout& layout, float availableWidth);
int   IconGridRowCount(const IconGridLayout& layout, int iconCount);
float IconGridRowStride(const IconGridLayout& layout);
float IconGridContentHeight(const IconGridLayout& layout, int iconCount);

IconGridRowWindow ComputeVisibleIconRows(const IconGridLayout& layout, int iconCount,
                                         float scrollOffsetY, float viewportHeight);

// False for an out-of-range index; `outPlacement` is untouched in that case.
bool ComputeIconCellPlacement(const IconAtlasManifest& manifest, const IconGridLayout& layout,
                              int iconIndex, IconGridCellPlacement& outPlacement);

// Visits ONLY the cells the viewport can show — O(visible), never O(iconCount). This is the same
// window the clipper-driven draw walks, exposed so the virtualization is testable headless.
void ForEachVisibleIconCell(const IconAtlasManifest& manifest, const IconGridLayout& layout,
                            float scrollOffsetY, float viewportHeight,
                            const std::function<void(const IconGridCellPlacement&)>& visitCell);

// Index of the cell under a point in grid-content space, or -1 (the inter-cell gap is not a hit).
int IconIndexAtContentPosition(const IconAtlasManifest& manifest, const IconGridLayout& layout,
                               float contentX, float contentY);

// ---- the widget (IconGridWidget_Draw_UI.cpp) — needs a live imgui frame ---------------------
// Draws the grid inside a `gridHeight`-tall scrolling child, clipper-virtualized by row, and
// returns true iff the selection changed this frame (state.selectedIconId is the emitted id).
bool DrawIconGrid(const char* label, const IconAtlasManifest& manifest, IconGridState& state,
                  float gridHeight);

} // namespace Ui
} // namespace SanmapGen
