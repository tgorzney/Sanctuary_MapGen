// Levels_UI.h — the shadows / midtones / highlights + output black/white control. Layer: UI.
// Accuracy class: Visual (it shapes a preview-facing curve; the sim reads the resulting values,
// not the widget).
// UI_FRAMEWORK_SPEC "Universal widget library": the one Levels control the Layer Editor and every
// layer stack that follows it draws, matching the v1 "Levels..." popup.
//
// The transfer function is the Photoshop convention: remap [inputShadows, inputHighlights] onto
// 0..1, apply the midtone gamma, then map onto [outputBlack, outputWhite]. Output inversion
// (outputBlack > outputWhite) is legal and useful, so it is NOT "repaired".
//
// Owns no app state: the caller holds the LevelsSettings and one Ui::RealtimeToggle and reads the
// WidgetChange back (ARCH §3.2). Everything here is pure; only Levels_UI.cpp includes imgui.
#pragma once
#include "LevelsHistogram_UI.h"
#include "RtToggleWidget_UI.h"
#include "WidgetHelpers_UI.h"
#include <cmath>

namespace SanmapGen {
namespace Ui {

// The five values the control edits. Defaults are the identity curve.
struct LevelsSettings {
    float inputShadows    = 0.0f;
    float inputMidtones   = 1.0f;    // gamma; 1 = linear, > 1 brightens the midtones
    float inputHighlights = 1.0f;
    float outputBlack     = 0.0f;
    float outputWhite     = 1.0f;
};

// The limits the control enforces — every one a setting rather than a literal in the drag code
// (Constitution §8). The midtone limits are v1's 0.01..9.99.
struct LevelsBounds {
    float midtonesMinimum        = 0.01f;
    float midtonesMaximum        = 9.99f;
    float minimumInputSeparation = 0.001f;   // keeps the input span invertible
};

// Forces a legal set: inputs and outputs into 0..1, midtones into its limits, and the input span
// at least minimumInputSeparation wide (an inverted input pair swaps, exactly as the range slider
// repairs its own — Constitution §6). Output order is deliberately left alone.
inline LevelsSettings ClampLevelsSettings(LevelsSettings settings, const LevelsBounds& bounds) {
    settings.inputShadows    = ClampToRange(settings.inputShadows, 0.0f, 1.0f);
    settings.inputHighlights = ClampToRange(settings.inputHighlights, 0.0f, 1.0f);
    settings.outputBlack     = ClampToRange(settings.outputBlack, 0.0f, 1.0f);
    settings.outputWhite     = ClampToRange(settings.outputWhite, 0.0f, 1.0f);
    settings.inputMidtones   = ClampToRange(settings.inputMidtones, bounds.midtonesMinimum, bounds.midtonesMaximum);
    if (settings.inputHighlights < settings.inputShadows) {
        const float swap = settings.inputShadows;
        settings.inputShadows = settings.inputHighlights;
        settings.inputHighlights = swap;
    }
    const float separation = bounds.minimumInputSeparation > 0.0f ? bounds.minimumInputSeparation : 0.0f;
    if (settings.inputHighlights - settings.inputShadows < separation) {
        settings.inputHighlights = settings.inputShadows + separation;
        if (settings.inputHighlights > 1.0f) { settings.inputHighlights = 1.0f; settings.inputShadows = 1.0f - separation; }
    }
    return settings;
}

// The transfer function itself. `settings` is clamped on entry so a caller can pass a raw recipe
// value straight in and still get a defined curve.
inline float ApplyLevels(float value, const LevelsSettings& rawSettings, const LevelsBounds& bounds = LevelsBounds()) {
    const LevelsSettings settings = ClampLevelsSettings(rawSettings, bounds);
    const float inputSpan = settings.inputHighlights - settings.inputShadows;
    if (!(inputSpan > kMinimumWidgetRange)) return settings.outputBlack;
    const float normalized = ClampToRange((value - settings.inputShadows) / inputSpan, 0.0f, 1.0f);
    const float shaped = settings.inputMidtones == 1.0f
                       ? normalized
                       : std::pow(normalized, 1.0f / settings.inputMidtones);
    return settings.outputBlack + shaped * (settings.outputWhite - settings.outputBlack);
}

// One frame of interaction over the five numeric fields, expressed so a synthetic sequence can
// drive it headless.
//   bFieldActive — any of the fields is being dragged this frame.
//   bFieldEdited — a field ALREADY wrote a value this frame; the step re-clamps and folds it into
//                  the commit decision, it does not re-apply it.
struct LevelsFieldInput {
    bool bFieldActive = false;
    bool bFieldEdited = false;
};

// Applies one frame and returns the live/expensive pair: with RT off the curve follows the drag
// every frame while the recompute is deferred to release (UI_FRAMEWORK_SPEC §7).
inline WidgetChange StepLevelsInteraction(RealtimeToggle& realtimeToggle, LevelsSettings& settings,
                                          const LevelsBounds& bounds, const LevelsFieldInput& input) {
    settings = ClampLevelsSettings(settings, bounds);
    return realtimeToggle.Update(input.bFieldActive, input.bFieldEdited);
}

// Draws the histogram, the shadow/highlight markers over it, the five fields and the RT button.
WidgetChange DrawLevels(const char* label, LevelsSettings& settings, const LevelsBounds& bounds,
                        const LevelsHistogramView& histogram, RealtimeToggle& realtimeToggle,
                        const WidgetStyle& style = WidgetStyle());

} // namespace Ui
} // namespace SanmapGen
