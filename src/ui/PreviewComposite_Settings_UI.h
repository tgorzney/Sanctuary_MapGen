// PreviewComposite_Settings_UI.h — the composite's adjustable presentation settings.
// Layer: UI. Constitution §8: every value the composite uses is reachable here — the preview
// resolution/quality (scrub fast, escalate on idle: ARCH §4.4), the clear color, each layer's
// blend + opacity + domain mapping, the splat normalization, and the entity mark. Nothing the
// kernels use is hardcoded in code or shader.
// These are PRESENTATION settings: they do not serialize into `mapGeneratorData`, so they are
// not part of the map recipe and do not live in `_PARAMS`. The color ramps themselves ARE
// settings (`Params::GradientRamp`, ARCH §8.2) and are referenced by index.
#pragma once
#include <vector>
#include "../params/GradientRamp_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// Which BAKED field a layer colorizes. Every entry names a field `Data::MapFields` actually
// carries. There is deliberately NO slope entry: no slope field is baked, and deriving one
// here would be exactly the shadow-sim this milestone deletes (ARCH §3.2,
// PREVIEW_COMPOSITING_SPEC "the shadow-sim problem"). A slope layer needs a baked slope
// field first — a DATA work-order, not a shader.
enum class PreviewLayerKind { HeightRamp, StratumSplat, Flow, Accumulation, Water };

// The preview-only Z-order blend (PREVIEW_COMPOSITING_SPEC). Distinct from the geometry
// `Params::HeightBlendMode`, which blends terrain height, not pixels.
enum class PreviewBlendMode { Replace, AlphaBlend, Add, Multiply, Maximum, Minimum };

// One composited layer. Layers are applied in vector order (UI order = Z order).
struct PreviewFieldLayer {
    PreviewLayerKind kind      = PreviewLayerKind::HeightRamp;
    PreviewBlendMode blendMode = PreviewBlendMode::AlphaBlend;
    bool  bEnabled = true;
    float opacity  = 1.0f;

    // Mapping this field's own domain onto the ramp's normalized 0..1 is the CONSUMER's job
    // (ARCH §8.2) — height in normalized field units, flow/accumulation in their own ranges,
    // water in game-unit depth.
    float domainMinimum = 0.0f;
    float domainMaximum = 1.0f;

    // Take the domain from the BAKED field's own minimum/maximum instead (the legacy
    // AutoLevelPreview — a CPU scan of the baked field, never a recomputation of it).
    bool bAutoDomainFromField = false;

    // Index into PreviewCompositeSettings::gradientRamps. Below zero = no ramp, and the layer
    // contributes nothing (StratumSplat needs no ramp: it uses the per-stratum preview tint).
    int gradientRampIndex = -1;
};

// Validation fences on the requested resolution (Constitution §6) — they bound nonsense
// input, they are not the setting itself.
enum : int { kMinimumPreviewResolution = 1, kMaximumPreviewResolution = 8192 };

struct PreviewCompositeSettings {
    // Preview quality: the fast scrub value during interaction; PIPELINE raises it on idle
    // (ARCH §4.4). The legacy preview had no such control and always ran full resolution.
    int previewResolution = 512;

    // Cleared image color (linear RGBA). The composite image is opaque: layer blending
    // touches color only, so this alpha survives to the readback.
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    std::vector<PreviewFieldLayer> fieldLayers;

    // One ramp per colorized field — the legacy "accumulation reuses the flow gradient"
    // aliasing is retired (ARCH §8.2).
    std::vector<Params::GradientRamp> gradientRamps;

    // --- Stratum splat (surfaceStratumWeights × the stratum preview tint).
    bool  bNormalizeSplatWeights = true;     // weighted average, else straight accumulation
    float splatWeightEpsilon     = 0.0001f;  // total weight at/below this paints nothing

    // --- Entities (the overlay + entity-id passes). The instances are the RESOLVED ones
    // Placement accepted: the composite draws them, it never re-tests a placement rule.
    bool  bEntitiesEnabled       = true;
    float entityMarkRadiusPixels = 2.0f;
    float entityMarkColor[4]     = {1.0f, 0.85f, 0.1f, 1.0f};

    // Heightfield cell -> game units (X/Z), the same quantity Placement emitted its instance
    // positions with (`Proc::PlacementConstants::worldUnitsPerCell`). UI may not include a
    // PROC header (ARCH §3.1), so PIPELINE sets both from one recipe value (M4-5).
    float worldUnitsPerCell = 1.0f;
};

} // namespace Ui
} // namespace SanmapGen
