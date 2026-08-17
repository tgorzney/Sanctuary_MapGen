// Application_PreviewRamps_UI.h — the colour ramps the shell's default preview composition ships
// with, one factory per colorized field. Layer: UI. Internal to the shell: only
// Application_PreviewSetup_UI.cpp includes it (the ARCH §1.5 split of one over-long file).
//
// ONE RAMP PER FIELD (ARCH §8.2): the legacy "accumulation reuses the flow gradient" aliasing is
// retired, so flow and accumulation each get their own even though they start out similar.
// The stop colours are DEFAULTS, not constants at a use site: every one of them is reachable
// through the tab that owns that overlay's `GradientEditor` (Constitution §8).
#pragma once
#include "../params/GradientRamp_PARAMS.h"

namespace SanmapGen {
namespace Ui {

Params::GradientRamp MakeTerrainHeightRamp();     // Application_PreviewRamps_UI.cpp
Params::GradientRamp MakeWaterDepthRamp();
Params::GradientRamp MakeSlopeRamp();
Params::GradientRamp MakeFlowRamp();
Params::GradientRamp MakeAccumulationRamp();

} // namespace Ui
} // namespace SanmapGen
