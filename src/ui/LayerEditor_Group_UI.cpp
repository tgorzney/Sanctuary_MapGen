// LayerEditor_Group_UI.cpp — one GeoLayer row's expanded body: the group settings, the group's
// layer list, and the per-row Import RAW / Duplicate / Bake affordances. Layer: UI.
// MUTATES NOTHING (the DraggableList contract): what the user asked for is recorded into the
// caller's LayerEditorFrameSignals and applied after every list has closed.
#include "LayerEditor_Draw_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {
namespace {

const char* const geoLayerModeLabels[]  = { "Material", "Shaper" };
const char* const groupBlendModeLabels[] = { "Add", "Subtract", "Multiply", "Overlay",
                                            "Maximum", "Minimum" };

// The group's own settings: rename, role, blend into the stack, erode-below, owned stratum.
void DrawGroupSettings(Params::GeoLayer& group, LayerEditorState& state,
                       Pipeline::PreviewDriver* previewDriver) {
    // A group rename is metadata no stage hashes, so — like the per-layer name — it does NOT
    // notify the driver; asking for a regeneration a rename cannot affect is the "cheap tweak
    // triggers a full regen" defect (UI_FRAMEWORK_SPEC "Known issues").
    DrawTextInput("Name", group.name, LayerEditorNameRules("GeoLayer"));
    DrawLayerEditorEnumRow("Mode", group.mode, geoLayerModeLabels,
                           IM_ARRAYSIZE(geoLayerModeLabels), previewDriver);
    DrawLayerEditorEnumRow("Group Blend Mode", group.blendMode, groupBlendModeLabels,
                           IM_ARRAYSIZE(groupBlendModeLabels), previewDriver);
    DrawLayerEditorCheckboxRow("Erode Below", group.bErodeBelow, previewDriver);
    DrawLayerEditorIntegerRow(LayerEditorScalar::GroupStratumIndex, group.stratumIndex,
                              state, previewDriver);
}

// The three row actions the plan puts on a NoiseLayer header. Drawn for the SELECTED row only, so
// the one import-path buffer the editor owns can never be shared by two live pickers at once
// (LayerEditor_UI.h SCOPE NOTE 2 — the path has no PARAMS home to live in yet).
void DrawLayerRowActions(int groupIndex, int rowIndex, LayerEditorState& state,
                         LayerEditorFrameSignals& signals) {
    const FilePathPickerResult picker =
        DrawFilePathPicker("Import RAW", state.importRawPath, state.importRawOptions);
    if (picker.change.bCommitted || picker.bBrowseRequested) {
        RecordLayerEditorAction(signals.action, LayerEditorActionKind::ImportRawRequested,
                                groupIndex, rowIndex);
        signals.action.importRawPath = state.importRawPath;
    }
    if (ImGui::SmallButton("Duplicate"))
        RecordLayerEditorAction(signals.action, LayerEditorActionKind::DuplicateLayer,
                                groupIndex, rowIndex);
    ImGui::SameLine();
    if (ImGui::SmallButton("Bake / Unbake"))
        RecordLayerEditorAction(signals.action, LayerEditorActionKind::BakeToggleRequested,
                                groupIndex, rowIndex);
}

// The group's layer list. Only the SELECTED group renders one, so a drag can never carry a row
// index from one group onto another group's list.
void DrawGroupLayerList(Params::GeoLayer& group, int groupIndex, LayerEditorState& state,
                        LayerEditorFrameSignals& signals) {
    if (ImGui::SmallButton("Add Layer"))
        RecordLayerEditorAction(signals.action, LayerEditorActionKind::AddLayer, groupIndex);
    // Borrowed by describeRow for the duration of this Render only. Wide enough for a full-length
    // name plus the stratum suffix; snprintf truncates rather than overruns either way.
    char rowLabel[72] = { 0 };
    const DraggableListSignal signal = DraggableList<Params::Layer>::Render(
        "layers", group.layers,
        [&](int rowIndex) {
            const Params::Layer& layer = group.layers[static_cast<std::size_t>(rowIndex)];
            std::snprintf(rowLabel, sizeof(rowLabel), "%s (stratum %d)",
                          LayerEditorRowLabel(layer), layer.stratumIndex);
            DraggableListRow row;
            row.label    = rowLabel;
            row.bVisible = layer.bEnabled;
            row.bLocked  = layer.bLocked;
            return row;
        },
        [&](int rowIndex) {
            if (rowIndex == state.selectedLayerIndex) DrawLayerRowActions(groupIndex, rowIndex, state, signals);
            else ImGui::TextUnformatted("Select this layer to edit it.");
        },
        state.selectedLayerIndex);
    if (signal.bHasSignal()) { signals.layerSignal = signal; signals.layerSignalGroupIndex = groupIndex; }
}

} // namespace

void DrawLayerEditorGroupBody(Params::LayerStack& layerStack, int groupIndex,
                              LayerEditorState& state, LayerEditorFrameSignals& signals,
                              Pipeline::PreviewDriver* previewDriver) {
    if (groupIndex < 0 || groupIndex >= static_cast<int>(layerStack.geoLayers.size())) return;
    Params::GeoLayer& group = layerStack.geoLayers[static_cast<std::size_t>(groupIndex)];
    if (groupIndex != state.selectedGeoLayerIndex) {
        ImGui::Text("%d layer(s) - select this group to edit them",
                    static_cast<int>(group.layers.size()));
        return;
    }
    DrawGroupSettings(group, state, previewDriver);
    ImGui::Separator();
    DrawGroupLayerList(group, groupIndex, state, signals);
}

} // namespace Ui
} // namespace SanmapGen
