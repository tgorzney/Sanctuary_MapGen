// ColorSwatch_UI.cpp — the imgui draw path of the picker-only color swatch. Layer: UI.
// One of the translation units that include imgui.h; all clamping, packing and the commit
// decision are pure and live in the header (WidgetHelpers_UI.h "THE SPLIT"), so this file is
// only the button, the popup and the flag set. Rendering is verified by eye against a live
// frame, never by test.
#include "ColorSwatch_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The picker is deliberately input-less: NoInputs drops the RGBA/hex fields the v1 pickers had,
// which is the whole point of this control in the v2 tab plan. NoAlpha additionally hides the
// alpha slider whenever the caller does not edit alpha, so nothing can write color[3].
ImGuiColorEditFlags ResolvePickerFlags(const ColorSwatchOptions& options) {
    ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
    if (!options.bAlphaEnabled)   flags |= ImGuiColorEditFlags_NoAlpha;
    else if (options.bAlphaBarShown) flags |= ImGuiColorEditFlags_AlphaBar;
    return flags;
}

// The swatch button itself shows alpha as a half-transparent split only when alpha is editable;
// an RGB-only swatch is drawn fully opaque so it reads as a flat color chip.
ImGuiColorEditFlags ResolveButtonFlags(const ColorSwatchOptions& options) {
    return options.bAlphaEnabled ? ImGuiColorEditFlags_AlphaPreviewHalf : ImGuiColorEditFlags_NoAlpha;
}

ImVec2 ResolveSwatchSize(const ColorSwatchOptions& options, const WidgetStyle& style) {
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float availableWidth = ImGui::GetContentRegionAvail().x - spacing - style.realtimeButtonWidth;
    const float width  = options.swatchWidth  > 0.0f ? options.swatchWidth
                                                     : (availableWidth > 1.0f ? availableWidth : 1.0f);
    const float height = options.swatchHeight > 0.0f ? options.swatchHeight : ResolveWidgetTrackHeight(style);
    return ImVec2(width, height);
}

} // namespace

WidgetChange DrawColorSwatch(const char* label, float color[kColorSwatchChannelCount],
                             const ColorSwatchOptions& options, RealtimeToggle& realtimeToggle,
                             const WidgetStyle& style) {
    ClampSwatchColor(color, options);

    ImGui::PushID(label);
    if (!options.bLabelHidden) ImGui::TextUnformatted(label);

    const ImVec4 previewColor(color[0], color[1], color[2], options.bAlphaEnabled ? color[3] : 1.0f);
    if (ImGui::ColorButton("##swatch", previewColor, ResolveButtonFlags(options),
                           ResolveSwatchSize(options, style)))
        ImGui::OpenPopup("##picker");
    if (!options.bRealtimeToggleHidden) {
        ImGui::SameLine();
        DrawRealtimeToggleButton("realtime", realtimeToggle, style);
    }

    ColorSwatchInput input;
    if (ImGui::BeginPopup("##picker")) {
        input.bPickerOpen  = true;
        input.bColorEdited = ImGui::ColorPicker4("##pickerBody", color, ResolvePickerFlags(options));
        ImGui::EndPopup();
    }

    const WidgetChange change = StepColorSwatchInteraction(realtimeToggle, color, options, input);
    ImGui::PopID();
    return change;
}

} // namespace Ui
} // namespace SanmapGen
