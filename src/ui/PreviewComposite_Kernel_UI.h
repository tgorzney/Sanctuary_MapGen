// PreviewComposite_Kernel_UI.h — the one composite kernel contract, shared by the Cpu twin
// and PreviewComposite_UI.glsl. Layer: UI.
// Declares (a) the pass ordering, (b) the buffer binding indices, and (c) the flattened
// records whose field order/type IS the std430 layout the GLSL twin mirrors EXACTLY
// (DISPATCH_INTERFACE_SPEC §4). Every record is a whole 16-byte multiple of plain 4-byte
// scalars, so the std430 array stride needs no implicit padding.
// The stage-wide floats travel inside these records because the SYS seam exposes integer
// uniforms only; nothing here is a hardcoded constant (Constitution §8) — every value is
// flattened out of PreviewCompositeSettings / Params.
#pragma once

namespace SanmapGen {
namespace Ui {

// Pass ordering (the work-order's contract, and the legacy permutation stack's shape):
// clear -> one pass per enabled field layer -> overlay -> entity id.
namespace CompositePass {
constexpr int kClear            = 0;   // per pixel: clear color + emptySentinel id
constexpr int kFieldLayer       = 1;   // per pixel: colorize + blend one BAKED field
constexpr int kOverlay          = 2;   // per resolved instance: draw its mark
constexpr int kEntityIdentifier = 3;   // per resolved instance: write its id under the mark
constexpr int kPassKindCount    = 4;
} // namespace CompositePass

// Buffer binding indices. Binding 0 stays the entity-id buffer, as in the legacy layout.
// Index 7 is deliberately vacant: it was the packed-uint image SSBO before the composite began
// writing a real GL texture (M5-5), and leaving the hole keeps every other index stable.
namespace CompositeBinding {
constexpr unsigned kEntityIdentifiers      = 0;
constexpr unsigned kHeightfield            = 1;
constexpr unsigned kFlow                   = 2;
constexpr unsigned kAccumulation           = 3;
constexpr unsigned kSurfaceStratumWeights  = 4;   // the 9 baked weight fields, concatenated
constexpr unsigned kGradientLookupTables   = 5;   // every baked ramp table, concatenated
constexpr unsigned kEntityPoints           = 6;
constexpr unsigned kConfiguration          = 8;
constexpr unsigned kLayerConfigurations    = 9;
constexpr unsigned kStratumConfigurations  = 10;
constexpr unsigned kSlope                  = 11;  // the Mask stage's baked slope (M5-0c)
constexpr unsigned kMapAreaRectangles      = 12;  // ARCH §14.17 — binding 7 stays vacant (see above)
} // namespace CompositeBinding

// IMAGE units are their own binding namespace in GL — an image unit never collides with an SSBO
// binding of the same number — so the composited RGBA8 image starts its own numbering at zero
// instead of borrowing the retired SSBO index. It is the surface the canvas samples (M5-5), so
// it is a real GL_RGBA8 texture, not a packed-uint buffer.
namespace CompositeImageUnit {
constexpr unsigned kCompositeImage = 0;
} // namespace CompositeImageUnit

// The names the persistent SSBOs are keyed by in GpuResourceManager. They are part of the
// kernel contract (the manager reallocates only when a named buffer's byte size changes), so
// they live beside the bindings rather than as loose literals in the dispatch file.
namespace CompositeBufferName {
constexpr const char* kEntityIdentifiers     = "previewCompositeEntityIdentifiers";
constexpr const char* kHeightfield           = "previewCompositeHeightfield";
constexpr const char* kFlow                  = "previewCompositeFlow";
constexpr const char* kAccumulation          = "previewCompositeAccumulation";
constexpr const char* kSurfaceStratumWeights = "previewCompositeSurfaceStratumWeights";
constexpr const char* kGradientLookupTables  = "previewCompositeGradientLookupTables";
constexpr const char* kEntityPoints          = "previewCompositeEntityPoints";
constexpr const char* kConfiguration         = "previewCompositeConfiguration";
constexpr const char* kLayerConfigurations   = "previewCompositeLayerConfigurations";
constexpr const char* kStratumConfigurations = "previewCompositeStratumConfigurations";
constexpr const char* kSlope                 = "previewCompositeSlope";
constexpr const char* kMapAreaRectangles     = "previewCompositeMapAreaRectangles";
} // namespace CompositeBufferName

// The name the composited image texture is keyed by in GpuResourceManager — same lifecycle rule
// as the buffers above: the manager reallocates it only when the preview resolution changes.
namespace CompositeTextureName {
constexpr const char* kCompositeImage = "previewCompositeImage";
} // namespace CompositeTextureName

// Every `*RangeReciprocal` field below is precomputed with this, so the kernels multiply and
// never divide inside a per-pixel loop (Constitution §3). A degenerate span answers zero,
// which pins the whole domain to the low end instead of producing an infinity.
inline float ReciprocalOrZero(float span) { return span > 0.0f ? 1.0f / span : 0.0f; }

// One flattened layer. 8 scalars = 32 bytes. `layerKind`/`blendMode` are the enum values as
// ints (the GLSL twin compares against the same numbering).
struct PreviewLayerConfiguration {
    int   layerKind                 = 0;
    int   blendMode                 = 0;
    int   gradientLookupOffset      = -1;   // first float of this layer's table; below zero = none
    int   gradientLookupEntryCount  = 0;
    float opacity                   = 1.0f;
    float domainMinimum             = 0.0f;
    float domainRangeReciprocal     = 0.0f; // precomputed: multiply, never divide in the loop
    float paddingFirst              = 0.0f;
};

// One flattened stratum: its preview tint (Params::Stratum tint = the legacy previewColor)
// and whether it contributes. 4 scalars = 16 bytes. The composite keeps no private
// per-stratum settings array — these are flattened from `Params::Stratum` (ARCH §7.1).
struct PreviewStratumConfiguration {
    float previewColorRed   = 1.0f;
    float previewColorGreen = 1.0f;
    float previewColorBlue  = 1.0f;
    int   bEnabled          = 1;
};

// One map area, flattened to the composite's own CELL space and its presentation color.
// 8 scalars = 32 bytes. Areas are PRESENTATION geometry: no placement rule stands behind them,
// so this record re-decides nothing a PROC stage resolved (ARCH §14.17 item 1).
struct PreviewMapAreaRectangle {
    float minimumX = 0.0f;
    float minimumZ = 0.0f;
    float maximumX = 0.0f;
    float maximumZ = 0.0f;
    float colorRed = 0.0f;
    float colorGreen = 0.0f;
    float colorBlue = 0.0f;
    float colorAlpha = 0.0f;
};

// One resolved entity, already mapped to preview-image pixel coordinates on the Cpu.
// `entityIdentifier` is the instance's index in `Data::PlacementInstances`, which is exactly
// what `Picking_UI` (M4-4) resolves a click to. 4 scalars = 16 bytes.
struct PreviewEntityPoint {
    float        pixelX           = 0.0f;
    float        pixelY           = 0.0f;
    unsigned int entityIdentifier = 0u;
    int          paddingFirst     = 0;
};

// The stage-wide record. 20 scalars = 80 bytes.
struct PreviewCompositeConfiguration {
    int   previewResolution             = 0;
    int   vertexSize                    = 0;     // baked field grid side (mapSize + 1)
    int   layerCount                    = 0;
    int   entityCount                   = 0;
    float splatWeightEpsilon            = 0.0001f;
    int   bNormalizeSplatWeights        = 1;
    int   bWaterEnabled                 = 0;
    float waterLevelMaximum             = 0.0f;  // game units
    float terrainMaxHeight              = 1.0f;  // normalized field height -> game units
    float deepWaterDepthMinimum         = 0.0f;
    float deepWaterDepthRangeReciprocal = 0.0f;
    float entityMarkRadiusPixels        = 0.0f;
    float clearColorRed                 = 0.0f;
    float clearColorGreen               = 0.0f;
    float clearColorBlue                = 0.0f;
    float clearColorAlpha               = 1.0f;
    float entityMarkColorRed            = 1.0f;
    float entityMarkColorGreen          = 1.0f;
    float entityMarkColorBlue           = 1.0f;
    float entityMarkColorAlpha          = 1.0f;
};

} // namespace Ui
} // namespace SanmapGen
