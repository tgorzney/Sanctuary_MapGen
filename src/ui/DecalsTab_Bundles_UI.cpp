// DecalsTab_Bundles_UI.cpp — the tree mechanics, delete logic, leaf/node body and header-extra
// controls for the Decals Group/Bundle tree (ARCH §20). Mirrors PropsTab_Bundles_UI.cpp minus the
// Type Section filtering — see DecalsTab_Bundles_UI.h for the full scope note.
#include "DecalsTab_Bundles_UI.h"
#include "Checkbox_UI.h"
#include "DecalsTab_Manual_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"
#include <algorithm>
#include <unordered_map>

namespace SanmapGen {
namespace Ui {
namespace {

void CollectBundleAndDescendants(int bundleIdentifier, const std::vector<Params::DecalLayerBundle>& bundles,
                                 std::vector<int>& outIdentifiers) {
    outIdentifiers.push_back(bundleIdentifier);
    for (const Params::DecalLayerBundle& candidate : bundles)
        if (candidate.parentBundleIdentifier == bundleIdentifier)
            CollectBundleAndDescendants(candidate.identifier, bundles, outIdentifiers);
}

bool Contains(const std::vector<int>& values, int value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

} // namespace

void DeleteDecalLayerBundleGroupOnly(int bundleIdentifier, std::vector<Params::DecalLayerBundle>& bundles,
                                     std::vector<Params::DecalInstanceLayer>& decalLayers) {
    const auto bundleIt = std::find_if(bundles.begin(), bundles.end(),
        [&](const Params::DecalLayerBundle& candidate) { return candidate.identifier == bundleIdentifier; });
    if (bundleIt == bundles.end()) return;
    const int parentIdentifier = bundleIt->parentBundleIdentifier;
    for (Params::DecalLayerBundle& candidate : bundles)
        if (candidate.parentBundleIdentifier == bundleIdentifier) candidate.parentBundleIdentifier = parentIdentifier;
    for (Params::DecalInstanceLayer& layer : decalLayers)
        if (layer.parentBundleIdentifier == bundleIdentifier) layer.parentBundleIdentifier = parentIdentifier;
    bundles.erase(bundleIt);
}

void DeleteDecalLayerBundleCascade(int bundleIdentifier, std::vector<Params::DecalLayerBundle>& bundles,
                                   std::vector<Params::DecalInstanceLayer>& decalLayers,
                                   std::vector<Params::DecalInstanceGroup>& decals) {
    std::vector<int> deletedBundleIdentifiers;
    CollectBundleAndDescendants(bundleIdentifier, bundles, deletedBundleIdentifiers);

    for (int layerIndex = static_cast<int>(decalLayers.size()) - 1; layerIndex >= 0; --layerIndex)
        if (Contains(deletedBundleIdentifiers, decalLayers[static_cast<std::size_t>(layerIndex)].parentBundleIdentifier))
            DeleteDecalInstanceLayerCascade(layerIndex, decalLayers, decals);

    for (int i = static_cast<int>(bundles.size()) - 1; i >= 0; --i)
        if (Contains(deletedBundleIdentifiers, bundles[static_cast<std::size_t>(i)].identifier))
            bundles.erase(bundles.begin() + i);
}

void DeleteDecalInstanceLayerOnly(int layerIndex, std::vector<Params::DecalInstanceLayer>& decalLayers,
                                  std::vector<Params::DecalInstanceGroup>& decals) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(decalLayers.size())) return;
    ClampDecalLayerIndicesForRemovedLayer(decals, layerIndex);
    decalLayers.erase(decalLayers.begin() + layerIndex);
}

void DeleteDecalInstanceLayerCascade(int layerIndex, std::vector<Params::DecalInstanceLayer>& decalLayers,
                                     std::vector<Params::DecalInstanceGroup>& decals) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(decalLayers.size())) return;
    for (Params::DecalInstanceGroup& group : decals)
        for (int t = static_cast<int>(group.transforms.size()) - 1; t >= 0; --t)
            if (group.transforms[static_cast<std::size_t>(t)].layerIndex == layerIndex)
                group.transforms.erase(group.transforms.begin() + t);
    for (Params::DecalInstanceGroup& group : decals)
        for (Params::DecalTransform& transform : group.transforms)
            if (transform.layerIndex > layerIndex) --transform.layerIndex;
    decalLayers.erase(decalLayers.begin() + layerIndex);
}

namespace {

float VisibleButtonWidth(const char* label) {
    return ImGui::CalcTextSize(label, nullptr, true).x + ImGui::GetStyle().FramePadding.x * 2.0f;
}

bool DrawRightAlignedDeleteButton(const char* label) {
    const float buttonWidth = VisibleButtonWidth(label);
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    if (availableWidth > buttonWidth)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - buttonWidth);
    return ImGui::SmallButton(label);
}

void DrawDecalLayerLeafBody(Params::DecalInstanceLayer& layer, ManualDecalLayersState& state) {
    if (!state.bUseGroupColor)
        DrawColorSwatch("Color", layer.color, state.previewColorOptions, state.selectedLayerColorToggle);
    DrawSliderScalar("Icon Scale", layer.iconScale, state.iconScaleRange,
                     state.selectedLayerIconScaleToggle, WidgetStyle(), "%.2f");
    DrawCheckbox("Locked", layer.bLocked);
    DrawCheckbox("Hidden", layer.bHidden);
    DrawCheckbox("Snap to Grid", layer.bGridSnapEnabled);
    if (layer.bGridSnapEnabled)
        DrawSliderScalar("Grid Size", layer.gridSnapSizeWorldUnits, state.gridSnapSizeRange,
                         state.selectedLayerGridSnapToggle, WidgetStyle(), "%.2f");
    DrawCheckbox("Color Override", layer.bColorOverrideEnabled);
    DrawCheckbox("Symmetry Enabled", layer.bSymmetryEnabled);
}

bool DrawDecalBundleHeaderNameOverlay(int bundleIdentifier, std::vector<Params::DecalLayerBundle>& bundles,
                                      DecalLayerBundlesState& state) {
    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const float labelStartX = itemMin.x + ImGui::GetTreeNodeToLabelSpacing();

    if (state.renamingBundleIdentifier == bundleIdentifier) {
        for (Params::DecalLayerBundle& bundle : bundles) {
            if (bundle.identifier != bundleIdentifier) continue;
            ImGui::SetCursorScreenPos(ImVec2(labelStartX, itemMin.y));
            if (state.bRenameBundleFocusPending) { ImGui::SetKeyboardFocusHere(); state.bRenameBundleFocusPending = false; }
            TextInputRules nameRules;
            nameRules.maximumLength = 48; nameRules.bAllowEmpty = false; nameRules.fallbackText = "Group";
            DrawTextInput("##renameGroup", state.renameBundleScratchText, nameRules, WidgetStyle(), nullptr,
                         /*bLabelHidden=*/true);
            if (ImGui::IsItemDeactivated()) {
                bundle.name = SanitizeTextInput(state.renameBundleScratchText, nameRules);
                state.renamingBundleIdentifier = -1;
            }
            return true;
        }
        state.renamingBundleIdentifier = -1;
        return true;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        state.renamingBundleIdentifier  = bundleIdentifier;
        state.bRenameBundleFocusPending = true;
        for (const Params::DecalLayerBundle& bundle : bundles)
            if (bundle.identifier == bundleIdentifier) { state.renameBundleScratchText = bundle.name; break; }
        return true;
    }
    return false;
}

bool DrawDecalLayerHeaderNameOverlay(int layerIndex, Params::DecalInstanceLayer& layer, DecalLayerBundlesState& state) {
    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const float labelStartX = itemMin.x + ImGui::GetTreeNodeToLabelSpacing();

    if (state.renamingLayerIndex == layerIndex) {
        ImGui::SetCursorScreenPos(ImVec2(labelStartX, itemMin.y));
        if (state.bRenameLayerFocusPending) { ImGui::SetKeyboardFocusHere(); state.bRenameLayerFocusPending = false; }
        TextInputRules nameRules;
        nameRules.maximumLength = 48; nameRules.bAllowEmpty = false; nameRules.fallbackText = "Decal Layer";
        DrawTextInput("##renameLayer", state.renameLayerScratchText, nameRules, WidgetStyle(), nullptr,
                     /*bLabelHidden=*/true);
        if (ImGui::IsItemDeactivated()) {
            layer.name = SanitizeTextInput(state.renameLayerScratchText, nameRules);
            state.renamingLayerIndex = -1;
        }
        return true;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        state.renamingLayerIndex       = layerIndex;
        state.renameLayerScratchText   = layer.name;
        state.bRenameLayerFocusPending = true;
        return true;
    }
    return false;
}

void DrawDecalBundleNodeHeaderExtra(int bundleIdentifier, std::vector<Params::DecalLayerBundle>& bundles,
                                    DecalLayerBundlesState& state) {
    if (DrawDecalBundleHeaderNameOverlay(bundleIdentifier, bundles, state)) return;
    if (DrawRightAlignedDeleteButton("X##deleteGroup")) ImGui::OpenPopup("deleteDecalGroupPopup");
    if (ImGui::BeginPopup("deleteDecalGroupPopup")) {
        if (ImGui::MenuItem("Delete Group Only (keep contents)")) {
            state.pendingDeleteBundleIdentifier = bundleIdentifier;
            state.bPendingDeleteBundleCascade   = false;
        }
        if (ImGui::MenuItem("Delete All (group + contents)")) {
            state.pendingDeleteBundleIdentifier = bundleIdentifier;
            state.bPendingDeleteBundleCascade   = true;
        }
        ImGui::EndPopup();
    }
}

void DrawDecalLayerLeafHeaderExtra(int layerIndex, std::vector<Params::DecalInstanceLayer>& decalLayers,
                                   DecalLayerBundlesState& state) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(decalLayers.size())) return;
    Params::DecalInstanceLayer& layer = decalLayers[static_cast<std::size_t>(layerIndex)];
    if (DrawDecalLayerHeaderNameOverlay(layerIndex, layer, state)) return;
    if (DrawRightAlignedDeleteButton("X##deleteLayer")) ImGui::OpenPopup("deleteDecalLayerPopup");
    if (ImGui::BeginPopup("deleteDecalLayerPopup")) {
        if (ImGui::MenuItem("Delete Layer Only (keep instances)")) {
            state.pendingDeleteLayerIndex    = layerIndex;
            state.bPendingDeleteLayerCascade = false;
        }
        if (ImGui::MenuItem("Delete All (layer + instances)")) {
            state.pendingDeleteLayerIndex    = layerIndex;
            state.bPendingDeleteLayerCascade = true;
        }
        ImGui::EndPopup();
    }
}

std::unordered_map<int, std::vector<int>> BuildDecalLayerBundleLeafIndex(
        const std::vector<Params::DecalInstanceLayer>& decalLayers) {
    std::unordered_map<int, std::vector<int>> leavesByBundleIdentifier;
    for (int i = 0; i < static_cast<int>(decalLayers.size()); ++i)
        if (decalLayers[static_cast<std::size_t>(i)].parentBundleIdentifier >= 0)
            leavesByBundleIdentifier[decalLayers[static_cast<std::size_t>(i)].parentBundleIdentifier].push_back(i);
    return leavesByBundleIdentifier;
}

void ApplyDecalLayerBundleTreeSignal(const TreeListSignal<int>& signal, std::vector<Params::DecalLayerBundle>& bundles,
                                     std::vector<Params::DecalInstanceLayer>& decalLayers, DecalLayerBundlesState& state) {
    if (signal.kind == TreeListSignalKind::Select) {
        if (signal.sourceKind == TreeNodeSourceKind::Node) state.selectedBundleIdentifier = signal.sourceNodeIdentifier;
        else state.selectedLeafLayerIndex = signal.sourceLeaf;
    }

    if (signal.kind == TreeListSignalKind::Reparent) {
        if (signal.sourceKind == TreeNodeSourceKind::Leaf) {
            if (signal.sourceLeaf >= 0 && signal.sourceLeaf < static_cast<int>(decalLayers.size()))
                decalLayers[static_cast<std::size_t>(signal.sourceLeaf)].parentBundleIdentifier = signal.targetNodeIdentifier;
        } else if (!Params::WouldReparentDecalLayerBundleCreateCycle(
                      signal.sourceNodeIdentifier, signal.targetNodeIdentifier, bundles)) {
            int newParent = signal.targetNodeIdentifier;
            if (signal.dropZone != TreeDropZone::OnAsChild) {
                newParent = -1;
                for (const Params::DecalLayerBundle& target : bundles)
                    if (target.identifier == signal.targetNodeIdentifier) { newParent = target.parentBundleIdentifier; break; }
            }
            for (Params::DecalLayerBundle& bundle : bundles)
                if (bundle.identifier == signal.sourceNodeIdentifier) { bundle.parentBundleIdentifier = newParent; break; }
        }
    }
}

} // namespace

void DrawDecalLayerBundleTree(std::vector<Params::DecalLayerBundle>& bundles,
                              std::vector<Params::DecalInstanceLayer>& decalLayers,
                              std::vector<Params::DecalInstanceGroup>& decals,
                              DecalLayerBundlesState& state, ManualDecalLayersState& manualLayersState) {
    const std::unordered_map<int, std::vector<int>> leafIndex = BuildDecalLayerBundleLeafIndex(decalLayers);

    const TreeListSignal<int> signal = TreeListWidget_UI<Params::DecalLayerBundle, int>::Render(
        "decalLayerBundles", bundles,
        [](const Params::DecalLayerBundle& bundle) { return bundle.identifier; },
        [](const Params::DecalLayerBundle& bundle) { return bundle.parentBundleIdentifier; },
        [](const Params::DecalLayerBundle& bundle) { return bundle.name.empty() ? "Group" : bundle.name.c_str(); },
        [](int) {},
        [&](int bundleIdentifier) -> const std::vector<int>& {
            static const std::vector<int> kNoLeaves;
            const auto it = leafIndex.find(bundleIdentifier);
            return it != leafIndex.end() ? it->second : kNoLeaves;
        },
        [&](int layerIndex) {
            return (layerIndex >= 0 && layerIndex < static_cast<int>(decalLayers.size()))
                 ? ManualDecalLayerRowLabel(decalLayers[static_cast<std::size_t>(layerIndex)]) : "Decal Layer";
        },
        [&](int layerIndex) {
            if (layerIndex < 0 || layerIndex >= static_cast<int>(decalLayers.size())) return;
            DrawDecalLayerLeafBody(decalLayers[static_cast<std::size_t>(layerIndex)], manualLayersState);
        },
        [&](int bundleIdentifier) { DrawDecalBundleNodeHeaderExtra(bundleIdentifier, bundles, state); },
        [&](int layerIndex) { DrawDecalLayerLeafHeaderExtra(layerIndex, decalLayers, state); },
        VisibleButtonWidth("X##deleteGroup"),
        state.treeState, state.selectedBundleIdentifier, state.selectedLeafLayerIndex);

    ApplyDecalLayerBundleTreeSignal(signal, bundles, decalLayers, state);

    if (state.pendingDeleteBundleIdentifier >= 0) {
        const int deletedBundleIdentifier = state.pendingDeleteBundleIdentifier;
        if (state.bPendingDeleteBundleCascade) DeleteDecalLayerBundleCascade(deletedBundleIdentifier, bundles, decalLayers, decals);
        else DeleteDecalLayerBundleGroupOnly(deletedBundleIdentifier, bundles, decalLayers);
        if (state.selectedBundleIdentifier == deletedBundleIdentifier) state.selectedBundleIdentifier = -1;
        state.pendingDeleteBundleIdentifier = -1;
    }
    if (state.pendingDeleteLayerIndex >= 0) {
        const int deletedLayerIndex = state.pendingDeleteLayerIndex;
        if (state.bPendingDeleteLayerCascade) DeleteDecalInstanceLayerCascade(deletedLayerIndex, decalLayers, decals);
        else DeleteDecalInstanceLayerOnly(deletedLayerIndex, decalLayers, decals);
        if (manualLayersState.selectedLayerIndex == deletedLayerIndex) manualLayersState.selectedLayerIndex = -1;
        else if (manualLayersState.selectedLayerIndex > deletedLayerIndex) --manualLayersState.selectedLayerIndex;
        if (state.selectedLeafLayerIndex == deletedLayerIndex) state.selectedLeafLayerIndex = -1;
        state.pendingDeleteLayerIndex = -1;
    }
}

} // namespace Ui
} // namespace SanmapGen
