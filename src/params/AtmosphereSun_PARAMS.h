// AtmosphereSun_PARAMS.h — the sun's own settings (direction, intensity, color temperature,
// volumetrics, cookie). Layer: PARAMS. Settings only (Constitution §1).
// Verbatim from ATMOSPHERE_PARAMS_SPEC.md's "The types" section — field names/defaults copied
// exactly from `Ui::AtmosphereSettings`, no renaming except the promotion's own out-of-scope retype
// (that one lives on AtmosphereExposureSkybox, not here).
#pragma once
#include <string>

namespace SanmapGen {
namespace Params {

struct AtmosphereSun {
    float sunRightAscension         = 0.0f;
    float sunDeclination            = 0.0f;
    float sunIntensity              = 15000.0f;
    float sunTint[4]                = { 1.0f, 1.0f, 1.0f, 1.0f };
    float sunTemperature            = 6300.0f;
    float sunAngularDiameter        = 0.5f;
    float sunVolumetricMultiplier   = 6.7f;
    float sunVolumetricShadowDimmer = 0.5f;
    float sunPosition[3]            = { 512.0f, 10.0f, 256.0f };
    std::string sunCookiePath;
    float sunCookieSize[2]          = { 1024.0f, 1024.0f };
};

} // namespace Params
} // namespace SanmapGen
