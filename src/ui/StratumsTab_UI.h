// StratumsTab_UI.h — the Stratums / Materials tab: the environment pack, and the nine per-stratum
// sections (identity, textures, mask mode, colors, remaps, tiling, soil physics). Layer: UI.
// Accuracy class: Visual (it edits settings; it simulates nothing). TAB_REBUILD_PLAN "6 · Stratums".
//
// It edits ONE recipe slice — `std::vector<Params::Stratum>`, the ONE per-stratum settings type
// (ARCH §7.1). The tier of any edit is `Pipeline::PreviewDriver`'s derivation from the stage
// hashes, never a per-widget decision here.
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing field; these are reported, not invented):
//  1. THE SHOW-OVERLAY TOGGLE is preview presentation, not recipe: it belongs to the composite's
//     `PreviewFieldLayer::bEnabled` (PreviewComposite_Settings_UI.h). It is caller-owned tab state
//     here; the host work-order (E) maps it onto the composite layer, as it does for the left
//     column's `[O]` toggles. Not serialized.
//  2. THE ENVIRONMENT `.sanpack` PATH has no PARAMS home in v2 (v1 kept `GlobalEnvironmentPath` on
//     the god object; the `.sanmap` writes it under `mapGeneratorData`). It is caller-owned tab
//     state, same standing as SystemTab_UI's asset-cache directory. A durable setting needs its
//     own work-order.
//  3. THE ENVIRONMENT / MATERIAL DROPDOWNS are filled from a sanpack, and reading one is IO — a
//     layer UI may not depend on (ARCH §3.1). The host hands the borrowed label tables in through
//     `StratumsTabAssetOptions`; picking a row copies the LABEL into the stratum's settings, so
//     the recipe stores the name, never a row index into a list that can change.
//  4. THE SLOPE GATE fields on `Params::Stratum` (bSlopeGateEnabled, the degree window, feather,
//     smoothstep, invert, strength) are consumed by the Mask stage but are NOT in this tab's plan
//     section, so no control is drawn for them. They are currently reachable from no tab at all —
//     that is a gap for a work-order, not something to add here.
#pragma once
#include <string>
#include <vector>
#include "ColorSwatch_UI.h"
#include "FilePathPicker_UI.h"
#include "Section_UI.h"
#include "StratumsTab_Options_UI.h"
#include "StratumsTab_Scalars_UI.h"
#include "TextInput_UI.h"
#include "../data/MapFields_DATA.h"

namespace SanmapGen {
namespace Pipeline { class GenerationAssembler; class PreviewDriver; }
namespace Ui {

// The palette the tab draws a section for — the same nine slots the fields carry.
inline constexpr int kStratumsTabStratumCount = Data::MapFields::stratumCount;

// What a legal stratum name is. Unlike a layer name an EMPTY one is legal: the section header then
// shows "Stratum <index>" (StratumsTab_Options_UI.h), which is what v1 did.
inline TextInputRules StratumNameRules() {
    TextInputRules rules;
    rules.maximumLength = 48;
    rules.bAllowEmpty   = true;
    return rules;
}

// Caller-owned state for ONE stratum's section. Every RealtimeToggle is per-stratum on purpose:
// nine sections are drawn in the same frame, and one shared toggle would let a drag on stratum 0
// report a release on stratum 8 — the very clobbering RtToggleWidget_UI.h exists to kill.
struct StratumRowState {
    SectionState   section;                       // seeded CLOSED by the tab state's constructor
    RealtimeToggle scalarToggles[kStratumsTabScalarCount];
    RealtimeToggle previewBaseColorToggle;
    RealtimeToggle farColorRemapColorToggle;
    RealtimeToggle maskRemapMinimumToggles[Params::kStratumColorChannelCount];
    RealtimeToggle maskRemapMaximumToggles[Params::kStratumColorChannelCount];

    // The swatch widget speaks RGBA; the recipe stores the preview base color as three loose
    // floats (Params::Stratum::tintRed/Green/Blue), so the picker edits this mirror.
    float previewBaseColorMirror[kColorSwatchChannelCount] = { 1.0f, 1.0f, 1.0f, 1.0f };

    int environmentIndex = -1;                    // resolved from the stored name every frame
    int materialIndex    = -1;
    int soilPresetIndex  = -1;                    // -1 = no preset picked since the last edit
};

// Caller-owned tab state: the limits (Constitution §8), the borrowed asset catalogues, and one row
// of interaction state per stratum.
struct StratumsTabState {
    StratumsTabState();                           // seeds the ranges (StratumsTab_Scalars_UI.cpp)

    ScalarSliderRange scalarRanges[kStratumsTabScalarCount];
    SectionState      environmentSection;

    bool        bShowStratumOverlay = true;       // SCOPE NOTE 1
    std::string environmentPackPath;              // SCOPE NOTE 2
    FilePathPickerOptions environmentPackOptions; // ".sanpack;.zip" — seeded by the constructor
    FilePathPickerOptions textureOptions;         // ".dds;.png;.tga;.jpg" — seeded likewise
    StratumsTabAssetOptions assetOptions;         // SCOPE NOTE 3 — borrowed, host-filled per frame

    StratumRowState rows[kStratumsTabStratumCount];
};

// recipe -> widget mirrors for one stratum. Run whenever no edit is pending, so a recipe loaded
// from disk or a sanpack swapped under it is picked up without the caller refreshing anything.
inline void LoadStratumRowValues(const Params::Stratum& stratum,
                                 const StratumsTabAssetOptions& assetOptions, StratumRowState& row) {
    row.previewBaseColorMirror[0] = stratum.tintRed;
    row.previewBaseColorMirror[1] = stratum.tintGreen;
    row.previewBaseColorMirror[2] = stratum.tintBlue;
    row.previewBaseColorMirror[3] = 1.0f;                 // the preview tint has no alpha channel
    row.environmentIndex = StratumOptionIndexOf(stratum.appearance.environmentName,
                                                assetOptions.environmentLabels,
                                                assetOptions.environmentCount);
    row.materialIndex    = StratumOptionIndexOf(stratum.appearance.materialName,
                                                assetOptions.materialLabels,
                                                assetOptions.materialCount);
}

// widget mirrors -> recipe. Reports whether the recipe actually moved.
inline bool StoreStratumRowValues(const StratumRowState& row, Params::Stratum& stratum) {
    const bool bMoved = row.previewBaseColorMirror[0] != stratum.tintRed
                     || row.previewBaseColorMirror[1] != stratum.tintGreen
                     || row.previewBaseColorMirror[2] != stratum.tintBlue;
    stratum.tintRed   = row.previewBaseColorMirror[0];
    stratum.tintGreen = row.previewBaseColorMirror[1];
    stratum.tintBlue  = row.previewBaseColorMirror[2];
    return bMoved;
}

// Grows the recipe's stratum array to the full palette so every section has something to edit. A
// recipe that already carries nine (or more) is left exactly as it is.
inline void EnsureStratumPalette(std::vector<Params::Stratum>& strata) {
    if (strata.size() < static_cast<std::size_t>(kStratumsTabStratumCount))
        strata.resize(static_cast<std::size_t>(kStratumsTabStratumCount));
}

// Draws the whole tab. `generationAssembler` may be null — the sections still edit the recipe; the
// soil panel simply reports that it has no runtime record to push onto.
void DrawStratumsTab(std::vector<Params::Stratum>& strata, StratumsTabState& state,
                     Pipeline::GenerationAssembler* generationAssembler,
                     Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
