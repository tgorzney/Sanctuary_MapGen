// MarkersTab_UI.cpp — the imgui composition of the marker tab. Layer: UI.
// Shared widgets only: DraggableList (via MarkersTab_RuleLayers_UI) for the procedural rule stack,
// VirtualList for the placed markers, IconGrid for the pickers, Section/Checkbox/Combo/RangeSlider/
// Dial for the scalars. No ImGui::SliderFloat / DragFloat / VSliderFloat in this file.
#include "MarkersTab_UI.h"
#include "MarkerInstanceId_UI.h"
#include "MarkerLayerId_UI.h"
#include "MarkersTab_ManualLayerHelpers_UI.h"
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "PlacementRuleSections_UI.h"
#include "SymmetryClusterInstanceList_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// STEP133/STEP135 — the gap between the Type-section header's own drawn content and its own
// right-aligned button cluster, and the gap BETWEEN each button in that cluster (Constitution §8: a
// named constant, never a bare literal at the call site), mirroring HeightmapTab_UI.cpp's
// kGeoLayerAddButtonSpacingPixels precedent exactly — reused uniformly for every gap in the cluster
// rather than minting one constant per gap.
constexpr float kHeaderButtonSpacingPixels = 8.0f;

float SmallButtonWidth(const char* label) {
    return ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
}

// STEP135/STEP137 — the header's own button cluster, human's own explicit left-to-right order: "+
// Instance", "+ Group", "+ Layer", then the pre-existing Hide/Unhide (STEP133) — "+" replaces the
// earlier "Add " prefix verbatim (human's own instruction). One combined width function so the
// reserved zone (below) and the actual draw (DrawRightAlignedTypeSectionHeaderButtons) can never
// drift apart.
float TypeSectionHeaderButtonClusterWidth(bool bHidden) {
    return SmallButtonWidth("+ Instance") + kHeaderButtonSpacingPixels
         + SmallButtonWidth("+ Group")    + kHeaderButtonSpacingPixels
         + SmallButtonWidth("+ Layer")    + kHeaderButtonSpacingPixels
         + SmallButtonWidth(bHidden ? "Unhide" : "Hide");
}

// STEP136 — the FULL reserved-right-width `SectionOptions` for one Type-section header: the
// relocated per-Type marker-settings row (MarkersTab_Globals_UI.h's own
// TypeSectionMarkerSettingsRowWidth) immediately followed by the button cluster above, human's own
// explicit "to the left of the buttons" ordering, plus the fixed leading spacing, mirroring
// HeightmapTab_UI.cpp's GeoLayerSectionOptions exactly (STEP133).
SectionOptions HeaderButtonsSectionOptions(const MarkersTabGlobals& globals, bool bHidden) {
    SectionOptions options;
    options.reservedRightWidth = TypeSectionMarkerSettingsRowWidth(globals) + kHeaderButtonSpacingPixels
                                + TypeSectionHeaderButtonClusterWidth(bHidden) + kHeaderButtonSpacingPixels;
    return options;
}

// What the header's own button cluster did this frame — "Add Layer" opens a Manual/Procedural
// choice (the human's own stated design for a Layer-adding affordance, ARCH §19's Group/Layer
// restructure) rather than guessing one kind, so it reports EITHER of two distinct clicks.
struct TypeSectionHeaderButtons_UI {
    bool bAddInstanceClicked        = false;
    bool bAddGroupClicked           = false;
    bool bAddManualLayerClicked     = false;
    bool bAddProceduralLayerClicked = false;
    bool bHideToggleClicked         = false;
};

// Right-aligns the marker-settings row + the four-button cluster within the header's own reserved
// right zone: the combined content's right edge lands at the same X every frame regardless of which
// Hide/Unhide label is current, rather than sitting flush-left of the reserved zone the way
// SameLine() alone would leave it (HeightmapTab_UI.cpp's single-label "Add GeoLayer" precedent never
// needed this). Must be called immediately after ImGui::SameLine(), with the reserved zone the ONLY
// content still ahead of the cursor on this line — GetContentRegionAvail().x is exactly that zone's
// remaining width, and DrawSectionBegin sized `barWidth` so this content's own right edge always
// lands at the header's own full right edge, independent of which Hide/Unhide label reserved the
// zone this frame (STEP133's own DrawRightAlignedHideToggleButton, widened one tier to the whole
// cluster, then STEP136 widened again to include the relocated marker-settings row).
TypeSectionHeaderButtons_UI DrawRightAlignedTypeSectionHeaderButtons(
        MarkersTabGlobals& globals, int rowIndex, Params::GlobalMarkerSettings& globalMarkerSettings,
        const IconAtlasManifest* iconManifest, const IconAtlasPairingLookup* pairingLookup, bool bHidden) {
    TypeSectionHeaderButtons_UI result;
    const char* const hideLabel = bHidden ? "Unhide" : "Hide";
    const float totalWidth = TypeSectionMarkerSettingsRowWidth(globals) + kHeaderButtonSpacingPixels
                            + TypeSectionHeaderButtonClusterWidth(bHidden);
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    if (availableWidth > totalWidth)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - totalWidth);

    DrawTypeSectionMarkerSettingsRow(globals, rowIndex, globalMarkerSettings, iconManifest, pairingLookup);
    ImGui::SameLine(0.0f, kHeaderButtonSpacingPixels);

    result.bAddInstanceClicked = ImGui::SmallButton("+ Instance");
    ImGui::SameLine(0.0f, kHeaderButtonSpacingPixels);
    result.bAddGroupClicked = ImGui::SmallButton("+ Group");
    ImGui::SameLine(0.0f, kHeaderButtonSpacingPixels);
    if (ImGui::SmallButton("+ Layer")) ImGui::OpenPopup("addLayerTypePopup");
    if (ImGui::BeginPopup("addLayerTypePopup")) {
        if (ImGui::MenuItem("Manual"))     result.bAddManualLayerClicked     = true;
        if (ImGui::MenuItem("Procedural")) result.bAddProceduralLayerClicked = true;
        ImGui::EndPopup();
    }
    ImGui::SameLine(0.0f, kHeaderButtonSpacingPixels);
    result.bHideToggleClicked = ImGui::SmallButton(hideLabel);
    return result;
}

// The `Params::MarkerInstanceGroup` (the legacy Type-keyed roster `recipe.markers` still actually
// stores authored transforms in — `MarkerInstanceLayer` above it is metadata-only, ARCH §19.13) whose
// `name` matches this Type-section, minting one on first use. Real map data (a human-confirmed live
// re-import of an existing .sanmap) already stores its Alloy/Spawn markers in exactly this group, so
// "Add Instance" appends to the SAME roster that data already occupies rather than a parallel one.
Params::MarkerInstanceGroup& FindOrCreateMarkerInstanceGroupByName(
        std::vector<Params::MarkerInstanceGroup>& markers, const std::string& name) {
    for (Params::MarkerInstanceGroup& group : markers)
        if (group.name == name) return group;
    Params::MarkerInstanceGroup group;
    group.name = name;
    markers.push_back(group);
    return markers.back();
}

// STEP137 — a new manual instance's default X/Z: the map's own center (human's own instruction —
// the struct's own (0,0,0) default sits at the map's CORNER under SanGen's corner-origin world-space
// convention, confirmed by Placement_Fields_PROC.cpp's own `mapCenter = (vertexSize - 1) * 0.5` and
// MarkerSymmetryDetection_PIPELINE.cpp's own `worldSize = mapSize * worldUnitsPerCell` extent).
float MapCenterWorldUnits(const Params::Geometry& geometry) {
    return static_cast<float>(geometry.mapSize) * geometry.worldUnitsPerCell * 0.5f;
}

// STEP137 — the selected Manual Layer's own plain vector position (`MarkerTransform::layerIndex`'s
// established convention, MarkerLayerIndexRepair_UI.h), when a Layer typed to THIS Type-section is
// currently selected; else the SAME "no specific layer" fallback every other layer-losing path
// already uses (`ClampMarkerLayerIndicesForRemovedLayer`'s own clamp-to-0 — layerIndex has no
// "unassigned" sentinel to invent here, only the existing ratified convention to reuse). Guards on
// `markerTypeName` so a Layer selected under a DIFFERENT Type-section's "+ Layer"/tree click never
// silently receives another Type's instance.
int ResolveAddInstanceLayerIndex(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                 int selectedLayerIndex, const std::string& typeName) {
    if (selectedLayerIndex < 0 || selectedLayerIndex >= static_cast<int>(markerLayers.size())) return 0;
    if (markerLayers[static_cast<std::size_t>(selectedLayerIndex)].markerTypeName != typeName) return 0;
    return selectedLayerIndex;
}

// STEP138 — a new Layer's own `parentBundleIdentifier`: the currently-selected Group (Bundle), when
// one typed to THIS Type-section is selected (human's own instruction — "+ Layer" should add under
// the selected Group); else root ("the base section"), the existing -1 convention every Bundle/
// Layer already carries. Guards on `markerTypeName` for the same cross-Type-section-selection
// reason `ResolveAddInstanceLayerIndex` above does.
int ResolveAddLayerParentBundleIdentifier(const std::vector<Params::MarkerLayerBundle>& bundles,
                                          int selectedBundleIdentifier, const std::string& typeName) {
    if (selectedBundleIdentifier < 0) return -1;
    for (const Params::MarkerLayerBundle& bundle : bundles)
        if (bundle.identifier == selectedBundleIdentifier)
            return bundle.markerTypeName == typeName ? selectedBundleIdentifier : -1;
    return -1;
}

// STEP138 — this Type's own instances whose `layerIndex` does not resolve to any Layer OF THIS
// TYPE (no manual Layer exists yet for it, or the index is a legacy/cross-type stale reference),
// rendered at the base of the section, after every Group and Layer, still indented under the
// collapsible Type-section. `MarkerTransform::layerIndex` has no real "unassigned" sentinel
// (MarkerLayerIndexRepair_UI.h's own clamp-to-0 convention) — this IS the one "no Layer" case the
// current data model can actually represent; an instance "in a Group but no Layer" (human's other
// stated case) needs a real PARAMS+IO field this ticket does not add (out of scope, flagged not
// guessed).
void DrawBaseSectionManualInstanceList(std::vector<Params::MarkerInstanceGroup>& markers,
                                       const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                       const std::string& typeName, int& selectedManualInstanceIdentifier,
                                       const std::function<void(int)>& selectManualMarkerInstanceCallback) {
    std::vector<std::pair<int, int>> baseInstances;
    for (int groupIndex = 0; groupIndex < static_cast<int>(markers.size()); ++groupIndex) {
        Params::MarkerInstanceGroup& group = markers[static_cast<std::size_t>(groupIndex)];
        if (group.name != typeName) continue;
        for (int transformIndex = 0; transformIndex < static_cast<int>(group.transforms.size()); ++transformIndex) {
            const int layerIndex = group.transforms[static_cast<std::size_t>(transformIndex)].layerIndex;
            const bool bHasOwnTypeLayer = layerIndex >= 0 && layerIndex < static_cast<int>(markerLayers.size())
                && markerLayers[static_cast<std::size_t>(layerIndex)].markerTypeName == typeName;
            if (!bHasOwnTypeLayer) baseInstances.push_back({groupIndex, transformIndex});
        }
    }
    ImGui::TextUnformatted("Instances");
    if (baseInstances.empty()) { ImGui::TextDisabled("(none)"); return; }
    DrawSymmetryClusterInstanceList<std::pair<int, int>>(baseInstances,
        [&](const std::pair<int, int>& groupTransformIndex) {
            return markers[static_cast<std::size_t>(groupTransformIndex.first)]
                .transforms[static_cast<std::size_t>(groupTransformIndex.second)].symmetryGroupIdentifier;
        },
        [](int groupIdentifier, int /*bucketSize*/) { return groupIdentifier != 0; },
        [&](const std::pair<int, int>& groupTransformIndex) {
            DrawManualInstanceRow(markers, groupTransformIndex, selectedManualInstanceIdentifier,
                                  selectManualMarkerInstanceCallback);
        });
}

} // namespace

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
                    const std::function<void(int)>& selectManualMarkerInstanceCallback,
                    const std::function<void(int)>& selectProceduralMarkerInstanceCallback) {
    ImGui::PushID("markersTab");
    DrawMarkersTabGlobals(state.globals);
    // Global plus three collapsible Type-sections (Alloy/Plasma/Spawn) — no free-floating Rule stack,
    // no old "Manual Markers"/"Placed Markers" editors. STEP133/STEP135/STEP136 add the header's own
    // Hide/Unhide, "+ Instance"/"+ Group"/"+ Layer", and the relocated per-Type marker-settings row.
    // STEP138 adds the body: the Group(Bundle)/Layer/Instance hierarchy those buttons populate.
    for (int rowIndex = 0; rowIndex < kMarkerGlobalScaleRowCount; ++rowIndex) {
        const char* const typeName = markerGlobalScaleRowLabels[rowIndex];
        ImGui::PushID(typeName);
        // STEP133 — a right-aligned Hide/Unhide button, per Type-section header, toggling that
        // Type's markers off the map preview entirely (both manual and procedural). STEP135 widens
        // the same reserved-right-zone mechanism to the "+ Instance"/"+ Group"/"+ Layer" buttons the
        // human asked for, in that order, immediately to the LEFT of Hide/Unhide. STEP136 widens it
        // again, one tier further left, for the relocated per-Type marker-settings row (icon / icon
        // color / select color / global scale), formerly the Global section's own stacked rows.
        const bool bHidden = state.markerTypeVisibility.IsHidden(typeName);
        if (DrawSectionBegin(typeName, state.typeSections.stateByTypeName[typeName].outerSection,
                             HeaderButtonsSectionOptions(state.globals, bHidden))) {
            ImGui::SameLine();
            const TypeSectionHeaderButtons_UI buttons = DrawRightAlignedTypeSectionHeaderButtons(
                state.globals, rowIndex, recipe.globalMarkerSettings, iconManifest, pairingLookup, bHidden);

            if (buttons.bAddInstanceClicked) {
                Params::MarkerInstanceGroup& group =
                    FindOrCreateMarkerInstanceGroupByName(recipe.markers, typeName);
                Params::MarkerTransform transform;
                transform.name = NextMarkerInstanceName(static_cast<int>(group.transforms.size()));
                transform.instanceIdentifier = NextMarkerInstanceIdentifier(recipe.markers);
                const float mapCenter = MapCenterWorldUnits(recipe.geometry);
                transform.transform.positionX = mapCenter;
                transform.transform.positionZ = mapCenter;
                transform.layerIndex = ResolveAddInstanceLayerIndex(
                    recipe.markerLayers, state.manualLayers.selectedLayerIndex, typeName);
                group.transforms.push_back(transform);
                state.selectedManualInstanceIdentifier = transform.instanceIdentifier;
            }
            if (buttons.bAddGroupClicked) {
                Params::MarkerLayerBundle bundle;
                bundle.identifier = NextMarkerLayerBundleId(recipe.markerLayerBundles);
                bundle.markerTypeName = typeName;
                recipe.markerLayerBundles.push_back(bundle);
                state.bundles.selectedBundleIdentifier = bundle.identifier;
            }
            if (buttons.bAddManualLayerClicked) {
                Params::MarkerInstanceLayer layer;
                layer.name     = NextMarkerLayerName(static_cast<int>(recipe.markerLayers.size()));
                layer.layerId  = NextMarkerLayerId(recipe.markerLayers);
                layer.parentBundleIdentifier = ResolveAddLayerParentBundleIdentifier(
                    recipe.markerLayerBundles, state.bundles.selectedBundleIdentifier, typeName);
                layer.markerTypeName = typeName;
                recipe.markerLayers.push_back(layer);
                state.manualLayers.selectedLayerIndex = static_cast<int>(recipe.markerLayers.size()) - 1;
            }
            bool bRecipeMoved = false;
            if (buttons.bAddProceduralLayerClicked) {
                Params::MarkerRuleLayer layer;
                layer.parentBundleIdentifier = ResolveAddLayerParentBundleIdentifier(
                    recipe.markerLayerBundles, state.bundles.selectedBundleIdentifier, typeName);
                layer.markerTypeName = typeName;
                recipe.markerRuleLayers.push_back(layer);
                state.selectedRuleLayerIndex = static_cast<int>(recipe.markerRuleLayers.size()) - 1;
                state.selectedRuleIndex      = 0;
                bRecipeMoved = true;
            }

            // STEP138 — the actual Group -> Layer -> Instance hierarchy the header's own buttons now
            // populate: the Bundle (Group) tree first (each Group's own Layers and their Instances
            // draw nested/indented inside it, DrawMarkerLayerBundleTree/DrawLayerRowBody), then this
            // Type's own UNGROUPED Layers (root `parentBundleIdentifier == -1`), then the base-section
            // Instance list — the "no Layer at all" case (see DrawBaseSectionManualInstanceList above).
            DrawMarkerLayerBundleTree(recipe.markerLayerBundles, recipe.markerRuleLayers, recipe.markerLayers,
                                      recipe.markers, recipe.geometry, recipe.globalSymmetryMask,
                                      recipe.radialSymmetryRepeatCount, recipe.markerSymmetryFixSettings,
                                      state.bundles, state, previewDriver, iconManifest, typeName,
                                      selectManualMarkerInstanceCallback);
            ImGui::Separator();
            bRecipeMoved = DrawRuleLayerListBody(recipe.markerRuleLayers, state, previewDriver, iconManifest,
                                                 typeName, placedMarkers, selectProceduralMarkerInstanceCallback)
                         || bRecipeMoved;
            DrawManualMarkerLayerListBody(state.manualLayers, recipe.markerLayers, recipe.markers,
                                          recipe.geometry, recipe.globalSymmetryMask,
                                          recipe.radialSymmetryRepeatCount, recipe.markerSymmetryFixSettings,
                                          typeName, state.selectedManualInstanceIdentifier,
                                          selectManualMarkerInstanceCallback);
            ImGui::Separator();
            DrawBaseSectionManualInstanceList(recipe.markers, recipe.markerLayers, typeName,
                                              state.selectedManualInstanceIdentifier,
                                              selectManualMarkerInstanceCallback);

            NotifyPlacementChange(bRecipeMoved, previewDriver);

            if (buttons.bHideToggleClicked)
                state.markerTypeVisibility.SetHidden(typeName, !bHidden);
            DrawSectionEnd();
        }
        ImGui::PopID();
    }
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
