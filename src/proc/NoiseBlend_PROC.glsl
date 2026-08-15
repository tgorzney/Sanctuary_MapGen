#version 430 core
// NoiseBlend_PROC.glsl — GPU speed path of the noise/blend stage; twin of NoiseBlend_PROC.cpp.
// Two passes over the same program: PASS_NOISE fills one layer's cached raw noise (so the
// two-level dirty hash works on the Gpu too — only layers whose structure changed are
// re-dispatched), PASS_BLEND reshapes, blends the whole stack into the heightfield, and
// writes the per-stratum material proportions by top-down occlusion.
// This unit owns the std430 LayerConfiguration block, declared ONCE, mirroring
// Proc::LayerKernelConfiguration field for field (DISPATCH_INTERFACE_SPEC §4). Every tile
// size, enum value and stratum count arrives as a #define built from the C++ constants —
// nothing is hardcoded here (Constitution §8).
layout(local_size_x = NOISE_BLEND_TILE_WIDTH, local_size_y = NOISE_BLEND_TILE_HEIGHT) in;

struct LayerConfiguration {
    int   noiseType;          int   fractalType;        int   layerSeed;         int   octaves;
    float frequency;          float gain;               float lacunarity;        float weightedStrength;
    float pingPongStrength;   float cellularJitter;     float fractalBounding;   float landDensityMultiplier;
    float mountainDensity;    float plateauDensity;     float rampDensity;       float terraceHeight;
    float terraceHeightReciprocal; float levelsShadows; float levelsMidtones;    float levelsHighlights;
    float levelsRangeReciprocal;   float levelsOutputBlack; float levelsOutputWhite;
    int   blendMode;          int   stratumIndex;
    float opacity;            float heightBlendContrast; float occlusionWindowLow; float occlusionWindowHigh;
    float heightMinimum;      float heightMaximum;       float padding;
};

layout(std430, binding = NOISE_BLEND_BINDING_LAYERS) readonly buffer LayerConfigurations {
    LayerConfiguration layerConfigurations[]; };
layout(std430, binding = NOISE_BLEND_BINDING_RAW_NOISE) buffer RawNoiseField { float rawNoiseValues[]; };
layout(std430, binding = NOISE_BLEND_BINDING_HEIGHT)    buffer HeightField   { float heightValues[]; };
layout(std430, binding = NOISE_BLEND_BINDING_PROPORTIONS)     buffer MaterialProportions { float proportionValues[]; };
layout(std430, binding = NOISE_BLEND_BINDING_THICKNESS) buffer LayerThickness { float thicknessValues[]; };

uniform int vertexSize;
uniform int layerCount;
uniform int passMode;
uniform int activeLayerIndex;

float fractalNoise(int noiseType, int fractalType, int seed, int octaves, float gain, float lacunarity,
                   float weightedStrength, float pingPongStrength, float cellularJitter,
                   float fractalBounding, vec2 point);
vec2  transformNoisePoint(vec2 point, float frequency, int noiseType);
float applyLayerToHeight(float baseHeight, float layerValue, int blendMode, float opacity,
                         float heightMinimum, float heightMaximum);
float occlusionAlpha(float thickness, float contrast, float windowLow, float windowHigh, float opacity);

// Density shaping then Photoshop-style Levels — mirrors ReshapeLayerValue in
// NoiseBlend_Shape_PROC.h expression for expression.
float reshapeLayerValue(float rawValue, LayerConfiguration configuration) {
    float shaped = rawValue * configuration.landDensityMultiplier;
    float originalShaped = shaped;
    if (configuration.mountainDensity > 0.0) {
        float mountain = configuration.mountainDensity;
        float smoothed = shaped * shaped * (3.0 - 2.0 * shaped);
        shaped = shaped * (1.0 - mountain) + smoothed * mountain;
        if (shaped > 0.5) shaped += (shaped - 0.5) * mountain;
        shaped = clamp(shaped, 0.0, 1.0);
    }
    if (configuration.plateauDensity > 0.0)
        shaped = floor(shaped * configuration.terraceHeightReciprocal) * configuration.terraceHeight;
    if (configuration.rampDensity > 0.0)
        shaped = shaped * (1.0 - configuration.rampDensity) + originalShaped * configuration.rampDensity;

    if (configuration.levelsRangeReciprocal > 0.0)
        shaped = clamp((shaped - configuration.levelsShadows) * configuration.levelsRangeReciprocal, 0.0, 1.0);
    else
        shaped = shaped >= configuration.levelsShadows ? 1.0 : 0.0;
    if (configuration.levelsMidtones != 1.0 && configuration.levelsMidtones > 0.0)
        shaped = pow(shaped, configuration.levelsMidtones);
    shaped = configuration.levelsOutputBlack
           + shaped * (configuration.levelsOutputWhite - configuration.levelsOutputBlack);
    return clamp(shaped, 0.0, 1.0);
}

void runNoisePass(ivec2 cell, int cellIndex, int cellCount) {
    LayerConfiguration configuration = layerConfigurations[activeLayerIndex];
    float rawValue = 0.0;
    if (configuration.noiseType != NOISE_TYPE_NONE) {
        vec2 point = transformNoisePoint(vec2(float(cell.x), float(cell.y)),
                                         configuration.frequency, configuration.noiseType);
        rawValue = (fractalNoise(configuration.noiseType, configuration.fractalType, configuration.layerSeed,
                                 configuration.octaves, configuration.gain, configuration.lacunarity,
                                 configuration.weightedStrength, configuration.pingPongStrength,
                                 configuration.cellularJitter, configuration.fractalBounding, point)
                     + NOISE_BLEND_RAW_OFFSET) * NOISE_BLEND_RAW_SCALE;
    }
    rawNoiseValues[activeLayerIndex * cellCount + cellIndex] = rawValue;
}

// Per-layer thickness lives in a scratch SSBO, not a per-thread array: a local
// float[maximumLayerCount] spills to local memory and collapses occupancy for EVERY
// invocation (measured ~30% of the blend pass), while the scratch buffer is a coalesced
// write-then-read of the same cells the pass already touches.
void runBlendPass(int cellIndex, int cellCount) {
    float height = layerConfigurations[0].heightMinimum;
    for (int layer = 0; layer < layerCount; ++layer) {
        LayerConfiguration configuration = layerConfigurations[layer];
        float shaped = reshapeLayerValue(rawNoiseValues[layer * cellCount + cellIndex], configuration);
        float blended = applyLayerToHeight(height, shaped, configuration.blendMode, configuration.opacity,
                                           configuration.heightMinimum, configuration.heightMaximum);
        thicknessValues[layer * cellCount + cellIndex] = blended - height;
        height = blended;
    }
    heightValues[cellIndex] = height;

    for (int stratum = 0; stratum < NOISE_BLEND_STRATUM_COUNT; ++stratum)
        proportionValues[stratum * cellCount + cellIndex] = 0.0;
    float remainingVisibility = 1.0;
    for (int layer = layerCount - 1; layer >= 0; --layer) {
        if (remainingVisibility <= 0.0) break;
        float layerThickness = thicknessValues[layer * cellCount + cellIndex];
        if (layerThickness <= 0.0) continue;
        LayerConfiguration configuration = layerConfigurations[layer];
        float alpha = occlusionAlpha(layerThickness, configuration.heightBlendContrast,
                                     configuration.occlusionWindowLow, configuration.occlusionWindowHigh,
                                     configuration.opacity);
        float contribution = min(max(alpha, 0.0), remainingVisibility);
        proportionValues[configuration.stratumIndex * cellCount + cellIndex] += contribution;
        remainingVisibility -= contribution;
    }
    if (remainingVisibility > 0.0)
        proportionValues[layerConfigurations[0].stratumIndex * cellCount + cellIndex] += remainingVisibility;
}

void main() {
    ivec2 cell = ivec2(gl_GlobalInvocationID.xy);
    if (cell.x >= vertexSize || cell.y >= vertexSize) return;
    int cellIndex = cell.y * vertexSize + cell.x;
    int cellCount = vertexSize * vertexSize;
    if (passMode == NOISE_BLEND_PASS_NOISE) runNoisePass(cell, cellIndex, cellCount);
    else                                    runBlendPass(cellIndex, cellCount);
}
