// MarkersTab_RuleLayers_UI.h — the two-level procedural-rule list: MarkerRuleLayer groups, each
// owning an ordered stack of MarkerRule rows. Layer: UI. Accuracy class: Visual. STEP80.
//
// Mirrors LayersTab_UI.h's GeoLayer/Layer two-level DraggableList verbatim
// (LayersTab_UI.cpp:84-116 DrawGeoLayerList): only the SELECTED layer renders its rule list, the
// inner signal is captured out of the row body and applied FIRST (a layer Delete in the same frame
// would move the indices the rule signal is expressed in), and there is no cross-layer drag — a
// rule moves between layers only by being removed and re-added.
//
// One MarkerRuleLayer carries far more than fits one file under ARCH_01_05's hard 150 (the same
// reason MarkerRule's own sections split across MarkersTab_Rules_UI.cpp / MarkersTab_Area_UI.cpp
// behind MarkersTab_Rules_UI.h): the list mechanics live in MarkersTab_RuleLayers_UI.cpp, the
// buttons and the Selected Layer settings block in MarkersTab_RuleLayerSettings_UI.cpp, both behind
// this one header.
//
// `ApplyMarkerRuleLayerListSignal` takes plain `int&` selection indices, not `MarkersTabState`, so
// this header never includes back into MarkersTab_UI.h (no cycle) — `MarkersTabState` is only
// FORWARD-DECLARED here. Every other declaration below DOES need the full state (mirror loads, the
// two selection indices), so each is defined in one of the two .cpp files, which include the full
// header — the same split MarkersTab_Rules_UI.h/.cpp already uses for `MarkersTabState`.
#pragma once
#include "DraggableListWidget_UI.h"
#include "../params/MarkerRule_PARAMS.h"

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct MarkersTabState;

// Applies one frame of OUTER (layer) list traffic. Reorder/Delete are the shared structural
// applier; ToggleVisibility is the layer's `bEnabled` generation gate (STEP80 §3), ToggleLock is
// `bHidden`, Select moves the selection and resets the inner rule selection to 0. Returns true only
// when the RECIPE moved (a Select did not) — same contract as `ApplyGeoLayerListSignal`.
inline bool ApplyMarkerRuleLayerListSignal(std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                                           const DraggableListSignal& signal,
                                           int& selectedRuleLayerIndex, int& selectedRuleIndex) {
    const int layerCount = static_cast<int>(markerRuleLayers.size());
    if (signal.sourceRowIndex < 0 || signal.sourceRowIndex >= layerCount) return false;
    Params::MarkerRuleLayer& layer =
        markerRuleLayers[static_cast<std::size_t>(signal.sourceRowIndex)];
    if (signal.kind == DraggableListSignalKind::ToggleVisibility) {
        layer.bEnabled = !layer.bEnabled;
        return true;
    }
    if (signal.kind == DraggableListSignalKind::ToggleLock) {
        layer.bHidden = !layer.bHidden;
        return true;
    }
    if (signal.kind == DraggableListSignalKind::Select) {
        selectedRuleLayerIndex = signal.sourceRowIndex;
        selectedRuleIndex      = 0;
        return false;
    }
    if (signal.kind == DraggableListSignalKind::Reorder) selectedRuleLayerIndex = signal.targetRowIndex;
    return ApplyDraggableListSignal(markerRuleLayers, signal);
}

// MarkersTab_RuleLayers_UI.cpp:

// The same for one layer's rule list — the existing per-rule `ApplyRuleListSignal` body, rehomed
// against `layer.rules`.
bool ApplyMarkerRuleListSignal(Params::MarkerRuleLayer& layer, const DraggableListSignal& signal,
                               MarkersTabState& state);

// The layer the "Selected Layer" settings block (name/enabled/hidden/symmetry) binds to, or null
// when the selection points at nothing (STEP80 §2, mirroring `SelectedLayer`, LayersTab_UI.cpp:120).
Params::MarkerRuleLayer* SelectedMarkerRuleLayer(std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                                                 const MarkersTabState& state);

// The outer/inner DraggableLists and their appliers — every commit runs through
// `NotifyPlacementChange` itself. Also draws the non-empty-layer Delete confirm, the Add/Remove
// buttons, and the Selected Layer settings block (both MarkersTab_RuleLayerSettings_UI.cpp) below
// the list.
void DrawMarkerRuleLayerList(std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                             MarkersTabState& state, Pipeline::PreviewDriver* previewDriver);

// MarkersTab_RuleLayerSettings_UI.cpp — the "aspect" file this header also fronts (ARCH §1.5):

// The non-empty-layer Delete confirm (STEP80 §4): drawn every frame a delete might be pending, so
// its own modal popup gets the chance to run every frame it might be open. Returns whether a layer
// was actually erased this frame. Called by DrawMarkerRuleLayerList only.
bool DrawPendingDeleteRuleLayerDialog(std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                                      MarkersTabState& state);
// Add Layer / Add Rule / Remove Selected Rule. Called by DrawMarkerRuleLayerList only.
void DrawRuleLayerButtons(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                          Pipeline::PreviewDriver* previewDriver);
// Name / Enabled / Hidden / Symmetry for `SelectedMarkerRuleLayer`. Called by DrawMarkerRuleLayerList only.
void DrawSelectedRuleLayerSettings(std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                                   MarkersTabState& state, Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
