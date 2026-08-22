// GlobalMarkerSettings_PARAMS.h — map-wide default icon/color/scale for the three resource marker
// kinds (Alloy/Plasma/Spawn). Layer: PARAMS. Settings only. ARCH_11_GlobalMarkerSettings.md
// §11 (completes SANMAP_FORMAT_SPEC Correction 7): map-scoped, not per-rule — the same
// global-vs-per-rule scope split as `Symmetry`/`SlopeDefaults` vs. their per-rule overrides.
// `Plasma` = Energy, a real planned resource type (ARCH §11), not the v1 invention
// IO_PARITY_REPORT.md Decision #5 flagged.
#pragma once
#include <string>

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

} // namespace Params
} // namespace SanmapGen
