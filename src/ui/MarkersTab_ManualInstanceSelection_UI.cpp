// MarkersTab_ManualInstanceSelection_UI.cpp — see MarkersTab_ManualInstanceSelection_UI.h.
#include "MarkersTab_ManualInstanceSelection_UI.h"
#include "imgui.h"
#include <algorithm>

namespace SanmapGen {
namespace Ui {

void ApplyManualInstanceSelectionClick(const std::vector<int>& rowOrder, int clickedIdentifier,
                                       bool bCtrlHeld, bool bShiftHeld,
                                       std::vector<int>& selectedIdentifiers, int& anchorIdentifier) {
    if (bShiftHeld && anchorIdentifier >= 0) {
        const auto anchorIt  = std::find(rowOrder.begin(), rowOrder.end(), anchorIdentifier);
        const auto clickedIt = std::find(rowOrder.begin(), rowOrder.end(), clickedIdentifier);
        if (anchorIt != rowOrder.end() && clickedIt != rowOrder.end()) {
            const int anchorIndex  = static_cast<int>(std::distance(rowOrder.begin(), anchorIt));
            const int clickedIndex = static_cast<int>(std::distance(rowOrder.begin(), clickedIt));
            const int lowIndex     = std::min(anchorIndex, clickedIndex);
            const int highIndex    = std::max(anchorIndex, clickedIndex);
            selectedIdentifiers.assign(rowOrder.begin() + lowIndex, rowOrder.begin() + highIndex + 1);
            return;   // anchor unchanged -- repeated Shift-clicks range from the same start
        }
    }
    if (bCtrlHeld) {
        const auto it = std::find(selectedIdentifiers.begin(), selectedIdentifiers.end(), clickedIdentifier);
        if (it != selectedIdentifiers.end()) selectedIdentifiers.erase(it);
        else selectedIdentifiers.push_back(clickedIdentifier);
        anchorIdentifier = clickedIdentifier;
        return;
    }
    selectedIdentifiers.assign(1, clickedIdentifier);
    anchorIdentifier = clickedIdentifier;
}

void ReassignManualInstanceLayers(std::vector<Params::MarkerInstanceGroup>& markers,
                                  const std::vector<int>& movedIdentifiers, int newLayerIndex) {
    for (Params::MarkerInstanceGroup& group : markers)
        for (Params::MarkerTransform& transform : group.transforms)
            if (IsManualInstanceSelected(movedIdentifiers, transform.instanceIdentifier))
                transform.layerIndex = newLayerIndex;
}

std::vector<int> DetectManualInstanceDropTarget(const std::vector<int>& selectedIdentifiers) {
    std::vector<int> movedIdentifiers;
    if (!ImGui::BeginDragDropTarget()) return movedIdentifiers;
    if (const ImGuiPayload* const payload = ImGui::AcceptDragDropPayload("markerInstanceDrag")) {
        if (payload->DataSize == static_cast<int>(sizeof(int))) {
            const int droppedIdentifier = *static_cast<const int*>(payload->Data);
            movedIdentifiers = IsManualInstanceSelected(selectedIdentifiers, droppedIdentifier)
                ? selectedIdentifiers : std::vector<int>{ droppedIdentifier };
        }
    }
    ImGui::EndDragDropTarget();
    return movedIdentifiers;
}

void DrawManualLayerInstanceDropTarget(int layerIndex, std::vector<Params::MarkerInstanceGroup>& markers,
                                       const std::vector<int>& selectedIdentifiers) {
    const std::vector<int> movedIdentifiers = DetectManualInstanceDropTarget(selectedIdentifiers);
    if (!movedIdentifiers.empty()) ReassignManualInstanceLayers(markers, movedIdentifiers, layerIndex);
}

} // namespace Ui
} // namespace SanmapGen
