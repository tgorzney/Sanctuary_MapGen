// DraggableListWidget_RowAffordances_UI.h — the visibility/lock/delete/extra-button strip and the
// drag-drop detector both DraggableList<T> row layouts (Collapsible and STEP200's Flat) share.
// Split out of DraggableListWidget_RowLayout_UI.h (ARCH §1.5); depends only on the wire types in
// DraggableListWidget_Types_UI.h.
// Owns NO application state and MUTATES NOTHING (ARCH §3.2): every affordance click/drag only
// RECORDS a DraggableListSignal, never touches the caller's vector.
#pragma once
#include <imgui.h>
#include "DraggableListWidget_Types_UI.h"

namespace SanmapGen {
namespace Ui {

// Extra strip width for `extraButtonLabel` (id suffix hidden). Zero when unset.
inline float DraggableListExtraButtonWidthPixels(const DraggableListRow& row) {
    if (row.extraButtonLabel == nullptr) return 0.0f;
    const ImVec2 textSize = ImGui::CalcTextSize(row.extraButtonLabel, nullptr, true);
    return textSize.x + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetStyle().ItemSpacing.x;
}

namespace RowLayoutDetail {

inline void RecordSignal(DraggableListSignal& signal, DraggableListSignalKind kind,
                         int sourceRowIndex, int targetRowIndex = -1) {
    if (signal.bHasSignal()) return;                       // first signal of the frame wins
    signal.kind = kind;
    signal.sourceRowIndex = sourceRowIndex;
    signal.targetRowIndex = targetRowIndex;
}

// Visibility, alone: the Collapsible strip's leftmost icon (unmoved) and, STEP200, the Flat row's
// leftmost icon of all — same "##visibility" id and the same ToggleVisibility signal wherever it
// is drawn, so a changing bVisible never drops imgui's active id mid-click.
inline void DrawVisibilityIcon(const DraggableListRow& row, int rowIndex, DraggableListSignal& signal) {
    if (ImGui::SmallButton(row.bVisible ? "[o]##visibility" : "[-]##visibility"))
        RecordSignal(signal, DraggableListSignalKind::ToggleVisibility, rowIndex);
}

// Lock, delete, the optional extra button — right-aligned to `rowAvailWidthPixels`, the content
// width the CALLER measured once at the row's own left margin, before this row drew anything. That
// is what keeps the strip at a constant offset whether it is called right after a full-width
// CollapsingHeader (Collapsible) or after an arbitrary-width inline row body (Flat, STEP200) —
// recomputing GetContentRegionAvail() here instead would let the Flat row's body width push the
// strip around frame to frame.
// `bSuppressVisibilityIcon`: Flat mode already drew it on the left; Collapsible keeps it here.
inline void DrawRowAffordances(const DraggableListRow& row, int rowIndex, DraggableListSignal& signal,
                               float extraButtonWidthPixels, float rowAvailWidthPixels,
                               bool bSuppressVisibilityIcon) {
    ImGui::SameLine(rowAvailWidthPixels - static_cast<float>(kAffordanceStripWidthPixels)
        - extraButtonWidthPixels);
    if (!bSuppressVisibilityIcon) {
        DrawVisibilityIcon(row, rowIndex, signal);
        ImGui::SameLine();
    }
    if (ImGui::SmallButton(row.bLocked ? "[L]##lock" : "[U]##lock"))
        RecordSignal(signal, DraggableListSignalKind::ToggleLock, rowIndex);
    ImGui::SameLine();
    if (ImGui::SmallButton("X##delete"))
        RecordSignal(signal, DraggableListSignalKind::Delete, rowIndex);
    // Right of X, opt-in only (STEP150) — null draws nothing, every other consumer unaffected.
    if (row.extraButtonLabel != nullptr) {
        ImGui::SameLine();
        if (ImGui::SmallButton(row.extraButtonLabel))
            RecordSignal(signal, DraggableListSignalKind::ExtraButton, rowIndex);
    }
}

// Every row is both drag source and drop target; the payload is the source row index. Its source
// binds to the LAST item imgui submitted before this call — the row header in Collapsible mode,
// the name Selectable in Flat mode (STEP200 point 3).
inline void DetectRowDragAndDrop(const char* payloadIdentifier, int rowIndex, DraggableListSignal& signal) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        ImGui::SetDragDropPayload(payloadIdentifier, &rowIndex, sizeof(int));
        ImGui::Text("Moving row %d", rowIndex);
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* const payload = ImGui::AcceptDragDropPayload(payloadIdentifier);
        if (payload != nullptr && payload->DataSize == static_cast<int>(sizeof(int))) {
            const int sourceRowIndex = *static_cast<const int*>(payload->Data);
            if (sourceRowIndex != rowIndex)
                RecordSignal(signal, DraggableListSignalKind::Reorder, sourceRowIndex, rowIndex);
        }
        ImGui::EndDragDropTarget();
    }
}

} // namespace RowLayoutDetail
} // namespace Ui
} // namespace SanmapGen
