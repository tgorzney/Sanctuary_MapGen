// MarkersTab_LinksHeaderExtras_UI.cpp — DrawMarkerLinkHeaderExtra/DrawMarkerLinkBody, the
// aspect-split sibling of MarkersTab_Links_UI.cpp (ARCH §1.5 — see MarkersTab_Links_UI.h). Mirrors
// MarkersTab_BundleHeaderExtras_UI.cpp's own precedent one tier up.
// STEP241/ARCH §19.31 correction — the Link's own header-extra widens from STEP239's
// [COL][swatch][X]-only cluster to the FULL [Icon Size][Grid][SYM][V/I][COL][swatch][X] set: a Link
// now governs every Section/Group-equivalent setting (master/slave, human's own ruling), not just
// color, and is itself the ONE editable surface for every one of them.
// STEP242 (ARCH §19.31's same-day follow-up amendment, governed field #7) adds [LOCK], right after
// [V/I] — mirroring the Layer tier's own built-in [o]/[L]/[X] affordance strip ordering.
#include "MarkersTab_Links_UI.h"
#include "MarkersTab_ManualInstanceSelection_UI.h"
#include "SymmetryClusterInstanceList_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Mirrors DrawManualMarkerLayerColorOverrideHeaderControl's own [COL][swatch] shape
// (MarkersTab_ManualLayerRowBody_UI.cpp) exactly, bound directly to Params::MarkerLink instead of
// Params::MarkerInstanceLayer — this Link IS the source of truth (ARCH §19.31), so there is no
// "effective"/disabled-while-linked concept here, unlike the Layer-tier control.
void DrawMarkerLinkColorOverrideHeaderControl(Params::MarkerLink& link, RealtimeToggle& colorToggle,
                                              bool& bAnyCommitted) {
    if (link.bColorOverrideEnabled)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    const bool bOverrideCommitted = ImGui::SmallButton("COL##linkColorOverride");
    if (link.bColorOverrideEnabled) ImGui::PopStyleColor();
    if (bOverrideCommitted) link.bColorOverrideEnabled = !link.bColorOverrideEnabled;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color Override");
    ImGui::SameLine();
    ImGui::BeginDisabled(!link.bColorOverrideEnabled);
    ColorSwatchOptions swatchOptions;
    swatchOptions.bLabelHidden          = true;
    swatchOptions.swatchWidth           = kMarkerLinkColorOverrideSwatchWidthPixels;
    swatchOptions.bRealtimeToggleHidden = true;   // color edits are always realtime, no choice
    const bool bColorCommitted =
        DrawColorSwatch("LinkColorOverrideSwatch", link.color, swatchOptions, colorToggle).bCommitted;
    ImGui::EndDisabled();
    if (bOverrideCommitted || bColorCommitted) bAnyCommitted = true;
}

// STEP241 — a straight V/I toggle bound directly to `link.bHidden`, mirroring
// DrawMarkerGroupLeafHeaderExtra's own Manual-leaf V/I button shape one tier up
// (MarkersTab_BundleHeaderExtras_UI.cpp) but with no disable concept — the Link IS the source of
// truth for a bound Layer's own bHidden while linked.
void DrawMarkerLinkVisibilityHeaderControl(Params::MarkerLink& link, bool& bAnyCommitted) {
    if (ImGui::SmallButton(link.bHidden ? "I##linkVisible" : "V##linkVisible")) {
        link.bHidden = !link.bHidden;
        bAnyCommitted = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Visible / Invisible in preview");
}

// STEP242, ARCH §19.31 follow-up amendment (governed field #7) — a straight L/U toggle bound
// directly to `link.bLocked`, mirroring DrawMarkerLinkVisibilityHeaderControl's own label-swap shape
// just above (no highlight-color concept, unlike SYM/GRID/COL) and the Layer tier's own built-in
// [L]/[U] affordance-strip label convention (DraggableListWidget_RowAffordances_UI.h) — no disable
// concept here either, the Link IS the source of truth for a bound Layer's own bLocked while linked.
void DrawMarkerLinkLockHeaderControl(Params::MarkerLink& link, bool& bAnyCommitted) {
    if (ImGui::SmallButton(link.bLocked ? "L##linkLocked" : "U##linkLocked")) {
        link.bLocked = !link.bLocked;
        bAnyCommitted = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Locked / Unlocked");
}

// STEP241 — mirrors DrawMarkerLayerIconSizeHeaderControl's own shape (MarkersTab_ManualLayerRowBody_UI.cpp)
// bound directly to `link.iconScale`, reusing the SAME kMarkerLayerIconSize* width constants.
void DrawMarkerLinkIconScaleHeaderControl(Params::MarkerLink& link, MarkerLinksState_UI& state,
                                          bool& bAnyCommitted) {
    if (DrawSliderScalarCompact("Icon Size", link.iconScale, state.iconScaleRange, state.iconScaleToggle,
                                kMarkerLayerIconSizeTrackWidthPixels, kMarkerLayerIconSizeFieldWidthPixels,
                                WidgetStyle(), "%.2f", /*bShowRealtimeToggle=*/false).bCommitted)
        bAnyCommitted = true;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Icon Size");
}

// STEP241 — mirrors DrawMarkerLayerGridSnapHeaderControl's own shape, bound directly to
// `link.bGridSnapEnabled`/`link.gridSnapSizeWorldUnits`.
void DrawMarkerLinkGridSnapHeaderControl(Params::MarkerLink& link, MarkerLinksState_UI& state,
                                         bool& bAnyCommitted) {
    if (link.bGridSnapEnabled)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    const bool bGridToggleCommitted = ImGui::SmallButton("GRID##linkGridSnap");
    if (link.bGridSnapEnabled) ImGui::PopStyleColor();
    if (bGridToggleCommitted) { link.bGridSnapEnabled = !link.bGridSnapEnabled; bAnyCommitted = true; }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Snap to Grid");
    ImGui::SameLine();
    ImGui::BeginDisabled(!link.bGridSnapEnabled);
    if (DrawSliderScalarCompact("Grid Size", link.gridSnapSizeWorldUnits, state.gridSnapSizeRange,
                                state.gridSnapSizeToggle, kMarkerLayerGridSizeTrackWidthPixels,
                                kMarkerLayerGridSizeFieldWidthPixels, WidgetStyle(), "%.2f",
                                /*bShowRealtimeToggle=*/false).bCommitted)
        bAnyCommitted = true;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Grid Size");
    ImGui::EndDisabled();
}

// STEP241 — mirrors DrawMarkerLayerSymmetryToggleHeaderControl's own shape, bound directly to
// `link.bSymmetryEnabled`; never touches `link.symmetry`'s own configured fields, exactly the same
// non-destructive-gate posture the Layer-tier control already has.
void DrawMarkerLinkSymmetryToggleHeaderControl(Params::MarkerLink& link, bool& bAnyCommitted) {
    if (link.bSymmetryEnabled)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    const bool bSymmetryCommitted = ImGui::SmallButton("SYM##linkSymmetry");
    if (link.bSymmetryEnabled) ImGui::PopStyleColor();
    if (bSymmetryCommitted) { link.bSymmetryEnabled = !link.bSymmetryEnabled; bAnyCommitted = true; }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Symmetry (global)");
}

} // namespace

// One Link's own header-extra: double-click-to-rename (a SCRATCH buffer, not `link.name` directly —
// DrawSectionBegin's own PushID(label) would otherwise churn every keystroke, the identical reason
// DrawMarkerLayerBundleNodeHeaderExtra's own rename uses one) committing ONLY `link.name` — STEP241
// retracts STEP239's cascade-write into every bound Bundle's own `name` (CommitMarkerLinkRename's
// own header comment, MarkersTab_Links_UI.h) — OR (while not renaming) the right-aligned
// [Icon Size][Grid][SYM][V/I][LOCK][COL][swatch][X] cluster. Must run FIRST, immediately after
// DrawSectionBegin — the "last item" this reads via GetItemRectMin/GetItemRectSize is that header's
// own InvisibleButton (Section_UI.cpp), the same "run first" contract the Bundle tree's own
// header-extras use.
void DrawMarkerLinkHeaderExtra(Params::MarkerLink& link, MarkerLinksState_UI& state, bool& bAnyCommitted) {
    const ImVec2 itemMin = ImGui::GetItemRectMin();
    // Matches Section_UI.cpp's own labelLeftX math for the default bArrowShown == true case: the
    // label starts one bar-height (== this header's own InvisibleButton height) right of its origin.
    const float labelStartX = itemMin.x + ImGui::GetItemRectSize().y;

    if (state.renamingLinkIdentifier == link.identifier) {
        ImGui::SetCursorScreenPos(ImVec2(labelStartX, itemMin.y));
        if (state.bRenameFocusPending) { ImGui::SetKeyboardFocusHere(); state.bRenameFocusPending = false; }
        TextInputRules nameRules;
        nameRules.maximumLength = 48; nameRules.bAllowEmpty = false; nameRules.fallbackText = "Link";
        DrawTextInput("##renameLink", state.renameScratchText, nameRules, WidgetStyle(), nullptr,
                     /*bLabelHidden=*/true);
        if (ImGui::IsItemDeactivated()) {
            CommitMarkerLinkRename(link, SanitizeTextInput(state.renameScratchText, nameRules));
            state.renamingLinkIdentifier = -1;
            bAnyCommitted = true;
        }
        return;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        state.renamingLinkIdentifier = link.identifier;
        state.renameScratchText      = link.name;
        state.bRenameFocusPending    = true;
        return;
    }

    const float availableWidth = ImGui::GetContentRegionAvail().x;
    if (availableWidth > kMarkerLinkHeaderClusterWidthPixels)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - kMarkerLinkHeaderClusterWidthPixels);
    DrawMarkerLinkIconScaleHeaderControl(link, state, bAnyCommitted);
    ImGui::SameLine(0.0f, kMarkerLinkHeaderClusterSpacingPixels);
    DrawMarkerLinkGridSnapHeaderControl(link, state, bAnyCommitted);
    ImGui::SameLine(0.0f, kMarkerLinkHeaderClusterSpacingPixels);
    DrawMarkerLinkSymmetryToggleHeaderControl(link, bAnyCommitted);
    ImGui::SameLine(0.0f, kMarkerLinkHeaderClusterSpacingPixels);
    DrawMarkerLinkVisibilityHeaderControl(link, bAnyCommitted);
    ImGui::SameLine(0.0f, kMarkerLinkHeaderClusterSpacingPixels);
    DrawMarkerLinkLockHeaderControl(link, bAnyCommitted);
    ImGui::SameLine(0.0f, kMarkerLinkHeaderClusterSpacingPixels);
    DrawMarkerLinkColorOverrideHeaderControl(link, state.colorToggle, bAnyCommitted);
    ImGui::SameLine(0.0f, kMarkerLinkHeaderClusterSpacingPixels);
    if (ImGui::SmallButton("X##deleteLink")) state.pendingDeleteLinkIdentifier = link.identifier;
}

// STEP248 — the hierarchical body: one plain label per represented Marker Type (from
// PartitionLinkedManualInstancesByType), each followed by that type's Link-tagged instances rendered
// through the SAME rowOrder/ManualInstanceRowInteractionContext_UI/DrawSymmetryClusterInstanceList/
// DrawManualInstanceRow block DrawBaseSectionManualInstanceList (MarkersTab_UI.cpp, the base
// "Instances" list) already uses — copied verbatim one tier over, item source swapped from
// "un-Layered instances of type X" to "Link-tagged instances of type X." No nested Section widget —
// a plain ImGui::TextUnformatted label, no per-type collapse/settings state of its own.
void DrawMarkerLinkBody(const Params::MarkerLink& link, Params::MapRecipe& recipe,
                        int& selectedManualInstanceIdentifier,
                        std::vector<int>& selectedManualInstanceIdentifiers,
                        int& anchorIdentifier,
                        const std::function<void(int, const std::vector<int>&)>&
                            selectManualMarkerInstanceCallback) {
    const auto byType = PartitionLinkedManualInstancesByType(recipe.markers, link.identifier);
    if (byType.empty()) { ImGui::TextDisabled("(no instances)"); return; }

    for (const auto& typeAndInstances : byType) {
        const std::string& typeName = typeAndInstances.first;
        const std::vector<std::pair<int, int>>& instances = typeAndInstances.second;
        ImGui::PushID(typeName.c_str());
        ImGui::TextUnformatted(typeName.c_str());   // a plain label, NOT a Section widget

        std::vector<int> rowOrder;   // this list's own display-order identifiers, for Shift-range selection
        rowOrder.reserve(instances.size());
        for (const std::pair<int, int>& groupTransformIndex : instances)
            rowOrder.push_back(recipe.markers[static_cast<std::size_t>(groupTransformIndex.first)]
                .transforms[static_cast<std::size_t>(groupTransformIndex.second)].instanceIdentifier);

        ManualInstanceRowInteractionContext_UI interaction;
        interaction.primaryIdentifier   = &selectedManualInstanceIdentifier;
        interaction.selectedIdentifiers = &selectedManualInstanceIdentifiers;
        interaction.anchorIdentifier    = &anchorIdentifier;
        interaction.rowOrder            = &rowOrder;
        interaction.selectManualMarkerInstanceCallback = selectManualMarkerInstanceCallback;

        DrawSymmetryClusterInstanceList<std::pair<int, int>>(instances,
            [&](const std::pair<int, int>& groupTransformIndex) {
                return recipe.markers[static_cast<std::size_t>(groupTransformIndex.first)]
                    .transforms[static_cast<std::size_t>(groupTransformIndex.second)].symmetryGroupIdentifier;
            },
            [](int groupIdentifier, int /*bucketSize*/) { return groupIdentifier != 0; },
            [&](const std::pair<int, int>& groupTransformIndex) {
                DrawManualInstanceRow(recipe.markers, groupTransformIndex, interaction);
            });
        ImGui::PopID();
    }
}

} // namespace Ui
} // namespace SanmapGen
