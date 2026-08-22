// Application_ViewLayersPopup_UI.h — the View toolbar popup's pure signal-application rule, split
// out of Application_ViewLayersPopup_UI.cpp so it is testable with no imgui frame (ARCH §1.5 — the
// same posture Application_Visibility_UI.h already has for ApplyPanelVisibility). A member file of
// Application_UI.h; nothing outside the popup and its own test reaches it.
#pragma once
#include <cstddef>
#include <vector>
#include "DraggableListWidget_UI.h"

namespace SanmapGen {
namespace Ui {

// Applies ONLY Reorder + ToggleVisibility. Delete is deliberately NOT wired here — adding/removing
// a whole layer is domain-tab authoring, never the View toolbar (ARCH_14_02_DataModel.md §14.2:
// "Sub-layer authoring (add/remove/toggle) lives in each domain's own tab... never the View
// toolbar, which only orders/blends/hides whole OverlayLayer_UIs"). ToggleLock/Select are inert by
// construction: neither PreviewFieldLayer nor OverlayLayer_UI carries a lock or "selected" concept,
// so those signal kinds fall through unapplied — the exact contract DraggableListWidget_UI.h's own
// ApplyDraggableListSignal doc states ("every other kind belongs to state this widget does not own
// and is left alone"). Templated: identical shape for PreviewFieldLayer and OverlayLayer_UI, both
// of which carry a plain `bEnabled` field.
template <typename T>
bool ApplyViewLayerSignal(std::vector<T>& items, const DraggableListSignal& signal) {
    if (signal.kind == DraggableListSignalKind::ToggleVisibility) {
        if (signal.sourceRowIndex < 0 || signal.sourceRowIndex >= static_cast<int>(items.size()))
            return false;
        T& item = items[static_cast<std::size_t>(signal.sourceRowIndex)];
        item.bEnabled = !item.bEnabled;
        return true;
    }
    if (signal.kind != DraggableListSignalKind::Reorder) return false;   // Delete/Lock/Select: no-op
    return ApplyDraggableListSignal(items, signal);
}

} // namespace Ui
} // namespace SanmapGen
