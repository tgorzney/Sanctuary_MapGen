// MarkersTab_UI.cpp — the imgui composition of the marker tab. Layer: UI.
// Shared widgets only: DraggableList (via MarkersTab_RuleLayers_UI) for the procedural rule stack,
// VirtualList for the placed markers, IconGrid for the pickers, Section/Checkbox/Combo/RangeSlider/
// Dial for the scalars. No ImGui::SliderFloat / DragFloat / VSliderFloat in this file.
#include "MarkersTab_UI.h"
#include "MarkerInstanceCreateSymmetric_UI.h"
#include "MarkerLayerId_UI.h"
#include "MarkersTab_BundleDelete_UI.h"
#include "MarkersTab_ManualInstanceSelection_UI.h"
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

// STEP138/STEP139 — a newly-added Layer's OR Group's own `parentBundleIdentifier`: the currently-
// selected Group (Bundle), when one typed to THIS Type-section is selected (human's own instruction
// — "+ Layer"/"+ Group" should add under the selected Group, and Groups stay nestable); else root
// ("the base section"), the existing -1 convention every Bundle/Layer already carries. Guards on
// `markerTypeName` for the same cross-Type-section-selection reason `ResolveAddInstanceLayerIndex`
// above does. Safe to reuse for a brand-new Group too (no cycle check needed — a NEW node can never
// already be its own ancestor; `WouldReparentMarkerLayerBundleCreateCycle` only guards RE-parenting
// an EXISTING node, a different, still-untouched code path).
int ResolveSelectedParentBundleIdentifier(const std::vector<Params::MarkerLayerBundle>& bundles,
                                          int selectedBundleIdentifier, const std::string& typeName) {
    if (selectedBundleIdentifier < 0) return -1;
    for (const Params::MarkerLayerBundle& bundle : bundles)
        if (bundle.identifier == selectedBundleIdentifier)
            return bundle.markerTypeName == typeName ? selectedBundleIdentifier : -1;
    return -1;
}

// STEP138/146 — this Type's own instances whose `layerIndex` does not resolve to any Layer OF THIS
// TYPE (no manual Layer exists yet for it, `-1` — genuinely unassigned, STEP146 — or a legacy/
// cross-type stale reference), rendered at the base of the section, after every Group and Layer,
// still indented under the collapsible Type-section. STEP146 (human's own bug report — dragging an
// instance here from a Layer did nothing) makes this list a real drop target too, reassigning to
// `layerIndex = -1`: this IS the one "no Layer" case the current data model can represent; an
// instance "in a Group but no Layer" (human's other stated case — a direct Group reference
// independent of any Layer) still needs a real PARAMS+IO field this ticket does not add (out of
// scope, flagged not guessed).
void DrawBaseSectionManualInstanceList(std::vector<Params::MarkerInstanceGroup>& markers,
                                       const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                       const std::string& typeName, int& selectedManualInstanceIdentifier,
                                       std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
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
    // STEP146 (human's own bug report — dragging an instance out of a Layer onto this base list did
    // nothing) — attached to the "Instances" text itself, not gated behind `!baseInstances.empty()`
    // below, so the list is a real drop target even while empty (the common starting case: no
    // unassigned instances yet). `DrawManualLayerInstanceDropTarget` already accepts ANY layerIndex
    // with no bounds-check of its own (MarkersTab_ManualInstanceSelection_UI.cpp) — passing -1 here
    // reassigns the dropped instance(s) to "no layer of my own type," which `bHasOwnTypeLayer` above
    // already treats as belonging in this exact list (any negative/out-of-range/different-type
    // layerIndex does). No PARAMS/IO change needed: -1 was always a safe value to WRITE into
    // `layerIndex` (every read site bounds-checks `>= 0` first, MarkerLayerIndexRepair_UI.h and
    // friends) — the only gap was that nothing ever wrote it.
    DrawManualLayerInstanceDropTarget(-1, markers, selectedManualInstanceIdentifiers);
    if (baseInstances.empty()) { ImGui::TextDisabled("(none)"); return; }

    // STEP141 — this list's own display-order identifiers, for Shift-range selection.
    std::vector<int> rowOrder;
    rowOrder.reserve(baseInstances.size());
    for (const std::pair<int, int>& groupTransformIndex : baseInstances)
        rowOrder.push_back(markers[static_cast<std::size_t>(groupTransformIndex.first)]
            .transforms[static_cast<std::size_t>(groupTransformIndex.second)].instanceIdentifier);

    ManualInstanceRowInteractionContext_UI interaction;
    interaction.primaryIdentifier   = &selectedManualInstanceIdentifier;
    interaction.selectedIdentifiers = &selectedManualInstanceIdentifiers;
    interaction.anchorIdentifier    = &anchorIdentifier;
    interaction.rowOrder            = &rowOrder;
    interaction.selectManualMarkerInstanceCallback = selectManualMarkerInstanceCallback;

    DrawSymmetryClusterInstanceList<std::pair<int, int>>(baseInstances,
        [&](const std::pair<int, int>& groupTransformIndex) {
            return markers[static_cast<std::size_t>(groupTransformIndex.first)]
                .transforms[static_cast<std::size_t>(groupTransformIndex.second)].symmetryGroupIdentifier;
        },
        [](int groupIdentifier, int /*bucketSize*/) { return groupIdentifier != 0; },
        [&](const std::pair<int, int>& groupTransformIndex) {
            DrawManualInstanceRow(markers, groupTransformIndex, interaction);
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
                // Human's own bug report — "When creating an Instance, symmetry needs to be checked
                // and duplicates created for proper symmetry": a plain single push_back never
                // consulted symmetry at all. CreateSymmetricManualMarkerInstances resolves the target
                // Layer's own effective mask/count and materializes every resulting orbit point (a
                // 1-point orbit — symmetry off — still creates exactly the one instance, unchanged
                // behavior for that case).
                Params::MarkerInstanceGroup& group =
                    FindOrCreateMarkerInstanceGroupByName(recipe.markers, typeName);
                const float mapCenter = MapCenterWorldUnits(recipe.geometry);
                const int layerIndex = ResolveAddInstanceLayerIndex(
                    recipe.markerLayers, state.manualLayers.selectedLayerIndex, typeName);
                state.selectedManualInstanceIdentifier = CreateSymmetricManualMarkerInstances(
                    group, recipe.markers, recipe.markerLayers, recipe.geometry, recipe.globalSymmetryMask,
                    recipe.radialSymmetryRepeatCount, layerIndex, mapCenter, 0.0f, mapCenter);
            }
            if (buttons.bAddGroupClicked) {
                Params::MarkerLayerBundle bundle;
                bundle.identifier = NextMarkerLayerBundleId(recipe.markerLayerBundles);
                bundle.markerTypeName = typeName;
                // Groups stay nestable (human's own confirmation) — a Group added while another
                // Group of this Type is selected nests under it, same "+ Layer" targeting rule below.
                bundle.parentBundleIdentifier = ResolveSelectedParentBundleIdentifier(
                    recipe.markerLayerBundles, state.bundles.selectedBundleIdentifier, typeName);
                recipe.markerLayerBundles.push_back(bundle);
                state.bundles.selectedBundleIdentifier = bundle.identifier;
            }
            if (buttons.bAddManualLayerClicked) {
                Params::MarkerInstanceLayer layer;
                layer.name     = NextMarkerLayerName(static_cast<int>(recipe.markerLayers.size()));
                layer.layerId  = NextMarkerLayerId(recipe.markerLayers);
                layer.parentBundleIdentifier = ResolveSelectedParentBundleIdentifier(
                    recipe.markerLayerBundles, state.bundles.selectedBundleIdentifier, typeName);
                layer.markerTypeName = typeName;
                recipe.markerLayers.push_back(layer);
                state.manualLayers.selectedLayerIndex = static_cast<int>(recipe.markerLayers.size()) - 1;
            }
            bool bRecipeMoved = false;
            if (buttons.bAddProceduralLayerClicked) {
                Params::MarkerRuleLayer layer;
                layer.parentBundleIdentifier = ResolveSelectedParentBundleIdentifier(
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

            // STEP140 — the tree's own header-extra "X" only RECORDS a pending choice (mutating
            // bundles/ruleLayers/instanceLayers mid-walk would desync the walk's own position-based
            // lookups for the rest of this frame); apply it now the walk above is fully done.
            if (state.bundles.pendingDeleteBundleIdentifier >= 0) {
                const int deletedBundleIdentifier = state.bundles.pendingDeleteBundleIdentifier;
                if (state.bundles.bPendingDeleteBundleCascade)
                    DeleteMarkerLayerBundleCascade(deletedBundleIdentifier, recipe.markerLayerBundles,
                                                   recipe.markerRuleLayers, recipe.markerLayers, recipe.markers);
                else
                    DeleteMarkerLayerBundleGroupOnly(deletedBundleIdentifier, recipe.markerLayerBundles,
                                                     recipe.markerRuleLayers, recipe.markerLayers);
                if (state.bundles.selectedBundleIdentifier == deletedBundleIdentifier)
                    state.bundles.selectedBundleIdentifier = -1;
                state.bundles.pendingDeleteBundleIdentifier = -1;
            }
            if (state.bundles.pendingDeleteManualLayerIndex >= 0) {
                const int deletedLayerIndex = state.bundles.pendingDeleteManualLayerIndex;
                if (state.bundles.bPendingDeleteManualLayerCascade)
                    DeleteMarkerInstanceLayerCascade(deletedLayerIndex, recipe.markerLayers, recipe.markers);
                else
                    DeleteMarkerInstanceLayerOnly(deletedLayerIndex, recipe.markerLayers, recipe.markers);
                if (state.manualLayers.selectedLayerIndex == deletedLayerIndex)
                    state.manualLayers.selectedLayerIndex = -1;
                else if (state.manualLayers.selectedLayerIndex > deletedLayerIndex)
                    --state.manualLayers.selectedLayerIndex;
                state.bundles.pendingDeleteManualLayerIndex = -1;
            }
            if (state.bundles.pendingDeleteProceduralLayerIndex >= 0) {
                const int deletedLayerIndex = state.bundles.pendingDeleteProceduralLayerIndex;
                DeleteMarkerRuleLayer(deletedLayerIndex, recipe.markerRuleLayers);
                if (state.selectedRuleLayerIndex == deletedLayerIndex) {
                    state.selectedRuleLayerIndex = -1;
                    state.selectedRuleIndex      = 0;
                } else if (state.selectedRuleLayerIndex > deletedLayerIndex) {
                    --state.selectedRuleLayerIndex;
                }
                state.bundles.pendingDeleteProceduralLayerIndex = -1;
                bRecipeMoved = true;
            }
            // STEP148 correction (human's own correction — "I thought I told you to have it create a
            // new layer if one did not exist") — a Group's own drop target records this instead of
            // reassigning immediately whenever it has no Manual Layer yet (structural push_back to
            // recipe.markerLayers, unsafe mid-walk, same reasoning as the pending-deletes above);
            // create the Layer AND reassign the recorded instances in one atomic step, now the walk
            // is fully done.
            if (state.bundles.pendingCreateLayerForBundleIdentifier >= 0) {
                ApplyPendingCreateLayerForBundle(state.bundles.pendingCreateLayerForBundleIdentifier,
                                                 state.bundles.pendingCreateLayerMarkerTypeName,
                                                 state.bundles.pendingCreateLayerInstanceIdentifiers,
                                                 recipe.markerLayers, recipe.markers);
                state.bundles.pendingCreateLayerForBundleIdentifier = -1;
                state.bundles.pendingCreateLayerMarkerTypeName.clear();
                state.bundles.pendingCreateLayerInstanceIdentifiers.clear();
            }
            ImGui::Separator();
            bRecipeMoved = DrawRuleLayerListBody(recipe.markerRuleLayers, state, previewDriver, iconManifest,
                                                 typeName, placedMarkers, selectProceduralMarkerInstanceCallback)
                         || bRecipeMoved;
            DrawManualMarkerLayerListBody(state.manualLayers, recipe.markerLayers, recipe.markers,
                                          recipe.geometry, recipe.globalSymmetryMask,
                                          recipe.radialSymmetryRepeatCount, recipe.markerSymmetryFixSettings,
                                          typeName, state.selectedManualInstanceIdentifier,
                                          state.selectedManualInstanceIdentifiers,
                                          state.manualInstanceSelectionAnchorIdentifier,
                                          selectManualMarkerInstanceCallback);
            ImGui::Separator();
            DrawBaseSectionManualInstanceList(recipe.markers, recipe.markerLayers, typeName,
                                              state.selectedManualInstanceIdentifier,
                                              state.selectedManualInstanceIdentifiers,
                                              state.manualInstanceSelectionAnchorIdentifier,
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
