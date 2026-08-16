// LayerEditor_UI_Test.cpp — tab-rebuild B acceptance, part 1: the scalar catalogue, the editor
// state and the value mirrors. Every check drives the PURE functions with synthetic values, so no
// imgui frame, no window and no GL context is needed — the same split every batch-A widget test
// relies on. This file owns main(); the row actions and one-frame signal order are
// LayerEditor_Signals_UI_Test.cpp, and the soil / erosion / gravity checks are
// LayerEditor_Erosion_UI_Test.cpp (ARCH §1.5 — one binary, three translation units).
#include "LayerEditor_TestSupport_UI.h"
#include "LayerEditor_UI.h"

using namespace SanmapGen;
using namespace SanmapGen::Ui;

void RunLayerEditorSignalChecks();
void RunLayerEditorErosionChecks();

namespace {

// Every catalogued control must name itself and carry a usable, ordered range — the table is the
// single home of ~36 limits, so a transposed row would silently mis-bound a slider.
void RunScalarCatalogueChecks() {
    for (int scalarIndex = 0; scalarIndex < kLayerEditorScalarCount; ++scalarIndex) {
        const LayerEditorScalarDescription& description =
            LayerEditorScalarDescriptionOf(static_cast<LayerEditorScalar>(scalarIndex));
        CheckLayerEditor(description.label != nullptr && description.label[0] != '\0',
                         "every catalogued control has a label");
        CheckLayerEditor(description.range.maximumValue > description.range.minimumValue,
                         "every catalogued range is ordered and non-empty");
        CheckLayerEditor(!description.bInteger || description.range.increment >= 1.0f,
                         "an integer control snaps to whole numbers");
    }
    const LayerEditorScalarDescription& outOfRange =
        LayerEditorScalarDescriptionOf(static_cast<LayerEditorScalar>(kLayerEditorScalarCount + 7));
    CheckLayerEditor(outOfRange.label[0] == '\0', "an out-of-range enumerator answers an empty row");

    const LayerEditorScalarDescription& droplets =
        LayerEditorScalarDescriptionOf(LayerEditorScalar::ErosionDropletCount);
    CheckLayerEditor(droplets.range.minimumValue == 1000.0f && droplets.range.maximumValue == 5000000.0f,
                     "Droplet Count carries the plan's 1k-5M limits");
}

void RunEditorStateChecks() {
    LayerEditorState state;
    CheckLayerEditor(state.scalarRanges[static_cast<int>(LayerEditorScalar::Opacity)].maximumValue == 1.0f,
                     "the state seeds its ranges from the catalogue");
    CheckLayerEditor(!state.advancedConstantsSection.bOpen, "Advanced (constants) starts collapsed");
    CheckLayerEditor(state.noiseSection.bOpen, "the working sections start open");
    CheckLayerEditor(state.importRawOptions.allowedExtensions != nullptr,
                     "the Import RAW picker carries an extension fence");
    CheckLayerEditor(!StoredFilePathIsAllowed(std::string("terrain.png"), state.importRawOptions),
                     "and it rejects a non-RAW file");
    CheckLayerEditor(StoredFilePathIsAllowed(std::string("heightmap.raw"), state.importRawOptions),
                     "while a RAW heightmap passes");
    CheckLayerEditor(state.soilPresetIndex == -1, "no soil preset is picked to begin with");

    // Two editors on screen at once must not share a drag — the v1 function-static defect.
    LayerEditorState secondEditor;
    secondEditor.selectedLayerIndex = 4;
    CheckLayerEditor(state.selectedLayerIndex == 0, "each editor owns its own selection");
}

// The Levels and height-mask mirrors: the layer stores five loose floats plus a pair, the widgets
// want two structs, and a round trip must not move a value.
void RunValueMirrorChecks() {
    LayerEditorState state;
    Params::Layer layer;
    layer.levelsShadows      = 0.2f;
    layer.levelsMidtones     = 1.5f;
    layer.levelsHighlights   = 0.9f;
    layer.heightBlendMinimum = 0.25f;
    layer.heightBlendMaximum = 0.75f;
    LoadLayerEditorValues(layer, state);
    CheckLayerEditor(state.levelsValues.inputMidtones == 1.5f, "the Levels mirror loaded the gamma");
    CheckLayerEditor(state.heightMaskValues.maximumValue == 0.75f, "the height mask mirror loaded");
    CheckLayerEditor(!StoreLayerEditorValues(state, layer), "an untouched round trip moves nothing");

    state.levelsValues.inputShadows = 0.4f;
    CheckLayerEditor(StoreLayerEditorValues(state, layer), "a moved value reports the recipe moved");
    CheckLayerEditor(layer.levelsShadows == 0.4f, "and reaches the layer");

    state.heightMaskValues.minimumValue = 0.95f;      // crosses its partner
    StoreLayerEditorValues(state, layer);
    CheckLayerEditor(layer.heightBlendMinimum < layer.heightBlendMaximum,
                     "the stored mask pair is repaired into order");

    state.levelsValues.inputMidtones = 99.0f;         // past the 0.01..9.99 limit
    StoreLayerEditorValues(state, layer);
    CheckLayerEditor(layer.levelsMidtones == state.levelsBounds.midtonesMaximum,
                     "and the stored gamma is held inside the Levels limits");
}

} // namespace

int main() {
    RunScalarCatalogueChecks();
    RunEditorStateChecks();
    RunValueMirrorChecks();
    RunLayerEditorSignalChecks();
    RunLayerEditorErosionChecks();
    return ReportLayerEditorTestResult();
}
