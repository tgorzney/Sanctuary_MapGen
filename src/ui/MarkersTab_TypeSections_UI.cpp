// MarkersTab_TypeSections_UI.cpp — EnumerateMarkerTypeSectionNames and DrawMarkerTypeSections, the
// Markers tab's dynamic Type-section outer loop (STEP125, ARCH §19.14/§19.15). DrawMarkerTypeSections
// draws THREE kinds of tab-wide-not-per-type control exactly once each, flagged as this ticket's own
// composition calls (not ARCH rulings): (a) the Manual block-wide settings, before the per-type loop;
// (b) Add Rule / Remove Selected Rule and the non-empty-layer delete confirm, after it (both operate
// on single, tab-wide selection scalars, not type-scoped ones); (c) `ImGui::PushID(typeName.c_str())`
// around each Type-section's entire body — load-bearing, per Section_UI.cpp:44/65's own PushID/PopID
// only wrapping the header bar, not the body, so every fixed-literal child widget id (the Bundle
// tree's "markerLayerBundles", both "Ungrouped ..." lists' own DraggableList ids) needs THIS salt to
// avoid colliding across N Type-sections.
#include "MarkersTab_TypeSections_UI.h"
#include "MarkersTab_Bundles_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include "MarkersTab_RuleLayers_UI.h"
#include "MarkersTab_UI.h"
#include "imgui.h"
#include "../params/MapRecipe_PARAMS.h"
#include <algorithm>

namespace SanmapGen {
namespace Ui {
namespace {

void CollectDistinctNonEmptyTypeName(const std::string& name, std::vector<std::string>& outDistinct) {
    if (name.empty()) return;
    for (const std::string& existing : outDistinct) if (existing == name) return;
    outDistinct.push_back(name);
}

} // namespace

std::vector<std::string> EnumerateMarkerTypeSectionNames(
        const std::vector<Params::MarkerLayerBundle>& bundles,
        const std::vector<Params::MarkerRuleLayer>& ruleLayers,
        const std::vector<Params::MarkerInstanceLayer>& instanceLayers) {
    std::vector<std::string> distinct;   // every non-empty markerTypeName present, union+dedup
    for (const Params::MarkerLayerBundle& bundle : bundles) CollectDistinctNonEmptyTypeName(bundle.markerTypeName, distinct);
    for (const Params::MarkerRuleLayer& layer : ruleLayers) CollectDistinctNonEmptyTypeName(layer.markerTypeName, distinct);
    for (const Params::MarkerInstanceLayer& layer : instanceLayers) CollectDistinctNonEmptyTypeName(layer.markerTypeName, distinct);

    std::vector<std::string> ordered;
    for (const char* fixedName : { "Alloy", "Plasma", "Spawn" })   // ARCH_19_14: fixed order, present-only
        for (const std::string& name : distinct)
            if (name == fixedName) { ordered.push_back(name); break; }
    std::vector<std::string> others;
    for (const std::string& name : distinct)
        if (name != "Alloy" && name != "Plasma" && name != "Spawn") others.push_back(name);
    std::sort(others.begin(), others.end());
    for (const std::string& name : others) ordered.push_back(name);
    ordered.push_back("");   // "(Unassigned)" — ALWAYS appended, not gated on presence (STEP125's own
                              // bootstrap ruling, see .h comment): a brand-new recipe with zero
                              // Bundles/Layers must still have SOMEWHERE to click "Add Group"/
                              // "Add Layer" for the very first one; every other section already
                              // requires the type to exist in data first (no chicken-and-egg problem
                              // for a NAMED type — its first Bundle/Layer is minted from an ALREADY-
                              // open Unassigned section, then typed into via the free-text field).
    return ordered;
}

void DrawMarkerTypeSections(Params::MapRecipe& recipe, MarkersTabState& state,
                            Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest) {
    DrawManualMarkerLayerBlockSettings(state.manualLayers);   // (a) — once, tab-wide

    const std::vector<std::string> typeNames = EnumerateMarkerTypeSectionNames(
        recipe.markerLayerBundles, recipe.markerRuleLayers, recipe.markerLayers);

    for (const std::string& typeName : typeNames) {
        ImGui::PushID(typeName.c_str());   // (c) — salts every fixed-literal child ID this section owns
        MarkerTypeSectionState_UI& perType = state.typeSections.stateByTypeName[typeName];
        const char* const label = typeName.empty() ? "(Unassigned)" : typeName.c_str();
        if (DrawSectionBegin(label, perType.outerSection)) {
            DrawMarkerLayerBundleTree(recipe.markerLayerBundles, recipe.markerRuleLayers, recipe.markerLayers,
                                      recipe.markers, recipe.geometry, recipe.globalSymmetryMask,
                                      recipe.radialSymmetryRepeatCount, recipe.markerSymmetryFixSettings,
                                      state.bundles, state, previewDriver, iconManifest, typeName);

            if (DrawSectionBegin("Ungrouped Procedural Rules", perType.ungroupedProceduralSection)) {
                bool bRecipeMoved = DrawRuleLayerListBody(recipe.markerRuleLayers, state, previewDriver,
                                                          iconManifest, typeName);
                bRecipeMoved = DrawAddMarkerRuleLayerButton(recipe.markerRuleLayers, state, -1, typeName)
                             || bRecipeMoved;
                NotifyPlacementChange(bRecipeMoved, previewDriver);
                DrawSectionEnd();
            }
            if (DrawSectionBegin("Ungrouped Manual Marker Layers", perType.ungroupedManualSection)) {
                DrawManualMarkerLayerListBody(state.manualLayers, recipe.markerLayers, recipe.markers,
                                              recipe.geometry, recipe.globalSymmetryMask,
                                              recipe.radialSymmetryRepeatCount, recipe.markerSymmetryFixSettings,
                                              typeName, state.selectedManualInstanceIdentifier);
                DrawSectionEnd();
            }
            DrawSectionEnd();   // outer Type-section
        }
        ImGui::PopID();
    }

    DrawMarkerRuleButtons(recipe.markerRuleLayers, state, previewDriver);   // (b) — once, tab-wide
    NotifyPlacementChange(DrawPendingDeleteRuleLayerDialog(recipe.markerRuleLayers, state), previewDriver);
}

} // namespace Ui
} // namespace SanmapGen
