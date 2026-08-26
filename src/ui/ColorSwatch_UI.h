// ColorSwatch_UI.h — the picker-only color swatch. Layer: UI. Accuracy class: Visual.
// UI_FRAMEWORK_SPEC "Universal widget library": every color a tab exposes (stratum preview /
// remap tints, sun and fog tints, team, marker, area colors) is edited through THIS control.
// Per the v2 tab plan the swatch is a PICKER ONLY — no RGBA number fields — so the popup opens
// with ImGuiColorEditFlags_NoInputs (ColorSwatch_UI.cpp).
//
// Storage is the house convention: four linear floats, byte-compatible with
// Params::GradientStop::color, so a caller hands its PARAMS array straight in. The control owns
// no app state: the caller holds the color and one Ui::RealtimeToggle and reads the WidgetChange
// back (ARCH §3.2). Everything here is pure and headless-testable (WidgetHelpers_UI.h "THE
// SPLIT"); only the .cpp includes imgui.
#pragma once
#include "RtToggleWidget_UI.h"
#include "WidgetHelpers_UI.h"

namespace SanmapGen {
namespace Ui {

// Linear RGBA — the same channel count as Ui::kGradientStopChannelCount.
enum : int { kColorSwatchChannelCount = 4 };

// Per-swatch tweakables (Constitution §8: the alpha policy and the sizes are settings, not
// literals in the draw code).
struct ColorSwatchOptions {
    bool  bAlphaEnabled  = false;   // false: the picker edits RGB only and alpha is held opaque
    bool  bAlphaBarShown = false;   // the picker's vertical alpha bar (the Areas tab wants it)
    float swatchWidth    = 0.0f;    // <= 0: the remaining content width, less the RT button
    float swatchHeight   = 0.0f;    // <= 0: imgui's frame height
    bool  bLabelHidden   = false;   // NEW — STEP123: skip the TextUnformatted(label) line so the
                                     // button + RT toggle sit on ONE line via SameLine (a header slot).
                                     // `label` is still used to scope ImGui::PushID; only the visible
                                     // text is skipped.
    // A caller whose field never triggers anything beyond a cheap preview repaint can set this to
    // drop the RT button entirely — `realtimeToggle` still governs the underlying commit timing
    // unchanged, the caller simply never draws the control that would let a user flip it off.
    bool  bRealtimeToggleHidden = false;
};

// Forces a legal color: every channel into 0..1, NaN to 0, and alpha to opaque while alpha
// editing is off — so a color read from an older recipe can never render as an invisible swatch
// (Constitution §6: repair the input, never obey it).
inline void ClampSwatchColor(float color[kColorSwatchChannelCount], const ColorSwatchOptions& options) {
    for (int channel = 0; channel < kColorSwatchChannelCount; ++channel)
        color[channel] = ClampToRange(color[channel], 0.0f, 1.0f);
    if (!options.bAlphaEnabled) color[3] = 1.0f;
}

// 0..1 float channels -> the packed 0xAABBGGRR that WidgetStyle and every ImDrawList call speak.
// Rounded rather than truncated, so 1.0 lands on 255 and a round trip is stable.
inline PackedColor PackedColorFromSwatchColor(const float color[kColorSwatchChannelCount]) {
    PackedColor packedColor = 0u;
    for (int channel = 0; channel < kColorSwatchChannelCount; ++channel) {
        const float clampedChannel = ClampToRange(color[channel], 0.0f, 1.0f);
        packedColor |= static_cast<PackedColor>(clampedChannel * 255.0f + 0.5f) << (channel * 8);
    }
    return packedColor;
}

// The inverse: a packed color back to 0..1 channels. Multiplies a precomputed reciprocal rather
// than dividing (Constitution §3).
inline void SwatchColorFromPackedColor(PackedColor packedColor, float outColor[kColorSwatchChannelCount]) {
    constexpr float kReciprocalOfChannelMaximum = 1.0f / 255.0f;
    for (int channel = 0; channel < kColorSwatchChannelCount; ++channel)
        outColor[channel] = static_cast<float>((packedColor >> (channel * 8)) & 0xFFu) * kReciprocalOfChannelMaximum;
}

// True when two colors agree in every channel — how a caller decides a picker frame actually
// moved the color rather than merely re-reporting it.
inline bool SwatchColorsMatch(const float firstColor[kColorSwatchChannelCount],
                              const float secondColor[kColorSwatchChannelCount]) {
    for (int channel = 0; channel < kColorSwatchChannelCount; ++channel)
        if (firstColor[channel] != secondColor[channel]) return false;
    return true;
}

// One frame of picker interaction.
//   bPickerOpen   — the popup is open this frame, i.e. an edit is in progress.
//   bColorEdited  — the picker wrote the color this frame.
struct ColorSwatchInput {
    bool bPickerOpen  = false;
    bool bColorEdited = false;
};

// Applies one frame and returns the live/expensive pair: with RT off the color tracks the picker
// every frame while the recomposite is deferred to the frame the popup CLOSES; with RT on every
// picker frame commits (UI_FRAMEWORK_SPEC §7).
inline WidgetChange StepColorSwatchInteraction(RealtimeToggle& realtimeToggle,
                                               float color[kColorSwatchChannelCount],
                                               const ColorSwatchOptions& options,
                                               const ColorSwatchInput& input) {
    ClampSwatchColor(color, options);
    return realtimeToggle.Update(input.bPickerOpen, input.bColorEdited);
}

// Draws the label, the swatch button and the RT button; clicking the swatch opens the
// picker-only popup and runs the interaction above.
WidgetChange DrawColorSwatch(const char* label, float color[kColorSwatchChannelCount],
                             const ColorSwatchOptions& options, RealtimeToggle& realtimeToggle,
                             const WidgetStyle& style = WidgetStyle());

} // namespace Ui
} // namespace SanmapGen
