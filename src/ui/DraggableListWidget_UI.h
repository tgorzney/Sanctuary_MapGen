// DraggableListWidget_UI.h — a reorderable list: drag-drop rows, per-row visibility / lock /
// delete. Layer: UI. Accuracy class: Visual. The UI_FRAMEWORK_SPEC `RenderDraggableLayerList<T>`
// generalized for the GeoLayer / layer stacks (Params::LayerStack; M5-6 wires them up).
// Owns NO application state and MUTATES NOTHING (ARCH §3.2): Render only DETECTS what the user
// asked for and returns one DraggableListSignal, which the caller applies to its own array
// (ApplyDraggableListSignal for the structural kinds). That is what makes a reorder testable
// without a window, and it retires the legacy defect of erasing from the vector while iterating it.
// Not virtualized on purpose — ordered stacks of tens of rows where every row is a drop target;
// a 100k-row list uses VirtualListWidget_UI.
#pragma once
#include <cstring>
#include <utility>
#include <vector>
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
enum class DraggableListSignalKind : int {
    None = 0, Reorder, Delete, ToggleVisibility, ToggleLock, Select, ExtraButton
};
// One frame produces at most ONE signal (first wins): every kind changes what the next frame draws.
struct DraggableListSignal {
    DraggableListSignalKind kind = DraggableListSignalKind::None;
    int sourceRowIndex = -1;   // the row the signal is about; the DRAGGED row for a reorder
    int targetRowIndex = -1;   // reorder only: the row it was dropped onto
    bool bHasSignal() const { return kind != DraggableListSignalKind::None; }
};
// What the caller says about one row. Strings are borrowed for the call, never retained.
struct DraggableListRow {
    const char* label    = "";
    bool        bVisible = true;
    bool        bLocked  = false;
    // OPTIONAL extra per-row affordance, right of the delete `X` — null (default) draws nothing,
    // so a consumer that never sets it (Props/Decals/Markers/the GeoLayer list) is unaffected.
    // STEP150's Bake/Unbake header button is the first user (a click reports `ExtraButton`); keep
    // the "##" id salt fixed across a changing label, same discipline as the icons below.
    const char* extraButtonLabel = nullptr;
};
// Affordance strip width — a NAMED constant, not the legacy bare `- 60`. Layout styling is a
// Constitution §8 tweakable that moves into the shared UI style settings with the tabs (M5-6).
enum : int { kAffordanceStripWidthPixels = 76 };
// Extra strip width for `extraButtonLabel` (id suffix hidden). Zero when unset.
inline float DraggableListExtraButtonWidthPixels(const DraggableListRow& row) {
    if (row.extraButtonLabel == nullptr) return 0.0f;
    const ImVec2 textSize = ImGui::CalcTextSize(row.extraButtonLabel, nullptr, true);
    return textSize.x + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetStyle().ItemSpacing.x;
}
// Applies the two STRUCTURAL signals — Reorder and Delete — to the caller's array; every other kind
// belongs to state this widget does not own and is left alone (returns false). Reorder postcondition:
// `items[targetRowIndex]` IS the dragged element, both directions — deliberately unlike the legacy
// `insert_i = (source < target) ? target - 1 : target`, which landed a downward drag one slot short
// of the drop. Indices are clamped or rejected, never trusted (§6).
template <typename T>
bool ApplyDraggableListSignal(std::vector<T>& items, const DraggableListSignal& signal) {
    const int itemCount = static_cast<int>(items.size());
    const int sourceRowIndex = signal.sourceRowIndex;
    if (sourceRowIndex < 0 || sourceRowIndex >= itemCount) return false;
    if (signal.kind == DraggableListSignalKind::Delete) {
        items.erase(items.begin() + sourceRowIndex);
        return true;
    }
    if (signal.kind != DraggableListSignalKind::Reorder) return false;
    int targetRowIndex = signal.targetRowIndex;
    if (targetRowIndex < 0) targetRowIndex = 0;
    if (targetRowIndex > itemCount - 1) targetRowIndex = itemCount - 1;
    if (targetRowIndex == sourceRowIndex) return false;
    T draggedItem = std::move(items[sourceRowIndex]);
    items.erase(items.begin() + sourceRowIndex);
    items.insert(items.begin() + targetRowIndex, std::move(draggedItem));
    return true;
}
// The widget. `describeRow(int rowIndex) -> DraggableListRow` gives the label and toggle states;
// `drawRowBody(int rowIndex)` draws the expanded content (a no-op for a header-only list). Both
// inline as template parameters — no std::function type erasure.
template <typename T>
class DraggableList {
public:
    template <typename DescribeRowFunction, typename DrawRowBodyFunction>
    static DraggableListSignal Render(const char* listIdentifier, const std::vector<T>& items,
                                      DescribeRowFunction describeRow,
                                      DrawRowBodyFunction drawRowBody,
                                      int selectedRowIndex = -1) {
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
            const float extraButtonWidthPixels = DraggableListExtraButtonWidthPixels(row);
            // Where the affordance strip starts: a press there belongs to a button, so the header
            // must not also claim it (overlap arbitration resolves a frame after the press).
            const float stripStartX = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x
                - static_cast<float>(kAffordanceStripWidthPixels) - extraButtonWidthPixels;
            const bool bExpanded = ImGui::CollapsingHeader(
                row.label != nullptr ? row.label : "",
                ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanFullWidth |
                ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen |
                (rowIndex == selectedRowIndex ? ImGuiTreeNodeFlags_Selected : 0));
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::GetIO().MousePos.x < stripStartX)
                RecordSignal(signal, DraggableListSignalKind::Select, rowIndex);
            DetectRowDragAndDrop(payloadIdentifier, rowIndex, signal);   // on the header item
            DrawRowAffordances(row, rowIndex, signal, extraButtonWidthPixels);
            if (bExpanded) { ImGui::Indent(); drawRowBody(rowIndex); ImGui::Unindent(); }
            ImGui::PopID();
        }
        ImGui::PopID();
        return signal;
    }

private:
    // Visibility, lock, delete. Each id is "##name"-stable so it does NOT change when the toggle
    // flips: an id that changes with state drops imgui's active id mid-click (and the legacy
    // rebuilt these ids from std::to_string every frame, allocating per row per frame).
    static void DrawRowAffordances(const DraggableListRow& row, int rowIndex,
                                   DraggableListSignal& signal, float extraButtonWidthPixels) {
        ImGui::SameLine(ImGui::GetContentRegionAvail().x
            - static_cast<float>(kAffordanceStripWidthPixels) - extraButtonWidthPixels);
        if (ImGui::SmallButton(row.bVisible ? "[o]##visibility" : "[-]##visibility"))
            RecordSignal(signal, DraggableListSignalKind::ToggleVisibility, rowIndex);
        ImGui::SameLine();
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
    // opens on the row HEADER, not on the delete button it followed in the legacy helper — imgui
    // binds a drag source to the LAST submitted item.
    static void DetectRowDragAndDrop(const char* payloadIdentifier, int rowIndex,
                                     DraggableListSignal& signal) {
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

    static void RecordSignal(DraggableListSignal& signal, DraggableListSignalKind kind,
                             int sourceRowIndex, int targetRowIndex = -1) {
        if (signal.bHasSignal()) return;                       // first signal of the frame wins
        signal.kind = kind;
        signal.sourceRowIndex = sourceRowIndex;
        signal.targetRowIndex = targetRowIndex;
    }
};

} // namespace Ui
} // namespace SanmapGen
