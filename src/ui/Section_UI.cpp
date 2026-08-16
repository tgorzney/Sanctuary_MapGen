// Section_UI.cpp — the imgui draw path of the collapsing section header. Layer: UI.
// A bar + arrow + label drawn with ImDrawList behind ONE InvisibleButton (the bypass toolkit),
// so a tab of forty sections costs forty items rather than forty framed imgui widgets. The
// open/closed decision is pure and lives in Section_UI.h (WidgetHelpers_UI.h "THE SPLIT").
#include "Section_UI.h"
#include "RtToggleWidget_UI.h"     // the shared style resolvers live beside the RT button
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The body's indent — the caller's width when it set one, else imgui's own.
void IndentSectionBody(const SectionOptions& options) {
    if (options.indentWidth > 0.0f) ImGui::Indent(options.indentWidth);
    else ImGui::Indent();
}

float ResolveHeaderRounding(const SectionOptions& options, const WidgetStyle& style) {
    return options.headerRounding >= 0.0f ? options.headerRounding : ResolveWidgetRounding(style);
}

// The disclosure arrow: pointing DOWN while the section is open, RIGHT while it is closed —
// drawn inside the square at the bar's left end, so the label always starts at the same x.
void DrawDisclosureArrow(ImDrawList* drawList, const ImVec2& origin, float barHeight, bool bOpen) {
    const float centerX = origin.x + barHeight * 0.5f;
    const float centerY = origin.y + barHeight * 0.5f;
    const float armLength = barHeight * 0.22f;
    const ImU32 arrowColor = ResolveWidgetColor(kThemeColor, ImGuiCol_Text);
    if (bOpen)
        drawList->AddTriangleFilled(ImVec2(centerX - armLength, centerY - armLength * 0.6f),
                                    ImVec2(centerX + armLength, centerY - armLength * 0.6f),
                                    ImVec2(centerX, centerY + armLength * 0.8f), arrowColor);
    else
        drawList->AddTriangleFilled(ImVec2(centerX - armLength * 0.6f, centerY - armLength),
                                    ImVec2(centerX + armLength * 0.8f, centerY),
                                    ImVec2(centerX - armLength * 0.6f, centerY + armLength), arrowColor);
}

} // namespace

bool DrawSectionBegin(const char* label, SectionState& state, const SectionOptions& options,
                      const WidgetStyle& style) {
    ImGui::PushID(label);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float barWidth  = ImGui::GetContentRegionAvail().x;
    const float barHeight = ResolveWidgetTrackHeight(style);
    ImGui::InvisibleButton("##header", ImVec2(barWidth > 1.0f ? barWidth : 1.0f, barHeight));
    const bool bHeaderClicked = ImGui::IsItemClicked();
    const bool bHeaderHovered = ImGui::IsItemHovered();

    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(origin, ImVec2(origin.x + barWidth, origin.y + barHeight),
                            ResolveWidgetColor(style.trackColor,
                                               bHeaderHovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header),
                            ResolveHeaderRounding(options, style));

    const SectionChange change = StepSectionHeader(state, bHeaderClicked);
    if (options.bArrowShown) DrawDisclosureArrow(drawList, origin, barHeight, state.bOpen);
    const float labelLeftX = origin.x + (options.bArrowShown ? barHeight : ImGui::GetStyle().FramePadding.x);
    drawList->AddText(ImVec2(labelLeftX, origin.y + (barHeight - ImGui::GetTextLineHeight()) * 0.5f),
                      ResolveWidgetColor(kThemeColor, ImGuiCol_Text), label);

    ImGui::PopID();
    if (change.bBodyVisible) IndentSectionBody(options);
    return change.bBodyVisible;
}

void DrawSectionEnd(const SectionOptions& options) {
    if (options.indentWidth > 0.0f) ImGui::Unindent(options.indentWidth);
    else ImGui::Unindent();
}

} // namespace Ui
} // namespace SanmapGen
