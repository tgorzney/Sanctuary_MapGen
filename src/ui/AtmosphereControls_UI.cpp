// AtmosphereControls_UI.cpp — the Atmosphere tab's control and section tables. Layer: UI.
// The single statement of every atmosphere row: its label, its widget, the setting it edits and
// the limits it edits between. Nothing here draws; AtmosphereTab_UI.cpp walks these two tables.
// Row order IS the drawn order, and it reproduces the v1 Atmosphere tab exactly.
#include "AtmosphereControls_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {
using Kind    = AtmosphereControlKind;
using Setting = AtmosphereSettings;
} // namespace

const char* const atmosphereVectorComponentLabels[kAtmosphereVectorComponentLimit] = { "X", "Y", "Z" };

const AtmosphereControl atmosphereControls[kAtmosphereControlCount] = {
    // --- Sun (rows 0..10)
    { "Right Ascension",          Kind::Scalar, &Setting::sunRightAscension,         -1, {   0.0f,    360.0f }, "%.1f" },
    { "Declination",              Kind::Scalar, &Setting::sunDeclination,            -1, { -90.0f,     90.0f }, "%.1f" },
    { "Intensity",                Kind::Scalar, &Setting::sunIntensity,              -1, {   0.0f, 100000.0f }, "%.0f" },
    { "Tint",                     Kind::Color,  nullptr,            kSunTintSlot,        {   0.0f,      1.0f }, "%.3f" },
    { "Temperature",              Kind::Scalar, &Setting::sunTemperature,            -1, {1000.0f,  10000.0f }, "%.0f" },
    { "Angular Diameter",         Kind::Scalar, &Setting::sunAngularDiameter,        -1, {   0.1f,      5.0f }, "%.2f" },
    { "Volumetric Multiplier",    Kind::Scalar, &Setting::sunVolumetricMultiplier,   -1, {   0.0f,     10.0f }, "%.2f" },
    { "Volumetric Shadow Dimmer", Kind::Scalar, &Setting::sunVolumetricShadowDimmer, -1, {   0.0f,      1.0f }, "%.3f" },
    { "Sun Position",             Kind::Vector, nullptr,            kSunPositionSlot,    {   0.0f,   4096.0f }, "%.1f" },
    { "Sun Cookie Path",          Kind::Text,   nullptr,            kSunCookiePathSlot,  {   0.0f,      0.0f }, nullptr },
    { "Sun Cookie Size",          Kind::Vector, nullptr,            kSunCookieSizeSlot,  {   0.0f,   4096.0f }, "%.0f" },
    // --- Skylight (rows 11..13)
    { "Skylight Intensity",       Kind::Scalar, &Setting::skylightIntensity,         -1, {   0.0f, 100000.0f }, "%.0f" },
    { "Skylight Tint",            Kind::Color,  nullptr,            kSkylightTintSlot,   {   0.0f,      1.0f }, "%.3f" },
    { "Skylight Temperature",     Kind::Scalar, &Setting::skylightTemperature,       -1, {1000.0f,  10000.0f }, "%.0f" },
    // --- Exposure & Skybox (rows 14..21)
    { "Exposure",                 Kind::Scalar, &Setting::exposure,                  -1, {   0.0f,     20.0f }, "%.2f" },
    { "Exposure Compensation",    Kind::Scalar, &Setting::exposureCompensation,      -1, {  -5.0f,      5.0f }, "%.2f" },
    { "Skybox Path",              Kind::Text,   nullptr,            kSkyboxPathSlot,     {   0.0f,      0.0f }, nullptr },
    { "Skybox Rotation",          Kind::Scalar, &Setting::skyboxRotation,            -1, {   0.0f,    360.0f }, "%.1f" },
    { "Intensity Mode",           Kind::Combo,  nullptr,                             -1, {   0.0f,      0.0f }, nullptr },
    { "Skybox Exposure",          Kind::Scalar, &Setting::skyboxExposure,            -1, {   0.0f,     20.0f }, "%.2f" },
    { "Skybox Multiplier",        Kind::Scalar, &Setting::skyboxMultiplier,          -1, {   0.0f,    100.0f }, "%.2f" },
    { "Skybox Lux Value",         Kind::Scalar, &Setting::skyboxLuxValue,            -1, {   0.0f, 100000.0f }, "%.0f" },
    // --- Legacy Fog (rows 22..26). The attenuation distance is a v1 DEFECT repaired here: v1
    // shipped the value 200 behind a 0..10 slider, so touching the control snapped the setting to
    // 10. The default is kept and the limit widened to the game units it is actually stated in,
    // because a value has to be reachable by its own control (Constitution §8).
    { "Attenuation Distance",     Kind::Scalar, &Setting::legacyFogAttenuationDistance, -1, {0.0f,  1000.0f }, "%.2f" },
    { "Base Height",              Kind::Scalar, &Setting::legacyFogBaseHeight,       -1, {-100.0f,   500.0f }, "%.1f" },
    { "Maximum Height",           Kind::Scalar, &Setting::legacyFogMaximumHeight,    -1, {   0.0f,  1000.0f }, "%.1f" },
    { "Maximum Distance",         Kind::Scalar, &Setting::legacyFogMaximumDistance,  -1, {   0.0f, 10000.0f }, "%.0f" },
    { "Anisotropy",               Kind::Scalar, &Setting::legacyFogAnisotropy,       -1, {   0.0f,     1.0f }, "%.3f" },
    // --- Background Fog (rows 27..34)
    { "Background Fog Intensity", Kind::Scalar, &Setting::backgroundFogIntensity,    -1, {   0.0f,    10.0f }, "%.2f" },
    { "Background Fog Range",     Kind::Scalar, &Setting::backgroundFogRange,        -1, {   0.0f, 10000.0f }, "%.0f" },
    { "Background Fog Minimum",   Kind::Scalar, &Setting::backgroundFogMinimum,      -1, {   0.0f,     1.0f }, "%.3f" },
    { "Sky Color Intensity",      Kind::Scalar, &Setting::backgroundSkyColorIntensity, -1, { 0.0f,    10.0f }, "%.2f" },
    { "Background Color",         Kind::Color,  nullptr,            kBackgroundColorSlot, {  0.0f,     1.0f }, "%.3f" },
    { "Background Color Intensity", Kind::Scalar, &Setting::backgroundColorIntensity, -1, {  0.0f,    10.0f }, "%.2f" },
    { "Color Fadeout Range",      Kind::Scalar, &Setting::backgroundColorFadeoutRange, -1, { 0.0f, 500000.0f }, "%.0f" },
    { "Color Fadeout Power",      Kind::Scalar, &Setting::backgroundColorFadeoutPower, -1, { 0.0f,    10.0f }, "%.2f" },
    // --- Height Fog (rows 35..39)
    { "Height Fog Intensity",     Kind::Scalar, &Setting::heightFogIntensity,        -1, {   0.0f,    10.0f }, "%.2f" },
    { "Height Fog Range",         Kind::Vector, nullptr,            kHeightFogRangeSlot, {-1000.0f, 1000.0f }, "%.1f" },
    { "Height Fog Start",         Kind::Scalar, &Setting::heightFogStart,            -1, {-1000.0f, 1000.0f }, "%.1f" },
    { "Height Fog End",           Kind::Scalar, &Setting::heightFogEnd,              -1, {-1000.0f, 5000.0f }, "%.1f" },
    { "Height Fog Power",         Kind::Scalar, &Setting::heightFogPower,            -1, {   0.01f,   10.0f }, "%.2f" },
    // --- Linear Fog (rows 40..46)
    { "Linear Fog Intensity",     Kind::Scalar, &Setting::linearFogIntensity,        -1, {   0.0f,    10.0f }, "%.2f" },
    { "Linear Fog Start",         Kind::Scalar, &Setting::linearFogStart,            -1, {   0.0f, 10000.0f }, "%.1f" },
    { "Linear Fog End",           Kind::Scalar, &Setting::linearFogEnd,              -1, {   0.0f, 50000.0f }, "%.0f" },
    { "Linear Fog Power",         Kind::Scalar, &Setting::linearFogPower,            -1, {   0.01f,   10.0f }, "%.2f" },
    { "Camera Intensity",         Kind::Scalar, &Setting::linearFogCameraIntensity,  -1, {   0.0f,     1.0f }, "%.3f" },
    { "Camera Start",             Kind::Scalar, &Setting::linearFogCameraStart,      -1, {   0.0f, 10000.0f }, "%.1f" },
    { "Camera End",               Kind::Scalar, &Setting::linearFogCameraEnd,        -1, {   0.0f, 50000.0f }, "%.0f" },
    // --- Wind (Global) (rows 47..48)
    { "Wind Speed",               Kind::Scalar, &Setting::globalWindSpeed,           -1, {   0.0f,    10.0f }, "%.2f" },
    { "Wind Direction",           Kind::Scalar, &Setting::globalWindDirection,       -1, {   0.0f,   360.0f }, "%.1f" },
};

const AtmosphereSection atmosphereSections[kAtmosphereSectionCount] = {
    { "Sun",                0, 11 },
    { "Skylight",          11,  3 },
    { "Exposure & Skybox", 14,  8 },
    { "Legacy Fog",        22,  5 },
    { "Background Fog",    27,  8 },
    { "Height Fog",        35,  5 },
    { "Linear Fog",        40,  7 },
    { "Wind (Global)",     47,  2 },
};

} // namespace Ui
} // namespace SanmapGen
