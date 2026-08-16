// GradientEditorWidget_Paint_UI.cpp — ImDrawList painting for the gradient editor (M5-3).
// UI_FRAMEWORK_SPEC bypass style: the strip and its handles are drawn directly into the window
// draw list rather than composed from built-in widgets. Pure output — no ramp mutation, no state
// change, so the interaction TU stays the single writer of the edit.
#include "GradientEditorWidget_Paint_UI.h"
#include <cstddef>

namespace SanmapGen {
namespace Ui {
namespace {

int ColorChannelToByte(float value) {
    const float clamped = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
    return static_cast<int>(clamped * 255.0f + 0.5f);
}

} // namespace

ImU32 GradientColorFromLookupEntry(const float* entry) {
    return IM_COL32(ColorChannelToByte(entry[0]), ColorChannelToByte(entry[1]),
                    ColorChannelToByte(entry[2]), ColorChannelToByte(entry[3]));
}

void PaintGradientStrip(ImDrawList* drawList, const std::vector<float>& lookupTable,
                        int sampleCount, const GradientEditorState& state, const ImVec2& origin,
                        float width) {
    if (drawList == nullptr || sampleCount < 2) return;
    const float segmentWidth = width / static_cast<float>(sampleCount - 1);
    const float bottom = origin.y + state.stripeHeight;
    for (int sample = 0; sample + 1 < sampleCount; ++sample) {
        const std::size_t entry = static_cast<std::size_t>(sample) * kGradientStopChannelCount;
        const ImU32 leftColor = GradientColorFromLookupEntry(&lookupTable[entry]);
        const ImU32 rightColor =
            GradientColorFromLookupEntry(&lookupTable[entry + kGradientStopChannelCount]);
        const float leftX = origin.x + segmentWidth * static_cast<float>(sample);
        drawList->AddRectFilledMultiColor(ImVec2(leftX, origin.y),
                                          ImVec2(leftX + segmentWidth, bottom),
                                          leftColor, rightColor, rightColor, leftColor);
    }
}

void PaintGradientStopHandles(ImDrawList* drawList, const Params::GradientRamp& ramp,
                              const GradientEditorState& state, const ImVec2& origin, float width) {
    if (drawList == nullptr) return;
    const float handleY = origin.y + state.stripeHeight + state.handleRadius;
    const int stopCount = static_cast<int>(ramp.stops.size());
    for (int stopIndex = 0; stopIndex < stopCount; ++stopIndex) {
        const Params::GradientStop& stop = ramp.stops[static_cast<std::size_t>(stopIndex)];
        const ImVec2 center(origin.x + width * stop.location, handleY);
        drawList->AddCircleFilled(center, state.handleRadius,
                                  GradientColorFromLookupEntry(stop.color));
        const bool bSelected = stopIndex == state.selectedStopIndex;
        drawList->AddCircle(center, state.handleRadius,
                            bSelected ? IM_COL32_WHITE : IM_COL32_BLACK, 0,
                            bSelected ? 2.0f : 1.0f);
    }
}

} // namespace Ui
} // namespace SanmapGen
