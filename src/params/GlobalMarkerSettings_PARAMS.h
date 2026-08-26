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
    float scaleAlloy  = 0.17f;
    float scalePlasma = 0.17f;
    float scaleSpawn  = 0.17f;
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

} // namespace Params
} // namespace SanmapGen
