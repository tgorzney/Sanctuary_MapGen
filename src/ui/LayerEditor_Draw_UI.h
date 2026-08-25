// LayerEditor_Draw_UI.h — the four composition helpers the Layer Editor's panels share.
// Layer: UI. The editor's draw path is split across four translation units under the ARCH §1.5
// ceilings (LayerEditor_UI / _Layer_UI / _Soil_UI / _Erosion_UI); these are the pieces every one
// of them needs, written once rather than four times.
//
// Every helper composes the batch-A shared widgets (SliderScalar_UI, Checkbox_UI, Combo_UI) —
// none of them draws a control of its own, and no panel calls ImGui::SliderFloat/Checkbox/Combo
// directly (UI_FRAMEWORK_SPEC "Universal widget library").
#pragma once
#include "Checkbox_UI.h"
#include "Combo_UI.h"
#include "LayerEditor_Signals_UI.h"
#include "LayerEditor_UI.h"

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

// The ONE thing a panel does with a commit. WHICH tier it becomes is the driver's derivation from
// the stage parameter hashes, never this call site's decision.
void NotifyLayerEditorChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver);

// One catalogued slider: the label, limits, format and RT toggle all come from the enumerator, so
// a panel names a control and nothing else (LayerEditor_Scalars_UI.h).
WidgetChange DrawLayerEditorScalar(LayerEditorScalar scalar, float& value, LayerEditorState& state);
WidgetChange DrawLayerEditorScalarInteger(LayerEditorScalar scalar, int& value, LayerEditorState& state);

// The two above, with the commit already routed to the driver — the shape almost every row wants.
inline void DrawLayerEditorScalarRow(LayerEditorScalar scalar, float& value, LayerEditorState& state,
                                     Pipeline::PreviewDriver* previewDriver) {
    NotifyLayerEditorChange(DrawLayerEditorScalar(scalar, value, state).bCommitted, previewDriver);
}
inline void DrawLayerEditorIntegerRow(LayerEditorScalar scalar, int& value, LayerEditorState& state,
                                      Pipeline::PreviewDriver* previewDriver) {
    NotifyLayerEditorChange(DrawLayerEditorScalarInteger(scalar, value, state).bCommitted, previewDriver);
}

// A tick box that routes its own commit. A boolean has no drag, so it commits on the click.
inline void DrawLayerEditorCheckboxRow(const char* label, bool& value,
                                       Pipeline::PreviewDriver* previewDriver) {
    NotifyLayerEditorChange(DrawCheckbox(label, value).bCommitted, previewDriver);
}

// An enum-valued setting drawn by the shared dropdown. The enum is cast through its index, which
// is why the label table must stay in enumerator order at every call site.
template <typename EnumType>
void DrawLayerEditorEnumRow(const char* label, EnumType& value, const char* const* labels,
                            int labelCount, Pipeline::PreviewDriver* previewDriver) {
    ComboOptions options;
    options.labels = labels;
    options.count  = labelCount;
    int selectedIndex = static_cast<int>(value);
    const WidgetChange change = DrawCombo(label, selectedIndex, options);
    if (change.bValueChanged) value = static_cast<EnumType>(selectedIndex);
    NotifyLayerEditorChange(change.bCommitted, previewDriver);
}

// The label the header's Bake/Unbake affordance shows, keyed on `bBaked` alone — pure so the flip
// is assertable with no imgui frame (LayerEditor_Signals_UI_Test.cpp). The "##bakeToggle" id salt
// stays fixed across both strings so a click cannot drop imgui's active id mid-press, same
// discipline as DraggableListWidget_UI.h's own "[o]##visibility"/"[-]##visibility" icons.
inline const char* LayerEditorBakeToggleButtonLabel(bool bBaked) {
    return bBaked ? "Unbake##bakeToggle" : "Bake##bakeToggle";
}

// The per-layer panels, one translation unit each. `generationAssembler` may be null.
// Name + Stratum Index: the layer's identity row (name wide, stratum compact to its right), drawn
// ABOVE the Import RAW/Duplicate row and ALWAYS regardless of `bBaked` — a baked layer still has a
// name and still owns a stratum slot (LayerEditor_Group_UI.cpp, STEP150).
void DrawLayerEditorNameRow(Params::Layer& layer, LayerEditorState& state,
                            Pipeline::PreviewDriver* previewDriver);
void DrawLayerEditorLayerSections(Params::Layer& layer, LayerEditorState& state,
                                  Pipeline::PreviewDriver* previewDriver);
void DrawLayerEditorSoilSection(int stratumIndex, LayerEditorState& state,
                                Pipeline::GenerationAssembler* generationAssembler,
                                Pipeline::PreviewDriver* previewDriver);
void DrawLayerEditorErosionSections(int stratumIndex, LayerEditorState& state,
                                    Pipeline::GenerationAssembler* generationAssembler,
                                    Pipeline::PreviewDriver* previewDriver);
void DrawLayerEditorAdvancedSection(int stratumIndex, LayerEditorState& state,
                                    Pipeline::GenerationAssembler* generationAssembler,
                                    Pipeline::PreviewDriver* previewDriver);

// One GeoLayer row's expanded body: the group settings and — for the SELECTED group only — its
// layer list, each layer row showing its own row actions (STEP104: Import RAW / Duplicate / Bake,
// SELECTED row only) followed by its OWN noise/soil/erosion sections inline, right under its own
// header (never another row's — STEP104 Fix part 1). It MUTATES NOTHING; what the user asked for
// lands in `signals`, which the caller applies once the lists are closed (LayerEditor_Signals_UI.h).
// LayerEditor_Group_UI.cpp.
void DrawLayerEditorGroupBody(Params::LayerStack& layerStack, int groupIndex,
                              LayerEditorState& state, LayerEditorFrameSignals& signals,
                              Pipeline::GenerationAssembler* generationAssembler,
                              Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
