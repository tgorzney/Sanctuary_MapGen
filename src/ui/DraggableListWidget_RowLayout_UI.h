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
// STEP123: `headerExtraWidthPixels`/`drawRowHeaderExtra` are the OPTIONAL header-extra slot — a
// fixed-width caller-drawn control sitting to the LEFT of the affordance strip, on the header's own
// line. `headerExtraWidthPixels == 0.0f` draws nothing and reserves nothing (byte-identical to the
// pre-STEP123 layout).
template <typename DrawRowBodyFunction, typename DrawRowHeaderExtraFunction>
void RenderCollapsibleRow(const char* payloadIdentifier, const DraggableListRow& row, int rowIndex,
                          float rowAvailWidthPixels, float extraButtonWidthPixels,
                          float headerExtraWidthPixels, int selectedRowIndex,
                          DrawRowBodyFunction drawRowBody, DrawRowHeaderExtraFunction drawRowHeaderExtra,
                          DraggableListSignal& signal) {
    const float stripStartX = ImGui::GetCursorScreenPos().x + rowAvailWidthPixels
        - static_cast<float>(kAffordanceStripWidthPixels) - extraButtonWidthPixels - headerExtraWidthPixels;
    const bool bExpanded = ImGui::CollapsingHeader(
        row.label != nullptr ? row.label : "",
        ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanFullWidth |
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen |
        (rowIndex == selectedRowIndex ? ImGuiTreeNodeFlags_Selected : 0));
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::GetIO().MousePos.x < stripStartX)
        RecordSignal(signal, DraggableListSignalKind::Select, rowIndex);
    DetectRowDragAndDrop(payloadIdentifier, rowIndex, signal);
    if (headerExtraWidthPixels > 0.0f) {
        ImGui::SameLine(rowAvailWidthPixels - static_cast<float>(kAffordanceStripWidthPixels)
            - extraButtonWidthPixels - headerExtraWidthPixels);
        drawRowHeaderExtra(rowIndex);
    }
    DrawRowAffordances(row, rowIndex, signal, extraButtonWidthPixels,
                       rowAvailWidthPixels - headerExtraWidthPixels, false);
    if (bExpanded) { ImGui::Indent(); drawRowBody(rowIndex); ImGui::Unindent(); }
}

// STEP200's single-line row: visibility -> name (the drag handle, fixed-width so nothing on this
// row can grow it) -> the caller's inline body -> the right-aligned lock/delete strip. Never
// collapsible — `drawRowBody` runs unconditionally every frame; there is no disclosure state to
// gate it behind. STEP123: gains the same optional header-extra slot as the Collapsible row above,
// for interface symmetry — no Flat-mode consumer opts in yet.
template <typename DrawRowBodyFunction, typename DrawRowHeaderExtraFunction>
void RenderFlatRow(const char* payloadIdentifier, const DraggableListRow& row, int rowIndex,
                   float rowAvailWidthPixels, float extraButtonWidthPixels, float headerExtraWidthPixels,
                   int selectedRowIndex, DrawRowBodyFunction drawRowBody,
                   DrawRowHeaderExtraFunction drawRowHeaderExtra, DraggableListSignal& signal) {
    DrawVisibilityIcon(row, rowIndex, signal);
    ImGui::SameLine();
    const bool bSelected = rowIndex == selectedRowIndex;
    if (ImGui::Selectable(row.label != nullptr ? row.label : "", bSelected, 0,
                          ImVec2(static_cast<float>(kFlatRowNameWidthPixels), 0.0f)))
        RecordSignal(signal, DraggableListSignalKind::Select, rowIndex);
    DetectRowDragAndDrop(payloadIdentifier, rowIndex, signal);   // binds to the name Selectable above
    ImGui::SameLine();
    drawRowBody(rowIndex);
    if (headerExtraWidthPixels > 0.0f) {
        ImGui::SameLine(rowAvailWidthPixels - static_cast<float>(kAffordanceStripWidthPixels)
            - extraButtonWidthPixels - headerExtraWidthPixels);
        drawRowHeaderExtra(rowIndex);
    }
    DrawRowAffordances(row, rowIndex, signal, extraButtonWidthPixels,
                       rowAvailWidthPixels - headerExtraWidthPixels, true);
}

} // namespace RowLayoutDetail

// The widget. `describeRow(int rowIndex) -> DraggableListRow` gives the label and toggle states;
// `drawRowBody(int rowIndex)` draws the expanded content (a no-op for a header-only list) in
// Collapsible mode, or the inline row content in Flat mode. Both inline as template parameters —
// no std::function type erasure.
template <typename T>
class DraggableList {
public:
    // The original 2-callback shape every existing call site binds. A thin delegator (STEP123) onto
    // the 3-callback overload below with a no-op header-extra callback and 0.0f reserved width — every
    // current call site recompiles unchanged; arity alone disambiguates which overload a 2-callback
    // caller resolves to.
    template <typename DescribeRowFunction, typename DrawRowBodyFunction>
    static DraggableListSignal Render(const char* listIdentifier, const std::vector<T>& items,
                                      DescribeRowFunction describeRow, DrawRowBodyFunction drawRowBody,
                                      int selectedRowIndex = -1,
                                      DraggableListRowLayout layout = DraggableListRowLayout::Collapsible) {
        return Render(listIdentifier, items, describeRow, drawRowBody, [](int) {}, 0.0f,
                      selectedRowIndex, layout);
    }

    // STEP123: the OPTIONAL per-row header-extra slot, drawn INLINE on the header/Flat row's own line,
    // to the LEFT of the [o]/[U]/X strip. `headerExtraWidthPixels` is a FIXED width the CALLER supplies
    // (not measured per row like extraButtonLabel's text width — Color Override's checkbox+swatch is a
    // constant size for every row) and is reserved UNCONDITIONALLY so the strip sits at one constant
    // offset regardless of any individual row's own state (bColorOverrideEnabled, bUseGroupColor, ...).
    // headerExtraWidthPixels == 0.0f (the overload above) draws nothing and reserves nothing.
    template <typename DescribeRowFunction, typename DrawRowBodyFunction, typename DrawRowHeaderExtraFunction>
    static DraggableListSignal Render(const char* listIdentifier, const std::vector<T>& items,
                                      DescribeRowFunction describeRow, DrawRowBodyFunction drawRowBody,
                                      DrawRowHeaderExtraFunction drawRowHeaderExtra,
                                      float headerExtraWidthPixels, int selectedRowIndex = -1,
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
                        extraButtonWidthPixels, headerExtraWidthPixels, selectedRowIndex, drawRowBody,
                        drawRowHeaderExtra, signal);
                else
                    RowLayoutDetail::RenderCollapsibleRow(payloadIdentifier, row, rowIndex, rowAvailWidthPixels,
                        extraButtonWidthPixels, headerExtraWidthPixels, selectedRowIndex, drawRowBody,
                        drawRowHeaderExtra, signal);
            }
            ImGui::PopID();
        }
        ImGui::PopID();
        return signal;
    }
};

} // namespace Ui
} // namespace SanmapGen
