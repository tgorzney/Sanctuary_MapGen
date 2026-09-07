// GradientEditorWidget_Draw_UI.cpp — the imgui interaction half of the gradient editor (M5-3):
// the strip, its stop handles and add/smooth-toggle. Split from the pure edit semantics
// (GradientEditorWidget_UI.cpp), the painting (GradientEditorWidget_Paint_UI.cpp) and the
// selected-stop sub-panel (GradientEditorWidget_StopControls_UI.cpp) per ARCH §1.5. Hit-testing
// is one InvisibleButton over the whole strip (UI_FRAMEWORK_SPEC bypass toolkit #1), and every
// mutation goes through the pure functions — this TU adds no edit rule of its own.
//
// The strip preview is baked with Ui::BakeGradientLut (M4-2) at the widget's own small sample
// count; the CALLER still re-bakes at its resolution when this returns true.
#include "GradientEditorWidget_Paint_UI.h"
#include "GradientEditorWidget_StopControls_UI.h"
#include "GradientLut_UI.h"
#include <cstddef>

namespace SanmapGen {
namespace Ui {
namespace {

int ResolveStripeSampleCount(const GradientEditorState& state) {
    return state.stripeSampleCount < 2 ? 2 : state.stripeSampleCount;
}

float ClampToUnitRange(float value) {
    return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}

// A click on bare strip adds a stop carrying the color already there, so adding never moves the
// curve; the new stop is selected and immediately draggable.
bool AddStopAtLocation(Params::GradientRamp& ramp, GradientEditorState& state,
                       const std::vector<float>& lookupTable, int sampleCount, float location) {
    const int entry = static_cast<int>(location * static_cast<float>(sampleCount - 1) + 0.5f);
    const int clampedEntry = entry < 0 ? 0 : (entry > sampleCount - 1 ? sampleCount - 1 : entry);
    const std::size_t channel = static_cast<std::size_t>(clampedEntry) * kGradientStopChannelCount;
    const int newStopIndex = AddGradientStop(ramp, location, &lookupTable[channel]);
    if (newStopIndex < 0) return false;
    state.selectedStopIndex = newStopIndex;
    state.draggedStopIndex = newStopIndex;
    return true;
}

bool BeginStripInteraction(Params::GradientRamp& ramp, GradientEditorState& state,
                           const std::vector<float>& lookupTable, int sampleCount,
                           float mouseLocation, float width) {
    const int nearestIndex = NearestGradientStopIndex(ramp, mouseLocation);
    if (nearestIndex >= 0) {
        const float offset =
            (ramp.stops[static_cast<std::size_t>(nearestIndex)].location - mouseLocation) * width;
        const float pixelDistance = offset < 0.0f ? -offset : offset;
        if (pixelDistance <= state.handleRadius * 2.0f) {
            state.selectedStopIndex = nearestIndex;
            state.draggedStopIndex = nearestIndex;
            return false;                       // picked up an existing stop — nothing changed yet
        }
    }
    return AddStopAtLocation(ramp, state, lookupTable, sampleCount, mouseLocation);
}

bool UpdateStripInteraction(Params::GradientRamp& ramp, GradientEditorState& state,
                            const std::vector<float>& lookupTable, int sampleCount,
                            const ImVec2& origin, float width) {
    if (width <= 0.0f) return false;
    const float reciprocalWidth = 1.0f / width;
    const float mouseLocation =
        ClampToUnitRange((ImGui::GetIO().MousePos.x - origin.x) * reciprocalWidth);

    bool bChanged = false;
    if (ImGui::IsItemActivated())
        bChanged = BeginStripInteraction(ramp, state, lookupTable, sampleCount, mouseLocation, width);
    if (ImGui::IsItemActive() && state.draggedStopIndex >= 0)
        bChanged = MoveGradientStop(ramp, state.draggedStopIndex, mouseLocation) || bChanged;
    if (ImGui::IsItemDeactivated()) state.draggedStopIndex = -1;
    return bChanged;
}

bool DrawStripAndHandles(Params::GradientRamp& ramp, GradientEditorState& state) {
    const int sampleCount = ResolveStripeSampleCount(state);
    const std::vector<float> lookupTable = BakeGradientLut(ramp, sampleCount);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    if (width < 1.0f) width = 1.0f;

    ImGui::InvisibleButton("gradientStrip",
                           ImVec2(width, state.stripeHeight + state.handleRadius * 2.0f));
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    PaintGradientStrip(drawList, lookupTable, sampleCount, state, origin, width);
    PaintGradientStopHandles(drawList, ramp, state, origin, width);
    return UpdateStripInteraction(ramp, state, lookupTable, sampleCount, origin, width);
}

} // namespace

bool DrawGradientEditor(const char* label, Params::GradientRamp& ramp, GradientEditorState& state,
                        const GradientEditorOptions& options) {
    bool bChanged = false;
    ImGui::PushID(label);                              // ID scoping always runs, label or not
    if (!options.bLabelHidden) ImGui::TextUnformatted(label);

    bool bSmoothInterpolation = ramp.bSmoothInterpolation;
    if (ImGui::Checkbox("Smooth interpolation", &bSmoothInterpolation))
        bChanged = SetGradientSmoothInterpolation(ramp, bSmoothInterpolation) || bChanged;
    ImGui::SameLine();
    if (ImGui::Button("Add stop")) {
        const int newStopIndex = AddGradientStop(ramp, 0.5f, nullptr);
        if (newStopIndex >= 0) { state.selectedStopIndex = newStopIndex; bChanged = true; }
    }

    bChanged = DrawStripAndHandles(ramp, state) || bChanged;
    bChanged = DrawSelectedStopControls(ramp, state, options) || bChanged;

    ImGui::PopID();
    return bChanged;
}

} // namespace Ui
} // namespace SanmapGen
