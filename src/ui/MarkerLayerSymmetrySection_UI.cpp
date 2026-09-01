// MarkerLayerSymmetrySection_UI.cpp — see MarkerLayerSymmetrySection_UI.h for the full rationale.
#include "MarkerLayerSymmetrySection_UI.h"
#include "Checkbox_UI.h"
#include "MarkerDragGesture_UI.h"
#include "MarkerSymmetryFixCommand_UI.h"
#include "PlacementRuleSections_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// STEP107: the tolerance slider, overwrite checkbox, button and result line — split out purely to
// keep DrawLayerSymmetrySection under the ARCH §1.5 40-line-per-function ceiling.
void DrawFixSymmetryCommand(int layerIndex, const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                           std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                           int globalSymmetryMask, int globalRadialRepeatCount,
                           Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                           ManualMarkerLayersState& state) {
    // STEP246, ARCH §19.33/§21.9 widened ResolveEffectiveMarkerSymmetry's signature; this is a
    // Layer-row header control with no specific transform in hand (§19.33's own carve-out for such
    // sites) — a synthetic layerIndex-only transform + an EMPTY Link roster resolves purely at the
    // Layer tier, behavior-IDENTICAL to this call's own pre-ticket Link-blind read.
    Params::MarkerTransform layerIndexOnlyTransform;
    layerIndexOnlyTransform.layerIndex = layerIndex;
    static const std::vector<Params::MarkerLink> kNoLinks;
    int effectiveMask = 0;
    int effectiveRadialRepeatCount = 0;
    ResolveEffectiveMarkerSymmetry(markerLayers, layerIndexOnlyTransform, kNoLinks, globalSymmetryMask,
                                   globalRadialRepeatCount, effectiveMask, effectiveRadialRepeatCount);
    DrawSliderScalar("Fix Symmetry Distance Tolerance", markerSymmetryFixSettings.distanceTolerance,
                     state.fixSymmetryToleranceRange, state.fixSymmetryToleranceToggle, WidgetStyle(), "%.2f");
    DrawCheckbox("Overwrite manually-adjusted positions", state.bFixSymmetryOverwrite);
    if (ImGui::Button("Fix Symmetry")) {
        state.lastFixSymmetryResult = FixMarkerLayerSymmetry(markers, geometry, layerIndex, effectiveMask,
            effectiveRadialRepeatCount, markerSymmetryFixSettings.distanceTolerance, state.bFixSymmetryOverwrite);
        state.bHasFixSymmetryResult = true;
        state.bFixSymmetryOverwrite = false;   // consumed per-use, STEP107 §2 — NOT sticky
    }
    if (state.bHasFixSymmetryResult) {
        ImGui::Text("Fix Symmetry: %d group(s) created, %d slot(s) unmatched",
                   state.lastFixSymmetryResult.confirmedGroupCount, state.lastFixSymmetryResult.unmatchedSlotCount);
    }
}

} // namespace

void DrawLayerSymmetrySection(Params::MarkerInstanceLayer& layer, int layerIndex,
                              const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                              std::vector<Params::MarkerInstanceGroup>& markers,
                              const Params::Geometry& geometry, int globalSymmetryMask,
                              int globalRadialRepeatCount,
                              Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                              ManualMarkerLayersState& state) {
    if (!DrawSectionBegin("Layer Symmetry", state.symmetrySection)) return;
    DrawPlacementSymmetryAxes("markerLayerSymmetry", layer.symmetry.bSymmetryUseGlobal,
                              layer.symmetry.symmetryMask, nullptr);
    DrawFixSymmetryCommand(layerIndex, markerLayers, markers, geometry, globalSymmetryMask,
                           globalRadialRepeatCount, markerSymmetryFixSettings, state);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
