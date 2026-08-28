// PropsTab_Manual_UI.cpp — the imgui composition of the manual prop layers block, plus (ARCH §20)
// its two Type Sections ("Prop"/"Reclaim") and the Group/Bundle tree each one hosts. Layer: UI.
// Shared widgets only: DraggableList for the ungrouped-layer stack, TreeListWidget for the Bundle
// tree (PropsTab_Bundles_UI.h), VirtualList for the read-only transform list, Checkbox / ColorSwatch
// / SliderScalar / TextInput / Section for the rest.
// Nothing here notifies Pipeline::PreviewDriver (PropsTab_Manual_UI.h SCOPE NOTE 1).
#include "PropsTab_Manual_UI.h"
#include "Checkbox_UI.h"
#include "DraggableListWidget_UI.h"
#include "TextInput_UI.h"
#include "VirtualListWidget_UI.h"
#include "../data/PlacementInstances_DATA.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

// The block-wide settings: one shared tint for every layer, and the icon scale the whole layer
// stack draws at.
void DrawLayerSettings(ManualPropLayersState& state) {
    DrawCheckbox("Use Group Color", state.bUseGroupColor);
    if (state.bUseGroupColor)
        DrawColorSwatch("Group Color", state.groupColor, state.previewColorOptions,
                        state.groupColorToggle);
    DrawSliderScalar("Layer Icon Scale", state.layerIconScale, state.iconScaleRange,
                     state.layerIconScaleToggle, WidgetStyle(), "%.2f");
}

// One row's own name, tint, icon scale, and (ARCH §20) lock/hidden/grid-snap/color-override/
// symmetry-enabled. The tint is hidden while the block is set to one shared color: two live
// controls over one drawn color would be a rival control (ARCH §4). Reports whether the name
// committed, the only field the uniqueness repair cares about.
bool DrawLayerRowSettings(Params::PropInstanceLayer& layer, ManualPropLayersState& state) {
    TextInputRules nameRules;
    nameRules.maximumLength = 48;
    nameRules.bAllowEmpty   = false;
    nameRules.fallbackText  = "Prop Layer";
    const bool bNameCommitted = DrawTextInput("Name", layer.name, nameRules).bCommitted;
    if (!state.bUseGroupColor)
        DrawColorSwatch("Color", layer.color, state.previewColorOptions, state.selectedLayerColorToggle);
    DrawSliderScalar("Icon Scale", layer.iconScale, state.iconScaleRange,
                     state.selectedLayerIconScaleToggle, WidgetStyle(), "%.2f");
    DrawCheckbox("Hidden", layer.bHidden);
    DrawCheckbox("Snap to Grid", layer.bGridSnapEnabled);
    if (layer.bGridSnapEnabled)
        DrawSliderScalar("Grid Size", layer.gridSnapSizeWorldUnits, state.gridSnapSizeRange,
                         state.selectedLayerGridSnapToggle, WidgetStyle(), "%.2f");
    DrawCheckbox("Color Override", layer.bColorOverrideEnabled);
    DrawCheckbox("Symmetry Enabled", layer.bSymmetryEnabled);
    return bNameCommitted;
}

// The "Ungrouped" layer stack for ONE Type Section — layers belonging to a Bundle (shown in the
// tree instead) or a different type are suppressed (bRowSuppressed), never filtered into a copy —
// reorder/delete still apply against real `propLayers` indices. `bAnyNameCommitted` is set (never
// cleared) when any row's name commits this frame.
DraggableListSignal DrawLayerList(std::vector<Params::PropInstanceLayer>& propLayers,
                                  ManualPropLayersState& state, const std::string& propTypeNameFilter,
                                  bool& bAnyNameCommitted) {
    return DraggableList<Params::PropInstanceLayer>::Render(
        "manualPropLayers", propLayers,
        [&](int rowIndex) {
            const Params::PropInstanceLayer& layer = propLayers[static_cast<std::size_t>(rowIndex)];
            DraggableListRow row;
            row.bRowSuppressed = IsPropInstanceLayerRowSuppressed(layer, propTypeNameFilter);
            row.label   = ManualPropLayerRowLabel(layer);
            row.bLocked = layer.bLocked;
            row.bVisible = !layer.bHidden;
            return row;
        },
        [&](int rowIndex) {
            Params::PropInstanceLayer& layer = propLayers[static_cast<std::size_t>(rowIndex)];
            if (DrawLayerRowSettings(layer, state)) bAnyNameCommitted = true;
        },
        state.selectedLayerIndex);
}

// A removed layer clamps every referencing `layerIndex` to 0 rather than dropping the prop instance
// (ClampPropLayerIndicesForRemovedLayer, STEP22 ruling #5); a reordered layer renumbers them
// (RenumberPropLayerIndicesForReorder, called BEFORE the layers vector itself moves — the renumber
// needs the pre-move layer count), mirroring ArmiesTab_UI.cpp's ApplyArmyListSignal. Reports whether
// `props` moved, which — unlike `recipe.unitRules`/`armyIndex` — feeds no pipeline stage either
// (PropsTab_Manual_UI.h SCOPE NOTE 1), so the caller never notifies the preview driver with it.
bool ApplyLayerListSignal(std::vector<Params::PropInstanceLayer>& propLayers,
                         std::vector<Params::PropInstanceGroup>& props, ManualPropLayersState& state,
                         const DraggableListSignal& signal) {
    if (signal.kind == DraggableListSignalKind::Select) {
        state.selectedLayerIndex = signal.sourceRowIndex;
        return false;
    }
    if (signal.kind == DraggableListSignalKind::ToggleLock) {
        if (signal.sourceRowIndex >= 0 && signal.sourceRowIndex < static_cast<int>(propLayers.size()))
            propLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bLocked =
                !propLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bLocked;
        return false;   // cosmetic-only: no structural move, same posture as Select
    }
    if (signal.kind == DraggableListSignalKind::ToggleVisibility) {
        if (signal.sourceRowIndex >= 0 && signal.sourceRowIndex < static_cast<int>(propLayers.size()))
            propLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bHidden =
                !propLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bHidden;
        return false;
    }
    const bool bDeleting            = signal.kind == DraggableListSignalKind::Delete;
    const bool bReordering          = signal.kind == DraggableListSignalKind::Reorder;
    const int  sourceLayerIndex     = signal.sourceRowIndex;
    const int  layerCountBeforeMove = static_cast<int>(propLayers.size());
    bool bPropsMoved = bReordering && RenumberPropLayerIndicesForReorder(
        props, sourceLayerIndex, signal.targetRowIndex, layerCountBeforeMove);
    if (!ApplyDraggableListSignal(propLayers, signal)) return bPropsMoved;
    if (bDeleting)
        bPropsMoved = ClampPropLayerIndicesForRemovedLayer(props, sourceLayerIndex) || bPropsMoved;
    if (state.selectedLayerIndex >= static_cast<int>(propLayers.size()))
        state.selectedLayerIndex = static_cast<int>(propLayers.size()) - 1;
    return bPropsMoved;
}

// The resolved transforms, read-only and virtualized (SCOPE NOTE 2). Unfiltered, unrelated to
// manual-layer membership: `Data::PlacementInstances` has no `layerIndex`-equivalent field.
void DrawTransformList(ManualPropLayersState& state, const Data::PlacementInstances* placedProps) {
    if (!DrawSectionBegin("Transforms (read-only)", state.transformListSection)) return;
    if (placedProps == nullptr || placedProps->IsEmpty()) {
        ImGui::TextUnformatted("No props have been generated yet.");
        DrawSectionEnd();
        return;
    }
    const int instanceCount = static_cast<int>(placedProps->Count());
    char rowLabel[80] = { 0 };
    RenderVirtualRows("manualPropTransforms", instanceCount, state.transformRowHeight,
                      state.transformListHeight, [&](int rowIndex) {
        const std::size_t instanceIndex = static_cast<std::size_t>(rowIndex);
        std::snprintf(rowLabel, sizeof(rowLabel), "%d: %.7s (%.1f, %.1f, %.1f) scale %.2f", rowIndex,
                      placedProps->templateIdentifier[instanceIndex].characters,
                      placedProps->positionX[instanceIndex], placedProps->positionY[instanceIndex],
                      placedProps->positionZ[instanceIndex], placedProps->scaleX[instanceIndex]);
        ImGui::TextUnformatted(rowLabel);
    });
    DrawSectionEnd();
}

// One Type Section's own body: "+ Group"/"+ Layer" buttons, the Bundle tree, then this type's own
// Ungrouped layer stack. Returns whether `propLayers`/`props` moved.
bool DrawPropTypeSectionBody(const char* typeName, std::vector<Params::PropLayerBundle>& propLayerBundles,
                             std::vector<Params::PropInstanceLayer>& propLayers,
                             std::vector<Params::PropInstanceGroup>& props, ManualPropLayersState& state) {
    if (ImGui::Button("+ Group")) {
        Params::PropLayerBundle bundle;
        bundle.identifier    = NextPropLayerBundleId(propLayerBundles);
        bundle.propTypeName  = typeName;
        bundle.parentBundleIdentifier = state.bundles.selectedBundleIdentifier;
        propLayerBundles.push_back(bundle);
        state.bundles.selectedBundleIdentifier = bundle.identifier;
    }
    ImGui::SameLine();
    if (ImGui::Button("+ Layer")) {
        Params::PropInstanceLayer layer;
        layer.name           = NextPropLayerName(static_cast<int>(propLayers.size()));
        layer.layerId         = NextPropLayerId(propLayers);
        layer.propTypeName    = typeName;
        layer.parentBundleIdentifier = state.bundles.selectedBundleIdentifier;
        propLayers.push_back(layer);
        state.selectedLayerIndex = static_cast<int>(propLayers.size()) - 1;
    }

    DrawPropLayerBundleTree(propLayerBundles, propLayers, props, state.bundles, state, typeName);

    bool bAnyNameCommitted = false;
    const DraggableListSignal signal = DrawLayerList(propLayers, state, typeName, bAnyNameCommitted);
    bool bPropsMoved = signal.bHasSignal() && ApplyLayerListSignal(propLayers, props, state, signal);
    bPropsMoved = bAnyNameCommitted || bPropsMoved;
    if (bPropsMoved) MakeNamesUnique(propLayers);
    return bPropsMoved;
}

} // namespace

void DrawManualPropLayers(ManualPropLayersState& state, std::vector<Params::PropInstanceLayer>& propLayers,
                          std::vector<Params::PropInstanceGroup>& props,
                          std::vector<Params::PropLayerBundle>& propLayerBundles,
                          const Data::PlacementInstances* placedProps) {
    if (!DrawSectionBegin("Manual Prop Layers", state.section)) return;
    bool bPropsMoved = false;
    for (int typeIndex = 0; typeIndex < kPropTypeSectionCount; ++typeIndex) {
        const char* const typeName = kPropTypeSectionNames[typeIndex];
        ImGui::PushID(typeName);
        if (DrawSectionBegin(typeName, state.typeSections[typeIndex])) {
            bPropsMoved = DrawPropTypeSectionBody(typeName, propLayerBundles, propLayers, props, state)
                        || bPropsMoved;
            DrawSectionEnd();
        }
        ImGui::PopID();
    }
    ImGui::Separator();
    DrawLayerSettings(state);
    DrawTransformList(state, placedProps);
    (void)bPropsMoved;   // SCOPE NOTE 1: no pipeline stage reads any of this — nothing to notify
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
