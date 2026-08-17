// Application_PreviewRamps_UI.cpp — the five default ramps. Layer: UI.
// Behind Application_PreviewRamps_UI.h (ARCH §1.5). Pure construction: no imgui, no GL, no field.
#include "Application_PreviewRamps_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

// One stop, so the factories below read as a colour table rather than as five lines of assignment
// each. Alpha is explicit: the overlays are alpha-blended over the terrain, so it carries meaning.
Params::GradientStop MakeStop(float location, float red, float green, float blue, float alpha) {
    Params::GradientStop stop;
    stop.location = location;
    stop.color[0] = red; stop.color[1] = green; stop.color[2] = blue; stop.color[3] = alpha;
    return stop;
}

Params::GradientRamp MakeRamp(const char* name) {
    Params::GradientRamp ramp;
    ramp.name = name;
    return ramp;
}

} // namespace

Params::GradientRamp MakeTerrainHeightRamp() {
    Params::GradientRamp ramp = MakeRamp("Height");
    ramp.stops.push_back(MakeStop(0.0f,  0.11f, 0.17f, 0.13f, 1.0f));
    ramp.stops.push_back(MakeStop(0.45f, 0.42f, 0.44f, 0.30f, 1.0f));
    ramp.stops.push_back(MakeStop(1.0f,  0.92f, 0.92f, 0.90f, 1.0f));
    return ramp;
}

Params::GradientRamp MakeWaterDepthRamp() {
    Params::GradientRamp ramp = MakeRamp("Water Depth");
    ramp.stops.push_back(MakeStop(0.0f, 0.28f, 0.55f, 0.68f, 0.55f));
    ramp.stops.push_back(MakeStop(1.0f, 0.04f, 0.13f, 0.34f, 0.95f));
    return ramp;
}

// Flat is transparent, steep is hot: an overlay reads as an overlay only if its low end lets the
// terrain through.
Params::GradientRamp MakeSlopeRamp() {
    Params::GradientRamp ramp = MakeRamp("Slope");
    ramp.stops.push_back(MakeStop(0.0f, 0.10f, 0.55f, 0.20f, 0.0f));
    ramp.stops.push_back(MakeStop(0.5f, 0.90f, 0.80f, 0.15f, 0.65f));
    ramp.stops.push_back(MakeStop(1.0f, 0.90f, 0.15f, 0.10f, 0.90f));
    return ramp;
}

Params::GradientRamp MakeFlowRamp() {
    Params::GradientRamp ramp = MakeRamp("Flow");
    ramp.stops.push_back(MakeStop(0.0f, 0.15f, 0.35f, 0.55f, 0.0f));
    ramp.stops.push_back(MakeStop(1.0f, 0.45f, 0.85f, 1.00f, 0.90f));
    return ramp;
}

Params::GradientRamp MakeAccumulationRamp() {
    Params::GradientRamp ramp = MakeRamp("Accumulation");
    ramp.stops.push_back(MakeStop(0.0f, 0.30f, 0.20f, 0.45f, 0.0f));
    ramp.stops.push_back(MakeStop(1.0f, 0.95f, 0.60f, 1.00f, 0.90f));
    return ramp;
}

} // namespace Ui
} // namespace SanmapGen
