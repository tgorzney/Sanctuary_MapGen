// GradientRamp_PARAMS_Test.cpp — acceptance test for GradientRamp_PARAMS (M4-0a).
//   g++ -O2 -std=c++17 GradientRamp_PARAMS_Test.cpp -o t && ./t
// Header must compile standalone: this file includes nothing but the header + STL.
#include "GradientRamp_PARAMS.h"
#include <algorithm>
#include <cstdio>

using namespace SanmapGen::Params;

static GradientStop MakeStop(float location, float red, float green, float blue) {
    GradientStop stop;
    stop.location = location;
    stop.color[0] = red; stop.color[1] = green; stop.color[2] = blue; stop.color[3] = 1.0f;
    return stop;
}

int main() {
    int failures = 0;

    // Default-constructed ramp is valid.
    GradientRamp ramp;
    if (!ramp.stops.empty())            { std::printf("FAIL default stops not empty\n"); ++failures; }
    if (ramp.lookupResolution != 256)   { std::printf("FAIL default lookupResolution\n"); ++failures; }
    if (!ramp.bSmoothInterpolation)     { std::printf("FAIL default bSmoothInterpolation\n"); ++failures; }
    if (ramp.name != "New Ramp")        { std::printf("FAIL default name\n"); ++failures; }

    // Default-constructed stop is opaque white at the ramp start.
    GradientStop stop;
    if (stop.location != 0.0f)          { std::printf("FAIL default stop location\n"); ++failures; }
    if (stop.color[0] != 1.0f || stop.color[1] != 1.0f ||
        stop.color[2] != 1.0f || stop.color[3] != 1.0f) {
        std::printf("FAIL default stop color\n"); ++failures;
    }

    // Locations are normalized 0..1 by contract — the caller stores them that way.
    ramp.stops.push_back(MakeStop(1.0f, 1.0f, 1.0f, 1.0f));
    ramp.stops.push_back(MakeStop(0.25f, 0.0f, 1.0f, 0.0f));
    ramp.stops.push_back(MakeStop(0.0f, 0.0f, 0.0f, 0.0f));
    ramp.stops.push_back(MakeStop(0.5f, 1.0f, 0.0f, 0.0f));

    // A stop list sorts by location.
    std::sort(ramp.stops.begin(), ramp.stops.end());
    const float expectedLocations[4] = {0.0f, 0.25f, 0.5f, 1.0f};
    for (std::size_t index = 0; index < ramp.stops.size(); ++index) {
        if (ramp.stops[index].location != expectedLocations[index]) {
            std::printf("FAIL sort order at %zu\n", index); ++failures;
        }
    }
    // Sorting moved whole stops, colors included (green key stayed with location 0.25).
    if (ramp.stops[1].color[1] != 1.0f || ramp.stops[1].color[0] != 0.0f) {
        std::printf("FAIL color travelled with stop\n"); ++failures;
    }
    if (!(ramp.stops[0] < ramp.stops[3]) || (ramp.stops[3] < ramp.stops[0])) {
        std::printf("FAIL operator< ordering\n"); ++failures;
    }

    // The ramp is a plain copyable settings value.
    GradientRamp copiedRamp = ramp;
    copiedRamp.lookupResolution = 512;
    if (ramp.lookupResolution != 256 || copiedRamp.stops.size() != ramp.stops.size()) {
        std::printf("FAIL value copy\n"); ++failures;
    }

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
