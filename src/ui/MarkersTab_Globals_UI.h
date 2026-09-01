// MarkersTab_Globals_UI.h — the Markers tab's global section (the gamedata root and the icon scan
// request) plus the per-Type marker-settings row (icon / icon color / select color / global scale)
// STEP136 relocates onto each Type-section's own header (MarkersTab_UI.cpp) — still binds directly
// to `Params::GlobalMarkerSettings`, only WHERE it draws moved out of this section.
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
#include "LabelledDialWidget_UI.h"   // NEW — STEP240: DialRange/DrawDialCompact for the base+selected
                                     // icon-size pair (ARCH §19.32)
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "imgui.h"
#include "../params/GlobalMarkerSettings_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// The three categories v1 gave a global scale row. Marker categories the recipe knows about are
// Params::MarkerCategory; these are the DISPLAY rows the plan names, in its order.
enum : int { kMarkerGlobalScaleRowCount = 3 };
inline const char* const markerGlobalScaleRowLabels[kMarkerGlobalScaleRowCount] = {
    "Alloy", "Plasma", "Spawn"
};

// STEP134 — the compact row's own fixed geometry. Named constants, not struct fields: a fixed
// layout footprint (like kMarkerLayerColorOverrideSwatchWidthPixels,
// MarkersTab_ManualLayerRowBody_UI.h), not a user-tunable recipe value, so Constitution §8's
// "settings, not literals" does not reach it. The design doc's own flagged width-budget risk
// (DESIGN_MarkersUICorrectionRound2_R1.md item 1+14) is why the track and icon are the two
// SHRUNK-FIRST values here rather than the plan's rough 90px/48px estimate.
// kMarkerGlobalScaleRowTrackWidthPixels retired — STEP240 converts the row's scale control(s) from
// DrawSliderScalarCompact (a linear TRACK) to DrawDialCompact (a circular knob sized from the row's
// own frame height, ARCH §19.32); a "track width" no longer names anything this row draws.
inline constexpr float kMarkerGlobalScaleRowFieldWidthPixels  = 42.0f;
inline constexpr float kMarkerGlobalScaleRowSwatchWidthPixels = 20.0f;

struct MarkerGlobalScaleRow {
    RealtimeToggle iconScaleToggle{true};
    RealtimeToggle previewColorToggle{true};
    // NEW — STEP134: the select-color swatch's own toggle, independent of the two above (the
    // struct's existing two-separate-toggles convention — each field keeps its own
    // RT-tweakability, never merged).
    RealtimeToggle selectColorToggle{true};
    // NEW — STEP240 (ARCH §19.32): the "selected" icon-size dial's own RT bookkeeping, independent
    // of iconScaleToggle above — mirrors selectColorToggle's own "each field keeps its own"
    // reasoning one control over. Never surfaced as a drawn RT button (bShowRealtimeToggle=false,
    // same "scale edits are always realtime" posture as the base-scale dial), but StepDialInteraction
    // still needs a live RealtimeToggle instance to run its commit-timing bookkeeping against.
    RealtimeToggle selectedScaleToggle{true};
    // NEW — STEP121: this row's OWN popup/highlight state, so each row's picker remembers its own
    // scroll position and highlighted cell independently. Replaces the single shared
    // MarkersTabGlobals::iconGridState + selectedScaleRowIndex "click a row to make it the active
    // target" model this ticket retires — only one popup can be open at a time regardless (imgui's
    // own popup-stack behavior), so nothing is lost by giving each row's picker its own state, and
    // the popup can now seed its highlight from THIS row's current icon on open (§3).
    IconGridState  iconGridState;
};

struct MarkersTabGlobals {
    SectionState      section;
    // No longer read by this file's own row draw as of STEP240 (the row's scale controls moved to
    // the narrower scaleDialRange below) — left in place rather than removed: MarkersTab_UI_Test.cpp
    // still asserts its default bounds, and retiring a field neither the ticket nor its own test
    // asked to change is out of this ticket's scope.
    ScalarSliderRange iconScaleRange{ 0.1f, 10.0f, 0.0f };
    // NEW — STEP240 (ARCH §19.32): the base+selected icon-size dial pair's own narrower range,
    // scoped to just this row — deliberately NOT a change to iconScaleRange above (that field's own
    // wider {0.1, 10.0} bounds are untouched, matching ARCH §19.32's explicit "does not change the
    // per-Manual-Layer iconScale control's own wider range" ruling).
    DialRange         scaleDialRange{ 0.25f, 2.0f, 0.0f, 200.0f };
    MarkerGlobalScaleRow scaleRows[kMarkerGlobalScaleRowCount];
    // STEP134: shrunk 48->32 — the FIRST of the ticket's own "shrink icon button and track width
    // first" width-budget remedies, now that the row carries 5 controls on one line instead of 3
    // across three ImGui::Columns.
    float iconButtonSizePixels = 32.0f;    // NEW — Constitution §8, the row's icon-button footprint

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
    float*       scale         = nullptr;
    // NEW — STEP240 (ARCH §19.32): scaleSelectedAlloy/Plasma/Spawn, the per-type "selected" icon-size
    // counterpart to `scale` above — same direct-binding posture, no scratch intermediary.
    float*       selectedScale = nullptr;
    float*       color         = nullptr;   // 4 floats: colorAlloy/colorPlasma/colorSpawn
    std::string* iconName      = nullptr;   // iconNameAlloy/iconNamePlasma/iconNameSpawn
    // NEW — STEP134 (ARCH §19.17's select-tint field, item 14): 4 floats:
    // selectColorAlloy/Plasma/Spawn. selectColorDefault is never resolved to by this per-type row —
    // it is the resolver's own unmatched-name fallback (GlobalMarkerSettings_PARAMS.h), not a 4th row.
    float*       selectColor   = nullptr;
};

// rowIndex -> the GlobalMarkerSettings fields that row edits (Alloy=0/Plasma=1/Spawn=2, the same
// order as markerGlobalScaleRowLabels). Out-of-range resolves to every pointer null (Constitution
// §6, mirroring IsMarkerInstanceLayerLocked's out-of-range-safe posture) — the fixed
// kMarkerGlobalScaleRowCount loop in DrawMarkersTabGlobals never passes one, but this helper does
// not trust that.
inline GlobalMarkerScaleRowFields ResolveGlobalMarkerScaleRowFields(
    Params::GlobalMarkerSettings& settings, int rowIndex) {
    switch (rowIndex) {
        case 0: return { &settings.scaleAlloy,  &settings.scaleSelectedAlloy,
                         settings.colorAlloy,  &settings.iconNameAlloy, settings.selectColorAlloy };
        case 1: return { &settings.scalePlasma, &settings.scaleSelectedPlasma,
                         settings.colorPlasma, &settings.iconNamePlasma, settings.selectColorPlasma };
        case 2: return { &settings.scaleSpawn,  &settings.scaleSelectedSpawn,
                         settings.colorSpawn,  &settings.iconNameSpawn, settings.selectColorSpawn };
        default: return {};
    }
}

// The row's icon button + its own popup grid — declared here (not file-local/anonymous), like
// DrawTypeSectionMarkerSettingsRow below, so the headless-frame acceptance test can drive
// DrawTypeSectionMarkerSettingsRow's own four constituent calls one at a time and capture each
// control's own item rect (there is no other way to see an INTERMEDIATE item rect from inside one
// opaque DrawTypeSectionMarkerSettingsRow call).
void DrawGlobalScaleRowIconButton(MarkerGlobalScaleRow& row, std::string& iconNameField,
                                  const MarkersTabGlobals& globals, const IconAtlasManifest* iconManifest,
                                  const IconAtlasPairingLookup* pairingLookup);

// STEP136 (was DrawGlobalScaleRow, STEP134): one Type-section header's own marker-settings row,
// drawn as a genuine single ImGui::SameLine()-chained line — declared here (not file-local/
// anonymous) so the headless-frame acceptance test can drive it directly, mirroring
// DrawManualMarkerLayerColorOverrideHeaderControl's own test-callable posture
// (MarkersTab_ManualLayerRowBody_UI.h). Human's own explicit order: icon thumbnail (unlabeled) ->
// icon-color swatch (unlabeled) -> "Selected" label + select-color swatch -> base icon-size dial ->
// selected icon-size dial (both unlabeled — DrawDialCompact's own `label` param is tooltip/ID-only,
// never rendered, STEP236). No type-name label: the Type-section's own header text already shows it
// (MarkersTab_UI.cpp). STEP240 (ARCH §19.32) converts the row's ONE scale slider into this pair of
// compact dials — DrawSliderScalarCompact is no longer used by this row.
void DrawTypeSectionMarkerSettingsRow(MarkersTabGlobals& globals, int rowIndex,
                                      Params::GlobalMarkerSettings& globalMarkerSettings,
                                      const IconAtlasManifest* iconManifest,
                                      const IconAtlasPairingLookup* pairingLookup);

// This row's own total rendered width (icon button + icon-color swatch + "Selected" label +
// select-color swatch + base icon-size dial + selected icon-size dial, the exact
// DrawTypeSectionMarkerSettingsRow sequence above), so MarkersTab_UI.cpp can fold it into a
// Type-section header's own reserved-right-zone math without the two ever drifting apart (mirrors
// that file's own TypeSectionHeaderButtonClusterWidth companion-function pattern).
inline float TypeSectionMarkerSettingsRowWidth(const MarkersTabGlobals& globals) {
    const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
    // Each DrawDialCompact call (bShowRealtimeToggle=false, matching the row's own "scale edits are
    // always realtime" posture) draws a knob (diameter = the default style.dialRadius fallback,
    // ImGui::GetFrameHeight()) SameLine'd with a fixed-width field — no RT-button term, mirroring
    // this row's own color swatches.
    const float perDialWidth = ImGui::GetFrameHeight() + itemSpacing + kMarkerGlobalScaleRowFieldWidthPixels;
    return globals.iconButtonSizePixels + itemSpacing
         + kMarkerGlobalScaleRowSwatchWidthPixels + itemSpacing
         + ImGui::CalcTextSize("Selected").x + itemSpacing
         + kMarkerGlobalScaleRowSwatchWidthPixels + itemSpacing
         + perDialWidth + itemSpacing
         + perDialWidth;
}

// Draws the global section: the gamedata root and the icon scan request only (STEP136 — the three
// per-Type rows this used to stack live on the Type-section headers now, see
// DrawTypeSectionMarkerSettingsRow above).
void DrawMarkersTabGlobals(MarkersTabGlobals& globals);

} // namespace Ui
} // namespace SanmapGen
