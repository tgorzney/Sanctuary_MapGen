// MapImporter_Atmosphere_IO.cpp — ~49 top-level `.sanmap` document keys -> `recipe.atmosphere`.
// Layer: IO. The exact inverse of MapExporter_Atmosphere_IO.cpp — see that file's header comment
// for the full field-name-mismatch table (sunRA/sunDA, sunVolumetricsMultiplier/
// sunVolumetricsShadowDimer, legacyFog*/fog*, globalWind*/wind*) and the wrapper-object/color/
// vector shapes (`{"path":...}`, `{r,g,b,a}`, `{x,y,z}`, `{x,y}`). Every reader here is total
// (Constitution §6): a missing or wrong-typed key leaves the destination on its own default.
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

void ReadJsonColorRgba(const nlohmann::json& parent, const char* key, float destination[4]) {
    if (!parent.contains(key) || !parent[key].is_object()) return;
    const nlohmann::json& color = parent[key];
    ReadJsonFloat(color, "r", destination[0]);
    ReadJsonFloat(color, "g", destination[1]);
    ReadJsonFloat(color, "b", destination[2]);
    ReadJsonFloat(color, "a", destination[3]);
}

void ReadJsonVector2(const nlohmann::json& parent, const char* key, float destination[2]) {
    if (!parent.contains(key) || !parent[key].is_object()) return;
    const nlohmann::json& vector = parent[key];
    ReadJsonFloat(vector, "x", destination[0]);
    ReadJsonFloat(vector, "y", destination[1]);
}

void ReadJsonVector3(const nlohmann::json& parent, const char* key, float destination[3]) {
    if (!parent.contains(key) || !parent[key].is_object()) return;
    const nlohmann::json& vector = parent[key];
    ReadJsonFloat(vector, "x", destination[0]);
    ReadJsonFloat(vector, "y", destination[1]);
    ReadJsonFloat(vector, "z", destination[2]);
}

// `TextureLoader{path}` — one-field wrapper object, not a bare string (sunCookie/skybox).
void ReadJsonPathWrapper(const nlohmann::json& parent, const char* key, std::string& destination) {
    if (!parent.contains(key) || !parent[key].is_object()) return;
    ReadJsonText(parent[key], "path", destination);
}

// The one string-typed enum in this domain (Format Expert catch): a small, local, string-
// comparison function — not promoted to a shared JsonPrimitives_IO.h primitive unless a second
// string-typed enum appears elsewhere. An unrecognized string fails safe to `Exposure`, LOGGED as
// a warning (Constitution §6), never a crash.
void ReadSkyboxIntensityMode(const nlohmann::json& parent, const char* key,
                             Params::SkyboxIntensityMode& destination, MapImportResult& result) {
    std::string modeText;
    if (!ReadJsonText(parent, key, modeText)) return;
    if (modeText == "Exposure")        { destination = Params::SkyboxIntensityMode::Exposure; return; }
    if (modeText == "Lux")             { destination = Params::SkyboxIntensityMode::Lux; return; }
    if (modeText == "Multiplier")      { destination = Params::SkyboxIntensityMode::Multiplier; return; }
    result.Warn("Unrecognized skyboxIntensityMode \"" + modeText + "\"; defaulting to Exposure.");
    destination = Params::SkyboxIntensityMode::Exposure;
}

} // namespace

void ReadAtmosphereJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                       MapImportResult& result) {
    Params::Atmosphere& atmosphere = outRecipe.atmosphere;

    // --- Sun (11 fields) ---
    Params::AtmosphereSun& sun = atmosphere.sun;
    ReadJsonFloat(document, "sunRA", sun.sunRightAscension);
    ReadJsonFloat(document, "sunDA", sun.sunDeclination);
    ReadJsonFloat(document, "sunIntensity", sun.sunIntensity);
    ReadJsonColorRgba(document, "sunTint", sun.sunTint);
    ReadJsonFloat(document, "sunTemperature", sun.sunTemperature);
    ReadJsonFloat(document, "sunAngularDiameter", sun.sunAngularDiameter);
    ReadJsonFloat(document, "sunVolumetricsMultiplier", sun.sunVolumetricMultiplier);
    ReadJsonFloat(document, "sunVolumetricsShadowDimer", sun.sunVolumetricShadowDimmer);  // sic
    ReadJsonVector3(document, "sunPosition", sun.sunPosition);
    ReadJsonPathWrapper(document, "sunCookie", sun.sunCookiePath);
    ReadJsonVector2(document, "sunCookieSize", sun.sunCookieSize);

    // --- Skylight (3 fields) ---
    Params::AtmosphereSkylight& skylight = atmosphere.skylight;
    ReadJsonFloat(document, "skylightIntensity", skylight.skylightIntensity);
    ReadJsonColorRgba(document, "skylightTint", skylight.skylightTint);
    ReadJsonFloat(document, "skylightTemperature", skylight.skylightTemperature);

    // --- Exposure & skybox (8 fields) ---
    Params::AtmosphereExposureSkybox& exposureSkybox = atmosphere.exposureSkybox;
    ReadJsonFloat(document, "exposure", exposureSkybox.exposure);
    ReadJsonFloat(document, "exposureCompensation", exposureSkybox.exposureCompensation);
    ReadJsonPathWrapper(document, "skybox", exposureSkybox.skyboxPath);
    ReadJsonFloat(document, "skyboxRotation", exposureSkybox.skyboxRotation);
    ReadSkyboxIntensityMode(document, "skyboxIntensityMode", exposureSkybox.skyboxIntensityMode, result);
    ReadJsonFloat(document, "skyboxExposure", exposureSkybox.skyboxExposure);
    ReadJsonFloat(document, "skyboxMultiplier", exposureSkybox.skyboxMultiplier);
    ReadJsonFloat(document, "skyboxLuxValue", exposureSkybox.skyboxLuxValue);

    // --- Legacy fog (5 fields) ---
    Params::AtmosphereLegacyFog& legacyFog = atmosphere.legacyFog;
    ReadJsonFloat(document, "fogAttenuationDistance", legacyFog.legacyFogAttenuationDistance);
    ReadJsonFloat(document, "fogBaseHeight", legacyFog.legacyFogBaseHeight);
    ReadJsonFloat(document, "fogMaximumHeight", legacyFog.legacyFogMaximumHeight);
    ReadJsonFloat(document, "fogMaximumDistance", legacyFog.legacyFogMaximumDistance);
    ReadJsonFloat(document, "fogAnisotropy", legacyFog.legacyFogAnisotropy);

    // --- Background fog (8 fields) ---
    Params::AtmosphereBackgroundFog& backgroundFog = atmosphere.backgroundFog;
    ReadJsonFloat(document, "backgroundFogIntensity", backgroundFog.backgroundFogIntensity);
    ReadJsonFloat(document, "backgroundFogRange", backgroundFog.backgroundFogRange);
    ReadJsonFloat(document, "backgroundFogMinimum", backgroundFog.backgroundFogMinimum);
    ReadJsonFloat(document, "backgroundSkyColorIntensity", backgroundFog.backgroundSkyColorIntensity);
    ReadJsonColorRgba(document, "backgroundColor", backgroundFog.backgroundColor);
    ReadJsonFloat(document, "backgroundColorIntensity", backgroundFog.backgroundColorIntensity);
    ReadJsonFloat(document, "backgroundColorFadeoutRange", backgroundFog.backgroundColorFadeoutRange);
    ReadJsonFloat(document, "backgroundColorFadeoutPower", backgroundFog.backgroundColorFadeoutPower);

    // --- Height fog (5 fields) ---
    Params::AtmosphereHeightFog& heightFog = atmosphere.heightFog;
    ReadJsonFloat(document, "heightFogIntensity", heightFog.heightFogIntensity);
    ReadJsonVector2(document, "heightFogRange", heightFog.heightFogRange);
    ReadJsonFloat(document, "heightFogStart", heightFog.heightFogStart);
    ReadJsonFloat(document, "heightFogEnd", heightFog.heightFogEnd);
    ReadJsonFloat(document, "heightFogPower", heightFog.heightFogPower);

    // --- Linear fog (7 fields) ---
    Params::AtmosphereLinearFog& linearFog = atmosphere.linearFog;
    ReadJsonFloat(document, "linearFogIntensity", linearFog.linearFogIntensity);
    ReadJsonFloat(document, "linearFogStart", linearFog.linearFogStart);
    ReadJsonFloat(document, "linearFogEnd", linearFog.linearFogEnd);
    ReadJsonFloat(document, "linearFogPower", linearFog.linearFogPower);
    ReadJsonFloat(document, "linearFogCameraIntensity", linearFog.linearFogCameraIntensity);
    ReadJsonFloat(document, "linearFogCameraStart", linearFog.linearFogCameraStart);
    ReadJsonFloat(document, "linearFogCameraEnd", linearFog.linearFogCameraEnd);

    // --- Global wind (2 fields) ---
    Params::AtmosphereGlobalWind& globalWind = atmosphere.globalWind;
    ReadJsonFloat(document, "windSpeed", globalWind.globalWindSpeed);
    ReadJsonFloat(document, "windDirection", globalWind.globalWindDirection);
}

} // namespace Io
} // namespace SanmapGen
