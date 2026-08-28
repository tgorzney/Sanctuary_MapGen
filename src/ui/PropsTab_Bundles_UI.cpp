// PropsTab_Bundles_UI.cpp — the tree mechanics, delete logic, leaf/node body and header-extra
// controls for the Props Group/Bundle tree (ARCH §20). See PropsTab_Bundles_UI.h for the scope
// trims relative to MarkersTab_Bundles_UI.h/MarkersTab_BundleHeaderExtras_UI.cpp/
// MarkersTab_BundleDelete_UI.cpp this file otherwise mirrors.
#include "PropsTab_Bundles_UI.h"
#include "Checkbox_UI.h"
#include "PropsTab_Manual_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"
#include <algorithm>
#include <unordered_map>

namespace SanmapGen {
namespace Ui {
namespace {

// Every bundle identifier equal to `bundleIdentifier` or descended from it (self-inclusive) — the
// exact set "All" reaches into. Mirrors MarkersTab_BundleDelete_UI.cpp's own CollectBundleAndDescendants.
void CollectBundleAndDescendants(int bundleIdentifier, const std::vector<Params::PropLayerBundle>& bundles,
                                 std::vector<int>& outIdentifiers) {
    outIdentifiers.push_back(bundleIdentifier);
    for (const Params::PropLayerBundle& candidate : bundles)
        if (candidate.parentBundleIdentifier == bundleIdentifier)
            CollectBundleAndDescendants(candidate.identifier, bundles, outIdentifiers);
}

bool Contains(const std::vector<int>& values, int value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

} // namespace

void DeletePropLayerBundleGroupOnly(int bundleIdentifier, std::vector<Params::PropLayerBundle>& bundles,
                                    std::vector<Params::PropInstanceLayer>& propLayers) {
    const auto bundleIt = std::find_if(bundles.begin(), bundles.end(),
        [&](const Params::PropLayerBundle& candidate) { return candidate.identifier == bundleIdentifier; });
    if (bundleIt == bundles.end()) return;
    const int parentIdentifier = bundleIt->parentBundleIdentifier;
    for (Params::PropLayerBundle& candidate : bundles)
        if (candidate.parentBundleIdentifier == bundleIdentifier) candidate.parentBundleIdentifier = parentIdentifier;
    for (Params::PropInstanceLayer& layer : propLayers)
        if (layer.parentBundleIdentifier == bundleIdentifier) layer.parentBundleIdentifier = parentIdentifier;
    bundles.erase(bundleIt);
}

void DeletePropLayerBundleCascade(int bundleIdentifier, std::vector<Params::PropLayerBundle>& bundles,
                                  std::vector<Params::PropInstanceLayer>& propLayers,
                                  std::vector<Params::PropInstanceGroup>& props) {
    std::vector<int> deletedBundleIdentifiers;
    CollectBundleAndDescendants(bundleIdentifier, bundles, deletedBundleIdentifiers);

    // Highest index first so each DeletePropInstanceLayerCascade's own index-shift math stays
    // correct for the rest.
    for (int layerIndex = static_cast<int>(propLayers.size()) - 1; layerIndex >= 0; --layerIndex)
        if (Contains(deletedBundleIdentifiers, propLayers[static_cast<std::size_t>(layerIndex)].parentBundleIdentifier))
            DeletePropInstanceLayerCascade(layerIndex, propLayers, props);

    for (int i = static_cast<int>(bundles.size()) - 1; i >= 0; --i)
        if (Contains(deletedBundleIdentifiers, bundles[static_cast<std::size_t>(i)].identifier))
            bundles.erase(bundles.begin() + i);
}

void DeletePropInstanceLayerOnly(int layerIndex, std::vector<Params::PropInstanceLayer>& propLayers,
                                 std::vector<Params::PropInstanceGroup>& props) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(propLayers.size())) return;
    ClampPropLayerIndicesForRemovedLayer(props, layerIndex);
    propLayers.erase(propLayers.begin() + layerIndex);
}

void DeletePropInstanceLayerCascade(int layerIndex, std::vector<Params::PropInstanceLayer>& propLayers,
                                    std::vector<Params::PropInstanceGroup>& props) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(propLayers.size())) return;
    for (Params::PropInstanceGroup& group : props)
        for (int t = static_cast<int>(group.transforms.size()) - 1; t >= 0; --t)
            if (group.transforms[static_cast<std::size_t>(t)].layerIndex == layerIndex)
                group.transforms.erase(group.transforms.begin() + t);
    for (Params::PropInstanceGroup& group : props)
        for (Params::PropTransform& transform : group.transforms)
            if (transform.layerIndex > layerIndex) --transform.layerIndex;
    propLayers.erase(propLayers.begin() + layerIndex);
}

namespace {

// STEP143's own fix, restated here: CalcTextSize's own `hide_text_after_double_hash` defaults to
// false, so measuring a "X##deleteGroup"-style label with the 2-arg overload counts the invisible
// "##..." suffix as visible text and over-estimates the button's own width, landing short of
// flush-right. Every measurement below passes `true` explicitly.
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

// One Layer leaf's own expanded body: color/icon-scale/lock/hidden/grid-snap/color-override/
// symmetry-enabled, all plain widgets (no header-cluster complexity — a tree leaf's expanded body
// has room a Marker-style always-visible header cluster does not need to economize). Name is NOT
// here — DrawPropLayerLeafHeaderExtra's own rename overlay owns it (see that function's comment).
void DrawPropLayerLeafBody(Params::PropInstanceLayer& layer, ManualPropLayersState& state) {
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

// Double-click-the-header rename, mirroring DrawLayerHeaderNameOverlay's exact contract
// (MarkersTab_ManualLayerRowBody_UI.cpp) — must be called FIRST in the header-extra callback,
// immediately after the row's own TreeNodeEx/CollapsingHeader (the "last item" this reads via
// GetItemRectMin, TreeListWidget_RowLayout_UI.h's own shared contract). Returns true while a
// rename is in progress THIS frame — the caller should then skip its own delete button and return.
bool DrawPropBundleHeaderNameOverlay(int bundleIdentifier, std::vector<Params::PropLayerBundle>& bundles,
                                     PropLayerBundlesState& state) {
    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const float labelStartX = itemMin.x + ImGui::GetTreeNodeToLabelSpacing();

    if (state.renamingBundleIdentifier == bundleIdentifier) {
        for (Params::PropLayerBundle& bundle : bundles) {
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
        state.renamingBundleIdentifier = -1;   // the bundle vanished (deleted elsewhere) this frame
        return true;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        state.renamingBundleIdentifier  = bundleIdentifier;
        state.bRenameBundleFocusPending = true;
        for (const Params::PropLayerBundle& bundle : bundles)
            if (bundle.identifier == bundleIdentifier) { state.renameBundleScratchText = bundle.name; break; }
        return true;
    }
    return false;
}

bool DrawPropLayerHeaderNameOverlay(int layerIndex, Params::PropInstanceLayer& layer, PropLayerBundlesState& state) {
    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const float labelStartX = itemMin.x + ImGui::GetTreeNodeToLabelSpacing();

    if (state.renamingLayerIndex == layerIndex) {
        ImGui::SetCursorScreenPos(ImVec2(labelStartX, itemMin.y));
        if (state.bRenameLayerFocusPending) { ImGui::SetKeyboardFocusHere(); state.bRenameLayerFocusPending = false; }
        TextInputRules nameRules;
        nameRules.maximumLength = 48; nameRules.bAllowEmpty = false; nameRules.fallbackText = "Prop Layer";
        DrawTextInput("##renameLayer", state.renameLayerScratchText, nameRules, WidgetStyle(), nullptr,
                     /*bLabelHidden=*/true);
        if (ImGui::IsItemDeactivated()) {
            layer.name = SanitizeTextInput(state.renameLayerScratchText, nameRules);
            state.renamingLayerIndex = -1;
        }
        return true;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        state.renamingLayerIndex        = layerIndex;
        state.renameLayerScratchText    = layer.name;
        state.bRenameLayerFocusPending  = true;
        return true;
    }
    return false;
}

// A Group's own header-extra: double-click-rename, else a right-aligned "X" delete popup
// (Group Only / All).
void DrawPropBundleNodeHeaderExtra(int bundleIdentifier, std::vector<Params::PropLayerBundle>& bundles,
                                   PropLayerBundlesState& state) {
    if (DrawPropBundleHeaderNameOverlay(bundleIdentifier, bundles, state)) return;
    if (DrawRightAlignedDeleteButton("X##deleteGroup")) ImGui::OpenPopup("deletePropGroupPopup");
    if (ImGui::BeginPopup("deletePropGroupPopup")) {
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

// A Layer leaf's own header-extra: double-click-rename, else a right-aligned "X" delete popup
// (Layer Only / All).
void DrawPropLayerLeafHeaderExtra(int layerIndex, std::vector<Params::PropInstanceLayer>& propLayers,
                                  PropLayerBundlesState& state) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(propLayers.size())) return;
    Params::PropInstanceLayer& layer = propLayers[static_cast<std::size_t>(layerIndex)];
    if (DrawPropLayerHeaderNameOverlay(layerIndex, layer, state)) return;
    if (DrawRightAlignedDeleteButton("X##deleteLayer")) ImGui::OpenPopup("deletePropLayerPopup");
    if (ImGui::BeginPopup("deletePropLayerPopup")) {
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

// The filtered COPY (mirrors BuildFilteredMarkerLayerBundlesByType) — safe to pass as
// TreeListWidget_UI's `nodes` parameter because Render uses it for tree LAYOUT only; every mutation
// path resolves the REAL Params::PropLayerBundle& by identifier-keyed lookup into the caller's own
// `bundles` vector, never by position within this copy.
std::vector<Params::PropLayerBundle> BuildFilteredPropLayerBundlesByType(
        const std::vector<Params::PropLayerBundle>& bundles, const std::string& propTypeNameFilter) {
    std::vector<Params::PropLayerBundle> filtered;
    for (const Params::PropLayerBundle& bundle : bundles)
        if (bundle.propTypeName == propTypeNameFilter) filtered.push_back(bundle);
    return filtered;
}

std::unordered_map<int, std::vector<int>> BuildPropLayerBundleLeafIndex(
        const std::vector<Params::PropInstanceLayer>& propLayers) {
    std::unordered_map<int, std::vector<int>> leavesByBundleIdentifier;
    for (int i = 0; i < static_cast<int>(propLayers.size()); ++i)
        if (propLayers[static_cast<std::size_t>(i)].parentBundleIdentifier >= 0)
            leavesByBundleIdentifier[propLayers[static_cast<std::size_t>(i)].parentBundleIdentifier].push_back(i);
    return leavesByBundleIdentifier;
}

// The Select/Reparent signal-application logic, mirroring ApplyMarkerLayerBundleTreeSignal minus
// the RuleLayer branch (doesn't exist) and the instance-multi-select cascade (gated, ARCH §20.4).
void ApplyPropLayerBundleTreeSignal(const TreeListSignal<int>& signal, std::vector<Params::PropLayerBundle>& bundles,
                                    std::vector<Params::PropInstanceLayer>& propLayers, PropLayerBundlesState& state) {
    if (signal.kind == TreeListSignalKind::Select) {
        if (signal.sourceKind == TreeNodeSourceKind::Node) state.selectedBundleIdentifier = signal.sourceNodeIdentifier;
        else state.selectedLeafLayerIndex = signal.sourceLeaf;
    }

    if (signal.kind == TreeListSignalKind::Reparent) {
        if (signal.sourceKind == TreeNodeSourceKind::Leaf) {
            if (signal.sourceLeaf >= 0 && signal.sourceLeaf < static_cast<int>(propLayers.size()))
                propLayers[static_cast<std::size_t>(signal.sourceLeaf)].parentBundleIdentifier = signal.targetNodeIdentifier;
        } else if (!Params::WouldReparentPropLayerBundleCreateCycle(
                      signal.sourceNodeIdentifier, signal.targetNodeIdentifier, bundles)) {
            int newParent = signal.targetNodeIdentifier;
            if (signal.dropZone != TreeDropZone::OnAsChild) {   // Above/Below: same parent as target (sibling)
                newParent = -1;
                for (const Params::PropLayerBundle& target : bundles)
                    if (target.identifier == signal.targetNodeIdentifier) { newParent = target.parentBundleIdentifier; break; }
            }
            for (Params::PropLayerBundle& bundle : bundles)
                if (bundle.identifier == signal.sourceNodeIdentifier) { bundle.parentBundleIdentifier = newParent; break; }
        }
    }
}

} // namespace

void DrawPropLayerBundleTree(std::vector<Params::PropLayerBundle>& bundles,
                             std::vector<Params::PropInstanceLayer>& propLayers,
                             std::vector<Params::PropInstanceGroup>& props,
                             PropLayerBundlesState& state, ManualPropLayersState& manualLayersState,
                             const std::string& propTypeNameFilter) {
    const std::vector<Params::PropLayerBundle> filteredBundles =
        BuildFilteredPropLayerBundlesByType(bundles, propTypeNameFilter);
    const std::unordered_map<int, std::vector<int>> leafIndex = BuildPropLayerBundleLeafIndex(propLayers);

    const TreeListSignal<int> signal = TreeListWidget_UI<Params::PropLayerBundle, int>::Render(
        "propLayerBundles", filteredBundles,
        [](const Params::PropLayerBundle& bundle) { return bundle.identifier; },
        [](const Params::PropLayerBundle& bundle) { return bundle.parentBundleIdentifier; },
        [](const Params::PropLayerBundle& bundle) { return bundle.name.empty() ? "Group" : bundle.name.c_str(); },
        [](int) {},   // a Bundle's own inline body — nothing to draw (mirrors Markers' current empty body)
        [&](int bundleIdentifier) -> const std::vector<int>& {
            static const std::vector<int> kNoLeaves;
            const auto it = leafIndex.find(bundleIdentifier);
            return it != leafIndex.end() ? it->second : kNoLeaves;
        },
        [&](int layerIndex) {
            return (layerIndex >= 0 && layerIndex < static_cast<int>(propLayers.size()))
                 ? ManualPropLayerRowLabel(propLayers[static_cast<std::size_t>(layerIndex)]) : "Prop Layer";
        },
        [&](int layerIndex) {
            if (layerIndex < 0 || layerIndex >= static_cast<int>(propLayers.size())) return;
            DrawPropLayerLeafBody(propLayers[static_cast<std::size_t>(layerIndex)], manualLayersState);
        },
        [&](int bundleIdentifier) { DrawPropBundleNodeHeaderExtra(bundleIdentifier, bundles, state); },
        [&](int layerIndex) { DrawPropLayerLeafHeaderExtra(layerIndex, propLayers, state); },
        VisibleButtonWidth("X##deleteGroup"),
        state.treeState, state.selectedBundleIdentifier, state.selectedLeafLayerIndex);

    ApplyPropLayerBundleTreeSignal(signal, bundles, propLayers, state);

    if (state.pendingDeleteBundleIdentifier >= 0) {
        const int deletedBundleIdentifier = state.pendingDeleteBundleIdentifier;
        if (state.bPendingDeleteBundleCascade) DeletePropLayerBundleCascade(deletedBundleIdentifier, bundles, propLayers, props);
        else DeletePropLayerBundleGroupOnly(deletedBundleIdentifier, bundles, propLayers);
        if (state.selectedBundleIdentifier == deletedBundleIdentifier) state.selectedBundleIdentifier = -1;
        state.pendingDeleteBundleIdentifier = -1;
    }
    if (state.pendingDeleteLayerIndex >= 0) {
        const int deletedLayerIndex = state.pendingDeleteLayerIndex;
        if (state.bPendingDeleteLayerCascade) DeletePropInstanceLayerCascade(deletedLayerIndex, propLayers, props);
        else DeletePropInstanceLayerOnly(deletedLayerIndex, propLayers, props);
        if (manualLayersState.selectedLayerIndex == deletedLayerIndex) manualLayersState.selectedLayerIndex = -1;
        else if (manualLayersState.selectedLayerIndex > deletedLayerIndex) --manualLayersState.selectedLayerIndex;
        if (state.selectedLeafLayerIndex == deletedLayerIndex) state.selectedLeafLayerIndex = -1;
        state.pendingDeleteLayerIndex = -1;
    }
}

} // namespace Ui
} // namespace SanmapGen
