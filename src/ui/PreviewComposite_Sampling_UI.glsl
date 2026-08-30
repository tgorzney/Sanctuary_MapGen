#version 430 core
// PreviewComposite_Sampling_UI.glsl — reading the BAKED fields and turning one of them into a
// color: bilinear field sampling, the ramp-table lookup, and the surface-weight splat.
// GPU twin of the sampling half of PreviewComposite_Cpu_UI.cpp / PreviewComposite_Color_UI.h.
// Every value here is a sample of a field some PROC stage already wrote — slope included, from
// the Mask stage's bake (M5-0c). Nothing is derived from a neighbourhood (no gradient is
// computed here), nothing is re-filtered, nothing is re-run — the WYSIWYG rule (ARCH §3.2).
// One compilation unit of the PreviewComposite program (linked, never #included); the unit that
// declares main() is PreviewComposite_UI.glsl. This unit owns the layer + stratum + field
// buffers, so they are declared exactly once; the pass unit reaches them through the accessors
// at the bottom. The one block it must repeat is `Configuration`, which both units read — GLSL
// has no #include, so that declaration is duplicated deliberately and must stay identical.
layout(local_size_x = PREVIEW_TILE_WIDTH, local_size_y = PREVIEW_TILE_HEIGHT) in;

struct CompositeConfiguration {
    int   previewResolution;    int   vertexSize;             int   layerCount;      int entityCount;
    float splatWeightEpsilon;   int   bNormalizeSplatWeights; int   bWaterEnabled;
    float waterLevelMaximum;    float terrainMaxHeight;       float deepWaterDepthMinimum;
    float deepWaterDepthRangeReciprocal;                      float entityMarkRadiusPixels;
    float clearColorRed;        float clearColorGreen;        float clearColorBlue;  float clearColorAlpha;
    float entityMarkColorRed;   float entityMarkColorGreen;   float entityMarkColorBlue;
    float entityMarkColorAlpha;
};
struct LayerConfiguration {
    int   layerKind;   int   blendMode;      int   gradientLookupOffset;  int gradientLookupEntryCount;
    float opacity;     float domainMinimum;  float domainRangeReciprocal; float paddingFirst;
};
struct StratumConfiguration {
    float previewColorRed; float previewColorGreen; float previewColorBlue; int bEnabled;
};

layout(std430, binding = PREVIEW_BINDING_HEIGHTFIELD)    readonly buffer Heightfield   { float heightValues[]; };
layout(std430, binding = PREVIEW_BINDING_FLOW)           readonly buffer FlowField     { float flowValues[]; };
layout(std430, binding = PREVIEW_BINDING_ACCUMULATION)   readonly buffer AccumulationField { float accumulationValues[]; };
layout(std430, binding = PREVIEW_BINDING_SLOPE)          readonly buffer SlopeField    { float slopeValues[]; };
layout(std430, binding = PREVIEW_BINDING_SURFACE_WEIGHTS) readonly buffer SurfaceWeights { float surfaceWeightValues[]; };
layout(std430, binding = PREVIEW_BINDING_GRADIENT_TABLES) readonly buffer GradientTables { float gradientLookupValues[]; };
layout(std430, binding = PREVIEW_BINDING_CONFIGURATION)  readonly buffer Configuration  { CompositeConfiguration configuration[]; };
layout(std430, binding = PREVIEW_BINDING_LAYERS)  readonly buffer LayerConfigurations   { LayerConfiguration layerConfigurations[]; };
layout(std430, binding = PREVIEW_BINDING_STRATA)  readonly buffer StratumConfigurations { StratumConfiguration stratumConfigurations[]; };

// ARCH §14.17 item 4 — one map area, cell-space bounds + resolved color. Declared in THIS unit only
// (the pass unit reaches it only through layerColorAtPixel, this file's own stated convention).
struct MapAreaRectangle {
    float minimumX;  float minimumZ;  float maximumX;  float maximumZ;
    float colorRed;  float colorGreen; float colorBlue; float colorAlpha;
};
layout(std430, binding = PREVIEW_BINDING_MAP_AREAS) readonly buffer MapAreaRectangles { MapAreaRectangle mapAreaRectangles[]; };

// Provided by PreviewComposite_Color_UI.glsl.
float clampUnit(float value);
float normalizeToDomain(float value, float domainMinimum, float domainRangeReciprocal);
float normalizedWaterDepth(float normalizedHeight, float terrainMaxHeight, float waterLevelMaximum,
                           float deepWaterDepthMinimum, float deepWaterDepthRangeReciprocal);

// Twin of Data::FloatField::SampleBilinear — clamped to the grid, same truncation, same lerp.
ivec4 bilinearCorners(inout float sampleX, inout float sampleY) {
    int vertexSize = configuration[0].vertexSize;
    float maximumCoordinate = float(vertexSize - 1);
    sampleX = clamp(sampleX, 0.0, maximumCoordinate);
    sampleY = clamp(sampleY, 0.0, maximumCoordinate);
    return ivec4(int(sampleX), int(sampleY), min(int(sampleX) + 1, vertexSize - 1),
                 min(int(sampleY) + 1, vertexSize - 1));
}

float bilinearBlend(float topLeft, float topRight, float bottomLeft, float bottomRight,
                    float fractionX, float fractionY) {
    float top    = topLeft + (topRight - topLeft) * fractionX;
    float bottom = bottomLeft + (bottomRight - bottomLeft) * fractionX;
    return top + (bottom - top) * fractionY;
}

float fieldValueAt(int layerKind, int valueIndex) {
    if (layerKind == PREVIEW_LAYER_FLOW)         return flowValues[valueIndex];
    if (layerKind == PREVIEW_LAYER_ACCUMULATION) return accumulationValues[valueIndex];
    if (layerKind == PREVIEW_LAYER_SLOPE)        return slopeValues[valueIndex];   // BAKED by Mask
    return heightValues[valueIndex];                       // the height ramp and the water depth
}

float sampleFieldBilinear(int layerKind, float sampleX, float sampleY) {
    ivec4 corners = bilinearCorners(sampleX, sampleY);
    int rowStride = configuration[0].vertexSize;
    return bilinearBlend(fieldValueAt(layerKind, corners.y * rowStride + corners.x),
                         fieldValueAt(layerKind, corners.y * rowStride + corners.z),
                         fieldValueAt(layerKind, corners.w * rowStride + corners.x),
                         fieldValueAt(layerKind, corners.w * rowStride + corners.z),
                         sampleX - float(corners.x), sampleY - float(corners.y));
}

float sampleSurfaceWeightBilinear(int stratum, float sampleX, float sampleY) {
    ivec4 corners = bilinearCorners(sampleX, sampleY);
    int rowStride = configuration[0].vertexSize;
    int fieldBase = stratum * rowStride * rowStride;
    return bilinearBlend(surfaceWeightValues[fieldBase + corners.y * rowStride + corners.x],
                         surfaceWeightValues[fieldBase + corners.y * rowStride + corners.z],
                         surfaceWeightValues[fieldBase + corners.w * rowStride + corners.x],
                         surfaceWeightValues[fieldBase + corners.w * rowStride + corners.z],
                         sampleX - float(corners.x), sampleY - float(corners.y));
}

// Twin of Ui::SampleGradientLookupTable — the M4-2 bake is endpoint-INCLUSIVE, so the sample
// position is normalized * (entryCount - 1), with no texel-center offset.
vec4 sampleGradientLookupTable(int tableOffset, int entryCount, float normalizedPosition) {
    if (tableOffset < 0 || entryCount <= 0) return vec4(0.0);
    float position = clampUnit(normalizedPosition) * float(entryCount - 1);
    int lowEntry = min(int(position), entryCount - 1);
    int highEntry = lowEntry < entryCount - 1 ? lowEntry + 1 : lowEntry;
    int lowBase  = tableOffset + PREVIEW_LOOKUP_CHANNEL_COUNT * lowEntry;
    int highBase = tableOffset + PREVIEW_LOOKUP_CHANNEL_COUNT * highEntry;
    vec4 low  = vec4(gradientLookupValues[lowBase],      gradientLookupValues[lowBase + 1],
                     gradientLookupValues[lowBase + 2],  gradientLookupValues[lowBase + 3]);
    vec4 high = vec4(gradientLookupValues[highBase],     gradientLookupValues[highBase + 1],
                     gradientLookupValues[highBase + 2], gradientLookupValues[highBase + 3]);
    return low + (high - low) * (position - float(lowEntry));
}

// The BAKED surface weights times each stratum's preview tint, consumed VERBATIM: the one remap
// already happened in the Mask stage (ARCH §7.2.5). At or below the epsilon the splat paints
// nothing and the layers underneath show through.
vec4 splatSurfaceStrata(float sampleX, float sampleY) {
    vec4 splat = vec4(0.0);
    float weightTotal = 0.0;
    for (int stratum = 0; stratum < PREVIEW_STRATUM_COUNT; ++stratum) {
        if (stratumConfigurations[stratum].bEnabled == 0) continue;
        float weight = sampleSurfaceWeightBilinear(stratum, sampleX, sampleY);
        if (!(weight > 0.0)) continue;
        splat.r += weight * stratumConfigurations[stratum].previewColorRed;
        splat.g += weight * stratumConfigurations[stratum].previewColorGreen;
        splat.b += weight * stratumConfigurations[stratum].previewColorBlue;
        weightTotal += weight;
    }
    if (weightTotal <= configuration[0].splatWeightEpsilon) return vec4(0.0);
    if (configuration[0].bNormalizeSplatWeights != 0) splat.rgb *= 1.0 / weightTotal;
    splat.a = clampUnit(weightTotal);
    return splat;
}

// ARCH §14.19 — forward iteration, FIRST containing match wins, early return: ascending array
// index is now Z-descending (index 0 = top). Must stay textually parallel with the CPU twin
// (PreviewComposite_Cpu_UI.cpp's LayerColorAtPixel MapAreas branch) for byte-identical parity.
vec4 mapAreaColorAtCell(float sampleX, float sampleY) {
    for (int index = 0; index < mapAreaRectangles.length(); ++index) {
        MapAreaRectangle area = mapAreaRectangles[index];
        if (sampleX < area.minimumX || sampleX > area.maximumX) continue;
        if (sampleY < area.minimumZ || sampleY > area.maximumZ) continue;
        return vec4(area.colorRed, area.colorGreen, area.colorBlue, area.colorAlpha);
    }
    return vec4(0.0);
}

// The pass unit reaches the layer records only through these, so the buffer is declared once.
int   layerBlendMode(int layerIndex) { return layerConfigurations[layerIndex].blendMode; }
float layerOpacity(int layerIndex)   { return layerConfigurations[layerIndex].opacity; }

vec4 layerColorAtPixel(int layerIndex, float sampleX, float sampleY) {
    LayerConfiguration layer = layerConfigurations[layerIndex];
    if (layer.layerKind == PREVIEW_LAYER_STRATUM_SPLAT) return splatSurfaceStrata(sampleX, sampleY);
    if (layer.layerKind == PREVIEW_LAYER_MAP_AREAS) return mapAreaColorAtCell(sampleX, sampleY);
    if (layer.layerKind == PREVIEW_LAYER_WATER) {
        if (configuration[0].bWaterEnabled == 0) return vec4(0.0);
        float depth = normalizedWaterDepth(sampleFieldBilinear(layer.layerKind, sampleX, sampleY),
                                           configuration[0].terrainMaxHeight,
                                           configuration[0].waterLevelMaximum,
                                           configuration[0].deepWaterDepthMinimum,
                                           configuration[0].deepWaterDepthRangeReciprocal);
        if (depth < 0.0) return vec4(0.0);              // the baked surface is above the water
        return sampleGradientLookupTable(layer.gradientLookupOffset, layer.gradientLookupEntryCount, depth);
    }
    return sampleGradientLookupTable(layer.gradientLookupOffset, layer.gradientLookupEntryCount,
                                     normalizeToDomain(sampleFieldBilinear(layer.layerKind, sampleX, sampleY),
                                                       layer.domainMinimum, layer.domainRangeReciprocal));
}
