// MarkersTab_TypeSectionHeaderButtons_UI.h — STEP254: one Type-section header's own button-cluster
// layout and draw ([+ Instance][+ Group][+ Layer][+ Link][Hide/Unhide]), relocated verbatim out of
// MarkersTab_UI.cpp's file-size-ceiling split — pure UI layout, zero PARAMS mutation, a legitimate
// single concern ("this Type-section header's own button row"), ARCH_19_MarkerLayerBundle.md §19.2's
// "pure mechanics get their own small file" posture.
#pragma once
#include "Section_UI.h"

namespace SanmapGen {
namespace Params { struct GlobalMarkerSettings; }
namespace Ui {

struct MarkersTabGlobals;
struct IconAtlasManifest;
class  IconAtlasPairingLookup;   // real declaration is `class`, not `struct` (IconAtlasPairing_UI.h)

// What the header's own button cluster did this frame — "Add Layer" opens a Manual/Procedural
// choice (the human's own stated design for a Layer-adding affordance, ARCH §19's Group/Layer
// restructure) rather than guessing one kind, so it reports EITHER of two distinct clicks.
struct TypeSectionHeaderButtons_UI {
    bool bAddInstanceClicked        = false;
    bool bAddGroupClicked           = false;
    bool bAddManualLayerClicked     = false;
    bool bAddProceduralLayerClicked = false;
    bool bAddLinkClicked            = false;
    bool bHideToggleClicked         = false;
};

// STEP136 — the FULL reserved-right-width `SectionOptions` for one Type-section header: the
// relocated per-Type marker-settings row (MarkersTab_Globals_UI.h's own
// TypeSectionMarkerSettingsRowWidth) immediately followed by the button cluster above, human's own
// explicit "to the left of the buttons" ordering, plus the fixed leading spacing, mirroring
// HeightmapTab_UI.cpp's GeoLayerSectionOptions exactly (STEP133).
SectionOptions HeaderButtonsSectionOptions(const MarkersTabGlobals& globals, bool bHidden);

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
        const IconAtlasManifest* iconManifest, const IconAtlasPairingLookup* pairingLookup, bool bHidden,
        bool bSelectionNonEmpty);

} // namespace Ui
} // namespace SanmapGen
