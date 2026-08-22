// MigrationReconciliationDialog_UI.cpp — see the header for the full contract. The one place in
// this widget that may include both imgui.h and Sanmap_MigrationPreview_IO.h (THE SPLIT,
// WidgetHelpers_UI.h) — ResetMigrationDialogFromReport's IO-copy, then the imgui draw path.
#include "MigrationReconciliationDialog_UI.h"
#include "Checkbox_UI.h"
#include "../io/Sanmap_MigrationPreview_IO.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

void ResetMigrationDialogFromReport(MigrationReconciliationDialogState& state,
                                    const Io::MigrationPreviewReport& report) {
    state.steps.clear();
    state.steps.reserve(report.steps.size());
    for (const Io::MigrationPreviewStep& previewStep : report.steps) {
        MigrationDialogStep step;
        step.sourceVersion = previewStep.sourceVersion;
        step.entries.reserve(previewStep.entries.size());
        for (const Io::MigrationPreviewEntry& previewEntry : previewStep.entries) {
            MigrationDialogEntry entry;
            entry.name                     = previewEntry.name;
            entry.description               = previewEntry.description;
            entry.bIndependentlySelectable   = previewEntry.bIndependentlySelectable;
            entry.bLosslessIfSkipped         = previewEntry.bLosslessIfSkipped;
            entry.bWouldChangeDocument       = previewEntry.bWouldChangeDocument;
            entry.bSelected                  = true;   // ruling 3: "apply all" is the default state
            step.entries.push_back(std::move(entry));
        }
        state.steps.push_back(std::move(step));
    }
}

std::vector<std::string> SelectedMigrationNames(const MigrationReconciliationDialogState& state) {
    std::vector<std::string> selectedNames;
    for (const MigrationDialogStep& step : state.steps)
        for (const MigrationDialogEntry& entry : step.entries)
            if (!IsMigrationDialogEntryCheckboxEligible(entry) || entry.bSelected)
                selectedNames.push_back(entry.name);
    return selectedNames;
}

namespace {

// One entry row: a real, tickable Checkbox_UI row for the one checkbox-eligible shape, or a plain
// greyed informational row (name + description, NO checkbox) for every other entry — ruling 2's
// dialog-gating law. `bWouldChangeDocument` is shown as the one simple indicator this ticket scopes
// in; a full diff view is explicitly out of scope.
void DrawMigrationDialogEntryRow(MigrationDialogEntry& entry) {
    const char* effectLabel = entry.bWouldChangeDocument ? "changes this document"
                                                          : "no effect on this document";
    if (IsMigrationDialogEntryCheckboxEligible(entry)) {
        DrawCheckbox(entry.name.c_str(), entry.bSelected);
        ImGui::TextWrapped("%s  (%s)", entry.description.c_str(), effectLabel);
        return;
    }
    ImGui::TextDisabled("%s", entry.name.c_str());
    ImGui::TextWrapped("%s  (%s)", entry.description.c_str(), effectLabel);
}

void DrawMigrationDialogStepGroup(MigrationDialogStep& step) {
    ImGui::Text("Version %d -> %d", step.sourceVersion, step.sourceVersion + 1);
    ImGui::Separator();
    for (MigrationDialogEntry& entry : step.entries) {
        DrawMigrationDialogEntryRow(entry);
        ImGui::Separator();
    }
}

} // namespace

MigrationReconciliationDialogChange DrawMigrationReconciliationDialog(
    const char* identifier, MigrationReconciliationDialogState& state) {
    MigrationReconciliationDialogChange change;
    if (state.bOpenRequested) {
        ImGui::OpenPopup(identifier);
        state.bOpenRequested = false;
    }

    const bool bBegan = ImGui::BeginPopupModal(
        identifier, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    if (!bBegan) return change;

    ImGui::TextUnformatted("Review what a full migration walk would find, then choose what to apply.");
    // A precedent-matching scrolling child (DrawLogSection, FilesTab_Draw_UI.cpp) — manifest scale
    // (single-digit steps, low entry counts, ruling 2) never warrants a VirtualList<T>.
    ImGui::BeginChild("migrationReconciliationList", ImVec2(480.0f, 320.0f), true,
                      ImGuiWindowFlags_HorizontalScrollbar);
    for (MigrationDialogStep& step : state.steps) DrawMigrationDialogStepGroup(step);
    ImGui::EndChild();

    if (ImGui::Button("Apply Selected")) {
        change.bApplyClicked = true;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Close")) {
        change.bCloseClicked = true;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
    return change;
}

} // namespace Ui
} // namespace SanmapGen
