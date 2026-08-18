// MapExporter_Atmosphere_IO.cpp — `recipe.atmosphere` -> ~49 top-level `.sanmap` document keys.
// Layer: IO.
//
// SHAPE NOTE (deliberate, not an oversight): every other `Build*Json` in this codebase returns one
// self-contained `nlohmann::ordered_json` object the caller assigns to a single document key
// (`document["armies"] = BuildArmiesJson(recipe);`). `BuildAtmosphereJson` cannot do that — per
// ATMOSPHERE_PARAMS_SPEC.md, all 49 fields are FLAT top-level `.sanmap` keys (`sunRA`,
// `skylightIntensity`, `fogAttenuationDistance`, `windSpeed`, ...), confirmed against the real
// `SanMap.cs` (every field is a direct member of the `SanMap` class, none nested under a
// sub-object). So this function takes `document` BY REFERENCE and writes ~49 entries directly into
// it, unlike its siblings — a future reader should read this comment, not assume a bug.
//
// Field-name mismatches (Format Expert catch, ARCH §1.1 — the JSON key is the format's own
// spelling verbatim, never "fixed" to match the corrected `Params::` field name):
//   sunRightAscension/sunDeclination            -> sunRA/sunDA
//   sunVolumetricMultiplier/sunVolumetricShadowDimmer -> sunVolumetricsMultiplier/
//                                                        sunVolumetricsShadowDimer (real C# typo)
//   legacyFog*                                  -> fog* (fogAttenuationDistance/fogBaseHeight/
//                                                        fogMaximumHeight/fogMaximumDistance/
//                                                        fogAnisotropy)
//   globalWindSpeed/globalWindDirection          -> windSpeed/windDirection
// `sunCookiePath`/`skyboxPath` are bare strings in `Params::` but wrap as `{"path": "..."}`
// (`TextureLoader` in the real C#), not bare string keys. `sunTint`/`skylightTint`/
// `backgroundColor` are `{r,g,b,a}` (the `armyColor`/`diffuseRemap` convention), NOT `{x,y,z,w}`.
// `sunPosition` is `{x,y,z}`; `sunCookieSize`/`heightFogRange` are `{x,y}`.
// `skyboxIntensityMode` is the ONE field in this whole domain written as a JSON STRING
// ("Exposure"/"Lux"/"Multiplier"), not an int like every other enum in this codebase.
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// The one string-typed enum in this codebase (Format Expert catch) — a small, local,
// string-comparison function; not promoted to a shared primitive unless a second string-typed
// enum appears elsewhere (per the work-order's own guidance).
const char* SkyboxIntensityModeToString(Params::SkyboxIntensityMode mode) {
    switch (mode) {
        case Params::SkyboxIntensityMode::Lux:        return "Lux";
        case Params::SkyboxIntensityMode::Multiplier: return "Multiplier";
        case Params::SkyboxIntensityMode::Exposure:   default: return "Exposure";
    }
}

} // namespace

void BuildAtmosphereJson(const Params::MapRecipe& recipe, nlohmann::ordered_json& document) {
    const Params::Atmosphere& atmosphere = recipe.atmosphere;

    // --- Sun (11 fields) ---
    const Params::AtmosphereSun& sun = atmosphere.sun;
    document["sunRA"]                     = sun.sunRightAscension;
    document["sunDA"]                     = sun.sunDeclination;
    document["sunIntensity"]              = sun.sunIntensity;
    document["sunTint"] = { { "r", sun.sunTint[0] }, { "g", sun.sunTint[1] },
                            { "b", sun.sunTint[2] }, { "a", sun.sunTint[3] } };
    document["sunTemperature"]            = sun.sunTemperature;
    document["sunAngularDiameter"]        = sun.sunAngularDiameter;
    document["sunVolumetricsMultiplier"]  = sun.sunVolumetricMultiplier;
    document["sunVolumetricsShadowDimer"] = sun.sunVolumetricShadowDimmer;   // sic — real C# typo
    document["sunPosition"] = { { "x", sun.sunPosition[0] }, { "y", sun.sunPosition[1] },
                               { "z", sun.sunPosition[2] } };
    document["sunCookie"]     = { { "path", sun.sunCookiePath } };
    document["sunCookieSize"] = { { "x", sun.sunCookieSize[0] }, { "y", sun.sunCookieSize[1] } };

    // --- Skylight (3 fields) ---
    const Params::AtmosphereSkylight& skylight = atmosphere.skylight;
    document["skylightIntensity"] = skylight.skylightIntensity;
    document["skylightTint"] = { { "r", skylight.skylightTint[0] }, { "g", skylight.skylightTint[1] },
                                { "b", skylight.skylightTint[2] }, { "a", skylight.skylightTint[3] } };
    document["skylightTemperature"] = skylight.skylightTemperature;

    // --- Exposure & skybox (8 fields) ---
    const Params::AtmosphereExposureSkybox& exposureSkybox = atmosphere.exposureSkybox;
    document["exposure"]             = exposureSkybox.exposure;
    document["exposureCompensation"] = exposureSkybox.exposureCompensation;
    document["skybox"]               = { { "path", exposureSkybox.skyboxPath } };
    document["skyboxRotation"]       = exposureSkybox.skyboxRotation;
    document["skyboxIntensityMode"]  = SkyboxIntensityModeToString(exposureSkybox.skyboxIntensityMode);
    document["skyboxExposure"]       = exposureSkybox.skyboxExposure;
    document["skyboxMultiplier"]     = exposureSkybox.skyboxMultiplier;
    document["skyboxLuxValue"]       = exposureSkybox.skyboxLuxValue;

    // --- Legacy fog (5 fields) ---
    const Params::AtmosphereLegacyFog& legacyFog = atmosphere.legacyFog;
    document["fogAttenuationDistance"] = legacyFog.legacyFogAttenuationDistance;
    document["fogBaseHeight"]          = legacyFog.legacyFogBaseHeight;
    document["fogMaximumHeight"]       = legacyFog.legacyFogMaximumHeight;
    document["fogMaximumDistance"]     = legacyFog.legacyFogMaximumDistance;
    document["fogAnisotropy"]          = legacyFog.legacyFogAnisotropy;

    // --- Background fog (8 fields) ---
    const Params::AtmosphereBackgroundFog& backgroundFog = atmosphere.backgroundFog;
    document["backgroundFogIntensity"]      = backgroundFog.backgroundFogIntensity;
    document["backgroundFogRange"]          = backgroundFog.backgroundFogRange;
    document["backgroundFogMinimum"]        = backgroundFog.backgroundFogMinimum;
    document["backgroundSkyColorIntensity"] = backgroundFog.backgroundSkyColorIntensity;
    document["backgroundColor"] = { { "r", backgroundFog.backgroundColor[0] },
                                    { "g", backgroundFog.backgroundColor[1] },
                                    { "b", backgroundFog.backgroundColor[2] },
                                    { "a", backgroundFog.backgroundColor[3] } };
    document["backgroundColorIntensity"]    = backgroundFog.backgroundColorIntensity;
    document["backgroundColorFadeoutRange"] = backgroundFog.backgroundColorFadeoutRange;
    document["backgroundColorFadeoutPower"] = backgroundFog.backgroundColorFadeoutPower;

    // --- Height fog (5 fields) ---
    const Params::AtmosphereHeightFog& heightFog = atmosphere.heightFog;
    document["heightFogIntensity"] = heightFog.heightFogIntensity;
    document["heightFogRange"] = { { "x", heightFog.heightFogRange[0] },
                                   { "y", heightFog.heightFogRange[1] } };
    document["heightFogStart"] = heightFog.heightFogStart;
    document["heightFogEnd"]   = heightFog.heightFogEnd;
    document["heightFogPower"] = heightFog.heightFogPower;

    // --- Linear fog (7 fields) ---
    const Params::AtmosphereLinearFog& linearFog = atmosphere.linearFog;
    document["linearFogIntensity"]       = linearFog.linearFogIntensity;
    document["linearFogStart"]           = linearFog.linearFogStart;
    document["linearFogEnd"]             = linearFog.linearFogEnd;
    document["linearFogPower"]           = linearFog.linearFogPower;
    document["linearFogCameraIntensity"] = linearFog.linearFogCameraIntensity;
    document["linearFogCameraStart"]     = linearFog.linearFogCameraStart;
    document["linearFogCameraEnd"]       = linearFog.linearFogCameraEnd;

    // --- Global wind (2 fields) ---
    const Params::AtmosphereGlobalWind& globalWind = atmosphere.globalWind;
    document["windSpeed"]     = globalWind.globalWindSpeed;
    document["windDirection"] = globalWind.globalWindDirection;
}

} // namespace Io
} // namespace SanmapGen
