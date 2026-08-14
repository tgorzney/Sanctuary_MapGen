// RadialClearance_MATH.h — largest obstacle-free radius around a cell.
// Layer: MATH (pure; operates on a raw height array, not the DATA FloatMask — fixes
// the old MATH->DATA include). Used by placement/scatter to size clearances. One
// gallop+binary-search driver, templated on the perimeter predicate (the two old
// copies are merged). Exact (Bresenham) and cheaper stochastic variants.
#pragma once
#include <cstdint>
#include "Trigonometry_MATH.h"

namespace SanmapGen {
namespace Math {
namespace RadialClearanceDetail {

constexpr uint32_t hashPrimeRadius = 19349663u;   // Teschner spatial-hash primes
constexpr uint32_t hashPrimeX      = 73856093u;
constexpr uint32_t hashPrimeY      = 83492791u;
constexpr float pi = 3.14159265358979323846f;

inline int RoundToInt(float value) { return static_cast<int>(value + (value >= 0.0f ? 0.5f : -0.5f)); }

// Largest radius in [minStartRadius, maxSearchRadius] for which isClear(radius) holds.
template <typename PredicateIsClear>
inline int FindLargestClearRadius(int minStartRadius, int maxSearchRadius, PredicateIsClear isClear) {
    if (minStartRadius > maxSearchRadius || !isClear(minStartRadius)) return 0;
    int clear = minStartRadius;
    int probe = minStartRadius * 2;
    while (probe <= maxSearchRadius && isClear(probe)) { clear = probe; probe *= 2; }
    int high = probe <= maxSearchRadius ? probe : maxSearchRadius;
    while (clear < high) {                       // invariant: isClear(clear) == true
        int mid = clear + (high - clear + 1) / 2;
        if (isClear(mid)) clear = mid; else high = mid - 1;
    }
    return clear;
}

inline bool CellInBand(const float* heightField, int width, int height, int px, int py,
                       float centerHeight, float minHeight, float maxHeight, float heightTolerance) {
    if (px < 0 || py < 0 || px >= width || py >= height) return false;
    float sample = heightField[py * width + px];
    if (sample < minHeight || sample > maxHeight) return false;
    float difference = sample - centerHeight;
    if (difference < 0.0f) difference = -difference;
    return difference <= heightTolerance;
}

} // namespace RadialClearanceDetail

// Full Bresenham-perimeter clearance: every perimeter cell must be in bounds, within
// [minHeight,maxHeight], and within heightTolerance of the center's height.
inline int ScoreRadialClearance(const float* heightField, int width, int height,
                                int centerX, int centerY, float minHeight, float maxHeight,
                                float heightTolerance, int maxSearchRadius, int minStartRadius = 1) {
    using namespace RadialClearanceDetail;
    const float centerHeight = heightField[centerY * width + centerX];
    auto perimeterClear = [&](int radius) -> bool {
        int x = radius, y = 0, err = 0;
        while (x >= y) {
            const int px[8] = { centerX+x, centerX-x, centerX+x, centerX-x, centerX+y, centerX-y, centerX+y, centerX-y };
            const int py[8] = { centerY+y, centerY+y, centerY-y, centerY-y, centerY+x, centerY+x, centerY-x, centerY-x };
            for (int i = 0; i < 8; ++i)
                if (!CellInBand(heightField, width, height, px[i], py[i], centerHeight, minHeight, maxHeight, heightTolerance))
                    return false;
            ++y;
            if (err <= 0) err += 2 * y + 1;
            if (err > 0) { --x; err -= 2 * x + 1; }
        }
        return true;
    };
    return FindLargestClearRadius(minStartRadius, maxSearchRadius, perimeterClear);
}

// Cheaper stochastic variant: 8 jittered angular samples per radius. Deterministic in
// (seed, centerX, centerY) via a position hash — same inputs always give same result.
inline int ScoreRadialClearanceStochastic(const float* heightField, int width, int height,
                                          int centerX, int centerY, float minHeight, float maxHeight,
                                          float heightTolerance, int maxSearchRadius,
                                          int minStartRadius = 1, uint32_t seed = 12345u) {
    using namespace RadialClearanceDetail;
    const float centerHeight = heightField[centerY * width + centerX];
    auto sampledClear = [&](int radius) -> bool {
        uint32_t hashed = seed ^ (static_cast<uint32_t>(radius) * hashPrimeRadius)
                               ^ (static_cast<uint32_t>(centerX) * hashPrimeX)
                               ^ (static_cast<uint32_t>(centerY) * hashPrimeY);
        float baseAngle = static_cast<float>(hashed % 360u) * (pi / 180.0f);
        for (int step = 0; step < 8; ++step) {
            float angle = baseAngle + static_cast<float>(step) * (pi / 4.0f);
            int px = centerX + RoundToInt(static_cast<float>(radius) * Cosine(angle));
            int py = centerY + RoundToInt(static_cast<float>(radius) * Sine(angle));
            if (!CellInBand(heightField, width, height, px, py, centerHeight, minHeight, maxHeight, heightTolerance))
                return false;
        }
        return true;
    };
    return FindLargestClearRadius(minStartRadius, maxSearchRadius, sampledClear);
}

} // namespace Math
} // namespace SanmapGen
