// MarkersTab_Globals_UI.h — the Markers tab's global section: the gamedata root, the icon scan
// request, and the three global marker scale rows (Alloy / Plasma / Spawn).
// Layer: UI. Accuracy class: Visual. TAB_REBUILD_PLAN "§ Markers · Global".
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing type; reported, not invented):
//  1. THE GAMEDATA ROOT AND THE ICON-SCAN REQUEST ARE CALLER-OWNED UI STATE — the app shell
//     (WO E) reads them, exactly the standing HeightmapTab_UI's global gravity and SystemTab_UI's
//     asset-cache directory already have. They are NOT serialized and they do NOT notify
//     Pipeline::PreviewDriver: no stage hashes them, and asking for a regeneration a preview tint
//     cannot affect is the "cheap tweak triggers a full regen" defect. A durable home for them is
//     its own work-order. The per-category Icon Scale, Preview Color and Icon Identity, by
//     contrast, bind directly to `Params::GlobalMarkerSettings` (STEP121) — see
//     `ResolveGlobalMarkerScaleRowFields` below — which IS a real, round-tripped
//     (`GlobalMarkerSettings_Migrate_V2_IO.cpp`), render-consumed struct.
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
#include "IconAtlasPairing_UI.h"
#include "IconGridWidget_UI.h"
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "../params/GlobalMarkerSettings_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// The three categories v1 gave a global scale row. Marker categories the recipe knows about are
// Params::MarkerCategory; these are the DISPLAY rows the plan names, in its order.
enum : int { kMarkerGlobalScaleRowCount = 3 };
inline const char* const markerGlobalScaleRowLabels[kMarkerGlobalScaleRowCount] = {
    "Alloy", "Plasma", "Spawn"
};

struct MarkerGlobalScaleRow {
    RealtimeToggle iconScaleToggle{true};
    RealtimeToggle previewColorToggle{true};
    // NEW — STEP121: this row's OWN popup/highlight state, so each row's picker remembers its own
    // scroll position and highlighted cell independently. Replaces the single shared
    // MarkersTabGlobals::iconGridState + selectedScaleRowIndex "click a row to make it the active
    // target" model this ticket retires — only one popup can be open at a time regardless (imgui's
    // own popup-stack behavior), so nothing is lost by giving each row its own state, and the
    // popup can now seed its highlight from THIS row's current icon on open (§3).
    IconGridState  iconGridState;
};

struct MarkersTabGlobals {
    SectionState      section;
    ScalarSliderRange iconScaleRange{ 0.1f, 10.0f, 0.0f };
    MarkerGlobalScaleRow scaleRows[kMarkerGlobalScaleRowCount];
    float iconButtonSizePixels = 48.0f;    // NEW — Constitution §8, the row's icon-button footprint

    std::string           gamedataDirectory;      // SCOPE NOTE 1
    FilePathPickerOptions gamedataOptions;        // a directory: no extension fence
    ColorSwatchOptions    previewColorOptions;    // picker only, no RGBA fields (plan rule)

    float iconGridHeight     = 160.0f;     // now the POPUP's height, shared layout tunable
    bool  bIconScanRequested = false;      // SCOPE NOTE 2 — the host clears it
};

// The GlobalMarkerSettings fields one scale row edits directly — the direct-binding mechanism,
// mirroring the posture MarkersTab_ManualLayerRowBody_UI.cpp already uses for `layer.iconScale`
// (bind straight to the PARAMS field, no scratch intermediary).
struct GlobalMarkerScaleRowFields {
    float*       scale    = nullptr;
    float*       color    = nullptr;   // 4 floats: colorAlloy/colorPlasma/colorSpawn
    std::string* iconName = nullptr;   // iconNameAlloy/iconNamePlasma/iconNameSpawn
};

// rowIndex -> the GlobalMarkerSettings fields that row edits (Alloy=0/Plasma=1/Spawn=2, the same
// order as markerGlobalScaleRowLabels). Out-of-range resolves to every pointer null (Constitution
// §6, mirroring IsMarkerInstanceLayerLocked's out-of-range-safe posture) — the fixed
// kMarkerGlobalScaleRowCount loop in DrawMarkersTabGlobals never passes one, but this helper does
// not trust that.
inline GlobalMarkerScaleRowFields ResolveGlobalMarkerScaleRowFields(
    Params::GlobalMarkerSettings& settings, int rowIndex) {
    switch (rowIndex) {
        case 0: return { &settings.scaleAlloy,  settings.colorAlloy,  &settings.iconNameAlloy };
        case 1: return { &settings.scalePlasma, settings.colorPlasma, &settings.iconNamePlasma };
        case 2: return { &settings.scaleSpawn,  settings.colorSpawn,  &settings.iconNameSpawn };
        default: return {};
    }
}

// Draws the global section. `iconManifest`/`pairingLookup` are both nullable: with no resident
// atlas the icon column shows a disabled placeholder button instead of a thumbnail.
void DrawMarkersTabGlobals(MarkersTabGlobals& globals, Params::GlobalMarkerSettings& globalMarkerSettings,
                           const IconAtlasManifest* iconManifest, const IconAtlasPairingLookup* pairingLookup);

} // namespace Ui
} // namespace SanmapGen
