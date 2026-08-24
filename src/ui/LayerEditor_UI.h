// LayerEditor_UI.h — the reusable GeoLayer / NoiseLayer editor. Layer: UI. Accuracy class: Visual.
// TAB_REBUILD_PLAN "§ Layer Editor": the core control the Heightmap tab hosts and that Detail
// Normal / Tint / Holes / Smoothness / Props reuse. It edits ONE recipe slice —
// `Params::LayerStack` — plus the soil/erosion/thermal constants reached through PIPELINE.
//
// THE TIER IS NOT DECIDED HERE (as in every v2 tab): a committed edit calls
// `Pipeline::PreviewDriver::NotifyParametersChanged()`, which derives bNeedsMapUpdate vs
// bNeedsPreviewRender from the stage that owns the field that moved. No per-widget flag map.
//
// WHY IT REACHES PIPELINE FOR SOIL/EROSION: soil physics and the per-stratum erosion settings have
// no `_PARAMS` home yet — `GenerationAssembler_PIPELINE.h` states the interim contract in so many
// words: "the tweakable constants each one owns are reached through these until the remaining
// *_PARAMS homes exist (UI wiring is M4/M5)". So the editor goes UI -> PIPELINE -> the stage's own
// accessor. It never includes a PROC stage, never picks a backend and never simulates (ARCH §3.1).
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing field; these are reported, not invented):
//  1. `Params::Layer` has no per-layer symmetry (`bUseGlobalSymmetry` + axis mask), no blend
//     SHARPNESS and no image BRIGHTNESS, so none of the three is drawn. (The per-layer `name`
//     this note used to list was landed by WO B2 and IS drawn, in the layer header row.)
//  2. (CLOSED by STEP102) Import RAW / Bake used to be detected, never applied — now
//     LayerEditor_BakedImage_UI.h's `ApplyBakedImageAction`, called from DrawLayerEditor
//     alongside `ApplyLayerEditorFrameSignals`, is the applier. `ApplyLayerEditorAction`
//     (LayerEditor_Action_UI.h) still refuses both kinds itself — it stays IO-free by
//     contract; the baked-image applier is the one place that legitimately needs IO.
//  3. "Base Absorption" is per-MATERIAL in v2 (soil physics), not per-erosion-layer, so it is
//     exposed exactly once, in Soil Physics. A second copy would be a rival control (ARCH §4).
#pragma once
#include <string>
#include "FilePathPicker_UI.h"
#include "LayerEditor_Action_UI.h"
#include "LayerEditor_Scalars_UI.h"
#include "LayerEditor_SoilPreset_UI.h"
#include "Levels_UI.h"
#include "RangeSliderWidget_UI.h"
#include "Section_UI.h"
#include "TextInput_UI.h"

namespace SanmapGen {
namespace Pipeline { class GenerationAssembler; class PreviewDriver; }
namespace Ui {

// Caller-owned editor state. One instance per stack on screen, so two Layer Editors in one window
// (Heightmap's GeoLayers and, later, Tint's) cannot share a drag — the v1 function-static bug.
struct LayerEditorState {
    LayerEditorState();                       // seeds the ranges (LayerEditor_Scalars_UI.cpp)

    ScalarSliderRange scalarRanges[kLayerEditorScalarCount];
    RealtimeToggle    scalarToggles[kLayerEditorScalarCount];

    SectionState noiseSection;
    SectionState densitySection;
    SectionState heightBlendSection;
    SectionState soilPhysicsSection;
    SectionState hydraulicErosionSection;
    SectionState precipitationSection;
    SectionState depositionSection;
    SectionState advancedConstantsSection;    // seeded CLOSED by the constructor

    RealtimeToggle    levelsToggle;
    LevelsSettings    levelsValues;           // Params::Layer levels* mirror
    LevelsBounds      levelsBounds;
    LevelsHistogramView levelsHistogram;      // caller-filled; empty draws an empty frame

    RealtimeToggle    heightMaskToggle;
    RangeSliderBounds heightMaskBounds{ 0.0f, 1.0f, 0.001f };
    RangeSliderValues heightMaskValues;       // heightBlendMinimum/Maximum mirror

    RealtimeToggle    spawnHeightToggle;
    RangeSliderBounds spawnHeightBounds{ 0.0f, 1.0f, 0.001f };
    RangeSliderValues spawnHeightValues;      // deposition spawn band mirror

    FilePathPickerOptions importRawOptions;   // ".raw;.r16" — seeded by the constructor
    std::string           importRawPath;      // the last path the picker answered (SCOPE NOTE 2)

    int selectedGeoLayerIndex = 0;
    int selectedLayerIndex    = 0;
    int soilPresetIndex       = -1;           // -1 = no preset picked since the last edit
};

// What a legal name is, for BOTH the GeoLayer group header and the per-layer header row —
// declared once so a group and a layer cannot disagree about the cap or the empty-name fallback.
// `fallbackText` is what an emptied field settles back to (TextInput_UI.h).
inline TextInputRules LayerEditorNameRules(const char* fallbackText) {
    TextInputRules rules;
    rules.maximumLength = 48;
    rules.bAllowEmpty   = false;
    rules.fallbackText  = fallbackText;
    return rules;
}

// The label a list row shows for one layer: its name, or the fallback when a recipe written
// before the field existed left it empty (Constitution §6 — never draw an empty row).
inline const char* LayerEditorRowLabel(const Params::Layer& layer) {
    return layer.name.empty() ? "Layer" : layer.name.c_str();
}

// recipe -> widget mirrors. Run whenever no edit is pending, so a layer selected in another tab or
// a recipe loaded from disk is picked up without the caller refreshing anything by hand.
inline void LoadLayerEditorValues(const Params::Layer& layer, LayerEditorState& state) {
    state.levelsValues.inputShadows    = layer.levelsShadows;
    state.levelsValues.inputMidtones   = layer.levelsMidtones;
    state.levelsValues.inputHighlights = layer.levelsHighlights;
    state.levelsValues.outputBlack     = layer.levelsOutputBlack;
    state.levelsValues.outputWhite     = layer.levelsOutputWhite;
    state.heightMaskValues.minimumValue = layer.heightBlendMinimum;
    state.heightMaskValues.maximumValue = layer.heightBlendMaximum;
}

// widget mirrors -> recipe. Reports whether the recipe actually moved.
inline bool StoreLayerEditorValues(const LayerEditorState& state, Params::Layer& layer) {
    const LevelsSettings levels = ClampLevelsSettings(state.levelsValues, state.levelsBounds);
    const RangeSliderValues mask = ClampRangeSliderValues(state.heightMaskValues, state.heightMaskBounds);
    const bool bMoved = levels.inputShadows != layer.levelsShadows
                     || levels.inputMidtones != layer.levelsMidtones
                     || levels.inputHighlights != layer.levelsHighlights
                     || levels.outputBlack != layer.levelsOutputBlack
                     || levels.outputWhite != layer.levelsOutputWhite
                     || mask.minimumValue != layer.heightBlendMinimum
                     || mask.maximumValue != layer.heightBlendMaximum;
    layer.levelsShadows      = levels.inputShadows;
    layer.levelsMidtones     = levels.inputMidtones;
    layer.levelsHighlights   = levels.inputHighlights;
    layer.levelsOutputBlack  = levels.outputBlack;
    layer.levelsOutputWhite  = levels.outputWhite;
    layer.heightBlendMinimum = mask.minimumValue;
    layer.heightBlendMaximum = mask.maximumValue;
    return bMoved;
}

// The layer the per-layer sections edit, or null when the selection points at nothing.
Params::Layer* SelectedLayerEditorLayer(Params::LayerStack& layerStack, const LayerEditorState& state);

// Draws the whole editor: the GeoLayer groups, each row's own noise/soil/erosion sections drawn
// inline right under that row whenever ITS OWN CollapsingHeader is open (STEP104 Fix part 1 — never
// bled from whatever else happens to be "selected"). Both pipeline pointers are nullable.
//
// `bDrawOwnAddGeoLayerButton` (default true) draws "Add GeoLayer" internally, above the list — the
// original behavior, for callers with no reserved header space (MaskLayerTab_UI.cpp's Tint/Holes/
// Smoothness/Props). A caller that reserved space via `SectionOptions::reservedRightWidth` (Fix
// part 2 — HeightmapTab_UI.cpp's "GeoLayers" section) passes false and reports its OWN click via
// `bAddGeoLayerRequestedExternally`; the action still fires from RecordLayerEditorAction — only
// WHERE the button is drawn moves.
void DrawLayerEditor(Params::LayerStack& layerStack, LayerEditorState& state,
                     Pipeline::GenerationAssembler* generationAssembler,
                     Pipeline::PreviewDriver* previewDriver,
                     bool bDrawOwnAddGeoLayerButton = true,
                     bool bAddGeoLayerRequestedExternally = false);

} // namespace Ui
} // namespace SanmapGen
