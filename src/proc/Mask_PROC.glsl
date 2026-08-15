#version 430 core
// Mask_PROC.glsl — GPU speed path of the mask stage; twin of Mask_Apply_PROC.cpp.
// Slope-gates every stratum's material proportion, merges the stored art and remaps once,
// writing `surfaceStratumWeights` and the baked `slope` field — one invocation per heightfield
// vertex. The proportion buffer is READONLY: input and output are different fields
// (ARCH §7.2/§3.4). Both outputs are this stage's own (§3.4.1); the slope write is the same
// gradient the gate consumes, so the two backends bake identical values (M5-0c).
// This unit owns the std430 MaskConfiguration block, declared ONCE, mirroring
// Proc::MaskStratumConfiguration field for field (DISPATCH_INTERFACE_SPEC §4). The two
// functions that read a buffer (the slope gradient and the stored-art resample) live here
// because GLSL cannot share a struct or a buffer across compilation units; the pure scalar
// math is in Mask_Slope_PROC.glsl / Mask_Merge_PROC.glsl. Every tile size, stratum count and
// enum value arrives as a #define built from the C++ constants (Constitution §8).
layout(local_size_x = MASK_TILE_WIDTH, local_size_y = MASK_TILE_HEIGHT) in;

struct MaskConfiguration {
    int   mergeMode;          int   storedMaskOffset;   int   storedMaskWidth;   int   storedMaskHeight;
    int   bSmoothstepEnabled; int   bInvertEnabled;     int   paddingFirst;      int   paddingSecond;
    float slopeGradientLow;   float slopeGradientHigh;  float inverseFeatherLow; float inverseFeatherHigh;
    float gateStrength;       float remapMinimum;       float inverseRemapRange; float heightScale;
    float inverseSingleSpan;  float inverseDoubleSpan;  float smoothstepShoulder; float smoothstepScale;
    float maskMinimum;        float maskMaximum;        float storedSampleScaleX; float storedSampleScaleY;
};

layout(std430, binding = 0) readonly  buffer MaskConfigurations   { MaskConfiguration maskConfigurations[]; };
layout(std430, binding = 1) readonly  buffer HeightField          { float heightValues[]; };
layout(std430, binding = 2) readonly  buffer MaterialProportions   { float proportionValues[]; };
layout(std430, binding = 3) readonly  buffer StoredArt             { float storedValues[]; };
layout(std430, binding = 4) writeonly buffer SurfaceStratumWeights { float surfaceWeightValues[]; };
layout(std430, binding = 5) writeonly buffer SlopeField            { float slopeValues[]; };

uniform int vertexSize;

float slopeGateWeight(float slopeGradient, float gradientLow, float gradientHigh, float inverseFeatherLow,
                      float inverseFeatherHigh, int bSmoothstepEnabled, int bInvertEnabled,
                      float gateStrength, float smoothstepShoulder, float smoothstepScale);
float mergeStoredMask(float proceduralWeight, float storedWeight, int mergeMode,
                      float maskMinimum, float maskMaximum);
float remapMaskValue(float value, float remapMinimum, float inverseRemapRange,
                     float maskMinimum, float maskMaximum);

// Finite-difference gradient magnitude — the pinned slope unit (rise/run). Interior cells use
// the central difference, edge cells the one-sided one; mirrors SlopeGradientMagnitude().
float slopeGradientMagnitude(int x, int y, float heightScale, float inverseSingleSpan, float inverseDoubleSpan) {
    int lowX  = max(x - 1, 0);
    int highX = min(x + 1, vertexSize - 1);
    int lowY  = max(y - 1, 0);
    int highY = min(y + 1, vertexSize - 1);
    float inverseSpanX = (highX - lowX) == 2 ? inverseDoubleSpan : inverseSingleSpan;
    float inverseSpanY = (highY - lowY) == 2 ? inverseDoubleSpan : inverseSingleSpan;
    float gradientX = (heightValues[y * vertexSize + highX] - heightValues[y * vertexSize + lowX])
                    * heightScale * inverseSpanX;
    float gradientY = (heightValues[highY * vertexSize + x] - heightValues[lowY * vertexSize + x])
                    * heightScale * inverseSpanY;
    return sqrt(gradientX * gradientX + gradientY * gradientY);
}

// The ONE resampler: bilinear, never nearest (MASKING_SPEC). Mirrors SampleStoredMaskBilinear().
float sampleStoredMaskBilinear(MaskConfiguration configuration, int x, int y) {
    if (configuration.storedMaskWidth <= 0 || configuration.storedMaskHeight <= 0) return 0.0;
    float maximumX = float(configuration.storedMaskWidth - 1);
    float maximumY = float(configuration.storedMaskHeight - 1);
    float sampleX = clamp(float(x) * configuration.storedSampleScaleX, 0.0, maximumX);
    float sampleY = clamp(float(y) * configuration.storedSampleScaleY, 0.0, maximumY);
    int lowX  = int(sampleX);
    int lowY  = int(sampleY);
    int highX = min(lowX + 1, configuration.storedMaskWidth - 1);
    int highY = min(lowY + 1, configuration.storedMaskHeight - 1);
    float fractionX = sampleX - float(lowX);
    float fractionY = sampleY - float(lowY);
    int lowRow  = configuration.storedMaskOffset + lowY * configuration.storedMaskWidth;
    int highRow = configuration.storedMaskOffset + highY * configuration.storedMaskWidth;
    float top    = storedValues[lowRow + lowX]  + (storedValues[lowRow + highX]  - storedValues[lowRow + lowX])  * fractionX;
    float bottom = storedValues[highRow + lowX] + (storedValues[highRow + highX] - storedValues[highRow + lowX]) * fractionX;
    return top + (bottom - top) * fractionY;
}

void main() {
    ivec2 cell = ivec2(gl_GlobalInvocationID.xy);
    if (cell.x >= vertexSize || cell.y >= vertexSize) return;
    int cellIndex = cell.y * vertexSize + cell.x;
    int cellCount = vertexSize * vertexSize;

    MaskConfiguration slopeConfiguration = maskConfigurations[0];
    float slopeGradient = slopeGradientMagnitude(cell.x, cell.y, slopeConfiguration.heightScale,
                                                 slopeConfiguration.inverseSingleSpan,
                                                 slopeConfiguration.inverseDoubleSpan);
    slopeValues[cellIndex] = slopeGradient;          // the baked field, in the pinned unit
    for (int stratum = 0; stratum < MASK_STRATUM_COUNT; ++stratum) {
        MaskConfiguration configuration = maskConfigurations[stratum];
        float gateWeight = slopeGateWeight(slopeGradient, configuration.slopeGradientLow,
                                           configuration.slopeGradientHigh, configuration.inverseFeatherLow,
                                           configuration.inverseFeatherHigh, configuration.bSmoothstepEnabled,
                                           configuration.bInvertEnabled, configuration.gateStrength,
                                           configuration.smoothstepShoulder, configuration.smoothstepScale);
        float proceduralWeight = proportionValues[stratum * cellCount + cellIndex] * gateWeight;
        float storedWeight = configuration.mergeMode == MASK_MERGE_DISABLED
                           ? 0.0 : sampleStoredMaskBilinear(configuration, cell.x, cell.y);
        float mergedWeight = mergeStoredMask(proceduralWeight, storedWeight, configuration.mergeMode,
                                             configuration.maskMinimum, configuration.maskMaximum);
        surfaceWeightValues[stratum * cellCount + cellIndex] =
            remapMaskValue(mergedWeight, configuration.remapMinimum, configuration.inverseRemapRange,
                           configuration.maskMinimum, configuration.maskMaximum);
    }
}
