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
// buttons and the per-layer settings block (STEP110: drawn inline per row, not "the selected
// layer") in MarkersTab_RuleLayerSettings_UI.cpp, both behind this one header.
//
// `ApplyMarkerRuleLayerListSignal` takes plain `int&` selection indices, not `MarkersTabState`, so
// this header never includes back into MarkersTab_UI.h (no cycle) — `MarkersTabState` is only
// FORWARD-DECLARED here. Every other declaration below DOES need the full state (mirror loads, the
// two selection indices), so each is defined in one of the two .cpp files, which include the full
// header — the same split MarkersTab_Rules_UI.h/.cpp already uses for `MarkersTabState`.
#pragma once
#include <string>
#include "DraggableListWidget_UI.h"
#include "../params/MarkerRule_PARAMS.h"

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct MarkersTabState;
struct IconAtlasManifest;

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

// ARCH §19.15(c), ratified: "composing two independent boolean filters with `||` is ordinary
// predicate usage, not a contract violation. Signed off as legal." A layer bundled under a Group
// (Item 3's own Bundle-tree membership) or belonging to a DIFFERENT Type-section than the one
// currently drawing is suppressed from this "Ungrouped Procedural Rules" list.
inline bool IsMarkerRuleLayerRowSuppressed(const Params::MarkerRuleLayer& layer,
                                           const std::string& markerTypeNameFilter) {
    return layer.parentBundleIdentifier != -1 || layer.markerTypeName != markerTypeNameFilter;
}

// MarkersTab_RuleLayers_UI.cpp:

// The same for one layer's rule list — the existing per-rule `ApplyRuleListSignal` body, rehomed
// against `layer.rules`.
bool ApplyMarkerRuleListSignal(Params::MarkerRuleLayer& layer, const DraggableListSignal& signal,
                               MarkersTabState& state);

// The layer `state.selectedRuleLayerIndex` points at, or null when it points at nothing (STEP80
// §2, mirroring `SelectedLayer`, LayersTab_UI.cpp:120). STEP110: no longer what the settings block
// binds to (that is now every row's own layer, inline) — still what Add Rule / Remove Selected
// Rule (DrawMarkerRuleButtons) and the nested rule list (DrawRuleLayerBody) operate on.
Params::MarkerRuleLayer* SelectedMarkerRuleLayer(std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                                                 const MarkersTabState& state);

// The outer/inner DraggableLists and their appliers — every commit runs through
// `NotifyPlacementChange` itself. STEP110: each outer row draws its OWN layer settings block and
// (selected row only, drag-safety) its own nested rule list inline, in its own row body — nothing is
// drawn a second time at the bottom for whatever happens to be "selected". STEP125: promoted out of
// the anonymous namespace, gains `markerTypeNameFilter` (ARCH §19.15(c)), and returns whether the
// LIST signals alone moved the recipe — its own DrawPendingDeleteRuleLayerDialog call and trailing
// NotifyPlacementChange moved OUT to the tab-wide DrawMarkerRuleButtons/DrawMarkerTypeSections
// (this ticket's own composition call, §5(b): both operate on tab-wide selection scalars, not
// type-scoped ones, so drawing them once per Type-section would desync N redundant copies).
bool DrawRuleLayerListBody(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                           Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                           const std::string& markerTypeNameFilter);

// MarkersTab_RuleLayerSettings_UI.cpp — the "aspect" file this header also fronts (ARCH §1.5):

// The non-empty-layer Delete confirm (STEP80 §4): drawn every frame a delete might be pending, so
// its own modal popup gets the chance to run every frame it might be open. Returns whether a layer
// was actually erased this frame. STEP125: called once, tab-wide, by DrawMarkerTypeSections.
bool DrawPendingDeleteRuleLayerDialog(std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                                      MarkersTabState& state);
// STEP125: Add Rule / Remove Selected Rule only (the "Add Layer" button is now per-Type-section, see
// DrawAddMarkerRuleLayerButton below) — both operate on the tab-wide `state.selectedRuleLayerIndex`/
// `selectedRuleIndex`, so this is called exactly ONCE, tab-wide, by DrawMarkerTypeSections (§5(b)),
// not once per Type-section. Replaces the retired DrawRuleLayerButtons.
void DrawMarkerRuleButtons(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                           Pipeline::PreviewDriver* previewDriver);
// STEP120: the "Add Layer" button alone, extracted so MarkersTab_Bundles_UI.cpp's per-Bundle "add a
// Layer here" can reuse it with a non-root parent. `parentBundleIdentifierForNewLayer < 0` (the
// default) is root scope. STEP125: gains `markerTypeNameForNewLayer` (§4 — extends ARCH §19.15(a)'s
// Bundle "Add Group" pattern to this button, called once per Type-section's own "Ungrouped
// Procedural Rules" sub-list). Returns whether a layer was actually added (the recipe moved).
bool DrawAddMarkerRuleLayerButton(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                                  int parentBundleIdentifierForNewLayer = -1,
                                  const std::string& markerTypeNameForNewLayer = "");
// Name / Enabled / Hidden / Symmetry for ONE rule layer — the caller's own row, not a "selected"
// lookup (STEP110: called per outer row, inline, by DrawRuleLayerListBody's row body, whenever
// THAT row's own CollapsingHeader is open — never bled from whatever else happens to be selected).
void DrawRuleLayerSettings(Params::MarkerRuleLayer& layer, Pipeline::PreviewDriver* previewDriver);
// Gates / Quantity / Area / Focus / Placement Gate / Transform / Template Picker for ONE rule —
// the caller's own row (STEP110: called per rule row, inline, by DrawRuleLayerBody's own row body,
// MarkersTab_RuleLayers_UI.cpp; moved out of MarkersTab_UI.cpp's DrawRuleStack). `iconManifest` is
// nullable, forwarded straight to the Template Picker.
void DrawRuleSettings(Params::MarkerRule& rule, MarkersTabState& state,
                      Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest);

} // namespace Ui
} // namespace SanmapGen
