// MarkersTab_Globals_UI.cpp — the imgui composition of the Markers tab's global section.
// Layer: UI. Shared widgets only: FilePathPicker / SliderScalar / ColorSwatch / IconGrid /
// Section. Nothing here notifies Pipeline::PreviewDriver (MarkersTab_Globals_UI.h SCOPE NOTE 1).
#include "MarkersTab_Globals_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The gamedata ROOT and the scan button. Neither reads a file: the picker only reports the path
// and the button only raises the request flag (SCOPE NOTE 2).
void DrawGamedataSource(MarkersTabGlobals& globals) {
    DrawFilePathPicker("Gamedata Folder", globals.gamedataDirectory, globals.gamedataOptions);
    if (ImGui::Button("Scan for Icons")) globals.bIconScanRequested = true;
    if (globals.bIconScanRequested) ImGui::TextUnformatted("Icon scan requested - waiting on the host.");
}

} // namespace

// The row's icon button + its own popup grid. Mirrors DrawColorSwatch's shape exactly
// (ColorButton -> OpenPopup on click -> unconditional BeginPopup/EndPopup every frame), swapping
// ColorButton/ColorPicker4 for ImageButton/DrawIconGrid.
void DrawGlobalScaleRowIconButton(MarkerGlobalScaleRow& row, std::string& iconNameField,
                                  const MarkersTabGlobals& globals, const IconAtlasManifest* iconManifest,
                                  const IconAtlasPairingLookup* pairingLookup) {
    const int currentIconId = pairingLookup != nullptr
        ? pairingLookup->Resolve(iconNameField).thumbnailIconId : kInvalidIconId;
    const bool bHasIcon = iconManifest != nullptr && currentIconId >= 0
                       && currentIconId < iconManifest->EntryCount();
    const ImVec2 buttonSize(globals.iconButtonSizePixels, globals.iconButtonSizePixels);

    bool bOpenRequested = false;
    if (bHasIcon) {
        const IconAtlasEntry& entry = iconManifest->entries[static_cast<std::size_t>(currentIconId)];
        const ImTextureID texture = static_cast<ImTextureID>(iconManifest->PageTextureIdentifier(entry.atlasPage));
        bOpenRequested = ImGui::ImageButton("##icon", texture, buttonSize,
                                            ImVec2(entry.uvMinimumX, entry.uvMinimumY),
                                            ImVec2(entry.uvMaximumX, entry.uvMaximumY));
    } else {
        ImGui::BeginDisabled(iconManifest == nullptr);
        bOpenRequested = ImGui::Button("?##icon", buttonSize);
        ImGui::EndDisabled();
    }
    if (bOpenRequested) {
        // Seed the popup's highlight with THIS row's CURRENT icon (or "none"), so it opens showing
        // what is already picked rather than whatever another row last touched.
        row.iconGridState.selectedIconIndex = currentIconId;
        row.iconGridState.selectedIconId    = currentIconId;
        ImGui::OpenPopup("##iconPicker");
    }
    if (ImGui::BeginPopup("##iconPicker")) {
        if (iconManifest == nullptr)
            ImGui::TextUnformatted("No resident icon atlas: run the host's icon scan first.");
        else
            DrawIconGrid("##globalMarkerIconGrid", *iconManifest, row.iconGridState, globals.iconGridHeight);
        ImGui::EndPopup();
    }
}

// STEP136 (was DrawGlobalScaleRow, STEP134) — one Type-section header's own marker-settings row:
// icon button / icon-color swatch (unlabeled) / "Selected" label + select-color swatch / compact
// scale slider (unlabeled) — every control bound directly to `Params::GlobalMarkerSettings`, no
// scratch intermediary. No type-name label here: the Type-section's own header text already shows
// it (MarkersTab_UI.cpp). Human's own instruction: color edits should ALWAYS be realtime — no RT
// button on either color swatch, and none on the scale slider either (GlobalMarkerSettings never
// triggers anything beyond a preview repaint). No AlignTextToFramePadding on any label: every item
// in this SameLine() run must share the row's own top Y (the acceptance test's literal contract),
// and align-to-frame-padding would shift a label's item rect down by FramePadding.y and break that.
void DrawTypeSectionMarkerSettingsRow(MarkersTabGlobals& globals, int rowIndex,
                                      Params::GlobalMarkerSettings& globalMarkerSettings,
                                      const IconAtlasManifest* iconManifest,
                                      const IconAtlasPairingLookup* pairingLookup) {
    const GlobalMarkerScaleRowFields fields = ResolveGlobalMarkerScaleRowFields(globalMarkerSettings, rowIndex);
    if (fields.scale == nullptr) return;   // Constitution §6 — an out-of-range row draws nothing
    MarkerGlobalScaleRow& row = globals.scaleRows[rowIndex];

    ImGui::PushID(rowIndex);
    DrawGlobalScaleRowIconButton(row, *fields.iconName, globals, iconManifest, pairingLookup);
    ImGui::SameLine();

    ColorSwatchOptions compactSwatchOptions = globals.previewColorOptions;   // COPY: do not mutate
                                                                              // the shared section-
                                                                              // level options struct
    compactSwatchOptions.bLabelHidden = true;
    compactSwatchOptions.swatchWidth  = kMarkerGlobalScaleRowSwatchWidthPixels;
    compactSwatchOptions.bRealtimeToggleHidden = true;   // color edits are always realtime, no choice
    DrawColorSwatch("PreviewColor", fields.color, compactSwatchOptions, row.previewColorToggle);
    ImGui::SameLine();

    ImGui::TextUnformatted("Selected");
    ImGui::SameLine();
    DrawColorSwatch("SelectColor", fields.selectColor, compactSwatchOptions, row.selectColorToggle);
    ImGui::SameLine();

    DrawSliderScalarCompact("Icon Scale (Global)", *fields.scale, globals.iconScaleRange,
                            row.iconScaleToggle, kMarkerGlobalScaleRowTrackWidthPixels,
                            kMarkerGlobalScaleRowFieldWidthPixels, WidgetStyle(), "%.2f",
                            /*bShowRealtimeToggle=*/false);
    ImGui::PopID();
}

void DrawMarkersTabGlobals(MarkersTabGlobals& globals) {
    if (!DrawSectionBegin("Global", globals.section)) return;
    DrawGamedataSource(globals);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
