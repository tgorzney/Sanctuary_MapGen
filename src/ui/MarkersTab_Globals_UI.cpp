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

// One global scale row: select it, scale it, tint it. Selecting a row is what the shared icon
// grid below the rows writes into, so three rows share ONE grid instead of drawing three.
void DrawGlobalScaleRow(MarkersTabGlobals& globals, int rowIndex) {
    MarkerGlobalScaleRow& row = globals.scaleRows[rowIndex];
    ImGui::PushID(rowIndex);
    if (ImGui::Selectable(markerGlobalScaleRowLabels[rowIndex], rowIndex == globals.selectedScaleRowIndex))
        globals.selectedScaleRowIndex = rowIndex;
    DrawSliderScalar("Icon Scale", row.iconScale, globals.iconScaleRange, row.iconScaleToggle,
                     WidgetStyle(), "%.2f");
    DrawColorSwatch("Preview Color", row.previewColor, globals.previewColorOptions,
                    row.previewColorToggle);
    ImGui::Text("Icon id: %d", row.iconId);
    ImGui::PopID();
}

// The ONE icon grid the three rows share. A pick writes the SELECTED row's icon id.
void DrawGlobalIconPicker(MarkersTabGlobals& globals, const IconAtlasManifest* iconManifest) {
    MarkerGlobalScaleRow* const row = SelectedMarkerScaleRow(globals);
    if (row == nullptr) {
        ImGui::TextUnformatted("Select a scale row to give it an icon.");
        return;
    }
    if (iconManifest == nullptr) {
        ImGui::TextUnformatted("No resident icon atlas: run the host's icon scan first.");
        return;
    }
    if (DrawIconGrid("Marker Icons", *iconManifest, globals.iconGridState, globals.iconGridHeight))
        row->iconId = globals.iconGridState.selectedIconId;
}

} // namespace

void DrawMarkersTabGlobals(MarkersTabGlobals& globals, const IconAtlasManifest* iconManifest) {
    if (!DrawSectionBegin("Global", globals.section)) return;
    DrawGamedataSource(globals);
    ImGui::Separator();
    for (int rowIndex = 0; rowIndex < kMarkerGlobalScaleRowCount; ++rowIndex)
        DrawGlobalScaleRow(globals, rowIndex);
    DrawGlobalIconPicker(globals, iconManifest);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
