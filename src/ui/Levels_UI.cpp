// Levels_UI.cpp — the imgui draw path of the Levels control. Layer: UI.
// The histogram strip and the shadow/highlight markers are ImDrawList rectangles (the bypass
// toolkit); the five values are numeric fields. All clamping, the transfer function and the
// commit decision are pure and live in Levels_UI.h (WidgetHelpers_UI.h "THE SPLIT"), so this
// file is geometry only. Rendering is verified by eye against a live frame, never by test.
#include "Levels_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Height of the histogram strip, in multiples of the frame height — a proportion so the strip
// scales with the font rather than pinning a pixel count.
constexpr float kHistogramHeightInFrames = 2.5f;

struct LevelsFieldTally {
    bool bFieldActive = false;
    bool bFieldEdited = false;
};

// One numeric field. The drag speed is a thousandth of the range, so every field scrubs at the
// same relative rate whatever its limits.
void DrawLevelsField(const char* identifier, float& value, float minimumValue, float maximumValue,
                     float fieldWidth, LevelsFieldTally& tally) {
    ImGui::SetNextItemWidth(fieldWidth);
    const float dragSpeed = (maximumValue - minimumValue) * 0.001f;
    if (ImGui::DragFloat(identifier, &value, dragSpeed, minimumValue, maximumValue, "%.3f"))
        tally.bFieldEdited = true;
    tally.bFieldActive = tally.bFieldActive || ImGui::IsItemActive();
}

// The strip: frame, one bar per bucket, then the shadow and highlight markers on top so the
// input span being kept is visible against the distribution it is cutting.
void DrawHistogramStrip(const LevelsHistogramView& histogram, const LevelsSettings& settings,
                        const WidgetStyle& style) {
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float width  = ImGui::GetContentRegionAvail().x;
    const float height = ImGui::GetFrameHeight() * kHistogramHeightInFrames;
    ImGui::Dummy(ImVec2(width, height));                                   // reserve the strip row
    const float rounding = ResolveWidgetRounding(style);
    drawList->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height),
                            ResolveWidgetColor(style.trackColor, ImGuiCol_FrameBg), rounding);

    if (LevelsHistogramViewIsDrawable(histogram)) {
        const ImU32 barColor = ResolveWidgetColor(style.fillColor, ImGuiCol_PlotHistogram);
        const float bucketWidth = width / static_cast<float>(histogram.bucketCount);
        for (int bucketIndex = 0; bucketIndex < histogram.bucketCount; ++bucketIndex) {
            const float barHeight = ClampToRange(histogram.bucketWeights[bucketIndex], 0.0f, 1.0f) * height;
            if (!(barHeight > 0.0f)) continue;
            const float barLeftX = origin.x + static_cast<float>(bucketIndex) * bucketWidth;
            drawList->AddRectFilled(ImVec2(barLeftX, origin.y + height - barHeight),
                                    ImVec2(barLeftX + bucketWidth, origin.y + height), barColor);
        }
    }

    const ImU32 markerColor = ResolveWidgetColor(style.handleColor, ImGuiCol_SliderGrab);
    const float markerValues[2] = {settings.inputShadows, settings.inputHighlights};
    for (int markerIndex = 0; markerIndex < 2; ++markerIndex) {
        const float markerX = origin.x + NormalizedPosition(markerValues[markerIndex], 0.0f, 1.0f) * width;
        drawList->AddRectFilled(ImVec2(markerX - 1.0f, origin.y), ImVec2(markerX + 1.0f, origin.y + height),
                                markerColor);
    }
}

} // namespace

WidgetChange DrawLevels(const char* label, LevelsSettings& settings, const LevelsBounds& bounds,
                        const LevelsHistogramView& histogram, RealtimeToggle& realtimeToggle,
                        const WidgetStyle& style) {
    settings = ClampLevelsSettings(settings, bounds);

    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    DrawHistogramStrip(histogram, settings, style);

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float inputFieldWidth = (ImGui::GetContentRegionAvail().x - spacing * 2.0f) / 3.0f;
    LevelsFieldTally tally;
    DrawLevelsField("##inputShadows", settings.inputShadows, 0.0f, 1.0f, inputFieldWidth, tally);
    ImGui::SameLine();
    DrawLevelsField("##inputMidtones", settings.inputMidtones, bounds.midtonesMinimum,
                    bounds.midtonesMaximum, inputFieldWidth, tally);
    ImGui::SameLine();
    DrawLevelsField("##inputHighlights", settings.inputHighlights, 0.0f, 1.0f, inputFieldWidth, tally);

    const float outputFieldWidth =
        (ImGui::GetContentRegionAvail().x - spacing * 2.0f - style.realtimeButtonWidth) * 0.5f;
    DrawLevelsField("##outputBlack", settings.outputBlack, 0.0f, 1.0f, outputFieldWidth, tally);
    ImGui::SameLine();
    DrawLevelsField("##outputWhite", settings.outputWhite, 0.0f, 1.0f, outputFieldWidth, tally);
    ImGui::SameLine();
    DrawRealtimeToggleButton("realtime", realtimeToggle, style);

    LevelsFieldInput input;
    input.bFieldActive = tally.bFieldActive;
    input.bFieldEdited = tally.bFieldEdited;
    const WidgetChange change = StepLevelsInteraction(realtimeToggle, settings, bounds, input);
    ImGui::PopID();
    return change;
}

} // namespace Ui
} // namespace SanmapGen
