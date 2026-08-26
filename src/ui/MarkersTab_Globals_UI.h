// MarkersTab_Globals_UI.h — the Markers tab's global section: the gamedata root, the icon scan
// request, and the three global marker scale rows (Alloy / Plasma / Spawn).
// Layer: UI. Accuracy class: Visual. TAB_REBUILD_PLAN "§ Markers · Global".
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing type; reported, not invented):
//  1. NOTHING IN THIS FILE IS RECIPE CONTENT. The gamedata root, the per-category icon scale and
//     the per-category preview color have no `_PARAMS` home in the tree, so they are CALLER-OWNED
//     UI state the app shell (WO E) reads — exactly the standing HeightmapTab_UI's global gravity
//     and SystemTab_UI's asset-cache directory already have. They are NOT serialized and they do
//     NOT notify Pipeline::PreviewDriver: no stage hashes them, and asking for a regeneration a
//     preview tint cannot affect is the "cheap tweak triggers a full regen" defect. A durable
//     home for them is its own work-order.
//  2. THE TAB NEVER SCANS. Reading gamedata and building an atlas is the IO layer's
//     (ASSET_LOADING_SPEC / M5-4) and the manifest owner is the app shell, so "Scan for Icons"
//     only RAISES `bIconScanRequested`; the host clears it after running the scan (ARCH §3.2).
//  3. NO "Use GPU" TOGGLE IS DRAWN even though v1 had one on this tab: backend choice is a
//     dispatcher decision owned by `Sys::DispatchPolicy` and already exposed by SystemTab_UI.
//     A second control over the same decision is exactly the rival toggle ARCH §4 forbids.
#pragma once
#include <string>
#include "ColorSwatch_UI.h"
#include "FilePathPicker_UI.h"
#include "IconGridWidget_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"

namespace SanmapGen {
namespace Ui {

// The three categories v1 gave a global scale row. Marker categories the recipe knows about are
// Params::MarkerCategory; these are the DISPLAY rows the plan names, in its order.
enum : int { kMarkerGlobalScaleRowCount = 3 };
inline const char* const markerGlobalScaleRowLabels[kMarkerGlobalScaleRowCount] = {
    "Alloy", "Plasma", "Spawn"
};

struct MarkerGlobalScaleRow {
    float          iconScale = 1.0f;
    float          previewColor[kColorSwatchChannelCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
    int            iconId = -1;          // the atlas id the picker last emitted for this row
    RealtimeToggle iconScaleToggle{true};
    RealtimeToggle previewColorToggle{true};
};

struct MarkersTabGlobals {
    SectionState      section;
    ScalarSliderRange iconScaleRange{ 0.1f, 10.0f, 0.0f };
    MarkerGlobalScaleRow scaleRows[kMarkerGlobalScaleRowCount];

    std::string           gamedataDirectory;      // SCOPE NOTE 1
    FilePathPickerOptions gamedataOptions;        // a directory: no extension fence
    ColorSwatchOptions    previewColorOptions;    // picker only, no RGBA fields (plan rule)

    IconGridState iconGridState;
    float iconGridHeight        = 160.0f;
    int   selectedScaleRowIndex = 0;
    bool  bIconScanRequested    = false;          // SCOPE NOTE 2 — the host clears it
};

// The scale row the icon picker is currently pointed at, or null when the selection points at
// nothing (Constitution §6 — an index is validated, never trusted).
inline MarkerGlobalScaleRow* SelectedMarkerScaleRow(MarkersTabGlobals& globals) {
    if (globals.selectedScaleRowIndex < 0 || globals.selectedScaleRowIndex >= kMarkerGlobalScaleRowCount)
        return nullptr;
    return &globals.scaleRows[globals.selectedScaleRowIndex];
}

// Draws the global section. `iconManifest` is nullable: with no resident atlas the icon column
// reports the row's stored id instead of a grid.
void DrawMarkersTabGlobals(MarkersTabGlobals& globals, const IconAtlasManifest* iconManifest);

} // namespace Ui
} // namespace SanmapGen
