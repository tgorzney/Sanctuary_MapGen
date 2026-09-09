// MarkersTab_UI.cpp — the imgui composition of the marker tab. Layer: UI.
// Shared widgets only: DraggableList (via MarkersTab_RuleLayers_UI) for the procedural rule stack,
// VirtualList for the placed markers, IconGrid for the pickers, Section/Checkbox/Combo/RangeSlider/
// Dial for the scalars. No ImGui::SliderFloat / DragFloat / VSliderFloat in this file.
//
// STEP254 (ARCH_01_05_FileSizeCeilings.md §1.5 — this file was 499 lines, ~3.3x the hard ceiling):
// the Type-section header's own button-cluster layout/draw, the 4 add-button handlers, the 4
// pending-apply mutations, and the base "no Layer" instance list all moved to sibling files
// (MarkersTab_TypeSectionHeaderButtons_UI.h/.cpp, MarkersTab_TypeSectionAddActions_UI.h/.cpp,
// MarkersTab_TypeSectionPendingApply_UI.h/.cpp, MarkersTab_BaseInstanceList_UI.h/.cpp); the
// per-Type-section loop body itself is now the named MarkersTab_TypeSectionBody_UI.h/.cpp function
// `DrawMarkerTypeSection`, leaving this file a thin Globals+Links+loop wrapper.
// `ResolveAddInstanceLayerIndex`/`SelectedMarkerRule` stay here — declared in `MarkersTab_UI.h`, the
// same-named header, the established pairing convention.
#include "MarkersTab_UI.h"
#include "MarkersTab_RuleLayers_UI.h"
#include "MarkersTab_TypeSectionBody_UI.h"
#include "imgui.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// STEP137 — the selected Manual Layer's own plain vector position (`MarkerTransform::layerIndex`'s
// established convention, MarkerLayerIndexRepair_UI.h), when a Layer typed to THIS Type-section is
// currently selected. Guards on `markerTypeName` so a Layer selected under a DIFFERENT Type-section's
// "+ Layer"/tree click never silently receives another Type's instance.
//
// Human's own bug report (STEP152 correction) — "+ Instance" for a non-Alloy type was silently
// landing under an ALLOY layer, selectable-together with Alloy's own instances, whenever no matching
// Layer for THIS type was selected: this function used to fall back to a bare `0` in both guard
// branches, i.e. "markerLayers[0]", not a real "no specific layer" — and `recipe.markerLayers[0]` is
// whichever Layer was created FIRST, roster-wide, regardless of type (the reported "if I delete the
// layer, it puts them under a DIFFERENT layer" is exactly this: deleting index 0 just shifts index 1
// into the same wrong role). STEP137's own comment reasoned "layerIndex has no unassigned sentinel
// to invent here" — true when it was written, but `-1` (Constitution §6/MarkerLayerIndexRepair_UI.h)
// is now the fully-supported "no Layer" convention (the drag-to-Instances-list "unassign" fix, and
// `DrawBaseSectionManualInstanceList`'s own "no Layer at all" case) — the RIGHT fallback here, not a
// positional guess. `ResolveEffectiveMarkerSymmetry`'s own `layerIndex < 0` branch already falls back
// to the recipe's GLOBAL symmetry settings for exactly this case, which is what actually caused the
// report's "only 1 created in preview" half: instances were being resolved against ALLOY's own layer
// settings instead of global ones. Promoted out of the anonymous namespace (was file-local) so a test
// can drive it directly without an imgui frame.
int ResolveAddInstanceLayerIndex(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                 int selectedLayerIndex, const std::string& typeName) {
    if (selectedLayerIndex < 0 || selectedLayerIndex >= static_cast<int>(markerLayers.size())) return -1;
    if (markerLayers[static_cast<std::size_t>(selectedLayerIndex)].markerTypeName != typeName) return -1;
    return selectedLayerIndex;
}

// The rule the detail controls edit: a two-index walk, both bounds-checked, null on either miss
// (STEP80, mirroring `SelectedLayer`, LayersTab_UI.cpp:120-127).
Params::MarkerRule* SelectedMarkerRule(std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                                       const MarkersTabState& state) {
    Params::MarkerRuleLayer* const layer = SelectedMarkerRuleLayer(markerRuleLayers, state);
    if (layer == nullptr) return nullptr;
    if (state.selectedRuleIndex < 0
        || state.selectedRuleIndex >= static_cast<int>(layer->rules.size())) return nullptr;
    return &layer->rules[static_cast<std::size_t>(state.selectedRuleIndex)];
}

void DrawMarkersTab(Params::MapRecipe& recipe, MarkersTabState& state,
                    Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                    const IconAtlasPairingLookup* pairingLookup,
                    const Data::PlacementInstances* placedMarkers,
                    const std::function<void(int clickedInstanceIdentifier,
                                             const std::vector<int>& selectedInstanceIdentifiers)>&
                        selectManualMarkerInstanceCallback,
                    const std::function<void(int, bool bCtrlHeld, bool bShiftHeld)>&
                        selectProceduralMarkerInstanceCallback) {
    ImGui::PushID("markersTab");
    DrawMarkersTabGlobals(state.globals);
    // STEP248 — the Links tier moves to right after the Global section (BRIEF_MarkerLinkCorrection_R1
    // ruling): a Link's own body now supports the same full Ctrl/Shift/drag selection every other
    // instance list in this tab has, so it needs the same shared selection state/callback every
    // Type-section body below already receives — passing it here, not inventing new plumbing.
    DrawMarkerLinksSection(recipe, state.links, state.selectedManualInstanceIdentifier,
                          state.selectedManualInstanceIdentifiers,
                          state.manualInstanceSelectionAnchorIdentifier,
                          selectManualMarkerInstanceCallback, previewDriver);
    // Global plus three collapsible Type-sections (Alloy/Plasma/Spawn) — no free-floating Rule stack,
    // no old "Manual Markers"/"Placed Markers" editors. STEP133/STEP135/STEP136 add the header's own
    // Hide/Unhide, "+ Instance"/"+ Group"/"+ Layer", and the relocated per-Type marker-settings row.
    // STEP138 adds the body: the Group(Bundle)/Layer/Instance hierarchy those buttons populate.
    for (int rowIndex = 0; rowIndex < kMarkerGlobalScaleRowCount; ++rowIndex) {
        DrawMarkerTypeSection(recipe, state, previewDriver, iconManifest, pairingLookup, placedMarkers,
                             rowIndex, markerGlobalScaleRowLabels[rowIndex],
                             selectManualMarkerInstanceCallback, selectProceduralMarkerInstanceCallback);
    }
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
