// GradientLut_UI.cpp — CPU bake of a color ramp into a sampled RGBA table. Layer: UI.
// One forward walk over the entries with a cursor into the sorted stops (O(entries + stops),
// no per-sample search) and a precomputed per-segment reciprocal span (Constitution §3:
// multiply a reciprocal, never divide inside the loop).
#include "GradientLut_UI.h"
#include <algorithm>
#include <cstddef>

namespace SanmapGen {
namespace Ui {
namespace {

// Clamped, sorted working copy — the PARAMS input is read-only to us (Constitution §6).
// The `!(location > 0)` form also traps NaN into the ramp start. stable_sort keeps the
// author's order for stops that share a location, so a hard edge bakes deterministically.
std::vector<Params::GradientStop> SanitizeStops(const std::vector<Params::GradientStop>& stops) {
    std::vector<Params::GradientStop> sanitizedStops(stops);
    for (Params::GradientStop& stop : sanitizedStops) {
        if (!(stop.location > 0.0f))      stop.location = 0.0f;
        else if (stop.location > 1.0f)    stop.location = 1.0f;
    }
    std::stable_sort(sanitizedStops.begin(), sanitizedStops.end());
    return sanitizedStops;
}

int ResolveEntryCount(const Params::GradientRamp& ramp, int resolution) {
    int entryCount = resolution < 0 ? ramp.lookupResolution : resolution;
    if (entryCount < kMinimumLookupResolution) entryCount = kMinimumLookupResolution;
    if (entryCount > kMaximumLookupResolution) entryCount = kMaximumLookupResolution;
    return entryCount;
}

float SegmentInverseSpan(const std::vector<Params::GradientStop>& stops, std::size_t segment) {
    const float span = stops[segment + 1].location - stops[segment].location;
    return span > 0.0f ? 1.0f / span : 0.0f;
}

inline float BlendFraction(float fraction, bool bSmoothInterpolation) {
    return bSmoothInterpolation ? fraction * fraction * (3.0f - 2.0f * fraction) : fraction;
}

void FillConstantEntries(const float color[kLookupChannelCount], std::vector<float>& lookupTable) {
    for (std::size_t offset = 0; offset < lookupTable.size(); offset += kLookupChannelCount)
        for (int channel = 0; channel < kLookupChannelCount; ++channel)
            lookupTable[offset + channel] = color[channel];
}

// The interpolating bake. `sortedStops` holds two or more stops. The two-term lerp form is
// exact at both ends of a segment, which is what makes the table's endpoints exact.
void FillInterpolatedEntries(const std::vector<Params::GradientStop>& sortedStops,
                             bool bSmoothInterpolation, int entryCount,
                             std::vector<float>& lookupTable) {
    const float inverseLastEntry = entryCount > 1 ? 1.0f / static_cast<float>(entryCount - 1) : 0.0f;
    std::size_t segment = 0;
    float inverseSpan = SegmentInverseSpan(sortedStops, segment);
    for (int entry = 0; entry < entryCount; ++entry) {
        const float position = static_cast<float>(entry) * inverseLastEntry;
        const std::size_t previousSegment = segment;
        while (segment + 2 < sortedStops.size() && position >= sortedStops[segment + 1].location)
            ++segment;
        if (segment != previousSegment) inverseSpan = SegmentInverseSpan(sortedStops, segment);
        const Params::GradientStop& lowStop = sortedStops[segment];
        const Params::GradientStop& highStop = sortedStops[segment + 1];
        // Positions before the first stop hold its color; positions past the last hold its own.
        float fraction = 0.0f;
        if (position >= highStop.location)     fraction = 1.0f;
        else if (position > lowStop.location)
            fraction = BlendFraction((position - lowStop.location) * inverseSpan, bSmoothInterpolation);

        float* const targetEntry = &lookupTable[static_cast<std::size_t>(entry) * kLookupChannelCount];
        const float lowWeight = 1.0f - fraction;
        for (int channel = 0; channel < kLookupChannelCount; ++channel)
            targetEntry[channel] = lowStop.color[channel] * lowWeight + highStop.color[channel] * fraction;
    }
}

} // namespace

std::vector<float> BakeGradientLut(const Params::GradientRamp& ramp, int resolution) {
    const int entryCount = ResolveEntryCount(ramp, resolution);
    std::vector<float> lookupTable(static_cast<std::size_t>(entryCount) * kLookupChannelCount, 0.0f);

    const std::vector<Params::GradientStop> sortedStops = SanitizeStops(ramp.stops);
    if (sortedStops.empty()) {
        // Safe placeholder rather than a crash or an undefined table (Constitution §6): the
        // default the PARAMS type itself declares, so no magic color is invented here.
        const Params::GradientStop fallbackStop;
        FillConstantEntries(fallbackStop.color, lookupTable);
        return lookupTable;
    }
    if (sortedStops.size() == 1) {
        FillConstantEntries(sortedStops[0].color, lookupTable);
        return lookupTable;
    }
    FillInterpolatedEntries(sortedStops, ramp.bSmoothInterpolation, entryCount, lookupTable);
    return lookupTable;
}

} // namespace Ui
} // namespace SanmapGen
