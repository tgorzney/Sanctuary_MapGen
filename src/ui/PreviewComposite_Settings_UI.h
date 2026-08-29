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
#include "AreaColorTable_UI.h"
#include "../params/GradientRamp_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// Which per-pixel COLOR SOURCE a layer draws — a baked `Data::MapFields` field, SAMPLED and never
// re-derived (Slope is the Mask stage's own bake, colorized as-is: this is what keeps the shadow-sim
// deleted, ARCH §3.2), a PARAMS-flattened analytic source with no baked field behind it at all
// (StratumSplat's nine weight fields + tints; MapAreas' rectangles + colors,
// ARCH_14_17_MapAreaFieldLayer.md §14.17 item 1), or a combination (Water: a threshold over the
// heightfield, parameterized by `Params::Water`). What a layer may NEVER be is a re-decision of a
// PLACEMENT rule (markers/props/decals/units/reclaim) — that is the shadow-sim defect this comment
// used to describe too narrowly by omission (PREVIEW_COMPOSITING_SPEC "the shadow-sim problem").
enum class PreviewLayerKind { HeightRamp, StratumSplat, Flow, Accumulation, Water, Slope, MapAreas };

// The preview-only Z-order blend (PREVIEW_COMPOSITING_SPEC). Distinct from the geometry
// `Params::HeightBlendMode`, which blends terrain height, not pixels.
// STEP200: append-only, order matters (existing indices are load-bearing on both the CPU switch in
// PreviewComposite_Color_UI.h and the GPU #defines PreviewComposite_GpuProgram_UI.cpp generates) —
// Subtract..HardLight are the six modes v1 (Widget_MapCanvas.cpp) had that v2 was missing; v1's
// "Normal" is this enum's existing AlphaBlend (cosmetic naming only, not a duplicate), and v1's
// "None" is superseded by the View popup's dedicated visibility toggle, not ported here.
enum class PreviewBlendMode {
    Replace, AlphaBlend, Add, Multiply, Maximum, Minimum,
    Subtract, Divide, Overlay, Screen, SoftLight, HardLight
};
// The enumerator count, named so the UI's `previewBlendModeNames[]` display table and any test can
// static_assert against it instead of drifting silently when the enum grows.
enum : int { kPreviewBlendModeCount = 12 };

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
    // positions with (`Params::Geometry::worldUnitsPerCell` — map geometry, M5-0a). PIPELINE
    // sets this mirror and Placement's reader from that one recipe value (M4-5).
    float worldUnitsPerCell = 1.0f;

    // ARCH_14_17_MapAreaFieldLayer.md §14.17 item 9 — the single owner of the per-area presentation
    // color, moved here from `AreasTabState::areaColors` (removed, not duplicated): the Areas tab,
    // MapCanvas's own drag gesture and the composite's own field-layer flattening all need the SAME
    // mutable table, and this is the category `gradientRamps`/`clearColor` already occupy —
    // presentation state that never serializes into `mapGeneratorData`.
    std::vector<AreaColorEntry> areaColors;

    // ARCH §14.17 item 11 — the ONE area currently mid-drag/resize/move on the canvas, omitted from
    // this frame's composited input so a live drag costs exactly two recomposites (begin+end), never
    // one per frame. Transient interaction state: NEVER serialized, and an out-of-range value
    // suppresses nothing (the safe degradation if a list reorder ever races a gesture).
    int mapAreaSuppressedIndex = -1;
};

} // namespace Ui
} // namespace SanmapGen
