// LayerEditor_Layer_UI.cpp — the per-NoiseLayer sections: Noise, Density Shaping, Height Blending
// (Levels + blend mode + contrast + the height mask). Layer: UI.
// Every control is a batch-A shared widget composed through LayerEditor_Draw_UI.h; the only raw
// imgui is the label vocabulary and the section body layout.
#include "LayerEditor_Draw_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Enumerator order is load-bearing: the dropdown maps its row index straight onto the enum.
const char* const noiseTypeLabels[] = { "OpenSimplex2", "OpenSimplex2Smooth", "Cellular",
                                        "Perlin", "ValueCubic", "Value", "None" };
const char* const fractalTypeLabels[] = { "None", "FractionalBrownian", "Ridged", "PingPong" };
const char* const heightBlendModeLabels[] = { "Add", "Subtract", "Multiply", "Overlay",
                                             "Maximum", "Minimum" };

// The noise source. The plan hides this group behind "UseImage"; `Params::Layer` carries no image
// source (LayerEditor_UI.h SCOPE NOTE 1/2), so there is nothing to hide it behind yet and it is
// always drawn. The three shaping terms below Gain apply only to the fractal/cellular types that
// read them — the kernels ignore them otherwise, so they are shown rather than conditionally hidden.
void DrawNoiseSection(Params::Layer& layer, LayerEditorState& state,
                      Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Noise", state.noiseSection)) return;
    DrawLayerEditorEnumRow("Noise Type", layer.noiseType, noiseTypeLabels,
                           IM_ARRAYSIZE(noiseTypeLabels), previewDriver);
    DrawLayerEditorEnumRow("Fractal", layer.fractalType, fractalTypeLabels,
                           IM_ARRAYSIZE(fractalTypeLabels), previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::NoiseFrequency, layer.frequency, state, previewDriver);
    DrawLayerEditorIntegerRow(LayerEditorScalar::NoiseOctaveCount, layer.octaves, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::NoiseGain, layer.gain, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::NoiseLacunarity, layer.lacunarity, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::NoiseWeightedStrength, layer.weightedStrength,
                             state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::NoisePingPongStrength, layer.pingPongStrength,
                             state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::NoiseCellularJitter, layer.cellularJitter,
                             state, previewDriver);
    DrawSectionEnd();
}

void DrawDensitySection(Params::Layer& layer, LayerEditorState& state,
                        Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Density Shaping", state.densitySection)) return;
    DrawLayerEditorScalarRow(LayerEditorScalar::LandDensity, layer.landDensity, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::PlateauDensity, layer.plateauDensity, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::MountainDensity, layer.mountainDensity, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::RampDensity, layer.rampDensity, state, previewDriver);
    DrawSectionEnd();
}

// Levels and the height mask edit MIRRORS, because both controls own a value pair the layer
// stores as loose floats. The mirrors are reloaded only while nothing is mid-drag, so a deferred
// commit is never overwritten by the recipe it has not reached yet.
void DrawHeightBlendSection(Params::Layer& layer, LayerEditorState& state,
                            Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Height Blending", state.heightBlendSection)) return;
    if (!state.levelsToggle.IsCommitDeferred() && !state.heightMaskToggle.IsCommitDeferred())
        LoadLayerEditorValues(layer, state);

    WidgetChange change = DrawLevels("Levels", state.levelsValues, state.levelsBounds,
                                     state.levelsHistogram, state.levelsToggle);
    if (change.bValueChanged) StoreLayerEditorValues(state, layer);
    NotifyLayerEditorChange(change.bCommitted, previewDriver);

    DrawLayerEditorEnumRow("Blend Mode", layer.blendMode, heightBlendModeLabels,
                           IM_ARRAYSIZE(heightBlendModeLabels), previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::Opacity, layer.opacity, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::HeightBlendContrast, layer.heightBlendContrast,
                             state, previewDriver);

    change = DrawRangeSlider("Height Mask", state.heightMaskValues, state.heightMaskBounds,
                             state.heightMaskToggle);
    if (change.bValueChanged) StoreLayerEditorValues(state, layer);
    NotifyLayerEditorChange(change.bCommitted, previewDriver);
    DrawSectionEnd();
}

} // namespace

void DrawLayerEditorLayerSections(Params::Layer& layer, LayerEditorState& state,
                                  Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("layerSections");
    // The name is pure metadata no stage hashes, so a commit is NOT routed to the driver: asking
    // for a regeneration a rename cannot affect would be the "cheap tweak triggers a full regen"
    // defect UI_FRAMEWORK_SPEC lists. It is the DraggableList row's label, so it is edited here
    // and read there (LayerEditor_Group_UI.cpp).
    DrawTextInput("Name", layer.name, LayerEditorNameRules("Layer"));
    DrawLayerEditorIntegerRow(LayerEditorScalar::StratumIndex, layer.stratumIndex, state, previewDriver);
    DrawNoiseSection(layer, state, previewDriver);
    DrawDensitySection(layer, state, previewDriver);
    DrawHeightBlendSection(layer, state, previewDriver);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
