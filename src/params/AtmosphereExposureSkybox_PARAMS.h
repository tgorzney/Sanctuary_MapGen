// AtmosphereExposureSkybox_PARAMS.h — camera exposure plus the skybox's own settings.
// Layer: PARAMS. Verbatim from ATMOSPHERE_PARAMS_SPEC.md's "The types" section, EXCEPT the one
// sanctioned retype (ARCH §1.8): `skyboxIntensityModeIndex` (a raw dropdown-row int in
// `Ui::AtmosphereSettings`) becomes the typed `skyboxIntensityMode : SkyboxIntensityMode` here —
// same move already made for `Army::faction`/`MarkerRule::category`. The `Index` suffix drops on
// retype, matching those precedents.
#pragma once
#include <string>
#include "GenerationEnums_PARAMS.h"

namespace SanmapGen {
namespace Params {

struct AtmosphereExposureSkybox {
    float exposure                          = 12.0f;
    float exposureCompensation              = 2.5f;
    std::string skyboxPath;
    float skyboxRotation                    = 0.0f;
    SkyboxIntensityMode skyboxIntensityMode = SkyboxIntensityMode::Exposure;  // retyped, see above
    float skyboxExposure                    = 12.0f;
    float skyboxMultiplier                  = 1.0f;
    float skyboxLuxValue                    = 10000.0f;
};

} // namespace Params
} // namespace SanmapGen
