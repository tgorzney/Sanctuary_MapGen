#version 430 core
// Bake_PROC.glsl — GPU speed path of the bake stage; twin of Bake_PROC.cpp +
// Bake_Composite_PROC.cpp. One dispatch over the output texture: per texel it samples each
// stratum's SURFACE weight (already remapped by the Mask stage — this kernel has no remap of
// its own, ARCH §7.2.5), samples that stratum's tiled albedo (or its flat preview tint),
// accumulates the weighted composite, and writes the packed stratum-mask pair the export
// ships as stratums_1_4 / stratums_5_8.
// This unit owns the std430 StratumConfiguration block, declared ONCE, mirroring
// Proc::StratumKernelConfiguration field for field (DISPATCH_INTERFACE_SPEC §4). Tile size
// and stratum count arrive as #defines built from the C++ constants; every other value
// travels in the configuration records, because the SYS seam exposes int uniforms only
// (Constitution §8 — nothing is hardcoded here).
layout(local_size_x = BAKE_TILE_WIDTH, local_size_y = BAKE_TILE_HEIGHT) in;

// 10 live scalars + 2 padding words = 48 bytes, matching Proc::StratumKernelConfiguration
// exactly (a 16-byte multiple, so the std430 array stride needs no implicit padding).
struct StratumConfiguration {
    int   albedoPixelOffset;  int   albedoWidth;      int   albedoHeight;   int bEnabled;
    float tintRed;            float tintGreen;        float tintBlue;       float tileCount;
    float weightEpsilon;      int   bNormalizeWeights; int  paddingFirst;   int paddingSecond;
};

layout(std430, binding = 0) readonly  buffer StratumConfigurations { StratumConfiguration stratumConfigurations[]; };
layout(std430, binding = 1) readonly  buffer SurfaceWeightValues   { float surfaceWeightValues[]; };
layout(std430, binding = 2) readonly  buffer AlbedoTexels          { uint albedoTexels[]; };
layout(std430, binding = 3) writeonly buffer CompositeAlbedo       { uint compositeAlbedoTexels[]; };
layout(std430, binding = 4) writeonly buffer StratumMaskLow        { uint stratumMaskLowTexels[]; };
layout(std430, binding = 5) writeonly buffer StratumMaskHigh       { uint stratumMaskHighTexels[]; };

uniform int vertexSize;          // weight grid side (mapSize + 1)
uniform int outputResolution;    // baked texture side
uniform int baseStratumIndex;    // the always-present base stratum
uniform int compositeAlphaByte;  // constants.compositeAlphaValue, pre-quantized on the Cpu

uint packByte(float value) { return uint(clamp(value, 0.0, 1.0) * 255.0 + 0.5); }

uint packRgba8(vec4 color) {
    return packByte(color.r) | (packByte(color.g) << 8) | (packByte(color.b) << 16) | (packByte(color.a) << 24);
}

// `packedTexel`, not `packed`: the latter is a reserved GLSL keyword.
float unpackChannel(uint packedTexel, int channel) {
    return float((packedTexel >> uint(channel * 8)) & 0xFFu) * (1.0 / 255.0);
}

int wrapTexelIndex(int value, int size) { int wrapped = value % size; return wrapped < 0 ? wrapped + size : wrapped; }

// Twin of Data::FloatField::SampleBilinear — clamped to the grid, same lerp form.
float sampleSurfaceWeightBilinear(int stratum, float sampleX, float sampleY) {
    float maximumCoordinate = float(vertexSize - 1);
    sampleX = clamp(sampleX, 0.0, maximumCoordinate);
    sampleY = clamp(sampleY, 0.0, maximumCoordinate);
    int lowX = int(sampleX);   int lowY = int(sampleY);
    int highX = min(lowX + 1, vertexSize - 1);
    int highY = min(lowY + 1, vertexSize - 1);
    float fractionX = sampleX - float(lowX);
    float fractionY = sampleY - float(lowY);
    int fieldBase = stratum * vertexSize * vertexSize;
    float top    = surfaceWeightValues[fieldBase + lowY * vertexSize + lowX] * (1.0 - fractionX)
                 + surfaceWeightValues[fieldBase + lowY * vertexSize + highX] * fractionX;
    float bottom = surfaceWeightValues[fieldBase + highY * vertexSize + lowX] * (1.0 - fractionX)
                 + surfaceWeightValues[fieldBase + highY * vertexSize + highX] * fractionX;
    return top * (1.0 - fractionY) + bottom * fractionY;
}

// Twin of SampleStratumColor in Bake_Sampling_PROC.h — tiled, wrapped, bilinear, tinted.
vec3 sampleStratumColor(StratumConfiguration configuration, float mapU, float mapV) {
    vec3 tint = vec3(configuration.tintRed, configuration.tintGreen, configuration.tintBlue);
    if (configuration.albedoWidth <= 0 || configuration.albedoHeight <= 0) return tint;
    float texelX = mapU * configuration.tileCount * float(configuration.albedoWidth) - 0.5;
    float texelY = mapV * configuration.tileCount * float(configuration.albedoHeight) - 0.5;
    int lowX = int(floor(texelX));
    int lowY = int(floor(texelY));
    float fractionX = texelX - float(lowX);
    float fractionY = texelY - float(lowY);
    int columnLow  = wrapTexelIndex(lowX, configuration.albedoWidth);
    int columnHigh = wrapTexelIndex(lowX + 1, configuration.albedoWidth);
    int rowLow     = wrapTexelIndex(lowY, configuration.albedoHeight);
    int rowHigh    = wrapTexelIndex(lowY + 1, configuration.albedoHeight);
    int offset = configuration.albedoPixelOffset;
    uint corners[4] = uint[4](albedoTexels[offset + rowLow  * configuration.albedoWidth + columnLow],
                              albedoTexels[offset + rowLow  * configuration.albedoWidth + columnHigh],
                              albedoTexels[offset + rowHigh * configuration.albedoWidth + columnLow],
                              albedoTexels[offset + rowHigh * configuration.albedoWidth + columnHigh]);
    vec3 sampled = vec3(0.0);
    for (int channel = 0; channel < 3; ++channel) {
        float top    = unpackChannel(corners[0], channel)
                     + (unpackChannel(corners[1], channel) - unpackChannel(corners[0], channel)) * fractionX;
        float bottom = unpackChannel(corners[2], channel)
                     + (unpackChannel(corners[3], channel) - unpackChannel(corners[2], channel)) * fractionX;
        sampled[channel] = top + (bottom - top) * fractionY;
    }
    return tint * sampled;
}

// Twin of CompositeTexel in Bake_Composite_PROC.cpp.
vec3 compositeTexel(float mapU, float mapV, float weightScale, out float weights[BAKE_STRATUM_COUNT]) {
    vec3 accumulated = vec3(0.0);
    float weightTotal = 0.0;
    for (int stratum = 0; stratum < BAKE_STRATUM_COUNT; ++stratum) {
        StratumConfiguration configuration = stratumConfigurations[stratum];
        float weight = configuration.bEnabled != 0
                     ? sampleSurfaceWeightBilinear(stratum, mapU * weightScale, mapV * weightScale) : 0.0;
        weights[stratum] = weight;
        if (weight > 0.0) accumulated += weight * sampleStratumColor(configuration, mapU, mapV);
        weightTotal += weight;
    }
    if (weightTotal <= stratumConfigurations[0].weightEpsilon)
        return sampleStratumColor(stratumConfigurations[baseStratumIndex], mapU, mapV);
    if (stratumConfigurations[0].bNormalizeWeights != 0) accumulated *= 1.0 / weightTotal;
    return accumulated;
}

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= outputResolution || pixel.y >= outputResolution) return;
    float resolutionReciprocal = 1.0 / float(outputResolution);
    float mapU = (float(pixel.x) + 0.5) * resolutionReciprocal;
    float mapV = (float(pixel.y) + 0.5) * resolutionReciprocal;
    float weights[BAKE_STRATUM_COUNT];
    vec3 color = compositeTexel(mapU, mapV, float(vertexSize - 1), weights);
    int texelIndex = pixel.y * outputResolution + pixel.x;
    compositeAlbedoTexels[texelIndex] = packByte(color.r) | (packByte(color.g) << 8)
                                      | (packByte(color.b) << 16) | (uint(compositeAlphaByte) << 24);
    stratumMaskLowTexels[texelIndex]  = packRgba8(vec4(weights[1], weights[2], weights[3], weights[4]));
    stratumMaskHighTexels[texelIndex] = packRgba8(vec4(weights[5], weights[6], weights[7], weights[8]));
}
