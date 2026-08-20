#version 430 core
// PreviewComposite_UI.glsl — GPU twin of the Cpu composite (PreviewComposite_Cpu_UI.cpp), and
// the unit that declares main(). One program, four passes over the same RGBA8 image TEXTURE,
// selected by `passIndex`: clear -> one dispatch per enabled field layer -> overlay -> entity id.
// It SAMPLES the baked fields and colorizes them (PreviewComposite_Sampling_UI.glsl) — slope
// included, read from the Mask stage's bake; there is deliberately NO slope derivation, NO
// marker/prop rule test and NO sim step anywhere in this program. Those existed here in v1 and are exactly the "preview truth != bake truth" defect M4
// deletes (ARCH §3.2, §5.4; PREVIEW_COMPOSITING_SPEC).
// Tile size, pass ids, blend modes, binding indices and the empty-entity sentinel all arrive as
// #defines built from the C++ constants — nothing is hardcoded here (Constitution §8).
layout(local_size_x = PREVIEW_TILE_WIDTH, local_size_y = PREVIEW_TILE_HEIGHT) in;

// Mirrors Ui::PreviewCompositeConfiguration field for field (DISPATCH_INTERFACE_SPEC §4). GLSL
// has no #include, so this block is repeated in PreviewComposite_Sampling_UI.glsl; the two
// declarations must stay identical.
struct CompositeConfiguration {
    int   previewResolution;    int   vertexSize;             int   layerCount;      int entityCount;
    float splatWeightEpsilon;   int   bNormalizeSplatWeights; int   bWaterEnabled;
    float waterLevelMaximum;    float terrainMaxHeight;       float deepWaterDepthMinimum;
    float deepWaterDepthRangeReciprocal;                      float entityMarkRadiusPixels;
    float clearColorRed;        float clearColorGreen;        float clearColorBlue;  float clearColorAlpha;
    float entityMarkColorRed;   float entityMarkColorGreen;   float entityMarkColorBlue;
    float entityMarkColorAlpha;
};
struct EntityPoint { float pixelX; float pixelY; uint entityIdentifier; int paddingFirst; };

layout(std430, binding = PREVIEW_BINDING_ENTITY_IDENTIFIERS)        buffer EntityIdentifiers { uint entityIdentifiers[]; };
layout(std430, binding = PREVIEW_BINDING_ENTITY_POINTS)    readonly buffer EntityPoints      { EntityPoint entityPoints[]; };
layout(std430, binding = PREVIEW_BINDING_CONFIGURATION)    readonly buffer Configuration     { CompositeConfiguration configuration[]; };

// The composited image is a real GL_RGBA8 texture bound to an IMAGE UNIT (its own binding
// namespace, unrelated to the SSBO numbers above), so the canvas samples this surface directly
// instead of an uploaded copy. rgba8 quantizes each store to a byte exactly as the Cpu twin's
// PackRgba8 does, so the passes still round-trip through 8 bits between blends. The passes
// read-modify-write it, so it carries no readonly/writeonly qualifier.
layout(rgba8, binding = PREVIEW_IMAGE_COMPOSITE) uniform image2D compositeImage;

uniform int passIndex;
uniform int layerIndex;

// Provided by PreviewComposite_Color_UI.glsl and PreviewComposite_Sampling_UI.glsl.
vec4  blendPreviewColor(vec4 destination, vec4 source, int blendMode, float amount);
vec4  layerColorAtPixel(int layerIndex, float sampleX, float sampleY);
int   layerBlendMode(int layerIndex);
float layerOpacity(int layerIndex);

void clearPass(ivec2 pixel) {
    imageStore(compositeImage, pixel, vec4(configuration[0].clearColorRed,
                                           configuration[0].clearColorGreen,
                                           configuration[0].clearColorBlue,
                                           configuration[0].clearColorAlpha));
    entityIdentifiers[pixel.y * configuration[0].previewResolution + pixel.x] =
        PREVIEW_EMPTY_ENTITY_SENTINEL;
}

// The pixel -> cell mapping is the same pixel-center form the bake uses, so the preview and the
// bake sample the same place.
void fieldLayerPass(ivec2 pixel) {
    int resolution = configuration[0].previewResolution;
    float cellsPerPixel = float(configuration[0].vertexSize - 1) / float(resolution);
    vec4 layerColor = layerColorAtPixel(layerIndex, (float(pixel.x) + 0.5) * cellsPerPixel,
                                                    (float(pixel.y) + 0.5) * cellsPerPixel);
    imageStore(compositeImage, pixel,
               blendPreviewColor(imageLoad(compositeImage, pixel), layerColor,
                                 layerBlendMode(layerIndex), layerOpacity(layerIndex) * layerColor.a));
}

// One thread per RESOLVED instance. The mark is DRAWN, never re-tested against a placement rule
// (the legacy shader shipped raw rule bounds and decided placement itself — deleted): a marker
// the bake rejected is not in this buffer. The id written is the instance's index in
// Data::PlacementInstances, which is what Picking_UI (M4-4) resolves a click to.
// Overlapping marks resolve last-writer-wins, unordered on the Gpu; legal for the Visual class.
void entityPass(int entityIndex, bool bWriteIdentifier) {
    EntityPoint point = entityPoints[entityIndex];
    int resolution = configuration[0].previewResolution;
    float radiusPixels = max(configuration[0].entityMarkRadiusPixels, 0.0);
    float radiusSquared = radiusPixels * radiusPixels;
    vec4 markColor = vec4(configuration[0].entityMarkColorRed, configuration[0].entityMarkColorGreen,
                          configuration[0].entityMarkColorBlue, configuration[0].entityMarkColorAlpha);
    int highX = int(point.pixelX + radiusPixels) + 1;
    int highY = int(point.pixelY + radiusPixels) + 1;
    for (int pixelY = max(int(point.pixelY - radiusPixels), 0); pixelY <= highY && pixelY < resolution; ++pixelY) {
        for (int pixelX = max(int(point.pixelX - radiusPixels), 0); pixelX <= highX && pixelX < resolution; ++pixelX) {
            float offsetX = float(pixelX) - point.pixelX;
            float offsetY = float(pixelY) - point.pixelY;
            if (offsetX * offsetX + offsetY * offsetY > radiusSquared) continue;
            if (bWriteIdentifier) {
                entityIdentifiers[pixelY * resolution + pixelX] = point.entityIdentifier;
                continue;
            }
            ivec2 markPixel = ivec2(pixelX, pixelY);
            imageStore(compositeImage, markPixel,
                       blendPreviewColor(imageLoad(compositeImage, markPixel), markColor,
                                         PREVIEW_BLEND_ALPHA, markColor.a));
        }
    }
}

void main() {
    if (passIndex == PREVIEW_PASS_CLEAR || passIndex == PREVIEW_PASS_FIELD_LAYER) {
        ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
        if (pixel.x >= configuration[0].previewResolution
            || pixel.y >= configuration[0].previewResolution) return;
        if (passIndex == PREVIEW_PASS_CLEAR) clearPass(pixel);
        else                                 fieldLayerPass(pixel);
        return;
    }
    // The entity passes are dispatched one-dimensionally, so the tile is linearized.
    uint entityIndex = gl_GlobalInvocationID.y * (gl_NumWorkGroups.x * uint(PREVIEW_TILE_WIDTH))
                     + gl_GlobalInvocationID.x;
    if (entityIndex >= uint(configuration[0].entityCount)) return;
    entityPass(int(entityIndex), passIndex == PREVIEW_PASS_ENTITY_IDENTIFIER);
}
