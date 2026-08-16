// Combo_UI.cpp — the imgui draw path of the shared dropdown. Layer: UI.
// One of the translation units that include imgui.h; the selection resolve/label/step logic is
// pure and lives in the header (see WidgetHelpers_UI.h "THE SPLIT"), so this file is only layout
// and the popup list. Rendering is verified by eye against a live frame, never by test.
//
// Unlike the sliders, the popup itself is imgui's BeginCombo: it opens on a click and closes on a
// pick, so it is never on a hot path and the ImDrawList bypass would buy nothing.
#include "Combo_UI.h"
#include "RtToggleWidget_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

WidgetChange DrawCombo(const char* label, int& selectedIndex, const ComboOptions& options,
                       const WidgetStyle& style) {
    const int resolvedIndex = ResolvedComboSelection(selectedIndex, options);
    int pickedIndex = -1;

    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ResolveWidgetRounding(style));
    if (ImGui::BeginCombo("##combo", ComboSelectionLabel(selectedIndex, options))) {
        for (int optionIndex = 0; optionIndex < options.count; ++optionIndex) {
            const char* const optionLabel = options.labels[optionIndex];
            const bool bSelected = optionIndex == resolvedIndex;
            ImGui::PushID(optionIndex);
            if (ImGui::Selectable(optionLabel != nullptr ? optionLabel : "", bSelected))
                pickedIndex = optionIndex;
            if (bSelected) ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::PopStyleVar();

    const WidgetChange change = StepComboInteraction(selectedIndex, options, pickedIndex);
    ImGui::PopID();
    return change;
}

} // namespace Ui
} // namespace SanmapGen
