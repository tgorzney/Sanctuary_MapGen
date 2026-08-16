// TerrainOverlayTab_UI.h — the two lookups every field-overlay tab needs. Layer: UI.
// Accuracy class: Visual. Pure and headless-testable — no imgui, no draw.
//
// Slope, Flow and Accumulation are all the same shape: a "show overlay" tick over ONE
// `Ui::PreviewFieldLayer` of the composite, and a `GradientEditor` over the ramp that layer points
// at. Written once here rather than three times, so a tab names its field and nothing else.
//
// These are PRESENTATION settings (PreviewComposite_Settings_UI.h): no generation stage hashes
// them, so an edit derives `bNeedsPreviewRender` by itself through PreviewDriver — the tabs
// contain no per-widget tier decision, exactly like every other v2 tab.
#pragma once
#include "PreviewComposite_Settings_UI.h"

namespace SanmapGen {
namespace Ui {

// The composite layer that colorizes `kind`, or null when the caller's settings carry none. The
// FIRST match wins: a settings object with two layers of one kind is a caller bug, and silently
// editing the second would hide it.
inline PreviewFieldLayer* PreviewFieldLayerOfKind(PreviewCompositeSettings& settings,
                                                  PreviewLayerKind kind) {
    for (PreviewFieldLayer& layer : settings.fieldLayers)
        if (layer.kind == kind) return &layer;
    return nullptr;
}

// The ramp a layer colorizes with, or null when it names none or names one the settings no longer
// carry (a recipe whose ramp list shrank — Constitution §6, resolve rather than index).
inline Params::GradientRamp* PreviewRampOfFieldLayer(PreviewCompositeSettings& settings,
                                                     const PreviewFieldLayer& layer) {
    if (layer.gradientRampIndex < 0
        || layer.gradientRampIndex >= static_cast<int>(settings.gradientRamps.size())) return nullptr;
    return &settings.gradientRamps[static_cast<std::size_t>(layer.gradientRampIndex)];
}

// True when the overlay for `kind` is present AND ticked — what a tab's "Show overlay" reports
// back when the caller asks whether its field is currently painted.
inline bool IsPreviewOverlayShown(PreviewCompositeSettings& settings, PreviewLayerKind kind) {
    const PreviewFieldLayer* const layer = PreviewFieldLayerOfKind(settings, kind);
    return layer != nullptr && layer->bEnabled;
}

} // namespace Ui
} // namespace SanmapGen
