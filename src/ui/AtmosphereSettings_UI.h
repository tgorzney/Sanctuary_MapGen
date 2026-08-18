// AtmosphereSettings_UI.h — the atmosphere/lighting settings the Atmosphere tab edits.
// Layer: UI. Accuracy class: Visual. TAB_REBUILD_PLAN "ENVIRONMENT / Atmosphere".
//
// RETIRED SCOPE NOTE: the promotion this file used to flag as deferred has landed —
// `Params::Atmosphere` now exists (ATMOSPHERE_PARAMS_SPEC.md, STEP9_Atmosphere_PARAMS_IO), with a
// full `.sanmap` IO round-trip of its own. `Ui::AtmosphereSettings` itself is UNCHANGED and still
// caller-owned PRESENTATION state, same as `PreviewCompositeSettings` — retyping the Atmosphere
// tab onto `Params::Atmosphere` is a separate, not-yet-done UI-wiring ticket.
//
// Storage is the house convention: linear RGBA colors as four floats (byte-compatible with
// Ui::kColorSwatchChannelCount and Params::GradientStop::color) and vectors as plain float
// arrays, so a control writes them in place with no conversion.
#pragma once
#include <string>

namespace SanmapGen {
namespace Ui {

// Which quantity the skybox intensity is stated in (v1 SkyIntensityMode), as dropdown rows.
inline constexpr int kSkyboxIntensityModeCount = 3;
inline const char* const skyboxIntensityModeLabels[kSkyboxIntensityModeCount] = {
    "Exposure", "Lux", "Multiplier"
};

struct AtmosphereSettings {
    // --- Sun
    float sunRightAscension          = 0.0f;
    float sunDeclination             = 0.0f;
    float sunIntensity               = 15000.0f;
    float sunTint[4]                 = { 1.0f, 1.0f, 1.0f, 1.0f };
    float sunTemperature             = 6300.0f;
    float sunAngularDiameter         = 0.5f;
    float sunVolumetricMultiplier    = 6.7f;
    float sunVolumetricShadowDimmer  = 0.5f;
    float sunPosition[3]             = { 512.0f, 10.0f, 256.0f };
    std::string sunCookiePath;
    float sunCookieSize[2]           = { 1024.0f, 1024.0f };

    // --- Skylight
    float skylightIntensity          = 0.0f;
    float skylightTint[4]            = { 1.0f, 1.0f, 1.0f, 1.0f };
    float skylightTemperature        = 9000.0f;

    // --- Exposure & skybox
    float exposure                   = 12.0f;
    float exposureCompensation       = 2.5f;
    std::string skyboxPath;
    float skyboxRotation             = 0.0f;
    int   skyboxIntensityModeIndex   = 0;          // a row of skyboxIntensityModeLabels
    float skyboxExposure             = 12.0f;
    float skyboxMultiplier           = 1.0f;
    float skyboxLuxValue             = 10000.0f;

    // --- Legacy fog
    float legacyFogAttenuationDistance = 200.0f;
    float legacyFogBaseHeight          = 15.0f;
    float legacyFogMaximumHeight       = 100.0f;
    float legacyFogMaximumDistance     = 1500.0f;
    float legacyFogAnisotropy          = 0.5f;

    // --- Background fog
    float backgroundFogIntensity       = 1.0f;
    float backgroundFogRange           = 1024.0f;
    float backgroundFogMinimum         = 0.1f;
    float backgroundSkyColorIntensity  = 1.0f;
    float backgroundColor[4]           = { 0.0f, 0.0f, 0.0f, 1.0f };
    float backgroundColorIntensity     = 0.0f;
    float backgroundColorFadeoutRange  = 150000.0f;
    float backgroundColorFadeoutPower  = 0.3f;

    // --- Height fog
    float heightFogIntensity           = 1.0f;
    float heightFogRange[2]            = { -10.0f, 100.0f };
    float heightFogStart               = -10.0f;
    float heightFogEnd                 = 500.0f;
    float heightFogPower               = 6.0f;

    // --- Linear fog
    float linearFogIntensity           = 0.24f;
    float linearFogStart               = 100.0f;
    float linearFogEnd                 = 5000.0f;
    float linearFogPower               = 1.0f;
    float linearFogCameraIntensity     = 0.0f;
    float linearFogCameraStart         = 500.0f;
    float linearFogCameraEnd           = 5000.0f;

    // --- Global wind
    float globalWindSpeed              = 0.25f;
    float globalWindDirection          = 160.0f;
};

// The colors / vectors / strings a control table addresses by SLOT, because a member pointer
// cannot name an array or a string alongside a float in one table.
enum : int { kSunTintSlot = 0, kSkylightTintSlot = 1, kBackgroundColorSlot = 2, kAtmosphereColorSlotCount = 3 };
enum : int { kSunPositionSlot = 0, kSunCookieSizeSlot = 1, kHeightFogRangeSlot = 2, kAtmosphereVectorSlotCount = 3 };
enum : int { kSunCookiePathSlot = 0, kSkyboxPathSlot = 1, kAtmosphereTextSlotCount = 2 };

// An out-of-range slot answers null / zero rather than reading off the end (Constitution §6).
inline float* AtmosphereColorAt(AtmosphereSettings& settings, int slotIndex) {
    if (slotIndex == kSunTintSlot)        return settings.sunTint;
    if (slotIndex == kSkylightTintSlot)   return settings.skylightTint;
    if (slotIndex == kBackgroundColorSlot) return settings.backgroundColor;
    return nullptr;
}

inline float* AtmosphereVectorAt(AtmosphereSettings& settings, int slotIndex) {
    if (slotIndex == kSunPositionSlot)    return settings.sunPosition;
    if (slotIndex == kSunCookieSizeSlot)  return settings.sunCookieSize;
    if (slotIndex == kHeightFogRangeSlot) return settings.heightFogRange;
    return nullptr;
}

inline int AtmosphereVectorComponentCount(int slotIndex) {
    if (slotIndex == kSunPositionSlot) return 3;
    if (slotIndex == kSunCookieSizeSlot || slotIndex == kHeightFogRangeSlot) return 2;
    return 0;
}

inline std::string* AtmosphereTextAt(AtmosphereSettings& settings, int slotIndex) {
    if (slotIndex == kSunCookiePathSlot) return &settings.sunCookiePath;
    if (slotIndex == kSkyboxPathSlot)    return &settings.skyboxPath;
    return nullptr;
}

} // namespace Ui
} // namespace SanmapGen
