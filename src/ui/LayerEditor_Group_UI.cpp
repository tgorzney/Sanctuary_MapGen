// LayerEditor_Group_UI.cpp — one GeoLayer row's expanded body: the group settings, the group's
// layer list, and each layer row's own Import RAW / Duplicate row plus its header-level Bake/
// Unbake affordance (STEP150). Layer: UI.
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

// The group's own settings: rename, role, blend into the stack, erode-below.
// STEP150: "Group Stratum Index" is no longer drawn here — `Params::GeoLayer::stratumIndex` has
// zero PROC consumer (only the per-Layer `stratumIndex` drives generation, LayerEditor_UI.h SCOPE
// NOTE and NoiseBlend_Prepare/NoiseBlend/NoiseBlend_Blend_PROC.cpp), so a second "Stratum Index"
// control beside the real, generation-driving one read as two competing controls for one concept.
// The field itself is untouched (Params::GeoLayer::stratumIndex, IO round-trip still needs it) —
// this is a UI-only removal (ARCH §8.4 — never invent OR delete a PARAMS field from a work-order
// that only asked for a draw-path change).
void DrawGroupSettings(Params::GeoLayer& group, Pipeline::PreviewDriver* previewDriver) {
    // A group rename is metadata no stage hashes, so — like the per-layer name — it does NOT
    // notify the driver; asking for a regeneration a rename cannot affect is the "cheap tweak
    // triggers a full regen" defect (UI_FRAMEWORK_SPEC "Known issues").
    DrawTextInput("Name", group.name, LayerEditorNameRules("GeoLayer"));
    DrawLayerEditorEnumRow("Mode", group.mode, geoLayerModeLabels,
                           IM_ARRAYSIZE(geoLayerModeLabels), previewDriver);
    DrawLayerEditorEnumRow("Group Blend Mode", group.blendMode, groupBlendModeLabels,
                           IM_ARRAYSIZE(groupBlendModeLabels), previewDriver);
    DrawLayerEditorCheckboxRow("Erode Below", group.bErodeBelow, previewDriver);
}

// Import RAW + Duplicate. Drawn for the SELECTED row only, so the one import-path buffer the
// editor owns can never be shared by two live pickers at once (LayerEditor_UI.h SCOPE NOTE 2 — the
// path has no PARAMS home to live in yet). Bake/Unbake moved OFF this row and onto the row's own
// header affordance (STEP150 — DrawGroupLayerList's describeRow below); it is visible on every row
// without needing to expand it, and applies through the exact same
// `LayerEditorActionKind::BakeToggleRequested` path (RecordBakeToggleFromRowSignal,
// LayerEditor_Signals_UI.h) this row used to record directly.
void DrawLayerRowActions(int groupIndex, int rowIndex, const Params::Layer& layer,
                         LayerEditorState& state, LayerEditorFrameSignals& signals) {
    // STEP150: sync from THIS layer's own bakedImagePath every frame it draws — the picker's
    // bound string is a single editor-wide scratch buffer with no per-layer home yet, so without
    // this it would keep showing whatever path some OTHER row last picked (or the empty string, if
    // none ever did), never this layer's real imported file (or genuine "None").
    state.importRawPath = layer.bakedImagePath;
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
}

// The group's layer list. Only the SELECTED group renders one, so a drag can never carry a row
// index from one group onto another group's list. STEP104: each row's body, whenever the row's
// own CollapsingHeader is open (DraggableList's own per-row expand/collapse state — never gated on
// `state.selectedLayerIndex`), draws that row's OWN noise/soil/erosion sections directly below its
// row actions, so an expanded row never shows another row's settings.
void DrawGroupLayerList(Params::GeoLayer& group, int groupIndex, LayerEditorState& state,
                        LayerEditorFrameSignals& signals,
                        Pipeline::GenerationAssembler* generationAssembler,
                        Pipeline::PreviewDriver* previewDriver) {
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
            // STEP150: a real per-row header affordance, right of the delete `X` — visible on
            // EVERY row without expanding it, labelled from `bBaked` alone.
            row.extraButtonLabel = LayerEditorBakeToggleButtonLabel(layer.bBaked);
            return row;
        },
        [&](int rowIndex) {
            Params::Layer& layer = group.layers[static_cast<std::size_t>(rowIndex)];
            // STEP150: Name + Stratum Index sit ABOVE the Import RAW/Duplicate row now — a layer's
            // identity is not a procedural setting, so it stays visible even while baked.
            DrawLayerEditorNameRow(layer, state, previewDriver);
            if (rowIndex == state.selectedLayerIndex)
                DrawLayerRowActions(groupIndex, rowIndex, layer, state, signals);
            else ImGui::TextUnformatted("Select this layer to edit it.");
            ImGui::Separator();
            // Own row, own layer, own settings — the same three panels DrawSelectedLayerPanels
            // used to draw once at the bottom for whatever was "selected" (LayerEditor_UI.cpp,
            // pre-STEP104), now drawn inline per expanded row so they can never bleed across rows.
            // STEP150: none of it does anything for a BAKED layer (NoiseBlend_PROC.cpp skips
            // rolling noise and reads the cached image instead), so it is gated behind `bBaked`
            // rather than shown as if live.
            if (!layer.bBaked) {
                DrawLayerEditorLayerSections(layer, state, previewDriver);
                DrawLayerEditorSoilSection(layer.stratumIndex, state, generationAssembler, previewDriver);
                DrawLayerEditorErosionSections(layer.stratumIndex, state, generationAssembler, previewDriver);
            } else {
                ImGui::TextUnformatted("Baked - procedural settings hidden. Unbake to edit.");
            }
        },
        state.selectedLayerIndex);
    // STEP150: the header's Bake/Unbake click arrives as the SAME kind of DraggableListSignal every
    // other row affordance does; translate it into the row action BEFORE folding the raw signal
    // into `signals.layerSignal` below (ApplyLayerListSignal, reached through
    // ApplyLayerEditorFrameSignals, does not know `ExtraButton` and safely no-ops on it either way).
    RecordBakeToggleFromRowSignal(signal, groupIndex, signals);
    if (signal.bHasSignal()) { signals.layerSignal = signal; signals.layerSignalGroupIndex = groupIndex; }
}

} // namespace

void DrawLayerEditorGroupBody(Params::LayerStack& layerStack, int groupIndex,
                              LayerEditorState& state, LayerEditorFrameSignals& signals,
                              Pipeline::GenerationAssembler* generationAssembler,
                              Pipeline::PreviewDriver* previewDriver) {
    if (groupIndex < 0 || groupIndex >= static_cast<int>(layerStack.geoLayers.size())) return;
    Params::GeoLayer& group = layerStack.geoLayers[static_cast<std::size_t>(groupIndex)];
    if (groupIndex != state.selectedGeoLayerIndex) {
        ImGui::Text("%d layer(s) - select this group to edit them",
                    static_cast<int>(group.layers.size()));
        return;
    }
    DrawGroupSettings(group, previewDriver);
    ImGui::Separator();
    DrawGroupLayerList(group, groupIndex, state, signals, generationAssembler, previewDriver);
}

} // namespace Ui
} // namespace SanmapGen
