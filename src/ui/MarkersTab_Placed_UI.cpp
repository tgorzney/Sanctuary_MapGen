// MarkersTab_Placed_UI.cpp — the imgui composition of the placed-marker list. Layer: UI.
// One shared VirtualList over the Placement stage's resolved marker SoA; read-only by design
// (MarkersTab_Placed_UI.h SCOPE NOTE).
#include "MarkersTab_Placed_UI.h"
#include "MarkersTab_Rules_UI.h"
#include "VirtualListWidget_UI.h"
#include "../data/PlacementInstances_DATA.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

// One row: the index, the category the rule asked for, the tpId, and the world position. The
// columns are read straight out of the SoA — no per-row record is materialized.
void DrawPlacedMarkerRow(const Data::PlacementInstances& placedMarkers, int rowIndex,
                         MarkersPlacedListState& state, char* rowLabel, int rowLabelCapacity) {
    const std::size_t instanceIndex = static_cast<std::size_t>(rowIndex);
    const int categoryIndex = placedMarkers.category[instanceIndex];
    const char* const categoryLabel = (categoryIndex >= 0 && categoryIndex < kMarkerCategoryCount)
        ? markerCategoryLabels[categoryIndex] : "Generic";
    // %.7s: the tpId is a fixed 8-byte field whose last byte need not be a terminator.
    std::snprintf(rowLabel, static_cast<std::size_t>(rowLabelCapacity), "%d: %s %.7s (%.1f, %.1f, %.1f)",
                  rowIndex, categoryLabel, placedMarkers.templateIdentifier[instanceIndex].characters,
                  placedMarkers.positionX[instanceIndex], placedMarkers.positionY[instanceIndex],
                  placedMarkers.positionZ[instanceIndex]);
    if (ImGui::Selectable(rowLabel, rowIndex == state.selectedInstanceIndex))
        state.selectedInstanceIndex = rowIndex;
}

} // namespace

void DrawPlacedMarkerList(const Data::PlacementInstances* placedMarkers,
                          MarkersPlacedListState& state) {
    if (!DrawSectionBegin("Placed Markers", state.section)) return;
    if (placedMarkers == nullptr || placedMarkers->IsEmpty()) {
        ImGui::TextUnformatted("No markers have been generated yet.");
        DrawSectionEnd();
        return;
    }
    const int instanceCount = static_cast<int>(placedMarkers->Count());
    state.selectedInstanceIndex = ResolvedPlacedMarkerSelection(state.selectedInstanceIndex,
                                                                instanceCount);
    ImGui::Text("%d marker(s) resolved - read-only (Placement owns this buffer).", instanceCount);
    // Borrowed by the row callback for the duration of this Render only.
    char rowLabel[80] = { 0 };
    const VirtualListVisibleRange visibleRange = RenderVirtualRows(
        "placedMarkers", instanceCount, state.rowHeight, state.listHeight,
        [&](int rowIndex) {
            DrawPlacedMarkerRow(*placedMarkers, rowIndex, state, rowLabel, IM_ARRAYSIZE(rowLabel));
        });
    ImGui::Text("Drew %d of %d row(s).", visibleRange.drawnRowCount, instanceCount);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
