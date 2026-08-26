// MarkersTab_UI.cpp — the imgui composition of the marker tab. Layer: UI.
// Shared widgets only: DraggableList (via MarkersTab_RuleLayers_UI) for the procedural rule stack,
// VirtualList for the placed markers, IconGrid for the pickers, Section/Checkbox/Combo/RangeSlider/
// Dial for the scalars. No ImGui::SliderFloat / DragFloat / VSliderFloat in this file.
#include "MarkersTab_UI.h"
#include "MarkerInstanceId_UI.h"
#include "MarkerLayerId_UI.h"
#include "MarkersTab_ManualLayerHelpers_UI.h"
#include "PlacementRuleSections_UI.h"
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

// STEP135 — the header's own button cluster, human's own explicit left-to-right order: "Add
// Instance", "Add Group", "Add Layer", then the pre-existing Hide/Unhide (STEP133). One combined
// width function so the reserved zone (below) and the actual draw (DrawRightAlignedTypeSectionHeader-
// Buttons) can never drift apart.
float TypeSectionHeaderButtonClusterWidth(bool bHidden) {
    return SmallButtonWidth("Add Instance") + kHeaderButtonSpacingPixels
         + SmallButtonWidth("Add Group")    + kHeaderButtonSpacingPixels
         + SmallButtonWidth("Add Layer")    + kHeaderButtonSpacingPixels
         + SmallButtonWidth(bHidden ? "Unhide" : "Hide");
}

// The reserved-right-width `SectionOptions` for one Type-section header: the whole button cluster's
// own measured width plus the fixed leading spacing above, mirroring HeightmapTab_UI.cpp's
// GeoLayerSectionOptions exactly (STEP133), now sized for four buttons instead of one.
SectionOptions HeaderButtonsSectionOptions(bool bHidden) {
    SectionOptions options;
    options.reservedRightWidth = TypeSectionHeaderButtonClusterWidth(bHidden) + kHeaderButtonSpacingPixels;
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

// Right-aligns the four-button cluster within the header's own reserved right zone: its right edge
// lands at the same X every frame regardless of which Hide/Unhide label is current, rather than
// sitting flush-left of the reserved zone the way SameLine() alone would leave it (HeightmapTab_UI.cpp's
// single-label "Add GeoLayer" precedent never needed this). Must be called immediately after
// ImGui::SameLine(), with the reserved zone the ONLY content still ahead of the cursor on this line —
// GetContentRegionAvail().x is exactly that zone's remaining width, and DrawSectionBegin sized
// `barWidth` so this cluster's own right edge always lands at the header's own full right edge,
// independent of which Hide/Unhide label reserved the zone this frame (STEP133's own
// DrawRightAlignedHideToggleButton, widened one tier to the whole cluster).
TypeSectionHeaderButtons_UI DrawRightAlignedTypeSectionHeaderButtons(bool bHidden) {
    TypeSectionHeaderButtons_UI result;
    const char* const hideLabel = bHidden ? "Unhide" : "Hide";
    const float totalWidth = TypeSectionHeaderButtonClusterWidth(bHidden);
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    if (availableWidth > totalWidth)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - totalWidth);

    result.bAddInstanceClicked = ImGui::SmallButton("Add Instance");
    ImGui::SameLine(0.0f, kHeaderButtonSpacingPixels);
    result.bAddGroupClicked = ImGui::SmallButton("Add Group");
    ImGui::SameLine(0.0f, kHeaderButtonSpacingPixels);
    if (ImGui::SmallButton("Add Layer")) ImGui::OpenPopup("addLayerTypePopup");
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
    (void)placedMarkers;
    (void)selectManualMarkerInstanceCallback; (void)selectProceduralMarkerInstanceCallback;
    ImGui::PushID("markersTab");
    DrawMarkersTabGlobals(state.globals, recipe.globalMarkerSettings, iconManifest, pairingLookup);
    // Human's own explicit instruction: strip the tab down to Global plus three EMPTY collapsible
    // sections (Alloy/Plasma/Spawn), nothing else, as a clean baseline to verify before anything
    // else is rebuilt on top. No Bundle tree, no Rule stack, no Manual Markers, no Placed Markers —
    // STEP133/STEP135 add the header's own Hide/Unhide and Add Instance/Group/Layer affordances
    // directly, still with no body content rendered underneath.
    for (const char* typeName : { "Alloy", "Plasma", "Spawn" }) {
        ImGui::PushID(typeName);
        // STEP133 — a right-aligned Hide/Unhide button, per Type-section header, toggling that
        // Type's markers off the map preview entirely (both manual and procedural). STEP135 widens
        // the same reserved-right-zone mechanism to the "Add Instance"/"Add Group"/"Add Layer"
        // buttons the human asked for, in that order, immediately to the LEFT of Hide/Unhide.
        const bool bHidden = state.markerTypeVisibility.IsHidden(typeName);
        if (DrawSectionBegin(typeName, state.typeSections.stateByTypeName[typeName].outerSection,
                             HeaderButtonsSectionOptions(bHidden))) {
            ImGui::SameLine();
            const TypeSectionHeaderButtons_UI buttons = DrawRightAlignedTypeSectionHeaderButtons(bHidden);

            if (buttons.bAddInstanceClicked) {
                Params::MarkerInstanceGroup& group =
                    FindOrCreateMarkerInstanceGroupByName(recipe.markers, typeName);
                Params::MarkerTransform transform;
                transform.name = NextMarkerInstanceName(static_cast<int>(group.transforms.size()));
                transform.instanceIdentifier = NextMarkerInstanceIdentifier(recipe.markers);
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
                layer.name                   = NextMarkerLayerName(static_cast<int>(recipe.markerLayers.size()));
                layer.layerId                = NextMarkerLayerId(recipe.markerLayers);
                layer.parentBundleIdentifier = -1;
                layer.markerTypeName         = typeName;
                recipe.markerLayers.push_back(layer);
                state.manualLayers.selectedLayerIndex = static_cast<int>(recipe.markerLayers.size()) - 1;
            }
            bool bRecipeMoved = false;
            if (buttons.bAddProceduralLayerClicked) {
                Params::MarkerRuleLayer layer;
                layer.parentBundleIdentifier = -1;
                layer.markerTypeName         = typeName;
                recipe.markerRuleLayers.push_back(layer);
                state.selectedRuleLayerIndex = static_cast<int>(recipe.markerRuleLayers.size()) - 1;
                state.selectedRuleIndex      = 0;
                bRecipeMoved = true;
            }
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
