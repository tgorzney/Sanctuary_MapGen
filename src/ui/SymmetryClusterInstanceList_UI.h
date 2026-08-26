// SymmetryClusterInstanceList_UI.h — the shared "draw N collapsible symmetry-cluster groups, then M
// flat rows" rendering helper (STEP132), ratified by ARCH_19_26_ManualInstanceSymmetryGrouping.md /
// ARCH_19_27_ProceduralInstanceSelectionMechanism.md's own explicit "share one rendering helper"
// instruction: the manual instance list (§19.26, DrawLayerRowBody) and the procedural instance list
// (§19.27, DrawRuleSettings) group a caller-owned item list by an int "symmetry group identifier"
// the caller resolves per item, then partition those groups into "real clusters" (rendered FIRST,
// each its own collapsible `ImGui::TreeNodeEx` node labeled "Symmetry Group N (k)", N = the group
// id, k = member count) versus "free/ungrouped" items (listed flat, individually, after every
// cluster, in the SAME relative order the caller's own item list carries them in). The ONE thing
// that differs between the two rulings — `== 0` is ungrouped for manual (§19.26), bucket SIZE == 1
// is ungrouped for procedural regardless of id (§19.27) — is the caller-supplied `isCluster`
// predicate; nothing else about this helper knows or cares which ruling is calling it.
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

// `groupIdentifierOf(item)` resolves an item's own group id; `isCluster(groupIdentifier, bucketSize)`
// decides whether that whole bucket renders as a collapsible cluster or contributes its members to
// the flat tail; `drawRow(item)` is the caller's own existing per-item row body (Selectable + click
// handling), called unchanged for every item, cluster or flat. Item order within a cluster, and flat
// row order among themselves, both preserve `items`' own original order.
template <typename Item, typename GroupIdentifierOfFn, typename IsClusterFn, typename DrawRowFn>
void DrawSymmetryClusterInstanceList(const std::vector<Item>& items, GroupIdentifierOfFn groupIdentifierOf,
                                     IsClusterFn isCluster, DrawRowFn drawRow) {
    std::unordered_map<int, std::vector<Item>> bucketsByGroupIdentifier;
    std::vector<int> groupOrder;   // first-appearance order — a stable, deterministic cluster order
    for (const Item& item : items) {
        const int groupIdentifier = groupIdentifierOf(item);
        std::vector<Item>& bucket = bucketsByGroupIdentifier[groupIdentifier];
        if (bucket.empty()) groupOrder.push_back(groupIdentifier);
        bucket.push_back(item);
    }

    for (const int groupIdentifier : groupOrder) {
        const std::vector<Item>& bucket = bucketsByGroupIdentifier.at(groupIdentifier);
        if (!isCluster(groupIdentifier, static_cast<int>(bucket.size()))) continue;
        ImGui::PushID(groupIdentifier);
        const std::string label = "Symmetry Group " + std::to_string(groupIdentifier)
            + " (" + std::to_string(bucket.size()) + ")";
        if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const Item& item : bucket) drawRow(item);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    for (const Item& item : items) {
        const int groupIdentifier = groupIdentifierOf(item);
        if (isCluster(groupIdentifier, static_cast<int>(bucketsByGroupIdentifier.at(groupIdentifier).size())))
            continue;
        drawRow(item);
    }
}

} // namespace Ui
} // namespace SanmapGen
