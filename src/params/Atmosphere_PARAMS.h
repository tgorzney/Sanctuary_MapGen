// Atmosphere_PARAMS.h — the aggregator of the sun/sky/fog/wind rendering-presentation recipe.
// Layer: PARAMS. Promotion of `Ui::AtmosphereSettings` per ATMOSPHERE_PARAMS_SPEC.md (that file's
// own header comment named this gap and deferred it to its own work-order — this is it).
//
// Composition, not a rival type (ARCH §7.1) — the same shape already used for Stratum/
// StratumAppearance/StratumSoilPhysics: one aggregator of 8 independently-toggled rendering
// subsystems, each kept in its own file per the spec's file-split ruling (uniform beats
// asymmetric, even for the two smallest sub-structs).
//
// No PROC/PIPELINE stage reads these values yet (out of scope for this ticket, per the spec's own
// "Where these land" section) — this only gives them a durable, round-tripping `Params::` home.
#pragma once
#include "AtmosphereBackgroundFog_PARAMS.h"
#include "AtmosphereExposureSkybox_PARAMS.h"
#include "AtmosphereGlobalWind_PARAMS.h"
#include "AtmosphereHeightFog_PARAMS.h"
#include "AtmosphereLegacyFog_PARAMS.h"
#include "AtmosphereLinearFog_PARAMS.h"
#include "AtmosphereSkylight_PARAMS.h"
#include "AtmosphereSun_PARAMS.h"

namespace SanmapGen {
namespace Params {

struct Atmosphere {
    AtmosphereSun            sun;
    AtmosphereSkylight       skylight;
    AtmosphereExposureSkybox exposureSkybox;
    AtmosphereLegacyFog      legacyFog;
    AtmosphereBackgroundFog  backgroundFog;
    AtmosphereHeightFog      heightFog;
    AtmosphereLinearFog      linearFog;
    AtmosphereGlobalWind     globalWind;
};

} // namespace Params
} // namespace SanmapGen
