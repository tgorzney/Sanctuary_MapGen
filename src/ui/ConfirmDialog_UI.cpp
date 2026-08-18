// ConfirmDialog_UI.cpp — the imgui draw path of the generic confirm popup. Layer: UI.
// All settings/result plumbing is pure and lives in the header (WidgetHelpers_UI.h "THE SPLIT");
// this file is only the popup open/begin/buttons, following ColorSwatch_UI.cpp's established split.
// Rendering is verified by eye against a live frame — a modal popup sequence is not headless-
// testable (no imgui frame in the acceptance binaries).
#include "ConfirmDialog_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

namespace {

// No title bar when the dialog must be answered with a button: with `p_open == nullptr` a modal
// popup has no X, and (Dear ImGui's NavUpdateCancelRequest) Escape only auto-closes NON-modal
// popups, so a BeginPopupModal with no close pointer genuinely cannot be dismissed any other way.
// `bClosableWithoutChoice` restores the title bar (and its X) for a caller that wants one.
ImGuiWindowFlags ResolveModalFlags(const ConfirmDialogOptions& options) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
    if (!options.bClosableWithoutChoice) flags |= ImGuiWindowFlags_NoTitleBar;
    return flags;
}

} // namespace

ConfirmDialogChange DrawConfirmDialog(const char* identifier, ConfirmDialogState& state,
                                      const ConfirmDialogOptions& options) {
    ConfirmDialogChange change;
    if (state.bOpenRequested) {
        ImGui::OpenPopup(identifier);
        state.bOpenRequested = false;
    }

    bool bStillOpen = true;
    const bool bBegan = ImGui::BeginPopupModal(identifier, options.bClosableWithoutChoice ? &bStillOpen : nullptr,
                                               ResolveModalFlags(options));
    if (!bBegan) return change;

    if (!options.title.empty()) {
        ImGui::TextUnformatted(options.title.c_str());
        ImGui::Separator();
    }
    ImGui::TextUnformatted(options.bodyText.c_str());
    ImGui::Separator();

    if (ImGui::Button(options.primaryButtonLabel.c_str())) {
        change.bPrimaryClicked = true;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(options.secondaryButtonLabel.c_str())) {
        change.bSecondaryClicked = true;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
    return change;
}

} // namespace Ui
} // namespace SanmapGen
