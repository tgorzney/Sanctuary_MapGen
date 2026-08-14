// Erosion_Rain_PROC.cpp — the precipitation field that decides where droplets are born.
// Layer: PROC (Cpu; the field is built once and its spawn list is uploaded to the Gpu, so both
// backends rain identically). FBm rain noise + orographic rain-shadow, the model
// SIM_ALGORITHMS_SPEC records — but on portable integer-hash value noise and Trigonometry_MATH,
// never libm, so the field is reproducible on any machine (DETERMINISM_SPEC).
#include "Erosion_PROC.h"
#include "../math/Trigonometry_MATH.h"

namespace SanmapGen {
namespace Proc {
namespace {

constexpr float degreesToRadians = 0.01745329251994329577f;

inline float SmoothStepFraction(float fraction) { return fraction * fraction * (3.0f - 2.0f * fraction); }
inline float ClampFloat(float value, float lowest, float highest) {
    return value < lowest ? lowest : (value > highest ? highest : value);
}

inline float LatticeValue(int latticeX, int latticeY, unsigned int seed) {
    const unsigned int cell = HashRandomCombine(static_cast<unsigned int>(latticeX),
                                                static_cast<unsigned int>(latticeY));
    return HashRandomUnitFloat(HashRandomCombine(seed, cell));
}

// Portable value noise in [0,1] — the deterministic stand-in for the old FastNoiseLite FBm.
float ValueNoise(float pointX, float pointY, unsigned int seed) {
    const int latticeX = static_cast<int>(pointX) - (pointX < 0.0f ? 1 : 0);
    const int latticeY = static_cast<int>(pointY) - (pointY < 0.0f ? 1 : 0);
    const float blendX = SmoothStepFraction(pointX - static_cast<float>(latticeX));
    const float blendY = SmoothStepFraction(pointY - static_cast<float>(latticeY));
    const float lowRow  = LatticeValue(latticeX, latticeY, seed) * (1.0f - blendX)
                        + LatticeValue(latticeX + 1, latticeY, seed) * blendX;
    const float highRow = LatticeValue(latticeX, latticeY + 1, seed) * (1.0f - blendX)
                        + LatticeValue(latticeX + 1, latticeY + 1, seed) * blendX;
    return lowRow * (1.0f - blendY) + highRow * blendY;
}

float FractalRainValue(float pointX, float pointY, unsigned int seed, const ErosionLayerSettings& settings) {
    float amplitude = 1.0f, amplitudeSum = 0.0f, total = 0.0f;
    float frequency = settings.rainNoiseFrequency;
    for (int octave = 0; octave < settings.rainNoiseOctaves; ++octave) {
        total += ValueNoise(pointX * frequency, pointY * frequency,
                            seed + static_cast<unsigned int>(octave)) * amplitude;
        amplitudeSum += amplitude;
        amplitude *= settings.rainNoiseGain;
        frequency *= settings.rainNoiseLacunarity;
    }
    return amplitudeSum > 0.0f ? total / amplitudeSum : 0.0f;
}

void ApplyRainNoise(std::vector<float>& rainMap, int vertexSize, unsigned int rainSeed,
                    const ErosionLayerSettings& settings) {
    const float thresholdSpan = 1.0f - settings.rainNoiseThreshold;
    const float thresholdReciprocal = thresholdSpan > 0.0f ? 1.0f / thresholdSpan : 0.0f;
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x) {
            const float value = FractalRainValue(static_cast<float>(x), static_cast<float>(y), rainSeed, settings);
            rainMap[static_cast<std::size_t>(y) * vertexSize + x] =
                value < settings.rainNoiseThreshold ? 0.0f
                                                    : (value - settings.rainNoiseThreshold) * thresholdReciprocal;
        }
}

// Windward slopes catch the rain, leeward slopes sit in its shadow; higher ground gets more.
void ApplyOrographicRain(std::vector<float>& rainMap, const std::vector<float>& columnHeights, int vertexSize,
                         const ErosionLayerSettings& settings) {
    const float windRadians = settings.windAngleDegrees * degreesToRadians;
    const float windX = Math::Cosine(windRadians);
    const float windY = Math::Sine(windRadians);
    for (int y = 1; y < vertexSize - 1; ++y)
        for (int x = 1; x < vertexSize - 1; ++x) {
            const std::size_t cellIndex = static_cast<std::size_t>(y) * vertexSize + x;
            const float normalX = (columnHeights[cellIndex - 1] - columnHeights[cellIndex + 1]) * 0.5f;
            const float normalY = (columnHeights[cellIndex - vertexSize] - columnHeights[cellIndex + vertexSize]) * 0.5f;
            const float orographic = ClampFloat(1.0f + (normalX * windX + normalY * windY) * settings.orographicStrength,
                                                settings.orographicMinimum, settings.orographicMaximum);
            const float heightMultiplier = ClampFloat(columnHeights[cellIndex] * settings.rainHeightScale,
                                                      settings.rainHeightMinimum, settings.rainHeightMaximum);
            rainMap[cellIndex] *= orographic * heightMultiplier;
        }
}

} // namespace

void ErosionStage::BuildRainMap(int stratumIndex) {
    const ErosionLayerSettings& settings = layerSettings[stratumIndex];
    const unsigned int rainSeed = static_cast<unsigned int>(static_cast<int>(geometry.seed)
                                                          + constants.rainNoiseSeedOffset + stratumIndex);
    const float fixedPointInverse = constants.HeightFixedPointInverse();
    std::vector<float> columnHeights(static_cast<std::size_t>(cellCount));
    for (int cellIndex = 0; cellIndex < cellCount; ++cellIndex) {
        rainMap[cellIndex] = 1.0f;
        columnHeights[cellIndex] = FixedPointToHeight(ColumnTotalFixedPointAt(cellIndex), fixedPointInverse);
    }

    if (settings.bUseRainNoise) ApplyRainNoise(rainMap, vertexSize, rainSeed, settings);
    if (settings.bUseOrographicRain) ApplyOrographicRain(rainMap, columnHeights, vertexSize, settings);
    if (settings.bDepositionMode)
        for (int cellIndex = 0; cellIndex < cellCount; ++cellIndex)
            if (columnHeights[cellIndex] < settings.spawnMinimumHeight
                || columnHeights[cellIndex] > settings.spawnMaximumHeight) rainMap[cellIndex] = 0.0f;
}

} // namespace Proc
} // namespace SanmapGen
