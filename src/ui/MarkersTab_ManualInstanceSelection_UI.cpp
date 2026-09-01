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

void TagManualInstancesWithLink(std::vector<Params::MarkerInstanceGroup>& markers,
                                const std::vector<int>& taggedIdentifiers, int linkIdentifier) {
    for (Params::MarkerInstanceGroup& group : markers)
        for (Params::MarkerTransform& transform : group.transforms)
            if (IsManualInstanceSelected(taggedIdentifiers, transform.instanceIdentifier))
                transform.linkIdentifier = linkIdentifier;
}

bool IsAnyManualInstanceSelectionAlreadyLinked(const std::vector<Params::MarkerInstanceGroup>& markers,
                                               const std::vector<int>& selectedIdentifiers) {
    for (const int identifier : selectedIdentifiers) {
        for (const Params::MarkerInstanceGroup& group : markers) {
            for (const Params::MarkerTransform& transform : group.transforms) {
                if (transform.instanceIdentifier != identifier) continue;
                if (transform.linkIdentifier >= 0) return true;
                break;
            }
        }
    }
    return false;
}

bool IsManualInstanceSelectionEntirelyType(const std::vector<Params::MarkerInstanceGroup>& markers,
                                           const std::vector<int>& selectedIdentifiers,
                                           const std::string& typeName) {
    if (selectedIdentifiers.empty()) return false;
    for (const int identifier : selectedIdentifiers) {
        bool bResolved = false;
        for (const Params::MarkerInstanceGroup& group : markers) {
            for (const Params::MarkerTransform& transform : group.transforms) {
                if (transform.instanceIdentifier != identifier) continue;
                if (Params::CanonicalMarkerTypeSectionName(group.name) != typeName) return false;
                bResolved = true;
                break;
            }
            if (bResolved) break;
        }
        if (!bResolved) return false;   // stale/out-of-range identifier -- can't be "entirely this type"
    }
    return true;
}

std::unordered_map<std::string, std::vector<int>> PartitionSelectedManualInstancesByType(
        const std::vector<Params::MarkerInstanceGroup>& markers, const std::vector<int>& selectedIdentifiers) {
    std::unordered_map<std::string, std::vector<int>> byType;
    for (const int identifier : selectedIdentifiers) {
        for (const Params::MarkerInstanceGroup& group : markers) {
            bool bResolved = false;
            for (const Params::MarkerTransform& transform : group.transforms) {
                if (transform.instanceIdentifier != identifier) continue;
                byType[Params::CanonicalMarkerTypeSectionName(group.name)].push_back(identifier);
                bResolved = true;
                break;
            }
            if (bResolved) break;
        }
    }
    return byType;
}

std::unordered_map<std::string, std::vector<std::pair<int, int>>> PartitionLinkedManualInstancesByType(
        const std::vector<Params::MarkerInstanceGroup>& markers, int linkIdentifier) {
    std::unordered_map<std::string, std::vector<std::pair<int, int>>> byType;
    for (int groupIndex = 0; groupIndex < static_cast<int>(markers.size()); ++groupIndex) {
        const Params::MarkerInstanceGroup& group = markers[static_cast<std::size_t>(groupIndex)];
        for (int transformIndex = 0; transformIndex < static_cast<int>(group.transforms.size()); ++transformIndex) {
            if (group.transforms[static_cast<std::size_t>(transformIndex)].linkIdentifier != linkIdentifier) continue;
            byType[Params::CanonicalMarkerTypeSectionName(group.name)].push_back({ groupIndex, transformIndex });
        }
    }
    return byType;
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
