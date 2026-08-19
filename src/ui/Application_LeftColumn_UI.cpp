// Application_LeftColumn_UI.cpp — the v1 left column: a vertical tab list under the three group
// headers, each row carrying the `[O]`/`[ ]` preview-visibility toggle. Layer: UI.
// Behind Application_UI.h (ARCH §1.5). TAB_REBUILD_PLAN "Layout (keep v1 shape)".
//
// The rows are NOT written out here: they are read from `applicationPanelEntries`
// (Application_Panels_UI.h), so the column, the panel bodies and the layout test can never disagree
// about which tabs exist or which of them carry a toggle. Which panel is visible is presentation
// state — it trips no dirty flag and moves no parameter; a `[O]` click is presentation too, and it
// lands on the composite's own layer flags (Application_Visibility_UI.h).
#include "Application_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

// v1's row: the toggle button, then the selectable. Reports whether the toggle moved.
bool DrawPanelRow(const ApplicationPanelEntry& entry, int panelIndex, ApplicationPanel& activePanel,
                  ApplicationVisibilityState& visibility) {
    bool bVisibilityMoved = false;
    ImGui::PushID(panelIndex);
    if (entry.bHasVisibilityToggle) {
        bool& bPanelVisible = visibility.bPanelVisible[panelIndex];
        if (ImGui::Button(bPanelVisible ? "[O]" : "[ ]")) {
            bPanelVisible = !bPanelVisible;
            bVisibilityMoved = true;
        }
        ImGui::SameLine();
    }
    if (ImGui::Selectable(entry.label, activePanel == entry.panel)) activePanel = entry.panel;
    ImGui::PopID();
    return bVisibilityMoved;
}

// One group: its header, its separator, then every row the catalogue files under it.
bool DrawPanelGroup(ApplicationPanelGroup group, ApplicationPanel& activePanel,
                    ApplicationVisibilityState& visibility) {
    ImGui::TextUnformatted(ApplicationPanelGroupLabel(group));
    ImGui::Separator();
    bool bVisibilityMoved = false;
    for (int panelIndex = 0; panelIndex < kApplicationPanelCount; ++panelIndex) {
        const ApplicationPanelEntry& entry = applicationPanelEntries[panelIndex];
        if (entry.group != group) continue;
        bVisibilityMoved = DrawPanelRow(entry, panelIndex, activePanel, visibility) || bVisibilityMoved;
    }
    ImGui::Spacing();
    return bVisibilityMoved;
}

} // namespace

void Application::DrawPanelSwitcher() {
    bool bVisibilityMoved = false;
    for (int groupIndex = 0; groupIndex < kApplicationPanelGroupCount; ++groupIndex)
        bVisibilityMoved = DrawPanelGroup(static_cast<ApplicationPanelGroup>(groupIndex),
                                          tabState.activePanel, tabState.visibility) || bVisibilityMoved;
    // A hidden layer is a PRESENTATION change: no stage's parameter hash can see it, so the driver
    // derives bNeedsPreviewRender and the image recomposites with no regeneration (ARCH §3.2).
    if (bVisibilityMoved && ApplyPanelVisibility(tabState.visibility, composite.Settings()))
        previewDriver.NotifyParametersChanged();
    ImGui::Separator();
    ImGui::Text("Regenerations: %d", previewDriver.PipelineRunCount());
    ImGui::Text("Composites: %d", previewDriver.PreviewCompositeCount());
    ImGui::Text("Stages last run: %d", static_cast<int>(previewDriver.StagesThatRan().size()));
    ImGui::TextWrapped("%s", AssetStatusMessage().c_str());
}

} // namespace Ui
} // namespace SanmapGen
