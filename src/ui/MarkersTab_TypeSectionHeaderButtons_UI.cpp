// MarkersTab_TypeSectionHeaderButtons_UI.cpp — see MarkersTab_TypeSectionHeaderButtons_UI.h.
#include "MarkersTab_TypeSectionHeaderButtons_UI.h"
#include "MarkersTab_Globals_UI.h"
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
         + SmallButtonWidth("+ Link")     + kHeaderButtonSpacingPixels
         + SmallButtonWidth(bHidden ? "Unhide" : "Hide");
}

} // namespace

SectionOptions HeaderButtonsSectionOptions(const MarkersTabGlobals& globals, bool bHidden) {
    SectionOptions options;
    options.reservedRightWidth = TypeSectionMarkerSettingsRowWidth(globals) + kHeaderButtonSpacingPixels
                                + TypeSectionHeaderButtonClusterWidth(bHidden) + kHeaderButtonSpacingPixels;
    return options;
}

TypeSectionHeaderButtons_UI DrawRightAlignedTypeSectionHeaderButtons(
        MarkersTabGlobals& globals, int rowIndex, Params::GlobalMarkerSettings& globalMarkerSettings,
        const IconAtlasManifest* iconManifest, const IconAtlasPairingLookup* pairingLookup, bool bHidden,
        bool bSelectionNonEmpty) {
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
    // STEP239 — unlike "+ Group"/"+ Layer" (same-type only, §2), "+ Link" always acts on the WHOLE
    // tab-wide selection regardless of which Type-section's own copy is clicked (DESIGN_MarkerLink_R1
    // §3.6) — an identical, always-available affordance drawn in every section header on purpose.
    // Enabled only while the tab-wide selection is non-empty (the ticket's own explicit gate).
    ImGui::BeginDisabled(!bSelectionNonEmpty);
    result.bAddLinkClicked = ImGui::SmallButton("+ Link");
    ImGui::EndDisabled();
    ImGui::SameLine(0.0f, kHeaderButtonSpacingPixels);
    result.bHideToggleClicked = ImGui::SmallButton(hideLabel);
    return result;
}

} // namespace Ui
} // namespace SanmapGen
