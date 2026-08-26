// DraggableListWidget_RowLayout_UI.h — DraggableList<T> itself, and its two per-row layouts: the
// CollapsingHeader row every existing consumer already has (Collapsible, unchanged) and the
// single-line row the View popup opts into (Flat, STEP200). Split out of DraggableListWidget_UI.h
// (ARCH §1.5); the affordance strip + drag-drop detector both layouts share live in
// DraggableListWidget_RowAffordances_UI.h, and the wire types both of those need live in
// DraggableListWidget_Types_UI.h — a one-way chain, so none of the three include each other back.
#pragma once
#include <cstring>
#include <vector>
#include <imgui.h>
#include "DraggableListWidget_RowAffordances_UI.h"

namespace SanmapGen {
namespace Ui {
namespace RowLayoutDetail {

// The existing whole-row CollapsingHeader — unchanged default behavior for every consumer but the
// View popup. `rowAvailWidthPixels` is measured by the caller before this row drew anything.
template <typename DrawRowBodyFunction>
void RenderCollapsibleRow(const char* payloadIdentifier, const DraggableListRow& row, int rowIndex,
                          float rowAvailWidthPixels, float extraButtonWidthPixels,
                          int selectedRowIndex, DrawRowBodyFunction drawRowBody,
                          DraggableListSignal& signal) {
    const float stripStartX = ImGui::GetCursorScreenPos().x + rowAvailWidthPixels
        - static_cast<float>(kAffordanceStripWidthPixels) - extraButtonWidthPixels;
    const bool bExpanded = ImGui::CollapsingHeader(
        row.label != nullptr ? row.label : "",
        ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanFullWidth |
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen |
        (rowIndex == selectedRowIndex ? ImGuiTreeNodeFlags_Selected : 0));
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::GetIO().MousePos.x < stripStartX)
        RecordSignal(signal, DraggableListSignalKind::Select, rowIndex);
    DetectRowDragAndDrop(payloadIdentifier, rowIndex, signal);
    DrawRowAffordances(row, rowIndex, signal, extraButtonWidthPixels, rowAvailWidthPixels, false);
    if (bExpanded) { ImGui::Indent(); drawRowBody(rowIndex); ImGui::Unindent(); }
}

// STEP200's single-line row: visibility -> name (the drag handle, fixed-width so nothing on this
// row can grow it) -> the caller's inline body -> the right-aligned lock/delete strip. Never
// collapsible — `drawRowBody` runs unconditionally every frame; there is no disclosure state to
// gate it behind.
template <typename DrawRowBodyFunction>
void RenderFlatRow(const char* payloadIdentifier, const DraggableListRow& row, int rowIndex,
                   float rowAvailWidthPixels, float extraButtonWidthPixels, int selectedRowIndex,
                   DrawRowBodyFunction drawRowBody, DraggableListSignal& signal) {
    DrawVisibilityIcon(row, rowIndex, signal);
    ImGui::SameLine();
    const bool bSelected = rowIndex == selectedRowIndex;
    if (ImGui::Selectable(row.label != nullptr ? row.label : "", bSelected, 0,
                          ImVec2(static_cast<float>(kFlatRowNameWidthPixels), 0.0f)))
        RecordSignal(signal, DraggableListSignalKind::Select, rowIndex);
    DetectRowDragAndDrop(payloadIdentifier, rowIndex, signal);   // binds to the name Selectable above
    ImGui::SameLine();
    drawRowBody(rowIndex);
    DrawRowAffordances(row, rowIndex, signal, extraButtonWidthPixels, rowAvailWidthPixels, true);
}

} // namespace RowLayoutDetail

// The widget. `describeRow(int rowIndex) -> DraggableListRow` gives the label and toggle states;
// `drawRowBody(int rowIndex)` draws the expanded content (a no-op for a header-only list) in
// Collapsible mode, or the inline row content in Flat mode. Both inline as template parameters —
// no std::function type erasure.
template <typename T>
class DraggableList {
public:
    template <typename DescribeRowFunction, typename DrawRowBodyFunction>
    static DraggableListSignal Render(const char* listIdentifier, const std::vector<T>& items,
                                      DescribeRowFunction describeRow,
                                      DrawRowBodyFunction drawRowBody,
                                      int selectedRowIndex = -1,
                                      DraggableListRowLayout layout = DraggableListRowLayout::Collapsible) {
        DraggableListSignal signal;
        if (listIdentifier == nullptr) return signal;
        // imgui caps a payload type at 32 characters; a longer id falls back, never asserts.
        const char* const payloadIdentifier =
            (std::strlen(listIdentifier) < 32u) ? listIdentifier : "SanGenDraggableListRow";
        ImGui::PushID(listIdentifier);
        const int rowCount = static_cast<int>(items.size());
        for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
            ImGui::PushID(rowIndex);
            const DraggableListRow row = describeRow(rowIndex);
            if (!row.bRowSuppressed) {
                const float extraButtonWidthPixels = DraggableListExtraButtonWidthPixels(row);
                const float rowAvailWidthPixels = ImGui::GetContentRegionAvail().x;
                if (layout == DraggableListRowLayout::Flat)
                    RowLayoutDetail::RenderFlatRow(payloadIdentifier, row, rowIndex, rowAvailWidthPixels,
                        extraButtonWidthPixels, selectedRowIndex, drawRowBody, signal);
                else
                    RowLayoutDetail::RenderCollapsibleRow(payloadIdentifier, row, rowIndex, rowAvailWidthPixels,
                        extraButtonWidthPixels, selectedRowIndex, drawRowBody, signal);
            }
            ImGui::PopID();
        }
        ImGui::PopID();
        return signal;
    }
};

} // namespace Ui
} // namespace SanmapGen
