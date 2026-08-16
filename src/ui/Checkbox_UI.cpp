// Checkbox_UI.cpp — the ImDrawList draw path of the tick box. Layer: UI.
// One of the translation units that include imgui.h; the toggle and exclusive-mask logic is pure
// and lives in the header (see WidgetHelpers_UI.h "THE SPLIT"), so this file is only geometry, one
// hit-test per box and rectangles. Rendering is verified by eye against a live frame, never by test.
#include "Checkbox_UI.h"
#include "RtToggleWidget_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The box itself: frame, fill and tick, all inside a square of `boxSize`. The tick is two strokes
// rather than a glyph, so the control needs no font range beyond ASCII.
void DrawTickBox(const ImVec2& origin, float boxSize, bool bChecked, bool bHeld,
                 const WidgetStyle& style) {
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const float rounding = ResolveWidgetRounding(style);
    const ImVec2 farCorner(origin.x + boxSize, origin.y + boxSize);

    drawList->AddRectFilled(origin, farCorner, ResolveWidgetColor(style.trackColor, ImGuiCol_FrameBg), rounding);
    if (bHeld)
        drawList->AddRect(origin, farCorner, ResolveWidgetColor(style.handleActiveColor, ImGuiCol_ButtonActive),
                          rounding);
    if (!bChecked) return;

    // Tick geometry is DERIVED from the box, not a second set of style knobs: a box that follows
    // the imgui frame height gets a tick that follows it too, and the shared WidgetStyle keeps
    // exactly one meaning per field.
    const float inset = boxSize * 0.25f;
    const ImU32 tickColor = ResolveWidgetColor(style.fillColor, ImGuiCol_CheckMark);
    const float thickness = boxSize * 0.125f > 1.0f ? boxSize * 0.125f : 1.0f;
    const ImVec2 tickLow(origin.x + inset, origin.y + boxSize * 0.55f);
    const ImVec2 tickBottom(origin.x + boxSize * 0.45f, farCorner.y - inset);
    drawList->AddLine(tickLow, tickBottom, tickColor, thickness);
    drawList->AddLine(tickBottom, ImVec2(farCorner.x - inset, origin.y + inset), tickColor, thickness);
}

// One box + its label, already inside the caller's PushID. Returns true on the frame it was
// clicked; the caller decides what that means for a single bool or for a mask bit.
bool TickBoxWasClicked(const char* identifier, const char* label, bool bChecked, const WidgetStyle& style) {
    const float boxSize = ResolveWidgetTrackHeight(style);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float labelWidth = (label != nullptr && label[0] != '\0')
        ? ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize(label).x : 0.0f;

    ImGui::InvisibleButton(identifier, ImVec2(boxSize + labelWidth, boxSize));
    const bool bClicked = ImGui::IsItemClicked();
    DrawTickBox(origin, boxSize, bChecked, ImGui::IsItemActive(), style);
    if (labelWidth > 0.0f)
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(origin.x + boxSize + ImGui::GetStyle().ItemInnerSpacing.x,
                   origin.y + (boxSize - ImGui::GetTextLineHeight()) * 0.5f),
            ImGui::GetColorU32(ImGuiCol_Text), label);
    return bClicked;
}

} // namespace

WidgetChange DrawCheckbox(const char* label, bool& value, const WidgetStyle& style) {
    ImGui::PushID(label);
    const bool bClicked = TickBoxWasClicked("##box", label, value, style);
    const WidgetChange change = StepCheckboxInteraction(value, bClicked);
    ImGui::PopID();
    return change;
}

WidgetChange DrawExclusiveCheckboxRow(const char* label, unsigned int& mask, const char* const* labels,
                                      int bitCount, bool bAllowNone, const WidgetStyle& style) {
    if (bitCount > kMaximumExclusiveCheckboxCount) bitCount = kMaximumExclusiveCheckboxCount;
    ImGui::PushID(label);
    if (label != nullptr && label[0] != '\0') ImGui::TextUnformatted(label);

    int clickedIndex = -1;
    const unsigned int drawnMask = ResolvedExclusiveCheckboxMask(mask, bitCount);
    for (int bitIndex = 0; bitIndex < bitCount; ++bitIndex) {
        if (bitIndex > 0) ImGui::SameLine();
        ImGui::PushID(bitIndex);
        const char* const bitLabel = labels != nullptr ? labels[bitIndex] : nullptr;
        if (TickBoxWasClicked("##bit", bitLabel, IsExclusiveCheckboxBitSet(drawnMask, bitIndex), style))
            clickedIndex = bitIndex;
        ImGui::PopID();
    }

    const WidgetChange change = StepExclusiveCheckboxInteraction(mask, bitCount, clickedIndex, bAllowNone);
    ImGui::PopID();
    return change;
}

} // namespace Ui
} // namespace SanmapGen
