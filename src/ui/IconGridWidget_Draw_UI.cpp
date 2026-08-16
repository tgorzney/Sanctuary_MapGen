// IconGridWidget_Draw_UI.cpp — the imgui half of the icon grid (M5-3), split from the pure
// layout/hit-testing TU per ARCH §1.5.
//
// Virtualization: ImGuiListClipper over ROWS (the M5-2 VirtualList semantics — UI_FRAMEWORK_SPEC
// bypass toolkit #2). Only DisplayStart..DisplayEnd rows submit cells, so a 100k-icon manifest
// costs the visible rows and nothing else. Item spacing is pinned to the layout's cellSpacing so
// the imgui row stride is exactly IconGridRowStride — the clipper and the pure
// ComputeVisibleIconRows therefore describe the SAME window.
//
// Each cell is an InvisibleButton for hit-testing plus a direct ImDrawList AddImage of the
// resident atlas page at the manifest's UV rect (bypass toolkit #1). No file is opened, no GPU
// handle is owned: the page draw identifier comes from the atlas owner (ARCH §3.2).
#include "IconGridWidget_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

ImU32 CellBackgroundColor()  { return IM_COL32(28, 28, 32, 255); }
ImU32 CellHoveredColor()     { return IM_COL32(64, 64, 72, 255); }
ImU32 CellSelectedColor()    { return IM_COL32(255, 200, 80, 255); }

bool DrawIconCell(const IconAtlasManifest& manifest, IconGridState& state,
                  const IconGridCellPlacement& placement) {
    const IconGridLayout& layout = state.layout;
    ImGui::PushID(placement.iconIndex);
    const ImVec2 cellMinimum = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("iconCell", ImVec2(layout.cellWidth, layout.cellHeight));
    const bool bClicked = ImGui::IsItemDeactivated() && ImGui::IsItemHovered();
    const bool bHovered = ImGui::IsItemHovered();

    const ImVec2 cellMaximum(cellMinimum.x + layout.cellWidth, cellMinimum.y + layout.cellHeight);
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(cellMinimum, cellMaximum,
                            bHovered ? CellHoveredColor() : CellBackgroundColor());

    const std::uint64_t pageTexture = manifest.PageTextureIdentifier(placement.atlasPage);
    if (pageTexture != 0)
        drawList->AddImage(static_cast<ImTextureID>(pageTexture), cellMinimum, cellMaximum,
                           ImVec2(placement.uvMinimumX, placement.uvMinimumY),
                           ImVec2(placement.uvMaximumX, placement.uvMaximumY));
    if (placement.iconIndex == state.selectedIconIndex)
        drawList->AddRect(cellMinimum, cellMaximum, CellSelectedColor(), 0.0f, 0, 2.0f);

    ImGui::PopID();
    if (!bClicked) return false;
    const bool bSelectionChanged = state.selectedIconIndex != placement.iconIndex;
    state.selectedIconIndex = placement.iconIndex;
    state.selectedIconId = placement.iconId;
    return bSelectionChanged;
}

bool DrawIconRow(const IconAtlasManifest& manifest, IconGridState& state, int row) {
    bool bSelectionChanged = false;
    const IconGridLayout& layout = state.layout;
    for (int column = 0; column < layout.columnCount; ++column) {
        const int iconIndex = row * layout.columnCount + column;
        IconGridCellPlacement placement;
        if (!ComputeIconCellPlacement(manifest, layout, iconIndex, placement)) break;
        if (column > 0) ImGui::SameLine();
        if (DrawIconCell(manifest, state, placement)) bSelectionChanged = true;
    }
    return bSelectionChanged;
}

} // namespace

bool DrawIconGrid(const char* label, const IconAtlasManifest& manifest, IconGridState& state,
                  float gridHeight) {
    bool bSelectionChanged = false;
    ImGui::PushID(label);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(state.layout.cellSpacing, state.layout.cellSpacing));
    if (ImGui::BeginChild("iconGridViewport", ImVec2(0.0f, gridHeight), ImGuiChildFlags_Borders)) {
        state.layout.columnCount =
            ResolveIconGridColumnCount(state.layout, ImGui::GetContentRegionAvail().x);
        const int rowCount = IconGridRowCount(state.layout, manifest.EntryCount());

        ImGuiListClipper clipper;                       // O(visible rows), never O(iconCount)
        clipper.Begin(rowCount, IconGridRowStride(state.layout));
        while (clipper.Step())
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
                if (DrawIconRow(manifest, state, row)) bSelectionChanged = true;
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopID();
    return bSelectionChanged;
}

} // namespace Ui
} // namespace SanmapGen
