// GradientEditorWidget_UI.cpp — the pure edit semantics of the gradient editor (M5-3).
// No imgui, no GL, no DATA: every function here is a total function of (ramp, edit) and is
// testable headless, which is what lets the acceptance test prove the add/move/delete round-trip
// straight through Ui::BakeGradientLut (M4-2). The drawing lives in the aspect twin,
// GradientEditorWidget_Draw_UI.cpp (ARCH §1.5).
#include "GradientEditorWidget_UI.h"
#include <cstddef>

namespace SanmapGen {
namespace Ui {
namespace {

float ClampToUnitRange(float value) {
    return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}

bool IsStopIndexValid(const Params::GradientRamp& ramp, int stopIndex) {
    return stopIndex >= 0 && stopIndex < static_cast<int>(ramp.stops.size());
}

bool ColorsAreEqual(const float* left, const float* right) {
    for (int channel = 0; channel < kGradientStopChannelCount; ++channel)
        if (left[channel] != right[channel]) return false;
    return true;
}

void CopyColor(const float* source, float* destination) {
    for (int channel = 0; channel < kGradientStopChannelCount; ++channel)
        destination[channel] = source[channel];
}

float AbsoluteDifference(float left, float right) {
    const float difference = left - right;
    return difference < 0.0f ? -difference : difference;
}

} // namespace

int AddGradientStop(Params::GradientRamp& ramp, float location,
                    const float color[kGradientStopChannelCount]) {
    if (static_cast<int>(ramp.stops.size()) >= kMaximumGradientStopCount) return -1;

    Params::GradientStop stop;
    stop.location = ClampToUnitRange(location);
    if (color != nullptr) CopyColor(color, stop.color);

    // Insert before the first stop that sits strictly later. That keeps a sorted ramp sorted and
    // is still a defined, stable rule for a ramp a drag has left unsorted (see MoveGradientStop).
    std::size_t insertionIndex = 0;
    while (insertionIndex < ramp.stops.size() &&
           ramp.stops[insertionIndex].location <= stop.location) ++insertionIndex;
    ramp.stops.insert(ramp.stops.begin() + static_cast<std::ptrdiff_t>(insertionIndex), stop);
    return static_cast<int>(insertionIndex);
}

bool MoveGradientStop(Params::GradientRamp& ramp, int stopIndex, float newLocation) {
    if (!IsStopIndexValid(ramp, stopIndex)) return false;
    const float clampedLocation = ClampToUnitRange(newLocation);
    if (ramp.stops[static_cast<std::size_t>(stopIndex)].location == clampedLocation) return false;
    ramp.stops[static_cast<std::size_t>(stopIndex)].location = clampedLocation;
    return true;
}

bool DeleteGradientStop(Params::GradientRamp& ramp, int stopIndex) {
    if (!IsStopIndexValid(ramp, stopIndex)) return false;
    ramp.stops.erase(ramp.stops.begin() + static_cast<std::ptrdiff_t>(stopIndex));
    return true;
}

bool RecolorGradientStop(Params::GradientRamp& ramp, int stopIndex,
                         const float color[kGradientStopChannelCount]) {
    if (!IsStopIndexValid(ramp, stopIndex) || color == nullptr) return false;
    float* const stopColor = ramp.stops[static_cast<std::size_t>(stopIndex)].color;
    if (ColorsAreEqual(stopColor, color)) return false;
    CopyColor(color, stopColor);
    return true;
}

bool SetGradientSmoothInterpolation(Params::GradientRamp& ramp, bool bSmoothInterpolation) {
    if (ramp.bSmoothInterpolation == bSmoothInterpolation) return false;
    ramp.bSmoothInterpolation = bSmoothInterpolation;
    return true;
}

int NearestGradientStopIndex(const Params::GradientRamp& ramp, float location) {
    const int stopCount = static_cast<int>(ramp.stops.size());
    if (stopCount <= 0) return -1;
    const float target = ClampToUnitRange(location);
    int nearestIndex = 0;
    float nearestDistance = AbsoluteDifference(ramp.stops[0].location, target);
    for (int stopIndex = 1; stopIndex < stopCount; ++stopIndex) {
        const float distance =
            AbsoluteDifference(ramp.stops[static_cast<std::size_t>(stopIndex)].location, target);
        if (distance < nearestDistance) { nearestDistance = distance; nearestIndex = stopIndex; }
    }
    return nearestIndex;
}

} // namespace Ui
} // namespace SanmapGen
