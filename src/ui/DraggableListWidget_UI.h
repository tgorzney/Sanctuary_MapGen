// DraggableListWidget_UI.h — a reorderable list: drag-drop rows, per-row visibility / lock /
// delete. Layer: UI. Accuracy class: Visual. The UI_FRAMEWORK_SPEC `RenderDraggableLayerList<T>`
// generalized for the GeoLayer / layer stacks (Params::LayerStack; M5-6 wires them up).
// Owns NO application state and MUTATES NOTHING (ARCH §3.2): Render only DETECTS what the user
// asked for and returns one DraggableListSignal, which the caller applies to its own array
// (ApplyDraggableListSignal for the structural kinds). That is what makes a reorder testable
// without a window, and it retires the legacy defect of erasing from the vector while iterating it.
// Not virtualized on purpose — ordered stacks of tens of rows where every row is a drop target;
// a 100k-row list uses VirtualListWidget_UI.
// This is the small facade every consumer includes unchanged; the wire types (including STEP200's
// `DraggableListRowLayout`) live in DraggableListWidget_Types_UI.h, the shared affordance strip +
// drag-drop detector in DraggableListWidget_RowAffordances_UI.h, and DraggableList<T> itself plus
// its two per-row layouts (Collapsible + Flat) in DraggableListWidget_RowLayout_UI.h — the ARCH
// §1.5 split that keeps each file under the ceiling now that Flat has its own logic to carry.
#pragma once
#include <utility>
#include <vector>
#include "DraggableListWidget_Types_UI.h"
#include "DraggableListWidget_RowLayout_UI.h"

namespace SanmapGen {
namespace Ui {

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

} // namespace Ui
} // namespace SanmapGen
