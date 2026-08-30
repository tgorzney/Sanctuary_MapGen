// GlobalMarkerSettings_PARAMS.h — map-wide default icon/color/scale for the three resource marker
// kinds (Alloy/Plasma/Spawn). Layer: PARAMS. Settings only. ARCH_11_GlobalMarkerSettings.md
// §11 (completes SANMAP_FORMAT_SPEC Correction 7): map-scoped, not per-rule — the same
// global-vs-per-rule scope split as `Symmetry`/`SlopeDefaults` vs. their per-rule overrides.
// `Plasma` = Energy, a real planned resource type (ARCH §11), not the v1 invention
// IO_PARITY_REPORT.md Decision #5 flagged.
#pragma once
#include <string>
#include "MarkerInstance_PARAMS.h"   // NEW — kSpawnMarkerGroupName

namespace SanmapGen {
namespace Params {

struct GlobalMarkerSettings {
    std::string iconNameAlloy  = "Alloy";
    std::string iconNamePlasma = "Plasma";
    std::string iconNameSpawn  = "Spawn";
    float colorAlloy[4]  = {0.8f, 0.8f, 0.2f, 1.0f};
    float colorPlasma[4] = {0.2f, 0.8f, 0.8f, 1.0f};
    float colorSpawn[4]  = {0.8f, 0.2f, 0.2f, 1.0f};
    float scaleAlloy  = 0.50f;   // STEP127 item 3 — was 0.17f
    float scalePlasma = 0.50f;   // STEP127 item 3 — was 0.17f
    float scaleSpawn  = 0.50f;   // STEP127 item 3 — was 0.17f

    // ARCH §19.17 — selection-highlight tint. selectColorAlloy/Plasma/Spawn strictly mirror
    // colorAlloy/Plasma/Spawn's shape/placement; selectColorDefault is the one signed-off 4th-field
    // deviation from that mirror (see ResolveMarkerGroupSelectTintColor below for why).
    // STEP231 — human's own bug report: the default selection tint (identical to every OTHER
    // selectColor* default — see the three lines below, all still {1,1,0,1}) was a plain yellow, not
    // visually distinct enough from a legitimate in-map color to read clearly as "selected." Changed
    // to a bright/lime green — a color no existing GlobalMarkerSettings field defaults to
    // (colorAlloy/Plasma/Spawn are yellow/teal/red, lines 18-20 above, confirmed by direct read), so
    // "selected" reads unambiguously against every one of them, and matches the SAME lime-green
    // literal the icon-atlas pass's own kIconLayerSelectedTint now uses
    // (MapCanvas_IconLayer_DrawInternal_UI.h) for a consistent cross-pass visual language.
    // selectColorPlasma/Spawn/Default are DELIBERATELY UNCHANGED (still yellow): the human's own
    // report named Alloy specifically; there is no confirmed bug against the other three, so changing
    // them too would be unrequested scope creep (see this ticket's own Interpretation calls) — the
    // underlying ResolveMarkerGroupSelectTintColor mechanism itself already works correctly for all
    // four (GlobalMarkerSettings_PARAMS_Test.cpp already exercises every one with distinct non-default
    // values), so this is purely a default-VALUE change, not a mechanism fix.
    float selectColorAlloy[4]   = {0.2f, 1.0f, 0.2f, 1.0f};
    float selectColorPlasma[4]  = {1.0f, 1.0f, 0.0f, 1.0f};
    float selectColorSpawn[4]   = {1.0f, 1.0f, 0.0f, 1.0f};
    float selectColorDefault[4] = {1.0f, 1.0f, 0.0f, 1.0f};
};

// STEP116: the group-name -> GlobalMarkerSettings-field mapping a manual marker resolves a
// TYPE-default color through, once "no explicit per-layer override" is established by the caller
// (MarkerInstanceLayer::bColorOverrideEnabled). Mirrors ResolveMarkerIconTemplateIdentifier's own
// vocabulary (MapCanvas_IconLayer_CullManual_UI.cpp, STEP114) — Spawn/Spawns, Alloy/Alloys,
// Plasma/Plasmas. Any other group name (Generic/Expansion/freeform) resolves to opaque white — the
// established "unset" convention (STEP115 ruling #5), not a strong color opinion.
inline void ResolveMarkerGroupTypeTintColor(const std::string& groupName, const GlobalMarkerSettings& settings,
                                            float& outRed, float& outGreen, float& outBlue) {
    const float* color = nullptr;
    if (groupName == kSpawnMarkerGroupName || groupName == "Spawns") color = settings.colorSpawn;
    else if (groupName == "Alloy" || groupName == "Alloys")          color = settings.colorAlloy;
    else if (groupName == "Plasma" || groupName == "Plasmas")        color = settings.colorPlasma;
    if (color == nullptr) { outRed = outGreen = outBlue = 1.0f; return; }
    outRed = color[0]; outGreen = color[1]; outBlue = color[2];
}

// STEP122: the group-name -> GlobalMarkerSettings scale-field mapping, mirroring
// ResolveMarkerGroupTypeTintColor's exact group-name vocabulary above. Unrecognized group name
// (Generic/Expansion/freeform) resolves to 1.0f — a multiplicative no-op, the correct "unset"
// convention for a scale factor (ResolveMarkerGroupTypeTintColor's own white-for-unset is a
// color-channel convention, not directly reusable here).
inline float ResolveMarkerGroupTypeScale(const std::string& groupName, const GlobalMarkerSettings& settings) {
    if (groupName == kSpawnMarkerGroupName || groupName == "Spawns") return settings.scaleSpawn;
    if (groupName == "Alloy" || groupName == "Alloys")               return settings.scaleAlloy;
    if (groupName == "Plasma" || groupName == "Plasmas")             return settings.scalePlasma;
    return 1.0f;
}

// ARCH §19.17: the select-tint counterpart to ResolveMarkerGroupTypeTintColor, same name-matching
// vocabulary (Spawn/Spawns, Alloy/Alloys, Plasma/Plasmas) — but an unmatched group name resolves to
// settings.selectColorDefault, NOT hardcoded white: a select tint that fell back to white would
// make "selected" indistinguishable from "unselected" for any Generic/Expansion/freeform group,
// since that same unmatched name's normal (unselected) fill already resolves to white via
// ResolveMarkerGroupTypeTintColor's own fallback absent a per-layer color override — a real
// correctness gap, not a cosmetic one (ARCH §19.17's signed-off deviation from the 3-field mirror).
inline void ResolveMarkerGroupSelectTintColor(const std::string& groupName, const GlobalMarkerSettings& settings,
                                              float& outRed, float& outGreen, float& outBlue) {
    const float* color = settings.selectColorDefault;
    if (groupName == kSpawnMarkerGroupName || groupName == "Spawns") color = settings.selectColorSpawn;
    else if (groupName == "Alloy" || groupName == "Alloys")          color = settings.selectColorAlloy;
    else if (groupName == "Plasma" || groupName == "Plasmas")        color = settings.selectColorPlasma;
    outRed = color[0]; outGreen = color[1]; outBlue = color[2];
}

} // namespace Params
} // namespace SanmapGen
