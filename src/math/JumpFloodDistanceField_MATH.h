// JumpFloodDistanceField_MATH.h — distance-to-nearest-obstacle via Jump Flooding.
// Layer: MATH (raw arrays, not FloatMask). For every cell, the Euclidean distance to
// the nearest "seed" cell (out of the [minHeight,maxHeight] band, or where the local
// gradient magnitude exceeds gradientTolerance), clamped to maxDistance. Used by
// placement to keep scatter away from boundaries/obstacles. O(w*h*log(max(w,h))).
// Fixes vs the old version: int seed coords (no 32767 cap), pointer-swap ping-pong
// (no full per-pass copy), and two extra step-1 passes (JFA+2) for exactness.
#pragma once
#include <cstdint>
#include <cmath>
#include <vector>

namespace SanmapGen {
namespace Math {

struct SeedCoordinate { int x; int y; };

inline void ComputeJumpFloodDistanceField(const float* heightField, int width, int height,
                                          float minHeight, float maxHeight, float gradientTolerance,
                                          float maxDistance, float* outDistance) {
    const int cellCount = width * height;
    std::vector<SeedCoordinate> bufferA(cellCount, SeedCoordinate{-1, -1});
    std::vector<SeedCoordinate> bufferB(cellCount, SeedCoordinate{-1, -1});

    // Seed the obstacle cells.
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            const int index = y * width + x;
            const float sample = heightField[index];
            bool isSeed = (sample < minHeight || sample > maxHeight);
            if (!isSeed && x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                float gradientX = heightField[index + 1]     - heightField[index - 1];
                float gradientY = heightField[index + width] - heightField[index - width];
                if (std::sqrt(gradientX * gradientX + gradientY * gradientY) > gradientTolerance) isSeed = true;
            }
            if (isSeed) bufferA[index] = SeedCoordinate{x, y};
        }

    auto squaredDistance = [](int x, int y, SeedCoordinate seed) -> long long {
        long long deltaX = x - seed.x, deltaY = y - seed.y;
        return deltaX * deltaX + deltaY * deltaY;
    };

    SeedCoordinate* readBuffer = bufferA.data();
    SeedCoordinate* writeBuffer = bufferB.data();

    auto relaxAtStep = [&](int step) {
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x) {
                const int index = y * width + x;
                SeedCoordinate best = readBuffer[index];
                long long bestSquared = (best.x < 0) ? -1 : squaredDistance(x, y, best);
                for (int offsetY = -1; offsetY <= 1; ++offsetY)
                    for (int offsetX = -1; offsetX <= 1; ++offsetX) {
                        if (offsetX == 0 && offsetY == 0) continue;
                        int neighborX = x + offsetX * step, neighborY = y + offsetY * step;
                        if (neighborX < 0 || neighborY < 0 || neighborX >= width || neighborY >= height) continue;
                        SeedCoordinate candidate = readBuffer[neighborY * width + neighborX];
                        if (candidate.x < 0) continue;
                        long long candidateSquared = squaredDistance(x, y, candidate);
                        if (bestSquared < 0 || candidateSquared < bestSquared) { bestSquared = candidateSquared; best = candidate; }
                    }
                writeBuffer[index] = best;
            }
        SeedCoordinate* temporary = readBuffer; readBuffer = writeBuffer; writeBuffer = temporary;
    };

    int step = 1;
    while (step * 2 <= (width > height ? width : height)) step *= 2;
    for (; step >= 1; step /= 2) relaxAtStep(step);
    relaxAtStep(1);   // JFA+2: two extra step-1 passes remove residual errors
    relaxAtStep(1);

    for (int index = 0; index < cellCount; ++index) {
        SeedCoordinate seed = readBuffer[index];
        float distance = (seed.x < 0) ? maxDistance
                       : std::sqrt(static_cast<float>(squaredDistance(index % width, index / width, seed)));
        outDistance[index] = distance > maxDistance ? maxDistance : distance;
    }
}

} // namespace Math
} // namespace SanmapGen
