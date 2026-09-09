// MarkersTab_BaseInstanceList_UI.cpp — see MarkersTab_BaseInstanceList_UI.h.
#include "MarkersTab_BaseInstanceList_UI.h"
#include "MarkersTab_ManualInstanceSelection_UI.h"
#include "MarkersTab_ManualLayerHelpers_UI.h"
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "SymmetryClusterInstanceList_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

void DrawBaseSectionManualInstanceList(std::vector<Params::MarkerInstanceGroup>& markers,
                                       const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                       const std::string& typeName, int& selectedManualInstanceIdentifier,
                                       std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
                                       const std::function<void(int clickedInstanceIdentifier,
                                                                const std::vector<int>& selectedInstanceIdentifiers)>&
                                           selectManualMarkerInstanceCallback) {
    std::vector<std::pair<int, int>> baseInstances;
    for (int groupIndex = 0; groupIndex < static_cast<int>(markers.size()); ++groupIndex) {
        Params::MarkerInstanceGroup& group = markers[static_cast<std::size_t>(groupIndex)];
        // Alias-folded (Params::CanonicalMarkerTypeSectionName) — a real import's plural group name
        // ("Alloys") must still land in the singular "Alloy" Type-section, human's own bug report.
        if (Params::CanonicalMarkerTypeSectionName(group.name) != typeName) continue;
        for (int transformIndex = 0; transformIndex < static_cast<int>(group.transforms.size()); ++transformIndex) {
            const int layerIndex = group.transforms[static_cast<std::size_t>(transformIndex)].layerIndex;
            const bool bHasOwnTypeLayer = layerIndex >= 0 && layerIndex < static_cast<int>(markerLayers.size())
                && markerLayers[static_cast<std::size_t>(layerIndex)].markerTypeName == typeName;
            if (!bHasOwnTypeLayer) baseInstances.push_back({groupIndex, transformIndex});
        }
    }
    ImGui::TextUnformatted("Instances");
    // STEP146 (human's own bug report — dragging an instance out of a Layer onto this base list did
    // nothing) — attached to the "Instances" text itself, not gated behind `!baseInstances.empty()`
    // below, so the list is a real drop target even while empty (the common starting case: no
    // unassigned instances yet). `DrawManualLayerInstanceDropTarget` already accepts ANY layerIndex
    // with no bounds-check of its own (MarkersTab_ManualInstanceSelection_UI.cpp) — passing -1 here
    // reassigns the dropped instance(s) to "no layer of my own type," which `bHasOwnTypeLayer` above
    // already treats as belonging in this exact list (any negative/out-of-range/different-type
    // layerIndex does). No PARAMS/IO change needed: -1 was always a safe value to WRITE into
    // `layerIndex` (every read site bounds-checks `>= 0` first, MarkerLayerIndexRepair_UI.h and
    // friends) — the only gap was that nothing ever wrote it.
    DrawManualLayerInstanceDropTarget(-1, markers, selectedManualInstanceIdentifiers);
    if (baseInstances.empty()) { ImGui::TextDisabled("(none)"); return; }

    // STEP141 — this list's own display-order identifiers, for Shift-range selection.
    std::vector<int> rowOrder;
    rowOrder.reserve(baseInstances.size());
    for (const std::pair<int, int>& groupTransformIndex : baseInstances)
        rowOrder.push_back(markers[static_cast<std::size_t>(groupTransformIndex.first)]
            .transforms[static_cast<std::size_t>(groupTransformIndex.second)].instanceIdentifier);

    ManualInstanceRowInteractionContext_UI interaction;
    interaction.primaryIdentifier   = &selectedManualInstanceIdentifier;
    interaction.selectedIdentifiers = &selectedManualInstanceIdentifiers;
    interaction.anchorIdentifier    = &anchorIdentifier;
    interaction.rowOrder            = &rowOrder;
    interaction.selectManualMarkerInstanceCallback = selectManualMarkerInstanceCallback;

    DrawSymmetryClusterInstanceList<std::pair<int, int>>(baseInstances,
        [&](const std::pair<int, int>& groupTransformIndex) {
            return markers[static_cast<std::size_t>(groupTransformIndex.first)]
                .transforms[static_cast<std::size_t>(groupTransformIndex.second)].symmetryGroupIdentifier;
        },
        [](int groupIdentifier, int /*bucketSize*/) { return groupIdentifier != 0; },
        [&](const std::pair<int, int>& groupTransformIndex) {
            DrawManualInstanceRow(markers, groupTransformIndex, interaction);
        });
}

} // namespace Ui
} // namespace SanmapGen
